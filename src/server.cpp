// This file is part of Fuxedo
// Copyright (C) 2017 Aivars Kalvans <aivars.kalvans@gmail.com>

#include <setjmp.h>
#include <userlog.h>
#include <xa.h>
#include <xatmi.h>

#include <atomic>
#include <clara.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "fields.h"
#include "fux.h"
#include "ipc.h"
#include "mib.h"
#include "misc.h"
#include "trx.h"

#if defined(__cplusplus)
extern "C" {
#endif
int _tmrunserver(int);
int _tmbuilt_with_thread_option;
int _tmstartserver(int argc, char **argv, struct tmsvrargs_t *tmsvrargs);
#if defined(__cplusplus)
}
#endif

static void dispatch();

int tprminit(char *, void *) { return 0; }
int tpsvrinit(int, char **) {
  if (tpopen() == -1) {
    userlog("tpopen() failed: %s", tpstrerror(tperrno));
    return -1;
  }
  return 0;
}
void tpsvrdone() {
  if (tpclose() == -1) {
    userlog("tpclose() failed: %s", tpstrerror(tperrno));
  }
}
int tpsvrthrinit(int, char **) {
  if (tpopen() == -1) {
    userlog("tpopen() failed: %s", tpstrerror(tperrno));
    return -1;
  }
  return 0;
}
void tpsvrthrdone() {
  if (tpclose() == -1) {
    userlog("tpclose() failed: %s", tpstrerror(tperrno));
  }
}

void ubb2mib(ubbconfig &u, mib &m);

int get_queue(const char *svc);

struct server_main {
  uint16_t srvid;
  uint16_t grpno;

  int request_queue;

  size_t mib_server;
  size_t mib_queue;

  int argc;
  char **argv;
  struct tmsvrargs_t *tmsvrargs;

  std::mutex mutex;
  std::map<const char *, void (*)(TPSVCINFO *), cmp_cstr> advertisements;

  mib &m_;
  std::atomic<bool> stop;

  server_main(mib &m) : m_(m), stop(false), req_counter_(0) {
    mtype_ = std::numeric_limits<long>::min();
  }

  void active() { m_.servers().at(mib_server).state = state_t::active; }

  int tpadvertise(const char *svcname, void (*func)(TPSVCINFO *)) {
    fux::scoped_fuxlock lock(mutex);

    if (svcname == nullptr || strlen(svcname) == 0) {
      TPERROR(TPEINVAL, "invalid svcname");
      return -1;
    }
    if (func == nullptr) {
      TPERROR(TPEINVAL, "invalid func");
      return -1;
    }
    /*
       [TPEPROTO]
       tpadvertise() was called in an improper context (for example, by a
       client).
       [TPESYSTEM]
       An Oracle Tuxedo system error has occurred. The exact nature of the error
       is written to a log file.
       [TPEOS]
       An operating system error has occurred.o
       */
    auto it = advertisements.find(svcname);

    if (it != advertisements.end()) {
      if (it->second != func) {
        TPERROR(TPEMATCH, "svcname advertised with a different func");
        return -1;
      }
    } else {
      try {
        auto lock = m_.data_lock();
        m_.advertise(svcname, mib_queue, mib_server);
      } catch (const std::out_of_range &e) {
        TPERROR(TPELIMIT, "%s", e.what());
        return -1;
      }
      advertisements.insert(std::make_pair(strdup(svcname), func));
    }
    return 0;
  }

  int tpunadvertise(const char *svcname) {
    fux::scoped_fuxlock lock(mutex);
    if (svcname == nullptr || strlen(svcname) == 0) {
      TPERROR(TPEINVAL, "invalid svcname");
      return -1;
    }
    auto it = advertisements.find(svcname);

    if (it != advertisements.end()) {
      auto freeme = it->first;
      advertisements.erase(it);
      free(const_cast<char *>(freeme));
      m_.unadvertise(svcname, mib_queue, mib_server);
    } else {
      TPERROR(TPENOENT, "svcname not advertised");
      return -1;
    }
    return 0;
  }

  bool handle(long mtype, fux::fml32buf &buf) {
    // Do not want to see this message again
    mtype_ = -(mtype - 1);
    req_counter_ = 0;

    if (buf.get<long>(FUX_SRVID, 0) == srvid &&
        buf.get<long>(FUX_GRPNO, 0) == grpno) {
      stop = true;
      return true;
    }
    return false;
  }

  long mtype() {
    req_counter_ = (req_counter_ + 1) % 8;
    if (req_counter_ == 0) {
      // every nth message screw priorities and read first message from queue
      return 0;
    }
    return mtype_;
  }

