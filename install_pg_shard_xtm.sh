#!/bin/sh

PG_SHARD_DIR=~/code/pg_shard_master
PG_DIR=~/code/postgresql
PG_XTM_DIR=$PG_DIR/contrib/pg_xtm


########################################################################
#  Stop old stuff
########################################################################
./install/bin/pg_ctl -D ./install/data1 stop
./install/bin/pg_ctl -D ./install/data2 stop
./install/bin/pg_ctl -D ./install/data3 stop
killall dtmd
rm -rf install


########################################################################
#  Build and run dtm and postgres
########################################################################
make install # assuming configured with --prefix=./install

cd $PG_SHARD_DIR
make clean
PATH=~/code/postgresql/install/bin/:$PATH make
PATH=~/code/postgresql/install/bin/:$PATH make install

cd $PG_XTM_DIR
make clean
make
make install

cd dtmd
make clean
make
rm -rf /tmp/clog/*
./bin/dtmd &

cd $PG_DIR

./install/bin/initdb -D ./install/data1
./install/bin/initdb -D ./install/data2
./install/bin/initdb -D ./install/data3

sed -i '' "s/shared_preload_libraries =.*/shared_preload_libraries = 'pg_dtm,pg_shard'/" ./install/data1/postgresql.conf
sed -i '' "s/shared_preload_libraries =.*/shared_preload_libraries = 'pg_dtm'/" ./install/data2/postgresql.conf
sed -i '' "s/shared_preload_libraries =.*/shared_preload_libraries = 'pg_dtm'/" ./install/data3/postgresql.conf

echo "port = 5433" >> ./install/data2/postgresql.conf
echo "port = 5434" >> ./install/data3/postgresql.conf

echo 'fsync = off' >> ./install/data1/postgresql.conf
echo 'fsync = off' >> ./install/data2/postgresql.conf
echo 'fsync = off' >> ./install/data3/postgresql.conf

echo 'pg_shard.use_dtm_transactions=1' >> ./install/data1/postgresql.conf
echo 'pg_shard.all_modifications_commutative=1' >> ./install/data1/postgresql.conf

echo "log_statement = 'all'" >> ./install/data1/postgresql.conf
echo "log_statement = 'all'" >> ./install/data2/postgresql.conf
echo "log_statement = 'all'" >> ./install/data3/postgresql.conf

./install/bin/pg_ctl -D ./install/data1 -l ./install/data1/log start
./install/bin/pg_ctl -D ./install/data2 -l ./install/data2/log start
./install/bin/pg_ctl -D ./install/data3 -l ./install/data3/log start


########################################################################
#  Configure pg_shard
########################################################################

echo "127.0.0.1    5433" >  ./install/data1/pg_worker_list.conf
echo "127.0.0.1    5434" >> ./install/data1/pg_worker_list.conf

echo "127.0.0.1    5433" >  ./install/data2/pg_worker_list.conf
echo "127.0.0.1    5434" >> ./install/data2/pg_worker_list.conf

echo "127.0.0.1    5433" >  ./install/data3/pg_worker_list.conf
echo "127.0.0.1    5434" >> ./install/data3/pg_worker_list.conf


./install/bin/createdb `whoami`
./install/bin/createdb `whoami` -p5433
./install/bin/createdb `whoami` -p5434


./install/bin/psql -p 5432 << SQL
CREATE EXTENSION pg_dtm;
SQL

./install/bin/psql -p 5433 << SQL
CREATE EXTENSION pg_dtm;
SQL

./install/bin/psql -p 5434 << SQL
CREATE EXTENSION pg_dtm;
SQL
