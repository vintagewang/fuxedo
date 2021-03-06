ifndef TUXDIR
$(error TUXDIR is not set)
endif

export PATH:=$(TUXDIR)/bin:$(PATH)
export LD_LIBRARY_PATH:=$(TUXDIR)/lib:$(LD_LIBRARY_PATH)
export TUXCONFIG:=$(CURDIR)/tuxconfig

check: server mtserver failserver client defserver defmtserver tuxconfig tpreturn_client tpforward_client
	-rm -f ULOG.*
	!(tmboot -zinvalid_option)
	!(tmshutdown -zinvalid_option)
	!(tmipcrm -zinvalid_option)
	!(tmloadcf -zinvalid_option)
	!(tmunloadcf -zinvalid_option)
	-tmloadcf -h
	-tmunloadcf -h
	-tmipcrm -y
	echo "n" | tmipcrm
	echo "y" | tmipcrm
	tmunloadcf > ubbconfig2
	tmloadcf -y ubbconfig2
	!(TUXCONFIG= tmunloadcf > ubbconfig2)
	!(TUXCONFIG= tmloadcf -y ubbconfig2)
	tmboot -y
	./client
	!(./tpforward_client)
	!(./tpreturn_client)
	tmshutdown -y
	grep -q ':server\..*:TEST: tpsvrinit called' ULOG.*
	! grep -q ':server\..*:TEST: tpsvrthrinit called' ULOG.*
	grep -q ':server\..*:TEST: SERVICE called' ULOG.*
	! grep -q ':server\..*:TEST: tpsvrthrdone called' ULOG.*
	grep -q ':server\..*:TEST: tpsvrdone called' ULOG.*
	grep -q ':mtserver\..*:TEST: tpsvrinit called' ULOG.*
	grep -q ':mtserver\..*:TEST: tpsvrthrinit called' ULOG.*
	grep -q ':server\..*:TEST: SERVICE called' ULOG.*
	grep -q ':mtserver\..*:TEST: MTSERVICE called' ULOG.*
	grep -q ':defserver\..*:TEST: DEFSERVICE called' ULOG.*
	grep -q ':defmtserver\..*:TEST: DEFMTSERVICE called' ULOG.*
	grep -q ':mtserver\..*:TEST: tpsvrdone called' ULOG.*
	grep -q ':mtserver\..*:TEST: tpsvrthrdone called' ULOG.*
	grep -q ':failserver\..*:TEST: tpsvrinit called' ULOG.*
	! grep -q ':failserver\..*:TEST: tpsvrthrinit called' ULOG.*
	! grep -q ':failserver\..*:TEST: tpsvrdone called' ULOG.*
	! grep -q ':failserver\..*:TEST: tpsvrthrdone called' ULOG.*

ubbconfig: ubbconfig.in
	cat $< \
          | sed s:@TUXDIR@:$(TUXDIR):g \
          | sed s:@UNAME@:`uname -n`:g \
          | sed s:@CURDIR@:$(CURDIR):g > $@

tuxconfig: ubbconfig
	tmloadcf -y $<

server: server.c
	buildserver -o $@ -f $< -s SERVICE -v -f "-Wl,--no-as-needed"

defserver: defserver.c
	buildserver -o $@ -f $< -s DEFSERVICE -v -f "-Wl,--no-as-needed"

failserver: failserver.c
	buildserver -o $@ -f $< -s FAILSERVICE -v -f "-Wl,--no-as-needed"

mtserver: mtserver.c
	buildserver -t -o $@ -f $< -s MTSERVICE -v -f "-Wl,--no-as-needed"

defmtserver: defmtserver.c
	buildserver -t -o $@ -f $< -s DEFMTSERVICE -v -f "-Wl,--no-as-needed"

client: client.c
	buildclient -o $@ -f $< -v -f "-Wl,--no-as-needed"
tpforward_client: tpforward_client.c
	buildclient -o $@ -f $< -v -f "-Wl,--no-as-needed"
tpreturn_client: tpreturn_client.c
	buildclient -o $@ -f $< -v -f "-Wl,--no-as-needed"

clean:
	-rm -f *.o ubbconfig tuxconfig tpreturn_client tpforward_client client server mtserver failserver defserver defmtserver ULOG.* stdout stderr access.*
