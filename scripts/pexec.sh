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
#exit
psql -p 5432 -c "CREATE EXTENSION execplan;"
psql -p 5433 -c "CREATE EXTENSION execplan;"
psql -p 5432 -c "SELECT proname, oid FROM pg_proc WHERE proname LIKE 'pg_exec%';"
psql -p 5432 -c "CREATE TABLE t1 (id	Serial, b	INT, PRIMARY KEY(id));"

#psql -p 5433 -c "CREATE TABLE t1 (id	Serial, b	INT, PRIMARY KEY(id));"

psql -p 5432 -f test.sql

psql -p 5432 -c "SELECT * FROM t1;"
psql -p 5433 -c "SELECT * FROM t1;"

# pgbench test
#pgbench -p 5433 -i -s 1
# If we will plan query at master consistency will be guaranteed
#psql -p 5433 -c "DELETE FROM pgbench_accounts;"
#psql -p 5433 -c "DELETE FROM pgbench_branches;"
#psql -p 5433 -c "DELETE FROM pgbench_tellers;"
psql -p 5433 -c "CREATE TABLE t2 (id Serial, b INT, PRIMARY KEY(id));"
pgbench -p 5432 -i -s 1

psql -c "select oid, relname from pg_class WHERE relname LIKE 'pgbench%';"
psql -p 5433 -c "select oid, relname from pg_class WHERE relname LIKE 'pgbench%';"
# Copy all tuples from pgbench_accounts
#psql -p 5433 -c "INSERT INTO pgbench_accounts (SELECT * FROM pgbench_accounts) ON CONFLICT (aid) DO UPDATE SET abalance=pgbench_accounts.abalance;"
psql -p 5432 -c "INSERT INTO pgbench_accounts (SELECT * FROM pgbench_accounts) ON CONFLICT (aid) DO NOTHING;"

#pgbench -t 1000 -j 4 -c 4

psql -p 5433 -c "SELECT count(*) FROM pgbench_accounts;"

#pg_ctl -D PGDATA_Master stop
pg_ctl -D PGDATA_Slave stop

