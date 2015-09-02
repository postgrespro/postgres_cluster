CC=clang
CFLAGS=-g -Wall -I"../../../src/include"


all: lib/libdtm.a bin/example

bin/example: bindir lib/libdtm.a src/example.c
	$(CC) $(CFLAGS) -o bin/example -Iinclude -Llib src/example.c -ldtm

lib/libdtm.a: libdir obj/dtm.o
	ar rcs lib/libdtm.a obj/dtm.o

obj/dtm.o: objdir src/dtm.c
	$(CC) $(CFLAGS) -c -o obj/dtm.o -Iinclude src/dtm.c

libdir:
	mkdir -p lib

objdir:
	mkdir -p obj

bindir:
	mkdir -p bin

clean:
	rm -rfv lib obj bin
