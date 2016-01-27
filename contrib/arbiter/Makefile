top_builddir = ../..
include $(top_builddir)/src/Makefile.global

override CFLAGS += -fPIC -O0
override CPPFLAGS += -I. -Iinclude -Iapi -D_LARGEFILE64_SOURCE

AR = ar
ARFLAGS = -cru

SOCKHUB_PREFIX=sockhub
SOCKHUB_CFLAGS=-I"$(SOCKHUB_PREFIX)"

SYSTEM=$(shell uname -s)
ifeq ($(SYSTEM),Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
endif

.PHONY: all clean check bindir objdir libdir

all: bin/arbiter bin/heart lib/libarbiter.a
	@echo Done.
	@echo Feel free to run the tests with \'make check\'.

lib/libarbiter.a: obj/api.o | libdir objdir
	$(AR) $(ARFLAGS) lib/libarbiter.a obj/api.o

bin/arbiter: obj/server.o obj/raft.o obj/main.o obj/clog.o obj/clogfile.o obj/util.o obj/transaction.o obj/snapshot.o obj/ddd.o | bindir objdir
	$(CC) -o bin/arbiter $(CFLAGS) $(CPPFLAGS) \
		obj/server.o obj/raft.o obj/main.o \
		obj/clog.o obj/clogfile.o obj/util.o obj/transaction.o \
		obj/snapshot.o obj/ddd.o

bin/heart: obj/heart.o obj/raft.o obj/util.o | bindir objdir
	$(CC) -o bin/heart $(CFLAGS) $(CPPFLAGS) \
		obj/heart.o obj/raft.o obj/util.o \
		obj/snapshot.o \
		$(SOCKHUB_LDFLAGS)

obj/api.o: api/arbiter.c | objdir
	$(CC) -c -o obj/api.o $(CFLAGS) $(CPPFLAGS) $(SOCKHUB_CFLAGS) api/arbiter.c

obj/server.o: src/server.c | objdir
	$(CC) -c -o obj/server.o $(CFLAGS) $(CPPFLAGS) $(SOCKHUB_CFLAGS) src/server.c

check: bin/util-test bin/clog-test
	./check.sh util clog

obj/%.o: src/%.c | objdir
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

bin/util-test: obj/util-test.o obj/util.o | bindir
	$(CC) -o bin/util-test $(CFLAGS) $(CPPFLAGS) obj/util-test.o obj/util.o

bin/clog-test: obj/clog-test.o obj/clog.o obj/clogfile.o obj/util.o | bindir
	$(CC) -o bin/clog-test $(CFLAGS) $(CPPFLAGS) obj/clog-test.o obj/clog.o obj/clogfile.o obj/util.o

bindir:
	mkdir -p bin

objdir:
	mkdir -p obj

libdir:
	mkdir -p lib

clean:
	rm -rfv bin obj lib test.log
