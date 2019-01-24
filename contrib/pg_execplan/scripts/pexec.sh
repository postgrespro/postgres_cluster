#!/bin/bash

# export LC_ALL=C
# export LANGUAGE="en_US:en"

. ./paths.sh
U=`whoami`
remoteSrvName=fdwremote

pkill -9 postgres || true
ulimit -c unlimited

rm master.log
rm slave.log
rm -rf PGDATA_Master
mkdir PGDATA_Master
rm -rf PGDATA_Slave
mkdir PGDATA_Slave

initdb -D PGDATA_Master
initdb -D PGDATA_Slave

echo "shared_preload_libraries = 'postgres_fdw, repeater'" >> PGDATA_Master/postgresql.conf
echo "repeater.host = 'localhost'" >> PGDATA_Master/postgresql.conf
echo "repeater.port = 5433" >> PGDATA_Master/postgresql.conf
echo "repeater.fdwname = '$remoteSrvName'" >> PGDATA_Master/postgresql.conf

pg_ctl -c -o "-p 5433" -D PGDATA_Slave -l slave.log start
pg_ctl -c -o "-p 5432" -D PGDATA_Master -l master.log start

createdb -p 5433 $U
createdb -p 5432 $U

# Prepare plan execution
#psql -p 5432 -f initial.sql
psql -p 5432 -c "CREATE EXTENSION postgres_fdw;"
psql -p 5432 -c "create server $remoteSrvName foreign data wrapper postgres_fdw options (port '5433', use_remote_estimate 'on');"
psql -p 5432 -c "create user mapping for current_user server $remoteSrvName;"
psql -p 5432 -c "CREATE USER regress_seq_user WITH PASSWORD '12345';"
psql -p 5433 -c "CREATE USER regress_seq_user WITH PASSWORD '12345';"
psql -p 5432 -c "CREATE USER MAPPING FOR regress_seq_user SERVER $remoteSrvName OPTIONS (user 'regress_seq_user', password '12345');"

# Now we are ready to enable repeater extension
psql -p 5432 -c "CREATE EXTENSION repeater;"

# Create table t2 only on the slave for modelling of Oid assign switching
psql -p 5433 -c "CREATE TABLE public.t2 (id Serial, b INT, PRIMARY KEY(id));"
psql -p 5432 -c "CREATE TABLE public.t3 (id Serial, b INT, PRIMARY KEY(id));"

# delete table for exclude Oid from database
psql -p 5433 -c "DROP TABLE public.t2;"

# Basic test on remote plan execution
psql -p 5432 -c "CREATE TABLE public.t1 (id	Serial, b	INT, PRIMARY KEY(id));"

psql -p 5432 -f test.sql
psql -p 5432 -c "SELECT * FROM public.t1;"
psql -p 5433 -c "SELECT * FROM public.t1;"

# Show the slave and master databases Oid difference for the table t3
psql -p 5432 -c "select oid, relname from pg_class WHERE relname = 't3';"
psql -p 5433 -c "select oid, relname from pg_class WHERE relname = 't3';"

# ------------
# pgbench test
# ------------

# Preparation activities
pgbench -p 5432 -i -s 1
pgbench -p 5433 -i -s 1 # re-create pgbench test tables with different oids and fill pgbench_accounts
psql -p 5432 -c "select oid, relname from pg_class WHERE relname LIKE 'pgbench%';"
psql -p 5433 -c "select oid, relname from pg_class WHERE relname LIKE 'pgbench%';"
psql -p 5432 -c "SELECT count(*) FROM pgbench_accounts;"
psql -p 5433 -c "SELECT count(*) FROM pgbench_accounts;"

pgbench -p 5432 -T 10 -M prepared

# Some simple checksum
psql -p 5432 -c "SELECT avg(abalance) FROM pgbench_accounts;"
psql -p 5433 -c "SELECT avg(abalance) FROM pgbench_accounts;"

pg_ctl -D PGDATA_Slave stop
pg_ctl -D PGDATA_Master stop

