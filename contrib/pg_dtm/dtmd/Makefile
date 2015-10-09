CC=gcc
CFLAGS=-g -Wall -Iinclude -D_LARGEFILE64_SOURCE # -DDEBUG
LIBUV_PREFIX=$(HOME)/libuv-build
LIBUV_CFLAGS=-I"$(LIBUV_PREFIX)/include" -L"$(LIBUV_PREFIX)/lib"
LIBUV_LDFLAGS=-luv -pthread -lrt

SYSTEM=$(shell uname -s)
ifeq ($(SYSTEM),Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
endif

.PHONY: all clean check bindir objdir

all: bin/dtmd
	@echo Done.
	@echo Feel free to run the tests with \'make check\'.

bin/dtmd: obj/eventwrap.o obj/main.o obj/parser.o obj/clog.o obj/clogfile.o obj/util.o obj/transaction.o obj/snapshot.o | bindir objdir
	$(CC) -o bin/dtmd $(CFLAGS) $(LIBUV_CFLAGS) \
		obj/eventwrap.o obj/main.o obj/parser.o \
		obj/clog.o obj/clogfile.o obj/util.o obj/transaction.o \
		obj/snapshot.o \
		$(LIBUV_LDFLAGS)

obj/eventwrap.o: src/eventwrap.c | objdir
	$(CC) -c -o obj/eventwrap.o $(CFLAGS) $(LIBUV_CFLAGS) src/eventwrap.c

check: bin/util-test bin/clog-test bin/parser-test
	./check.sh util parser clog

obj/%.o: src/%.c | objdir
	$(CC) $(CFLAGS) -c -o $@ $<

bin/util-test: obj/util-test.o obj/util.o | bindir
	$(CC) -o bin/util-test $(CFLAGS) obj/util-test.o obj/util.o

bin/clog-test: obj/clog-test.o obj/clog.o obj/clogfile.o obj/util.o | bindir
	$(CC) -o bin/clog-test $(CFLAGS) obj/clog-test.o obj/clog.o obj/clogfile.o obj/util.o

bin/parser-test: obj/parser-test.o obj/parser.o | bindir
	$(CC) -o bin/parser-test $(CFLAGS) obj/parser-test.o obj/parser.o

bindir:
	mkdir -p bin

objdir:
	mkdir -p obj

clean:
	rm -rfv bin obj test.log
