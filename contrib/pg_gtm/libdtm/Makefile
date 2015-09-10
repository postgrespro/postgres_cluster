CC=clang
CFLAGS=-g -Wall -I"../../../src/include"

all: lib/libdtm.a bin/example bin/perf

bin/perf: bindir lib/libdtm.a src/perf.c
	$(CC) $(CFLAGS) -o bin/perf -Iinclude -Llib src/perf.c -ldtm

bin/example: bindir lib/libdtm.a src/example.c
	$(CC) $(CFLAGS) -o bin/example -Iinclude -Llib src/example.c -ldtm

lib/libdtm.a: libdir obj/libdtm.o
	ar rcs lib/libdtm.a obj/libdtm.o

obj/libdtm.o: objdir src/libdtm.c
	$(CC) $(CFLAGS) -c -o obj/libdtm.o -Iinclude src/libdtm.c

libdir:
	mkdir -p lib

objdir:
	mkdir -p obj

bindir:
	mkdir -p bin

clean:
	rm -rfv lib obj bin
