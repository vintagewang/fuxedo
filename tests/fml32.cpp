#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include <fml32.h>
#include <xatmi.h>

#define DECONST(x) const_cast<char *>(x)

static void fldid32_check(int type) {
  long num = 13;
  FLDID32 id = Fmkfldid32(type, num);
  REQUIRE(id != BADFLDID);
  REQUIRE(Fldtype32(id) == type);
  REQUIRE(Fldno32(id) == num);
}

TEST_CASE("fldid32", "[fml32]") {
  int types[] = {FLD_SHORT,  FLD_LONG,   FLD_CHAR,   FLD_FLOAT,
                 FLD_DOUBLE, FLD_STRING, FLD_CARRAY, FLD_FML32};

  for (auto type : types) {
    fldid32_check(type);
  }
}

TEST_CASE("Finit32-Fsizeof32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto size = Fsizeof32(fbfr);
  REQUIRE(size > 0);

  REQUIRE(Finit32(fbfr, Fsizeof32(fbfr)) != -1);
  REQUIRE(Fsizeof32(fbfr) == size);

  REQUIRE(Finit32(fbfr, Fsizeof32(fbfr)) != -1);
  REQUIRE(Fsizeof32(fbfr) == size);

  Ffree32(fbfr);
}

TEST_CASE("Frealloc32", "[fml32]") {
  auto fbfr = Falloc32(2, 1);
  REQUIRE(fbfr != nullptr);

  auto size = Fsizeof32(fbfr);
  REQUIRE(size > 0);

  fbfr = Frealloc32(fbfr, 3, 1);
  REQUIRE(fbfr != nullptr);
  REQUIRE(Fsizeof32(fbfr) > size);

  fbfr = Frealloc32(fbfr, 1, 1);
  REQUIRE(fbfr != nullptr);
  REQUIRE(Fsizeof32(fbfr) < size);

  Ffree32(fbfr);
}

TEST_CASE("Fused32-Funused32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto size = Fsizeof32(fbfr);
  REQUIRE(size > 0);
  auto used = Fused32(fbfr);
  REQUIRE(used > 0);
  auto unused = Funused32(fbfr);
  REQUIRE(unused > 0);

  REQUIRE(used + unused == size);

  auto fld_short = Fmkfldid32(FLD_SHORT, 10);
  short i = 0;
  REQUIRE(Fchg32(fbfr, fld_short, i, reinterpret_cast<char *>(&i), 0) != -1);

  REQUIRE(Fsizeof32(fbfr) == size);
  REQUIRE(Fused32(fbfr) > used);
  REQUIRE(Funused32(fbfr) < unused);

  REQUIRE(Fused32(fbfr) + Funused32(fbfr) == size);

  Ffree32(fbfr);
}

struct FieldFixture {
  short s = 13;
  long l = 13;
  char c = 13;
  float f = 13;
  double d = 13;
  std::string str = "13131313131313131313131313";
  std::string bytes = "1\003";

  FLDID32 fld_short = Fmkfldid32(FLD_SHORT, 10);
  FLDID32 fld_long = Fmkfldid32(FLD_LONG, 11);
  FLDID32 fld_char = Fmkfldid32(FLD_CHAR, 12);
  FLDID32 fld_double = Fmkfldid32(FLD_DOUBLE, 13);
  FLDID32 fld_float = Fmkfldid32(FLD_FLOAT, 14);
  FLDID32 fld_string = Fmkfldid32(FLD_STRING, 15);
  FLDID32 fld_carray = Fmkfldid32(FLD_CARRAY, 16);

  void set_fields(FBFR32 *fbfr) {
    REQUIRE(Fchg32(fbfr, fld_short, 0, reinterpret_cast<char *>(&s), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_long, 0, reinterpret_cast<char *>(&l), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_char, 0, reinterpret_cast<char *>(&c), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_float, 0, reinterpret_cast<char *>(&f), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_double, 0, reinterpret_cast<char *>(&d), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_string, 0, DECONST(str.c_str()), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_carray, 0, DECONST(bytes.data()), bytes.size()) !=
            -1);
  }

  void get_fields(FBFR32 *fbfr) {
    REQUIRE(*reinterpret_cast<short *>(Ffind32(fbfr, fld_short, 0, nullptr)) ==
            s);
    REQUIRE(*reinterpret_cast<long *>(Ffind32(fbfr, fld_long, 0, nullptr)) ==
            l);
    REQUIRE(*reinterpret_cast<char *>(Ffind32(fbfr, fld_char, 0, nullptr)) ==
            c);
    REQUIRE(*reinterpret_cast<float *>(Ffind32(fbfr, fld_float, 0, nullptr)) ==
            f);
    REQUIRE(*reinterpret_cast<double *>(
                Ffind32(fbfr, fld_double, 0, nullptr)) == d);
    REQUIRE(reinterpret_cast<char *>(Ffind32(fbfr, fld_string, 0, nullptr)) ==
            str);
    REQUIRE(std::string(
                reinterpret_cast<char *>(Ffind32(fbfr, fld_carray, 0, nullptr)),
                bytes.size()) == bytes);
  }
};

