#!/bin/sh
go run transfers.go \
	-d 'dbname=postgres port=5432' \
	-d 'dbname=postgres port=5433' \
	-d 'dbname=postgres port=5434' \
	-g
