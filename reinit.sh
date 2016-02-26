#!/bin/sh

reinit_master() {
	rm -rf install/data

	./install/bin/initdb -A trust -D ./install/data

	echo "max_prepared_transactions = 100" >> ./install/data/postgresql.conf
	echo "shared_buffers = 512MB" >> ./install/data/postgresql.conf
	echo "fsync = off" >> ./install/data/postgresql.conf
	echo "log_checkpoints = on" >> ./install/data/postgresql.conf
	echo "max_wal_size = 48MB" >> ./install/data/postgresql.conf
	echo "min_wal_size = 32MB" >> ./install/data/postgresql.conf
	echo "wal_level = logical" >> ./install/data/postgresql.conf
	echo "wal_keep_segments = 64" >> ./install/data/postgresql.conf
	echo "max_wal_senders = 2" >> ./install/data/postgresql.conf
	echo "max_replication_slots = 2" >> ./install/data/postgresql.conf

	echo '' > ./install/data/logfile

	echo 'local replication stas trust' >> ./install/data/pg_hba.conf

	./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile start
	./install/bin/createdb stas
	./install/bin/psql -c "create table t(id int);"
	./install/bin/psql -c "SELECT 'init' FROM pg_create_logical_replication_slot('regression_slot', 'pglogical_output');"
	./install/bin/psql <<SQL
		begin;
		insert into t values (42);
		prepare transaction 'x';
		commit prepared 'x';
SQL
}

reinit_slave() {
	rm -rf install/data_slave

	./install/bin/pg_basebackup -D ./install/data_slave/ -R

	echo "port = 5433" >> ./install/data_slave/postgresql.conf
	echo "hot_standby = on" >> ./install/data_slave/postgresql.conf

	echo '' > ./install/data_slave/logfile

	./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/data_slave/logfile start
}

make install > /dev/null


pkill -9 postgres
reinit_master >> /dev/null



# SELECT * FROM pg_logical_slot_peek_changes('regression_slot',
# 	NULL, NULL,
# 	'expected_encoding', 'UTF8',
# 	'min_proto_version', '1',
# 	'max_proto_version', '1',
# 	'startup_params_format', '1',
# 	'proto_format', 'json',
# 	'no_txinfo', 't');


