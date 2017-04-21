#!/usr/bin/env bash

set -e -x

export CFLAGS="-O0"

cd postgresql

./configure --enable-debug --enable-cassert --enable-tap-tests --enable-depend 

make -j4

make check

