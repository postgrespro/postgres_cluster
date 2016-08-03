#override CC := clang
override CFLAGS += -fpic -Wall -Wfatal-errors -O0 -g -pedantic -std=c99 -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE
override CPPFLAGS += -I. -Iinclude #-DDEBUG
override SERVER_LDFLAGS += -Llib -lraft -ljansson

AR = ar
ARFLAGS = -cru

.PHONY: all clean bindir objdir libdir

lib/libraft.a: obj/raft.o obj/util.o | libdir objdir
	$(AR) $(ARFLAGS) lib/libraft.a obj/raft.o obj/util.o

all: lib/libraft.a bin/server bin/client
	@echo Done.

bin/server: obj/server.o lib/libraft.a | bindir objdir
	$(CC) -o bin/server $(CFLAGS) $(CPPFLAGS) \
		obj/server.o $(SERVER_LDFLAGS)

bin/client: obj/client.o obj/timeout.o | bindir objdir
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

obj/%.o: src/%.c | objdir
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

obj/%.o: example/%.c | objdir
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

bindir:
	mkdir -p bin

objdir:
	mkdir -p obj

libdir:
	mkdir -p lib

clean:
	rm -rfv bin obj lib
