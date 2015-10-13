CC=gcc
CFLAGS=-g -O2 -Wall -Iinclude -D_LARGEFILE64_SOURCE # -DDEBUG
SOCKHUB_PREFIX=../sockhub
SOCKHUB_CFLAGS=-I"$(SOCKHUB_PREFIX)"
SOCKHUB_LDFLAGS=-lsockhub -L"$(SOCKHUB_PREFIX)"

SYSTEM=$(shell uname -s)
ifeq ($(SYSTEM),Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
endif

.PHONY: all clean check bindir objdir

all: bin/dtmd
	@echo Done.
	@echo Feel free to run the tests with \'make check\'.

bin/dtmd: obj/server.o obj/main.o obj/clog.o obj/clogfile.o obj/util.o obj/transaction.o obj/snapshot.o | bindir objdir
	$(CC) -o bin/dtmd $(CFLAGS) \
		obj/server.o obj/main.o \
		obj/clog.o obj/clogfile.o obj/util.o obj/transaction.o \
		obj/snapshot.o \
		$(SOCKHUB_LDFLAGS)

obj/server.o: src/server.c | objdir
	$(CC) -c -o obj/server.o $(CFLAGS) $(SOCKHUB_CFLAGS) src/server.c

check: bin/util-test bin/clog-test
	./check.sh util clog

obj/%.o: src/%.c | objdir
	$(CC) $(CFLAGS) -c -o $@ $<

bin/util-test: obj/util-test.o obj/util.o | bindir
	$(CC) -o bin/util-test $(CFLAGS) obj/util-test.o obj/util.o

bin/clog-test: obj/clog-test.o obj/clog.o obj/clogfile.o obj/util.o | bindir
	$(CC) -o bin/clog-test $(CFLAGS) obj/clog-test.o obj/clog.o obj/clogfile.o obj/util.o

bindir:
	mkdir -p bin

objdir:
	mkdir -p obj

clean:
	rm -rfv bin obj test.log
