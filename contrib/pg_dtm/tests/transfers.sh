#!/bin/sh

go run transfers.go \
	-d 'dbname=postgres port=5432' \
	-d 'dbname=postgres port=5433' \
	-v \
	-m \
	-u 10000 \
	-w 10 \
	-g
