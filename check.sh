#!/bin/sh
set -e -x

export CFLAGS="-O0"

./configure --enable-debug --enable-cassert --enable-tap-tests --enable-depend 

make check


