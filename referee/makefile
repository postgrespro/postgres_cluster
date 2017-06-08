CC=gcc
CFLAGS=-g -Wall -O0 -pthread 

all: referee

referee: referee.c
	$(CC) $(CFLAGS) -o referee referee.c -lpq

clean:
	rm -f referee