TEST_CASE("Ffindocc32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_long = Fmkfldid32(FLD_LONG, 10);
  for (long i = 10; i < 13; i++) {
    REQUIRE(Fchg32(fbfr, fld_long, i - 10, reinterpret_cast<char *>(&i), 0) !=
            -1);
  }

  long l;
  l = 9;
  REQUIRE((Ffindocc32(fbfr, fld_long, reinterpret_cast<char *>(&l), 0) == -1 &&
           Ferror32 == FNOTPRES));
  l = 10;
  REQUIRE(Ffindocc32(fbfr, fld_long, reinterpret_cast<char *>(&l), 0) == 0);
  l = 11;
  REQUIRE(Ffindocc32(fbfr, fld_long, reinterpret_cast<char *>(&l), 0) == 1);
  l = 12;
  REQUIRE(Ffindocc32(fbfr, fld_long, reinterpret_cast<char *>(&l), 0) == 2);

  auto fld_string = Fmkfldid32(FLD_STRING, 10);

  for (long i = 20; i < 23; i++) {
    REQUIRE(Fchg32(fbfr, fld_string, i - 20, DECONST(std::to_string(i).c_str()),
                   0) != -1);
  }
  REQUIRE((Ffindocc32(fbfr, fld_string, DECONST("19"), 0) == -1 &&
           Ferror32 == FNOTPRES));
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("20"), 0) == 0);
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("21"), 0) == 1);
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("22"), 0) == 2);

  // regex if len != 0
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("20"), 1) == 0);
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("21"), 1) == 1);
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("22"), 1) == 2);
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("22|21"), 1) == 1);
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("a|22|b"), 1) == 2);
  REQUIRE(Ffindocc32(fbfr, fld_string, DECONST("[abc2][0efg]"), 1) == 0);

  Ffree32(fbfr);
}

TEST_CASE_METHOD(FieldFixture, "Fchksum32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto chksum_empty = Fchksum32(fbfr);
  REQUIRE(chksum_empty != -1);

  set_fields(fbfr);
  auto chksum = Fchksum32(fbfr);
  REQUIRE(chksum != -1);
  REQUIRE(chksum != chksum_empty);

  auto fbfr2 = Falloc32(100, 100);
  REQUIRE(fbfr2 != nullptr);
  REQUIRE(Fcpy32(fbfr2, fbfr) != -1);

  auto chksum_cpy = Fchksum32(fbfr2);
  REQUIRE(chksum == chksum_cpy);

  Ffree32(fbfr);
  Ffree32(fbfr2);
}

TEST_CASE_METHOD(FieldFixture, "Fcpy32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  set_fields(fbfr);

  auto fbfr2 = Falloc32(100, 100);
  REQUIRE(fbfr2 != nullptr);
  REQUIRE(Fcpy32(fbfr2, fbfr) != -1);

  get_fields(fbfr2);

  auto fbfr3 = Falloc32(2, 10);
  REQUIRE(fbfr3 != nullptr);
  REQUIRE((Fcpy32(fbfr3, fbfr) == -1 && Ferror32 == FNOSPACE));

  Ffree32(fbfr);
  Ffree32(fbfr2);
  Ffree32(fbfr3);
}

TEST_CASE_METHOD(FieldFixture, "Fcpy32-tpalloc", "[fml32]") {
  auto fbfr = (FBFR32 *)tpalloc(DECONST("FML32"), DECONST("*"), 2 * 1024);
  REQUIRE(fbfr != nullptr);

  set_fields(fbfr);

  auto fbfr2 = (FBFR32 *)tpalloc(DECONST("FML32"), DECONST("*"), 2 * 1024);
  REQUIRE(fbfr2 != nullptr);
  REQUIRE(Fcpy32(fbfr2, fbfr) != -1);

  get_fields(fbfr2);

  std::string s(1024, 'x');
  REQUIRE(Fchg32(fbfr, fld_string, 1, DECONST(s.c_str()), 0) != -1);

  auto fbfr3 = (FBFR32 *)tpalloc(DECONST("FML32"), DECONST("*"), 50);
  REQUIRE(fbfr3 != nullptr);
  REQUIRE((Fcpy32(fbfr3, fbfr) == -1 && Ferror32 == FNOSPACE));

  tpfree((char *)fbfr);
  tpfree((char *)fbfr2);
  tpfree((char *)fbfr3);
}

