#!/bin/sh

reinit_master() {
	rm -rf install/data

	./install/bin/initdb -A trust -D ./install/data
	echo "max_worker_processes = 10" >> ./install/data/postgresql.conf
	echo "shared_preload_libraries = 'pg_tsdtm'" >> ./install/data/postgresql.conf
	echo "postgres_fdw.use_tsdtm = 1" >> ./install/data/postgresql.conf
	echo '' > ./install/data/logfile
	./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile start
	./install/bin/createdb stas
}

reinit_slave1() {
	rm -rf install/data_slave
	./install/bin/initdb -A trust -D ./install/data_slave
	echo "port = 5433" >> ./install/data_slave/postgresql.conf
	echo "shared_preload_libraries = 'pg_tsdtm'" >> ./install/data/postgresql.conf
	echo "postgres_fdw.use_tsdtm = 1" >> ./install/data/postgresql.conf
	echo '' > ./install/data_slave/logfile
	./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/data_slave/logfile start
	./install/bin/createdb -p5433 stas
}

reinit_slave2() {
	rm -rf install/data_slave2
	./install/bin/initdb -A trust -D ./install/data_slave2
	echo "port = 5434" >> ./install/data_slave2/postgresql.conf
	echo "shared_preload_libraries = 'pg_tsdtm'" >> ./install/data/postgresql.conf
	echo "postgres_fdw.use_tsdtm = 1" >> ./install/data/postgresql.conf
	echo '' > ./install/data_slave2/logfile
	./install/bin/pg_ctl -sw -D ./install/data_slave2 -l ./install/data_slave2/logfile start
	./install/bin/createdb -p5434 stas
}

make install > /dev/null


cat <<MSG
###############################################################################
# Check that we can commit and abort after soft restart.
# Here checkpoint happens before shutdown and no WAL replay will not occur
# during start. So code should re-create memory state from files.
###############################################################################
MSG

pkill -9 postgres
ulimit -c unlimited
reinit_master
reinit_slave1
reinit_slave2

user=`whoami`

./install/bin/psql -c "CREATE EXTENSION postgres_fdw"
./install/bin/psql -c "CREATE TABLE t(u integer primary key, v integer)"

./install/bin/psql -p 5433 -c "CREATE EXTENSION pg_tsdtm"
./install/bin/psql -p 5433 -c "CREATE TABLE t(u integer primary key, v integer)"

./install/bin/psql -c "CREATE SERVER shard1 FOREIGN DATA WRAPPER postgres_fdw options(dbname '$user', host 'localhost', port '5433')"
./install/bin/psql -c "CREATE FOREIGN TABLE t_fdw1() inherits (t) server shard1 options(table_name 't')"
./install/bin/psql -c "CREATE USER MAPPING for $user SERVER shard1 options (user '$user')"

./install/bin/psql -p 5434 -c "CREATE EXTENSION pg_tsdtm"
./install/bin/psql -p 5434 -c "CREATE TABLE t(u integer primary key, v integer)"

./install/bin/psql -c "CREATE SERVER shard2 FOREIGN DATA WRAPPER postgres_fdw options(dbname '$user', host 'localhost', port '5434')"
./install/bin/psql -c "CREATE FOREIGN TABLE t_fdw2() inherits (t) server shard2 options(table_name 't')"
./install/bin/psql -c "CREATE USER MAPPING for $user SERVER shard2 options (user '$user')"

###########

./install/bin/psql -c "insert into t_fdw1 (select generate_series(0, 100), 0)"
./install/bin/psql -c "insert into t_fdw2 (select generate_series(100, 200), 0)"














