#!/bin/bash

DBNAME=postgres
DBUSER=$USER
N_HOSTS=3
PORT=5432
ARBITER_PORT=4321

while getopts ":d:u:n:a:p:" opt; do
  case $opt in
    p) PORT="$OPTARG"
    ;;
    a) ARBITER_PORT="$OPTARG"
    ;;
    d) DBNAME="$OPTARG"
    ;;
    u) DBUSER="$OPTARG"
    ;;
    n) N_HOSTS="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
		echo "Supported options:"
		echo "	-p PORT  		postgres server port ($PORT)"
		echo "	-a PORT  		multimaster arbiter port ($ARBITER_PORT)"
		echo "	-d DBNAME		postgres database name ($DBNAME)"
		echo "	-u DBUSER		postgres database user ($USER)"
		echo "	-n NODES  		number of cluster nodes ($N_HOSTS)"
		exit 1
    ;;
  esac
done

echo "Setup multimaster dbname=$DBNAME user=$DBUSER hosts=$HOSTS port=$PORT arbiter_port=$ARBITER_PORT"

#perform cleanup
rm -fr node? node*.log nodes.lst

for ((i=1;i<=N_HOSTS;i++))
do
	pg_port=$((PORT+i-1))
	mm_port=$((ARBITER_PORT+i-1))
    echo "dbname=$DBNAME user=$DBUSER host=localhost port=$pg_port arbiter_port=$mm_port sslmode=disable" >> nodes.lst 
done

for ((i=1;i<=N_HOSTS;i++))
do
	pg_port=$((PORT+i-1))
	mm_port=$((ARBITER_PORT+i-1))
	echo "Setup database node$i"
	initdb node$i
	if [ "$DBNAME" != "postgres" ]
	then
	    pg_ctl -w -D node$i -l node$i.log start
		createdb $DBNAME
		pg_ctl -w -D node$i -l node$i.log stop
	fi
	cp pg_hba.conf.template node$i/pg_hba.conf
	cp nodes.lst node$i
	cp postgresql.conf.template node$i/postgresql.conf
	echo port=$pg_port >> node$i/postgresql.conf
	echo multimaster.arbiter_port=$mm_port >> node$i/postgresql.conf
done

for ((i=1;i<=N_HOSTS;i++))
do
	pg_ctl -D node$i -l node$i.log start
done

sleep 5

echo Done