TEST_CASE_METHOD(FieldFixture, "Fchg32-Flen32-Ffind32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  set_fields(fbfr);

  REQUIRE(Flen32(fbfr, fld_short, 0) == sizeof(short));
  REQUIRE(Flen32(fbfr, fld_long, 0) == sizeof(long));
  REQUIRE(Flen32(fbfr, fld_char, 0) == sizeof(char));
  REQUIRE(Flen32(fbfr, fld_float, 0) == sizeof(float));
  REQUIRE(Flen32(fbfr, fld_double, 0) == sizeof(double));
  REQUIRE(Flen32(fbfr, fld_string, 0) == str.size() + 1);
  REQUIRE(Flen32(fbfr, fld_carray, 0) == bytes.size());

  get_fields(fbfr);

  FLDLEN32 len;
  REQUIRE(*reinterpret_cast<short *>(Ffind32(fbfr, fld_short, 0, &len)) == s);
  REQUIRE(len == sizeof(short));
  REQUIRE(*reinterpret_cast<long *>(Ffind32(fbfr, fld_long, 0, &len)) == l);
  REQUIRE(len == sizeof(long));
  REQUIRE(*reinterpret_cast<char *>(Ffind32(fbfr, fld_char, 0, &len)) == c);
  REQUIRE(len == sizeof(char));
  REQUIRE(*reinterpret_cast<float *>(Ffind32(fbfr, fld_float, 0, &len)) == f);
  REQUIRE(len == sizeof(float));
  REQUIRE(*reinterpret_cast<double *>(Ffind32(fbfr, fld_double, 0, &len)) == d);
  REQUIRE(len == sizeof(double));
  REQUIRE(reinterpret_cast<char *>(Ffind32(fbfr, fld_string, 0, &len)) == str);
  REQUIRE(len == str.size() + 1);
  REQUIRE(
      std::string(reinterpret_cast<char *>(Ffind32(fbfr, fld_carray, 0, &len)),
                  bytes.size()) == bytes);
  REQUIRE(len == bytes.size());

  Ffree32(fbfr);
}

TEST_CASE_METHOD(FieldFixture, "Fget32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  set_fields(fbfr);

  char buf[512];
  FLDLEN32 len;

  len = 1;
  REQUIRE(
      (Fget32(fbfr, fld_short, 0, buf, &len) == -1 && Ferror32 == FNOSPACE));
  //  REQUIRE(len == sizeof(short));
  len = sizeof(buf);
  REQUIRE(Fget32(fbfr, fld_short, 0, buf, &len) != -1);
  REQUIRE(*reinterpret_cast<short *>(buf) == s);

  len = 1;
  REQUIRE((Fget32(fbfr, fld_long, 0, buf, &len) == -1 && Ferror32 == FNOSPACE));
  //  REQUIRE(len == sizeof(long));
  len = sizeof(buf);
  REQUIRE(Fget32(fbfr, fld_long, 0, buf, &len) != -1);
  REQUIRE(*reinterpret_cast<long *>(buf) == l);

  len = 0;
  REQUIRE((Fget32(fbfr, fld_char, 0, buf, &len) == -1 && Ferror32 == FNOSPACE));
  REQUIRE(len == sizeof(char));
  len = sizeof(buf);
  REQUIRE(Fget32(fbfr, fld_char, 0, buf, &len) != -1);
  REQUIRE(*reinterpret_cast<char *>(buf) == c);

  len = 1;
  REQUIRE(
      (Fget32(fbfr, fld_float, 0, buf, &len) == -1 && Ferror32 == FNOSPACE));
  REQUIRE(len == sizeof(float));
  len = sizeof(buf);
  REQUIRE(Fget32(fbfr, fld_float, 0, buf, &len) != -1);
  REQUIRE(*reinterpret_cast<float *>(buf) == f);

  len = 1;
  REQUIRE(
      (Fget32(fbfr, fld_double, 0, buf, &len) == -1 && Ferror32 == FNOSPACE));
  REQUIRE(len == sizeof(double));
  len = sizeof(buf);
  REQUIRE(Fget32(fbfr, fld_double, 0, buf, &len) != -1);
  REQUIRE(*reinterpret_cast<double *>(buf) == d);

  len = 1;
  REQUIRE(
      (Fget32(fbfr, fld_string, 0, buf, &len) == -1 && Ferror32 == FNOSPACE));
  REQUIRE(len == str.size() + 1);
  len = sizeof(buf);
  REQUIRE(Fget32(fbfr, fld_string, 0, buf, &len) != -1);
  REQUIRE(reinterpret_cast<char *>(buf) == str);

  len = 1;
  REQUIRE(
      (Fget32(fbfr, fld_carray, 0, buf, &len) == -1 && Ferror32 == FNOSPACE));
  REQUIRE(len == bytes.size());
  len = sizeof(buf);
  REQUIRE(Fget32(fbfr, fld_carray, 0, buf, &len) != -1);
  REQUIRE(std::string(reinterpret_cast<char *>(buf), bytes.size()) == bytes);

  Ffree32(fbfr);
}

