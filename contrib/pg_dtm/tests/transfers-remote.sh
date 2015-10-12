#!/bin/sh

if [ ! -f ./transfers.linux ]; then
    GOOS=linux GOARCH=amd64 go build -o ./transfers ./transfers.go
fi

ansible master-workers -i farms/mephi -m copy -a "src=transfers dest=~/transfers mode=a+x"



ssh -p4207 br.theor.mephi.ru "./transfers -d 'dbname=postgres user=stas port=5432' -d 'dbname=postgres user=stas port=5433' -g -m"


ssh s.kelvich@158.250.29.10 "./transfers -d 'host=158.250.29.10 dbname=postgres' -d 'host=158.250.29.9 dbname=postgres' -d 'host=158.250.29.8 dbname=postgres' -g -w 64"


ssh -p4207 br.theor.mephi.ru "./transfers -d 'host=blade8 dbname=postgres user=stas port=5432' -d 'host=blade7 dbname=postgres user=stas port=5432' -d 'host=blade6 dbname=postgres user=stas port=5432' -d 'host=blade5 dbname=postgres user=stas port=5432' -d 'host=blade4 dbname=postgres user=stas port=5432' -d 'host=blade3 dbname=postgres user=stas port=5432' -g -m"



