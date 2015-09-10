CC=clang -DTEST -DNODELAY
CFLAGS=-g -Wall -I"../../../src/include"

all: lib/libdtm.a bin/example bin/bench bin/redbench

bin/bench: bindir lib/libdtm.a src/bench.c
	$(CC) $(CFLAGS) -o bin/bench -Iinclude -Llib src/bench.c -ldtm

bin/redbench: bindir lib/libdtm.a src/redbench.c
	$(CC) $(CFLAGS) -o bin/redbench -Iinclude -Llib src/redbench.c -ldtm -lhiredis -I"/usr/local/include/hiredis/"

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