TEST_CASE_METHOD(FieldFixture, "Fgetalloc32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  set_fields(fbfr);

  char *buf;
  FLDLEN32 len = 0;

  REQUIRE((buf = Fgetalloc32(fbfr, fld_short, 0, &len)) != nullptr);
  REQUIRE(*reinterpret_cast<short *>(buf) == s);
  REQUIRE(len == sizeof(short));
  free(buf);

  REQUIRE((buf = Fgetalloc32(fbfr, fld_long, 0, &len)) != nullptr);
  REQUIRE(*reinterpret_cast<long *>(buf) == l);
  REQUIRE(len == sizeof(long));
  free(buf);

  REQUIRE((buf = Fgetalloc32(fbfr, fld_char, 0, &len)) != nullptr);
  REQUIRE(*reinterpret_cast<char *>(buf) == c);
  REQUIRE(len == sizeof(char));
  free(buf);

  REQUIRE((buf = Fgetalloc32(fbfr, fld_float, 0, &len)) != nullptr);
  REQUIRE(*reinterpret_cast<float *>(buf) == f);
  REQUIRE(len == sizeof(float));
  free(buf);

  REQUIRE((buf = Fgetalloc32(fbfr, fld_double, 0, &len)) != nullptr);
  REQUIRE(*reinterpret_cast<double *>(buf) == d);
  REQUIRE(len == sizeof(double));
  free(buf);

  len = 0;
  REQUIRE((buf = Fgetalloc32(fbfr, fld_string, 0, &len)) != nullptr);
  REQUIRE(reinterpret_cast<char *>(buf) == str);
  REQUIRE(len == str.size() + 1);
  free(buf);

  REQUIRE((buf = Fgetalloc32(fbfr, fld_carray, 0, &len)) != nullptr);
  REQUIRE(std::string(reinterpret_cast<char *>(buf), bytes.size()) == bytes);
  REQUIRE(len == bytes.size());
  free(buf);

  Ffree32(fbfr);
}

TEST_CASE("Fpres32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_short = Fmkfldid32(FLD_SHORT, 10);

  for (short i = 0; i < 3; i++) {
    REQUIRE(!Fpres32(fbfr, fld_short, i));
    REQUIRE(Fadd32(fbfr, fld_short, reinterpret_cast<char *>(&i), 0) != -1);
    REQUIRE(Fpres32(fbfr, fld_short, i));
  }

  Ffree32(fbfr);
}

TEST_CASE("Fadd32-Foccur32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_short = Fmkfldid32(FLD_SHORT, 10);

  for (short i = 0; i < 3; i++) {
    REQUIRE(Foccur32(fbfr, fld_short) == i);
    REQUIRE(Fadd32(fbfr, fld_short, reinterpret_cast<char *>(&i), 0) != -1);
    REQUIRE(Foccur32(fbfr, fld_short) == i + 1);
  }

  REQUIRE(Foccur32(fbfr, fld_short) == 3);

  for (short i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_short, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(*reinterpret_cast<short *>(ptr) == i);
  }

  Ffree32(fbfr);
}

TEST_CASE("Fdel32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_short = Fmkfldid32(FLD_SHORT, 10);

  for (short i = 0; i < 3; i++) {
    REQUIRE(Fadd32(fbfr, fld_short, reinterpret_cast<char *>(&i), 0) != -1);
  }

  char *ptr;

  REQUIRE(Foccur32(fbfr, fld_short) == 3);
  ptr = Ffind32(fbfr, fld_short, 0, nullptr);
  REQUIRE((ptr != nullptr && *reinterpret_cast<short *>(ptr) == 0));
  ptr = Ffind32(fbfr, fld_short, 1, nullptr);
  REQUIRE((ptr != nullptr && *reinterpret_cast<short *>(ptr) == 1));
  ptr = Ffind32(fbfr, fld_short, 2, nullptr);
  REQUIRE((ptr != nullptr && *reinterpret_cast<short *>(ptr) == 2));

  REQUIRE(Fdel32(fbfr, fld_short, 1) != -1);

  REQUIRE(Foccur32(fbfr, fld_short) == 2);
  ptr = Ffind32(fbfr, fld_short, 0, nullptr);
  REQUIRE((ptr != nullptr && *reinterpret_cast<short *>(ptr) == 0));
  ptr = Ffind32(fbfr, fld_short, 1, nullptr);
  REQUIRE((ptr != nullptr && *reinterpret_cast<short *>(ptr) == 2));

  REQUIRE(Fdel32(fbfr, fld_short, 0) != -1);

  REQUIRE(Foccur32(fbfr, fld_short) == 1);
  ptr = Ffind32(fbfr, fld_short, 0, nullptr);
  REQUIRE((ptr != nullptr && *reinterpret_cast<short *>(ptr) == 2));

  REQUIRE(Fdel32(fbfr, fld_short, 0) != -1);

  REQUIRE(Foccur32(fbfr, fld_short) == 0);

  REQUIRE((Fdel32(fbfr, fld_short, 0) == -1 && Ferror32 == FNOTPRES));

  Ffree32(fbfr);
}

