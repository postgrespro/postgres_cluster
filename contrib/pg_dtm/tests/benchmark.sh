#!/bin/sh

go run benchmark.go \
	-d 'dbname=postgres port=5432' \
	-d 'dbname=postgres port=5433' \
	-m \
	-w 4 \
	-r 4 \
    -t 10 
