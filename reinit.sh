#!/bin/sh

reinit_master() {
	rm -rf install/data

	./install/bin/initdb -D ./install/data

	echo "max_prepared_transactions = 100" >> ./install/data/postgresql.conf
	echo "shared_buffers = 512MB" >> ./install/data/postgresql.conf
	echo "fsync = off" >> ./install/data/postgresql.conf
	echo "log_checkpoints = on" >> ./install/data/postgresql.conf
	echo "max_wal_size = 48MB" >> ./install/data/postgresql.conf
	echo "min_wal_size = 32MB" >> ./install/data/postgresql.conf
	echo "wal_level = hot_standby" >> ./install/data/postgresql.conf
	echo "wal_keep_segments = 64" >> ./install/data/postgresql.conf
	echo "max_wal_senders = 2" >> ./install/data/postgresql.conf
	echo "max_replication_slots = 2" >> ./install/data/postgresql.conf

	echo '' > ./install/data/logfile

	echo 'local replication stas trust' >> ./install/data/pg_hba.conf

	./install/bin/pg_ctl -w -D ./install/data -l ./install/data/logfile start
	./install/bin/createdb stas
	./install/bin/psql -c "create table t(id int);"
}

reinit_slave() {
	rm -rf install/data_slave

	./install/bin/pg_basebackup -D ./install/data_slave/ -R

	echo "port = 5433" >> ./install/data_slave/postgresql.conf
	echo "hot_standby = on" >> ./install/data_slave/postgresql.conf

	echo '' > ./install/data_slave/logfile

	./install/bin/pg_ctl -w -D ./install/data_slave -l ./install/data_slave/logfile start
}

make install > /dev/null


cat <<MSG
###############################################################################
# Check that we can commit after soft restart.
# Here checkpoint happens before shutdown and no WAL replay will not occur
# during start. So code should re-create memory state from files.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
SQL
./install/bin/pg_ctl -w -D ./install/data -l ./install/data/logfile restart
psql <<SQL
	commit prepared 'x';
SQL



cat <<MSG
###############################################################################
# Check that we can commit after hard restart.
# On startup WAL replay will re-create memory for global transactions that 
# happend after last checkpoint and stored. After that  
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
SQL
pkill -9 postgres
./install/bin/pg_ctl -w -D ./install/data -l ./install/data/logfile start
psql <<SQL
	commit prepared 'x';
SQL
























