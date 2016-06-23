#override CC := clang
override CFLAGS += -fpic -Wall -Wfatal-errors -O0 -g -pedantic -std=c99 -D_POSIX_C_SOURCE=200112L
override CPPFLAGS += -I. -Iinclude #-DDEBUG
override HEART_LDFLAGS += -Llib -lraft -ljansson

AR = ar
ARFLAGS = -cru

.PHONY: all clean bindir objdir libdir

lib/libraft.a: obj/raft.o obj/util.o | libdir objdir
	$(AR) $(ARFLAGS) lib/libraft.a obj/raft.o obj/util.o

all: lib/libraft.a bin/heart
	@echo Done.

bin/heart: obj/heart.o lib/libraft.a | bindir objdir
	$(CC) -o bin/heart $(CFLAGS) $(CPPFLAGS) \
		obj/heart.o $(HEART_LDFLAGS)

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
