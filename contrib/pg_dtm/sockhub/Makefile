CC = gcc
CFLAGS = -c -I. -Wall -O0 -g
LD = $(CC)
LDFLAGS = -g
AR = ar
ARFLAGS = -cru

all: sockhub libsockhub.a

sockhup.o: sockhub.c sockhub.h
	$(CC) $(CFLAGS) sockhub.c

sockhup_main.o: sockhub_main.c sockhub.h
	$(CC) $(CFLAGS) sockhub_main.c

libsockhub.a: sockhub.o
	rm -f libsockhub.a
	$(AR) $(ARFLAGS) libsockhub.a sockhub.o

sockhub: sockhub_main.o libsockhub.a
	$(LD) $(LDFLAGS) -o sockhub sockhub_main.o libsockhub.a

clean:
	rm *.o *.a