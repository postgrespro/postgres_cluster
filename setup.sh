#!/bin/bash

DBNAME=postgres
DBUSER=$USER
HOSTS=hosts.lst
PORT=5432
ARBITER_PORT=4321

while getopts ":d:u:l:a:p:" opt; do
  case $opt in
    p) PORT="$OPTARG"
    ;;
    a) ARBITER_PORT="$OPTARG"
    ;;
    d) DBNAME="$OPTARG"
    ;;
    u) DBUSER="$OPTARG"
    ;;
    h) HOSTS="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
		echo "Supported options:"
		echo "	-p PORT  		postgres server port ($PORT)"
		echo "	-a PORT  		multimaster arbiter port ($ARBITER_PORT)"
		echo "	-d DBNAME		postgres database name ($DBNAME)"
		echo "	-u DBUSER		postgres database user ($USER)"
		echo "	-h FILE  		file with list of hosts ($HOSTS)"
		exit 1
    ;;
  esac
done

echo "Setup multimaster dbname=$DBNAME user=$DBUSER hosts=$HOSTS port=$PORT arbiter_port=$ARBITER_PORT"

rm -f nodes.lst # nodes.lst will be contructed from 

for host in $HOSTS
do
    echo "dbname=$DBNAME user=$DBUSER host=$host port=$PORT arbiter_port=$ARBITER_PORT sslmode=disable" >> nodes.lst 
done

i=1
for host in $HOSTS
do
	echo "Setup database at node $host"
	ssh $host "rm -fr node$i node$i.log ; initdb node$i"	
	if [ "$DBNAME" != "postgres" ]
	then
		ssh $host "pg_ctl -w -D node$i -l node$i.log start; createdb $DBNAME; pg_ctl -w -D node$i -l node$i.log stop"
	fi
	scp pg_hba.conf.template $host:node$i/pg_hba.conf
	scp nodes.lst n$host:node$i
	scp postgresql.conf.template $host:node$i/postgresql.conf
	ssh $host "echo port=$PORT >> node$i/postgresql.conf"
	ssh $host "echo multimaster.arbiter_port=$ARBITER_PORT >> node$i/postgresql.conf"
	((i++))
done

i=1
for host in $HOSTS
do
	ssh $host "pg_ctl -D node$i -l node$i.log start"
	((i++))
done

sleep 5

echo Done
