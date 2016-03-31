#override CC := clang
override CFLAGS += -Wfatal-errors -O0 -g
override CPPFLAGS += -I. -Iinclude -DDEBUG
override HEART_LDFLAGS += -Llib -lraft -ljansson

AR = ar
ARFLAGS = -cru

.PHONY: all clean bindir objdir libdir

all: lib/libraft.a bin/heart
	@echo Done.

lib/libraft.a: obj/raft.o obj/util.o | libdir objdir
	$(AR) $(ARFLAGS) lib/libraft.a obj/raft.o obj/util.o

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
