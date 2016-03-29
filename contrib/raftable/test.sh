#!/bin/sh
CC=clang
set -ex
$CC -o test-blockmem test-blockmem.c blockmem.c
./test-blockmem
