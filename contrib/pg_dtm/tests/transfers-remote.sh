#!/bin/sh

if [ ! -f ./transfers.linux ]; then
    GOOS=linux GOARCH=386 go build -o ./transfers.linux ./transfers.go
fi

ansible master-workers -i farms/sai -m copy -a "src=transfers.linux dest=~/transfers"

# ssh -p4208 br.theor.mephi.ru "./transfers -d 'dbname=postgres port=5432' -d 'dbname=postgres port=5433' -g -m"

ansible single-worker -i farms/mephi -a "./transfers -d 'dbname=postgres port=5432' -d 'dbname=postgres port=5433' -g -m"




ssh s.kelvich@158.250.29.10 "./transfers -d 'host=158.250.29.10 dbname=postgres' -d 'host=158.250.29.9 dbname=postgres' -d 'host=158.250.29.8 dbname=postgres' -g -m -w 64"
