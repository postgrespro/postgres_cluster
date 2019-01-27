#!/bin/bash

# Script for the plan passing between separate instances
U=`whoami`

# Paths
PGINSTALL=`pwd`/tmp_install/
LD_LIBRARY_PATH=$PGINSTALL/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
export PATH=$PGINSTALL/bin:$PATH

pkill -9 postgres || true
sleep 1
rm -rf $PGINSTALL || true
rm -rf PGDATA_Master || true
rm -rf PGDATA_Slave || true
rm -rf master.log || true
rm -rf slave.log || true

# Building project
make > /dev/null
make -C contrib > /dev/null
make install > /dev/null
make -C contrib install > /dev/null

mkdir PGDATA_Master
mkdir PGDATA_Slave
initdb -D PGDATA_Master
initdb -D PGDATA_Slave
echo "shared_preload_libraries = 'postgres_fdw, pg_execplan'" >> PGDATA_Master/postgresql.conf
echo "shared_preload_libraries = 'postgres_fdw, pg_execplan'" >> PGDATA_Slave/postgresql.conf

pg_ctl -w -D PGDATA_Master -o "-p 5432" -l master.log start
pg_ctl -w -D PGDATA_Slave -o "-p 5433" -l slave.log start
createdb $U -p 5432
createdb $U -p 5433

psql -p 5432 -c "CREATE EXTENSION postgres_fdw;"
psql -p 5433 -c "CREATE EXTENSION postgres_fdw;"
psql -p 5432 -c "CREATE EXTENSION pg_execplan;"
psql -p 5433 -c "CREATE EXTENSION pg_execplan;"

# shift oids
psql -p 5433 -c "CREATE TABLE t0 (id int);"
psql -p 5433 -c "DROP TABLE t0;"

#create database objects for check of oid switching
psql -p 5432 -f contrib/pg_execplan/tests/create_objects.sql
psql -p 5433 -f contrib/pg_execplan/tests/create_objects.sql

# TEST ON RELOID and TYPEOID objects.
psql -p 5432 -c "SELECT pg_store_query_plan('../test.txt', 'SELECT * FROM t1;');"
psql -p 5433 -c "SELECT pg_exec_query_plan('../test.txt');"