 private:
  long mtype_;
  int req_counter_;
};

static std::unique_ptr<server_main> main_ptr;

namespace fux {
bool is_server() { return main_ptr.get() != nullptr; }
}  // namespace fux

int _tmrunserver(int) {
  // Just a placeholder
  return 0;
}

struct server_thread {
  server_thread() : atmibuf(nullptr) {}

  void prepare() {
    if (atmibuf == nullptr) {
      atmibuf = tpalloc(const_cast<char *>("STRING"), nullptr, 4 * 1024);
      fux::mem::setowner(atmibuf, &atmibuf);
    }
  }

  fux::ipc::msg req;
  fux::ipc::msg res;
  char *atmibuf;
  jmp_buf tpreturn_env;

  void tpforward(char *svc, char *data, long len, long flags) {
    auto gttid = fux::tx::gttid();
    if (fux::tx::transactional()) {
      fux::tx_end(true);
    }

    int msqid = get_queue(svc);
    res.set_data(data, len);

    if (data != nullptr && data != atmibuf) {
      tpfree(data);
    }

    if (flags != 0) {
      userlog("tpforward with flags!=0");
    }
    checked_copy(svc, res->servicename);
    res->flags = flags;
    res->replyq = req->replyq;
    res->mtype = req->cd;
    res->cd = req->cd;
    res->gttid = gttid;

    fux::ipc::qsend(msqid, res, 0, fux::ipc::flags::notime);

    longjmp(tpreturn_env, 1);
  }

  void tpreturn(int rval, long rcode, char *data, long len, long flags) {
    if (fux::tx::transactional()) {
      fux::tx_end(rval == TPSUCCESS);
    }

    if (req->replyq != -1) {
      res.set_data(data, len);

      if (data != nullptr && data != atmibuf) {
        tpfree(data);
      }

      if (flags != 0) {
        userlog("tpreturn with flags!=0");
        res->rval = TPESVCERR;
      } else {
        res->rval = rval;
      }
      res->rcode = rcode;
      res->flags = flags;
      res->mtype = req->cd;
      res->cd = req->cd;

      fux::ipc::qsend(req->replyq, res, 0, fux::ipc::flags::notime);
    }

    longjmp(tpreturn_env, 1);
  }
};

static thread_local std::unique_ptr<server_thread> thread_ptr;

static void dispatch() {
  thread_ptr = std::make_unique<server_thread>();

  while (true) {
    if (main_ptr->stop) {
      break;
    }

    TPSVCINFO tpsvcinfo;
    {
      fux::scoped_fuxlock lock(main_ptr->mutex);
      if (main_ptr->stop) {
        break;
      }

      fux::ipc::qrecv(main_ptr->request_queue, thread_ptr->req,
                      main_ptr->mtype(), 0);

      thread_ptr->prepare();
      thread_ptr->req.get_data(&thread_ptr->atmibuf);
      tpsvcinfo.data = thread_ptr->atmibuf;
      if (thread_ptr->req->cat == fux::ipc::admin) {
        fux::fml32buf buf(&tpsvcinfo);

        userlog("Received admin message");
        if (!main_ptr->handle(thread_ptr->req->mtype, buf)) {
          // return for processing by other MSSQ servers
          userlog("Not the target receiver of message, put back in queue");
          fux::ipc::qsend(main_ptr->request_queue, thread_ptr->req, 0,
                          fux::ipc::flags::notime);
        }
        continue;
      } else {
        // else continue processing without global lock
      }
    }

    checked_copy(thread_ptr->req->servicename, tpsvcinfo.name);
    tpsvcinfo.flags = thread_ptr->req->flags;
    tpsvcinfo.len = thread_ptr->req.size_data();
    tpsvcinfo.cd = thread_ptr->req->cd;

    if (thread_ptr->req->flags & TPTRAN) {
      fux::tx_join(thread_ptr->req->gttid);
    }

    auto it = main_ptr->advertisements.find(tpsvcinfo.name);
    if (setjmp(thread_ptr->tpreturn_env) == 0) {
      it->second(&tpsvcinfo);
    } else {
    }
  }

  thread_ptr.reset();
}

static void thread_dispatch(tmsvrargs_t *tmsvrargs, int argc, char *argv[]) {
  if (int n = tmsvrargs->svrthrinit(argc, argv); n != 0) {
    userlog("tpsvrthrinit() = %d", n);
    return;
  }
  dispatch();
  tmsvrargs->svrthrdone();
}

namespace fux::glob {
extern xa_switch_t *xasw;
}

