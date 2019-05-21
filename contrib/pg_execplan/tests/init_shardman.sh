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
rm -rf PGDATA_n0 || true
rm -rf PGDATA_n1 || true
rm -rf PGDATA_n2 || true
rm -rf n0.log || true
rm -rf n1.log || true
rm -rf n2.log || true
mkdir PGDATA_n0
mkdir PGDATA_n1
mkdir PGDATA_n2

# Building project
make > /dev/null
make -C contrib > /dev/null
make install > /dev/null
make -C contrib install > /dev/null

initdb -D PGDATA_n0 -E UTF8 --locale=C
initdb -D PGDATA_n1 -E UTF8 --locale=C
initdb -D PGDATA_n2 -E UTF8 --locale=C

echo "shared_preload_libraries = 'postgres_fdw, pg_exchange'" >> PGDATA_n0/postgresql.conf
echo "shared_preload_libraries = 'postgres_fdw, pg_exchange'" >> PGDATA_n1/postgresql.conf
echo "shared_preload_libraries = 'postgres_fdw, pg_exchange'" >> PGDATA_n2/postgresql.conf
echo "listen_addresses = '*'" >> PGDATA_n0/postgresql.conf
echo "listen_addresses = '*'" >> PGDATA_n1/postgresql.conf
echo "listen_addresses = '*'" >> PGDATA_n2/postgresql.conf
echo "host    all             all             0.0.0.0/0                 trust" >> PGDATA_n0/pg_hba.conf
echo "host    all             all             0.0.0.0/0                 trust" >> PGDATA_n1/pg_hba.conf
echo "host    all             all             0.0.0.0/0                 trust" >> PGDATA_n2/pg_hba.conf

#echo "log_min_messages = debug1" >>  PGDATA_Slave/postgresql.conf

pg_ctl -w -c -o "-p 5434" -D PGDATA_n2 -l n2.log start
pg_ctl -w -c -o "-p 5433" -D PGDATA_n1 -l n1.log start
pg_ctl -w -c -o "-p 5432" -D PGDATA_n0 -l n0.log start
createdb -p 5432
createdb -p 5433
createdb -p 5434

psql -p 5434 -f "contrib/pg_execplan/tests/init_node_overall.sql" &
psql -p 5433 -f "contrib/pg_execplan/tests/init_node_overall.sql" &
psql -p 5432 -f "contrib/pg_execplan/tests/init_node_overall.sql"

#psql -p 5432 -c "CREATE SERVER remote1 FOREIGN DATA WRAPPER postgres_fdw
#			OPTIONS (port '5433', use_remote_estimate 'on');
#			CREATE USER MAPPING FOR PUBLIC SERVER remote1;"
#psql -p 5432 -c "CREATE SERVER remote2 FOREIGN DATA WRAPPER postgres_fdw
#			OPTIONS (port '5434', use_remote_estimate 'on');
#			CREATE USER MAPPING FOR PUBLIC SERVER remote2;"

#psql -p 5433 -c "CREATE SERVER remote1 FOREIGN DATA WRAPPER postgres_fdw
#     	     		OPTIONS (port '5432', use_remote_estimate 'on');
#			CREATE USER MAPPING FOR PUBLIC SERVER remote1;"
#psql -p 5433 -c "CREATE SERVER remote2 FOREIGN DATA WRAPPER postgres_fdw
#			OPTIONS (port '5434', use_remote_estimate 'on');
#			CREATE USER MAPPING FOR PUBLIC SERVER remote2;"

#psql -p 5434 -c "CREATE SERVER remote1 FOREIGN DATA WRAPPER postgres_fdw
#     	     		OPTIONS (port '5432', use_remote_estimate 'on');
#			CREATE USER MAPPING FOR PUBLIC SERVER remote1;"
#psql -p 5434 -c "CREATE SERVER remote2 FOREIGN DATA WRAPPER postgres_fdw
#			OPTIONS (port '5433', use_remote_estimate 'on');
#			CREATE USER MAPPING FOR PUBLIC SERVER remote2;"

# Change OID counter at slave node.
psql -p 5433 -c "CREATE TABLE t2 (id Serial, b INT, PRIMARY KEY(id));"
psql -p 5433 -c "DROP TABLE t2;"

psql -p 5432 -f "contrib/pg_execplan/tests/init_node0.sql"
psql -p 5433 -f "contrib/pg_execplan/tests/init_node1.sql"
psql -p 5434 -f "contrib/pg_execplan/tests/init_node2.sql"

psql -p 5432 -c "INSERT INTO pt (id, payload, test) (SELECT a.*, b.*,0 FROM
generate_series(1, 10) as a, generate_series(1, 3) as b);"
psql -p 5432 -c "INSERT INTO rt (id, payload, test) (SELECT a.*, 0,0 FROM 
generate_series(1, 10) as a);"

psql -p 5432 -c "INSERT INTO a (a1, a2) (SELECT *, 1 FROM generate_series(1, 50));"
psql -p 5432 -c "INSERT INTO b (b1, b2) (SELECT *, 0 FROM generate_series(1, 10));"

psql -p 5432 -c "VACUUM FULL;"
psql -p 5433 -c "VACUUM FULL;"
psql -p 5434 -c "VACUUM FULL;"