TEST_CASE("Fdelall32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_short = Fmkfldid32(FLD_SHORT, 10);

  for (short i = 0; i < 3; i++) {
    REQUIRE(Fadd32(fbfr, fld_short, reinterpret_cast<char *>(&i), 0) != -1);
  }

  REQUIRE(Foccur32(fbfr, fld_short) == 3);

  REQUIRE(Fdelall32(fbfr, fld_short) != -1);

  REQUIRE(Foccur32(fbfr, fld_short) == 0);

  REQUIRE((Fdelall32(fbfr, fld_short) == -1 && Ferror32 == FNOTPRES));

  Ffree32(fbfr);
}

TEST_CASE("Fnext32-with-value", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_string = Fmkfldid32(FLD_STRING, 10);
  auto fld_long = Fmkfldid32(FLD_LONG, 11);
  auto fld_short = Fmkfldid32(FLD_SHORT, 11);

  for (short i = 0; i < 1; i++) {
    short s = i + 100;
    long l = i + 100;
    std::string str = std::to_string(i + 100);

    REQUIRE(Fchg32(fbfr, fld_string, i, DECONST(str.c_str()), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_long, i, reinterpret_cast<char *>(&l), 0) != -1);
    REQUIRE(Fchg32(fbfr, fld_short, i, reinterpret_cast<char *>(&s), 0) != -1);
  }

  FLDID32 fieldid;
  FLDOCC32 oc;
  char buf[512];
  FLDLEN32 len;

  fieldid = fld_long;
  oc = 0;
  // FIXMEE
  //  len = 1;
  len = sizeof(buf);
  REQUIRE(Fnext32(fbfr, &fieldid, &oc, buf, &len) == 1);
  REQUIRE((fieldid == fld_string && oc == 0));
  REQUIRE(len == 4);

  fieldid = fld_long;
  oc = 0;
  len = sizeof(buf);
  REQUIRE(Fnext32(fbfr, &fieldid, &oc, buf, &len) == 1);
  REQUIRE((fieldid == fld_string && oc == 0));
  REQUIRE(len == 4);
  REQUIRE(std::string(buf, len - 1) == "100");

  fieldid = fld_short;
  oc = 0;
  len = 1;
  REQUIRE(Fnext32(fbfr, &fieldid, &oc, buf, &len) == 1);
  REQUIRE((fieldid == fld_long && oc == 0));
  REQUIRE(len == sizeof(long));

  fieldid = fld_short;
  oc = 0;
  len = sizeof(buf);
  REQUIRE(Fnext32(fbfr, &fieldid, &oc, buf, &len) == 1);
  REQUIRE((fieldid == fld_long && oc == 0));
  REQUIRE(len == sizeof(long));
  REQUIRE(*reinterpret_cast<long *>(buf) == 100);

  fieldid = BADFLDID;
  oc = 0;
  len = 1;
  REQUIRE(Fnext32(fbfr, &fieldid, &oc, buf, &len) == 1);
  REQUIRE((fieldid == fld_short && oc == 0));
  REQUIRE(len == sizeof(short));

  fieldid = BADFLDID;
  oc = 0;
  len = sizeof(buf);
  REQUIRE(Fnext32(fbfr, &fieldid, &oc, buf, &len) == 1);
  REQUIRE((fieldid == fld_short && oc == 0));
  REQUIRE(len == sizeof(short));
  REQUIRE(*reinterpret_cast<short *>(buf) == 100);

  Ffree32(fbfr);
}

TEST_CASE("Fchg32-short", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_short = Fmkfldid32(FLD_SHORT, 10);

  for (short i = 0; i < 3; i++) {
    REQUIRE(Fchg32(fbfr, fld_short, i, reinterpret_cast<char *>(&i), 0) != -1);
  }

  for (short i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_short, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(*reinterpret_cast<short *>(ptr) == i);
  }

  Ffree32(fbfr);
}

