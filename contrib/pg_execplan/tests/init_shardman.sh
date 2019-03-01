#!/bin/bash

# ------------------------------------------------------------------------------
# This script performs initialization of fdw+partitioning infrastructure for
# parallel query execution purposes.
# ------------------------------------------------------------------------------

export LC_ALL=C
export LANGUAGE="en_US:en"

# Paths
PGINSTALL=`pwd`/tmp_install/
SCRIPTS=`pwd`/contrib/pg_execplan/tests
LD_LIBRARY_PATH=$PGINSTALL/lib
remoteSrvName=fdwremote
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
export PATH=$PGINSTALL/bin:$PATH
export PGDATABASE=test_base

pkill -9 postgres || true
sleep 1
rm -rf $PGINSTALL || true
rm -rf PGDATA_Master || true
rm -rf PGDATA_Slave || true
rm -rf master.log || true
rm -rf slave.log || true
mkdir PGDATA_Master
mkdir PGDATA_Slave

# Building project
make > /dev/null
make -C contrib > /dev/null
make install > /dev/null
make -C contrib install > /dev/null

initdb -D PGDATA_Master -E UTF8 --locale=C
initdb -D PGDATA_Slave -E UTF8 --locale=C
echo "shared_preload_libraries = 'postgres_fdw, pg_execplan, pg_exchange'" >> PGDATA_Master/postgresql.conf
echo "pg_exchange.node_number1=0" >> PGDATA_Master/postgresql.conf
#echo "log_min_messages = debug1" >>  PGDATA_Master/postgresql.conf
echo "shared_preload_libraries = 'postgres_fdw, pg_execplan, pg_exchange'" >> PGDATA_Slave/postgresql.conf
echo "pg_exchange.node_number1=1" >> PGDATA_Slave/postgresql.conf
#echo "log_min_messages = debug1" >>  PGDATA_Slave/postgresql.conf

pg_ctl -w -c -o "-p 5433" -D PGDATA_Slave -l slave.log start
pg_ctl -w -c -o "-p 5432" -D PGDATA_Master -l master.log start
createdb -p 5432
createdb -p 5433

psql -p 5433 -f "contrib/pg_execplan/tests/init_node_overall.sql" &
psql -p 5432 -f "contrib/pg_execplan/tests/init_node_overall.sql"

psql -p 5432 -c "CREATE SERVER $remoteSrvName FOREIGN DATA WRAPPER postgres_fdw
			OPTIONS (port '5433', use_remote_estimate 'on');
			CREATE USER MAPPING FOR PUBLIC SERVER $remoteSrvName;"
psql -p 5433 -c "CREATE SERVER $remoteSrvName FOREIGN DATA WRAPPER postgres_fdw
     	     		OPTIONS (port '5432', use_remote_estimate 'on');
			CREATE USER MAPPING FOR PUBLIC SERVER $remoteSrvName;"

# Change OID counter at slave node.
psql -p 5433 -c "CREATE TABLE t2 (id Serial, b INT, PRIMARY KEY(id));"
psql -p 5433 -c "DROP TABLE t2;"

psql -p 5432 -f "contrib/pg_execplan/tests/init_node0.sql"
psql -p 5433 -f "contrib/pg_execplan/tests/init_node1.sql"

psql -p 5432 -c "INSERT INTO pt (id, payload, test) (SELECT *, 0,0 FROM generate_series(1, 1000));"
