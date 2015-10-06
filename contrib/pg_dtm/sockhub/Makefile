CC = gcc
CFLAGS = -c -I. -Wall -O2 -g
LD = $(CC)
LDFLAGS = -g
AR = ar
ARFLAGS = -cru

all: sockhub library tests

library: libsockhub.a 

tests: test-client test-server

sockhup.o: sockhub.c sockhub.h
	$(CC) $(CFLAGS) sockhub.c

sockhup_main.o: sockhub_main.c sockhub.h
	$(CC) $(CFLAGS) sockhub_main.c

libsockhub.a: sockhub.o
	rm -f libsockhub.a
	$(AR) $(ARFLAGS) libsockhub.a sockhub.o

sockhub: sockhub_main.o libsockhub.a
	$(LD) $(LDFLAGS) -o sockhub sockhub_main.o libsockhub.a

test-client.o: test-client.c sockhub.h
	$(CC) $(CFLAGS) test-client.c

test-client: test-client.o libsockhub.a
	$(LD) $(LDFLAGS) -o test-client test-client.o libsockhub.a

test-server.o: test-server.c sockhub.h
	$(CC) $(CFLAGS) test-server.c

test-server: test-server.o libsockhub.a
	$(LD) $(LDFLAGS) -o test-server test-server.o libsockhub.a

clean:
	rm -f *.o *.a

tgz: clean
	cd .. ; tar cvzf sockhub.tgz sockhub 