TEST_CASE("Fchg32-long", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_long = Fmkfldid32(FLD_LONG, 10);

  for (long i = 0; i < 3; i++) {
    REQUIRE(Fchg32(fbfr, fld_long, i, reinterpret_cast<char *>(&i), 0) != -1);
  }

  for (long i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_long, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(*reinterpret_cast<long *>(ptr) == i);
  }

  Ffree32(fbfr);
}

TEST_CASE("Fchg32-string", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_string = Fmkfldid32(FLD_STRING, 10);

  for (long i = 0; i < 3; i++) {
    REQUIRE(Fchg32(fbfr, fld_string, i, DECONST(std::to_string(i).c_str()),
                   0) != -1);
  }

  for (long i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_string, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(ptr == std::to_string(i));
  }

  Ffree32(fbfr);
}

TEST_CASE("Fchg32-short-overwrite", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_short = Fmkfldid32(FLD_SHORT, 10);

  for (short i = 0; i < 3; i++) {
    short dummy = 13;
    REQUIRE(Fchg32(fbfr, fld_short, i, reinterpret_cast<char *>(&dummy), 0) !=
            -1);
  }
  for (short i = 0; i < 3; i++) {
    REQUIRE(Fchg32(fbfr, fld_short, i, reinterpret_cast<char *>(&i), 0) != -1);
  }

  for (short i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_short, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(*reinterpret_cast<short *>(ptr) == i);
  }

  Ffree32(fbfr);
}

TEST_CASE("Fchg32-long-overwrite", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_long = Fmkfldid32(FLD_LONG, 10);

  for (long i = 0; i < 3; i++) {
    long dummy = 13;
    REQUIRE(Fchg32(fbfr, fld_long, i, reinterpret_cast<char *>(&dummy), 0) !=
            -1);
  }
  for (long i = 0; i < 3; i++) {
    REQUIRE(Fchg32(fbfr, fld_long, i, reinterpret_cast<char *>(&i), 0) != -1);
  }

  for (long i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_long, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(*reinterpret_cast<long *>(ptr) == i);
  }

  Ffree32(fbfr);
}

TEST_CASE("Fchg32-string-overwrite-shrink", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_string = Fmkfldid32(FLD_STRING, 10);

  for (long i = 0; i < 3; i++) {
    long dummy = 13;
    REQUIRE(Fchg32(fbfr, fld_string, i, DECONST(std::to_string(dummy).c_str()),
                   0) != -1);
  }
  for (long i = 0; i < 3; i++) {
    REQUIRE(Fchg32(fbfr, fld_string, i, DECONST(std::to_string(i).c_str()),
                   0) != -1);
  }

  for (long i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_string, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(ptr == std::to_string(i));
  }

  Ffree32(fbfr);
}

TEST_CASE("Fchg32-string-overwrite-grow", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto fld_string = Fmkfldid32(FLD_STRING, 10);

  for (long i = 0; i < 3; i++) {
    long dummy = 13;
    REQUIRE(Fchg32(fbfr, fld_string, i, DECONST(std::to_string(dummy).c_str()),
                   0) != -1);
  }
  for (long i = 0; i < 3; i++) {
    REQUIRE(Fchg32(fbfr, fld_string, i,
                   DECONST(std::to_string(100 + i).c_str()), 0) != -1);
  }

  for (long i = 0; i < 3; i++) {
    auto ptr = Ffind32(fbfr, fld_string, i, nullptr);
    REQUIRE(ptr != nullptr);
    REQUIRE(ptr == std::to_string(100 + i));
  }

  Ffree32(fbfr);
}

struct FieldSetFixture {
  FLDID32 fld1 = Fmkfldid32(FLD_SHORT, 10);
  FLDID32 fld2 = Fmkfldid32(FLD_SHORT, 11);
  FLDID32 fld3 = Fmkfldid32(FLD_SHORT, 12);

  void set_fields(FBFR32 *fbfr) {
    short s;
    s = 100;
    REQUIRE(Fchg32(fbfr, fld1, 0, reinterpret_cast<char *>(&s), 0) != -1);
    s = 101;
    REQUIRE(Fchg32(fbfr, fld1, 1, reinterpret_cast<char *>(&s), 0) != -1);

    s = 200;
    REQUIRE(Fchg32(fbfr, fld2, 0, reinterpret_cast<char *>(&s), 0) != -1);
    s = 201;
    REQUIRE(Fchg32(fbfr, fld2, 1, reinterpret_cast<char *>(&s), 0) != -1);

    s = 300;
    REQUIRE(Fchg32(fbfr, fld3, 0, reinterpret_cast<char *>(&s), 0) != -1);
    s = 301;
    REQUIRE(Fchg32(fbfr, fld3, 1, reinterpret_cast<char *>(&s), 0) != -1);
  }
};

