#!/bin/sh
clang -g -o server server.c -D_LARGEFILE64_SOURCE -L../../sockhub -I../../sockhub -I../include -lsockhub -DDEBUG -O0