int _tmstartserver(int argc, char **argv, struct tmsvrargs_t *tmsvrargs) {
  bool show_help = false;
  bool verbose = false;

  bool all = false;
  int grpno = -1;
  int srvid = -1;
  std::vector<std::string> services;

  auto parser =
      clara::Help(show_help) |
      clara::Opt(srvid, "SRVID")["-i"]("server's SRVID in TUXCONFIG") |
      clara::Opt(grpno, "GRPNO")["-g"]("server's GRPNO in TUXCONFIG") |
      clara::Opt(services, "SERVICES")["-s"]("services to advertise") |
      clara::Opt(all)["-A"]("advertise all services") |
      clara::Opt(verbose)["-v"]("display built-in services");

  int sep = 0;
  for (; sep < argc; sep++) {
    if (strcmp(argv[sep], "--") == 0) {
      break;
    }
  }

  auto result = parser.parse(clara::Args(sep, argv));
  if (!result) {
    std::cerr << parser;
    return -1;
  }
  if (show_help) {
    std::cout << parser;
    return 0;
  }

  if (verbose) {
    struct tmdsptchtbl_t *rec = tmsvrargs->tmdsptchtbl;
    std::cout << "# Service : Function mapping" << std::endl;
    while (rec->svcname != nullptr) {
      std::cout << rec->svcname << ":" << rec->funcname << std::endl;
      rec++;
    }
    return 0;
  }

  if (_tmbuilt_with_thread_option) {
    fux::threaded = true;
  }

  mib &m = getmib();
  main_ptr = std::make_unique<server_main>(m);

  main_ptr->mib_server = m.find_server(srvid, grpno);
  if (main_ptr->mib_server == m.badoff) {
    return -1;
  }

  main_ptr->srvid = srvid;
  main_ptr->grpno = grpno;
  fux::tx::grpno = grpno;

  main_ptr->request_queue = m.make_service_rqaddr(main_ptr->mib_server);
  main_ptr->mib_queue = m.servers().at(main_ptr->mib_server).rqaddr;
  main_ptr->argc = argc;
  main_ptr->argv = argv;
  main_ptr->tmsvrargs = tmsvrargs;
  fux::glob::xasw = tmsvrargs->xa_switch;

  if (int n = tmsvrargs->svrinit(argc, argv); n != 0) {
    userlog("tpsvrinit() = %d", n);
    return -1;
  }

  if (all) {
    struct tmdsptchtbl_t *rec = tmsvrargs->tmdsptchtbl;
    while (rec->svcname != nullptr && strlen(rec->svcname) > 0) {
      if (tpadvertise(const_cast<char *>(rec->svcname), rec->svcfunc) == -1) {
        userlog("Failed to advertise service %s / function %s : %s",
                rec->svcname, rec->funcname, tpstrerror(tperrno));
        return -1;
      }
      rec++;
    }
  }

  main_ptr->active();

  if (_tmbuilt_with_thread_option) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; i++) {
      threads.emplace_back(std::thread(thread_dispatch, tmsvrargs, argc, argv));
    }

    for (auto &t : threads) {
      t.join();
    }
  } else {
    dispatch();
  }

  tmsvrargs->mainloop(0);

  tmsvrargs->svrdone();
  return 0;
}

int tpadvertise(char *svcname, void (*func)(TPSVCINFO *)) {
  if (!main_ptr) {
    TPERROR(TPEPROTO, "%s can't be called from client", __func__);
    return -1;
  }
  return main_ptr->tpadvertise(svcname, func);
}

int tpunadvertise(char *svcname) {
  if (!main_ptr) {
    TPERROR(TPEPROTO, "%s can't be called from client", __func__);
    return -1;
  }
  return main_ptr->tpunadvertise(svcname);
}

void tpreturn(int rval, long rcode, char *data, long len, long flags) try {
  if (!main_ptr) {
    TPERROR(TPEPROTO, "%s can't be called from client", __func__);
    abort();
  }
  if (rval == TPSUCCESS) {
    rval = TPMINVAL;
  } else if (rval == TPFAIL) {
    rval = TPESVCFAIL;
  } else if (rval == TPEXIT) {
    rval = TPESVCERR;
  } else {
    rval = TPESVCERR;
  }
  return thread_ptr->tpreturn(rval, rcode, data, len, flags);
} catch (...) {
  userlog("tpreturn failed");
  longjmp(thread_ptr->tpreturn_env, -1);
}

void tpforward(char *svc, char *data, long len, long flags) try {
  if (!main_ptr) {
    TPERROR(TPEPROTO, "%s can't be called from client", __func__);
    abort();
  }
  return thread_ptr->tpforward(svc, data, len, flags);
} catch (...) {
  userlog("tpforward failed");
  tpreturn(TPESVCERR, 0, data, len, flags);
}