TEST_CASE_METHOD(FieldSetFixture, "Fconcat32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);
  set_fields(fbfr);
  REQUIRE(Foccur32(fbfr, fld1) == 2);

  auto fbfr2 = Falloc32(100, 100);
  REQUIRE(fbfr2 != nullptr);
  set_fields(fbfr2);
  REQUIRE(Foccur32(fbfr2, fld1) == 2);

  REQUIRE(Fconcat32(fbfr, fbfr2) != -1);

  REQUIRE(Foccur32(fbfr, fld1) == 4);

  char *ptr;

  REQUIRE((ptr = Ffind32(fbfr, fld1, 0, nullptr)) != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 100);
  REQUIRE((ptr = Ffind32(fbfr, fld1, 1, nullptr)) != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 101);
  REQUIRE((ptr = Ffind32(fbfr, fld1, 2, nullptr)) != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 100);
  REQUIRE((ptr = Ffind32(fbfr, fld1, 3, nullptr)) != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 101);

  Ffree32(fbfr);
  Ffree32(fbfr2);
}

TEST_CASE_METHOD(FieldSetFixture, "Fdelete32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  set_fields(fbfr);

  FLDID32 del[] = {fld3, fld1, BADFLDID};
  REQUIRE(Fdelete32(fbfr, del) != -1);

  REQUIRE(Foccur32(fbfr, fld1) == 0);
  REQUIRE(Foccur32(fbfr, fld2) == 2);
  REQUIRE(Foccur32(fbfr, fld3) == 0);

  char *ptr;
  ptr = Ffind32(fbfr, fld2, 0, nullptr);
  REQUIRE(ptr != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 200);
  ptr = Ffind32(fbfr, fld2, 1, nullptr);
  REQUIRE(ptr != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 201);

  Ffree32(fbfr);
}

TEST_CASE_METHOD(FieldSetFixture, "Fproj32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  set_fields(fbfr);

  FLDID32 keep[] = {fld2, BADFLDID};
  REQUIRE(Fproj32(fbfr, keep) != -1);

  REQUIRE(Foccur32(fbfr, fld1) == 0);
  REQUIRE(Foccur32(fbfr, fld2) == 2);
  REQUIRE(Foccur32(fbfr, fld3) == 0);

  char *ptr;
  ptr = Ffind32(fbfr, fld2, 0, nullptr);
  REQUIRE(ptr != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 200);
  ptr = Ffind32(fbfr, fld2, 1, nullptr);
  REQUIRE(ptr != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 201);

  Ffree32(fbfr);
}

TEST_CASE_METHOD(FieldSetFixture, "Fprojcpy32", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  REQUIRE(fbfr != nullptr);

  auto dest = Falloc32(100, 100);
  REQUIRE(dest != nullptr);

  set_fields(fbfr);

  FLDID32 keep[] = {fld2, BADFLDID};
  REQUIRE(Fprojcpy32(dest, fbfr, keep) != -1);

  REQUIRE(Foccur32(dest, fld1) == 0);
  REQUIRE(Foccur32(dest, fld2) == 2);
  REQUIRE(Foccur32(dest, fld3) == 0);

  char *ptr;
  ptr = Ffind32(dest, fld2, 0, nullptr);
  REQUIRE(ptr != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 200);
  ptr = Ffind32(dest, fld2, 1, nullptr);
  REQUIRE(ptr != nullptr);
  REQUIRE(*reinterpret_cast<short *>(ptr) == 201);

  Ffree32(fbfr);
  Ffree32(dest);
}

TEST_CASE("nested fml32", "[fml32]") {
  auto fbfr = (FBFR32 *)tpalloc(DECONST("FML32"), DECONST("*"), 1024);
  auto args = (FBFR32 *)tpalloc(DECONST("FML32"), DECONST("*"), 1024);

  auto ARGS = Fldid32(DECONST("ARGS"));
  auto NAME = Fldid32(DECONST("NAME"));
  auto VALUE = Fldid32(DECONST("VALUE"));

  REQUIRE(Fchg32(args, NAME, 0, DECONST("name1"), 0) != -1);
  REQUIRE(Fchg32(args, VALUE, 0, DECONST("000001"), 0) != -1);
  REQUIRE(Fchg32(fbfr, ARGS, 0, (char *)args, 0) != -1);

  REQUIRE(Fchg32(args, NAME, 0, DECONST("name2"), 0) != -1);
  REQUIRE(Fchg32(args, VALUE, 0, DECONST("000002"), 0) != -1);
  REQUIRE(Fchg32(fbfr, ARGS, 1, (char *)args, 0) != -1);

  FBFR32 *nested;
  REQUIRE((nested = (FBFR32 *)Ffind32(fbfr, ARGS, 0, nullptr)) != nullptr);
  auto *c = reinterpret_cast<char *>(Ffind32(nested, NAME, 0, nullptr));
  REQUIRE(reinterpret_cast<char *>(Ffind32(nested, NAME, 0, nullptr)) ==
          std::string("name1"));
  REQUIRE(reinterpret_cast<char *>(Ffind32(nested, VALUE, 0, nullptr)) ==
          std::string("000001"));
  REQUIRE((nested = (FBFR32 *)Ffind32(fbfr, ARGS, 1, nullptr)) != nullptr);
  REQUIRE(reinterpret_cast<char *>(Ffind32(nested, NAME, 0, nullptr)) ==
          std::string("name2"));
  REQUIRE(reinterpret_cast<char *>(Ffind32(nested, VALUE, 0, nullptr)) ==
          std::string("000002"));
  REQUIRE((nested = (FBFR32 *)Ffind32(fbfr, ARGS, 2, nullptr)) == nullptr);

  Fprint32(fbfr);

  tpfree((char *)args);
  tpfree((char *)fbfr);
}

