#!/bin/bash

. ./paths.sh
U=`whoami`

pkill -e postgres
ulimit -c unlimited

rm master.log
rm slave.log
rm -rf PGDATA_Master
mkdir PGDATA_Master
rm -rf PGDATA_Slave
mkdir PGDATA_Slave

initdb -D PGDATA_Master
initdb -D PGDATA_Slave
	
echo "shared_preload_libraries = 'execplan'" >> PGDATA_Master/postgresql.conf
echo "lc_messages='en_US.utf8'" >> PGDATA_Master/postgresql.conf
echo "shared_preload_libraries = 'execplan'" >> PGDATA_Slave/postgresql.conf
echo "lc_messages='en_US.utf8'" >> PGDATA_Slave/postgresql.conf
echo "pargres.node = 1" >> PGDATA_Slave/postgresql.conf

pg_ctl -c -o "-p 5432" -D PGDATA_Master -l master.log start
pg_ctl -c -o "-p 5433" -D PGDATA_Slave -l slave.log start

createdb -p 5432 $U
createdb -p 5433 $U

# Prepare plan execution
psql -p 5432 -c "CREATE EXTENSION execplan;"
psql -p 5433 -c "CREATE EXTENSION execplan;"

# Basic test on remote plan execution
psql -p 5432 -c "SELECT proname, oid FROM pg_proc WHERE proname LIKE 'pg_exec%';"
psql -p 5432 -c "CREATE TABLE t1 (id	Serial, b	INT, PRIMARY KEY(id));"
psql -p 5432 -f test.sql
psql -p 5432 -c "SELECT * FROM t1;"
psql -p 5433 -c "SELECT * FROM t1;"

# ---------------------------
# Table Oid localization test
# ---------------------------

# Create table t2 only on the slave for modelling of Oid assign switching
psql -p 5433 -c "CREATE TABLE t2 (id Serial, b INT, PRIMARY KEY(id));"
psql -p 5432 -c "CREATE TABLE t3 (id Serial, b INT, PRIMARY KEY(id));"

# delete table for exclude Oid from database
psql -p 5433 -c "DROP TABLE t2;"

# Show the slave and master databases Oid difference for the table t3
psql -p 5432 -c "select oid, relname from pg_class WHERE relname = 't3';"
psql -p 5433 -c "select oid, relname from pg_class WHERE relname = 't3';"

# In the case of non-localize table oid this will cause an error
psql -p 5432 -c "SELECT count(*) FROM t3 AS talias;"

# ------------
# pgbench test
# ------------

# Preparation activities
pgbench -p 5432 -i -s 1
pgbench -p 5433 -i -s 1 # re-create pgbench test tables with different oids and fill pgbench_accounts
psql -p 5432 -c "select oid, relname from pg_class WHERE relname LIKE 'pgbench%';"
psql -p 5433 -c "select oid, relname from pg_class WHERE relname LIKE 'pgbench%';"
psql -p 5433 -c "explain SELECT abalance FROM pgbench_accounts WHERE aid = 1;"
psql -p 5432 -c "SELECT count(*) FROM pgbench_accounts;"
psql -p 5433 -c "SELECT count(*) FROM pgbench_accounts;"

pgbench -p 5432 -T 10 -M prepared

pg_ctl -D PGDATA_Master stop
pg_ctl -D PGDATA_Slave stop

