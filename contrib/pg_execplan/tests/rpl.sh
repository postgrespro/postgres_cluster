#!/bin/bash

# This script to pass some plans between separate instances.
U=`whoami`
export LC_ALL=C
export LANGUAGE="en_US:en"

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
initdb -D PGDATA_Master -E UTF8 --locale=C
initdb -D PGDATA_Slave -E UTF8 --locale=C
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
psql -p 5433 -c "SELECT current_schemas(true);"

# TEST ON RELOID and TYPEOID objects.
psql -p 5432 -c "SELECT pg_store_query_plan('../test.txt', 'SELECT * FROM tests.t1;');"
psql -p 5433 -c "SELECT pg_exec_stored_plan('../test.txt');"

psql -p 5432 -c "SELECT pg_store_query_plan('../test.txt', 'SELECT tests.select1(42);');"
psql -p 5433 -c "SELECT pg_exec_stored_plan('../test.txt');"
psql -p 5432 -c "SELECT pg_exec_stored_plan('../test.txt');"

psql -p 5432 -c "SELECT * FROM tests.t2;"
psql -p 5433 -c "SELECT * FROM tests.t2;"

# COLLOID ----------------------------------------------------------------------
# Check on different oids
psql -p 5432 -c "SELECT oid, * FROM pg_collation WHERE collname LIKE 'test%';"
psql -p 5433 -c "SELECT oid, * FROM pg_collation WHERE collname LIKE 'test%';"

psql -p 5432 -c "SELECT pg_store_query_plan('../test.txt', 'SELECT max(id) FROM tests.ttest1 WHERE a < b COLLATE tests.test1');"
psql -p 5433 -c "SELECT pg_exec_stored_plan('../test.txt');"

# OPEROID ----------------------------------------------------------------------
# Check on different oids
psql -p 5432 -c "SELECT oid, oprname, oprnamespace FROM pg_operator WHERE oprname LIKE '###';"
psql -p 5433 -c "SELECT oid, oprname, oprnamespace FROM pg_operator WHERE oprname LIKE '###';"

# Test
psql -p 5432 -c "SELECT pg_store_query_plan('../test.txt', 'SELECT id ### 1 FROM tests.ttest1;');"
psql -p 5433 -c "SELECT pg_exec_stored_plan('../test.txt');"