TEST_CASE("field tables", "[fml32]") {
  REQUIRE(Fldid32(DECONST("FCHAR")) == Fmkfldid32(FLD_CHAR, 11));
  REQUIRE(Fldid32(DECONST("FLONG")) == Fmkfldid32(FLD_LONG, 21));
  REQUIRE(Fldid32(DECONST("FFML32")) == Fmkfldid32(FLD_FML32, 31));
  REQUIRE(Fname32(Fmkfldid32(FLD_CHAR, 11)) == std::string("FCHAR"));
  REQUIRE(Fname32(Fmkfldid32(FLD_LONG, 21)) == std::string("FLONG"));
  REQUIRE(Fname32(Fmkfldid32(FLD_FML32, 31)) == std::string("FFML32"));
}

TEST_CASE("field table reload", "[fml32]") {
  Fidnm_unload32();
  Fnmid_unload32();

  REQUIRE(Fldid32(DECONST("FCHAR")) == Fmkfldid32(FLD_CHAR, 11));
  REQUIRE(Fldid32(DECONST("FLONG")) == Fmkfldid32(FLD_LONG, 21));
  REQUIRE(Fldid32(DECONST("FFML32")) == Fmkfldid32(FLD_FML32, 31));
  REQUIRE(Fname32(Fmkfldid32(FLD_CHAR, 11)) == std::string("FCHAR"));
  REQUIRE(Fname32(Fmkfldid32(FLD_LONG, 21)) == std::string("FLONG"));
  REQUIRE(Fname32(Fmkfldid32(FLD_FML32, 31)) == std::string("FFML32"));
}

TEST_CASE("invalid boolean expr", "[fml32]") {
  REQUIRE((Fboolco32(nullptr) == nullptr && Ferror32 == FEINVAL));
  REQUIRE(
      (Fboolco32(DECONST("NAME == 'John")) == nullptr && Ferror32 == FSYNTAX));
  REQUIRE((Fboolco32(DECONST("foobar == 'John'")) == nullptr &&
           Ferror32 == FBADNAME));
  REQUIRE((Fboolco32(DECONST("FFML32 == 'John'")) == nullptr &&
           Ferror32 == FEBADOP));
  REQUIRE(
      (Fboolco32(DECONST("NAME <> 'John'")) == nullptr && Ferror32 == FSYNTAX));
  REQUIRE((Fboolco32(DECONST("NAME[-1] == 'John'")) == nullptr &&
           Ferror32 == FSYNTAX));
  REQUIRE((Fboolco32(DECONST("NAME %% 1")) == nullptr && Ferror32 == FSYNTAX));
  REQUIRE(
      (Fboolco32(DECONST("1 %% 'John'")) == nullptr && Ferror32 == FSYNTAX));
}

TEST_CASE("boolean expr", "[fml32]") {
  char *tree;

  REQUIRE((tree = Fboolco32(DECONST("EMPID[2] %% '123.*' && AGE < 32"))) !=
          nullptr);
  Fboolpr32(tree, stdout);
  free(tree);

  REQUIRE((tree = Fboolco32(DECONST("NAME[?] == 'dog'"))) != nullptr);
  Fboolpr32(tree, stdout);
  free(tree);
}

TEST_CASE("boolean eval", "[fml32]") {
  auto fbfr = Falloc32(100, 100);
  char *tree;

  REQUIRE((tree = Fboolco32(DECONST("1 == 1"))) != nullptr);
  REQUIRE(Fboolev32(fbfr, tree) == 1);
  free(tree);
  REQUIRE((tree = Fboolco32(DECONST("1 != 1"))) != nullptr);
  REQUIRE(Fboolev32(fbfr, tree) == 0);
  free(tree);

  Ffree32(fbfr);
}