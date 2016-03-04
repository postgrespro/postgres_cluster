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
	echo "max_wal_senders = 10" >> ./install/data/postgresql.conf
	echo "max_replication_slots = 10" >> ./install/data/postgresql.conf

	echo "max_worker_processes = 10" >> ./install/data/postgresql.conf
	echo "shared_preload_libraries = 'pglogical'" >> ./install/data/postgresql.conf
	echo "track_commit_timestamp = on" >> ./install/data/postgresql.conf
	# echo "client_min_messages = debug3" >> ./install/data/postgresql.conf
	# echo "log_min_messages = debug3" >> ./install/data/postgresql.conf

	echo '' > ./install/data/logfile

	echo 'local replication stas trust' >> ./install/data/pg_hba.conf

	./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile start
	./install/bin/createdb stas
	./install/bin/psql -c "create table t(id int primary key, v int);"
}

reinit_master2() {
	rm -rf install/data2

	./install/bin/initdb -A trust -D ./install/data2

	echo "port = 5433" >> ./install/data2/postgresql.conf

	echo "max_prepared_transactions = 100" >> ./install/data2/postgresql.conf
	echo "shared_buffers = 512MB" >> ./install/data2/postgresql.conf
	echo "fsync = off" >> ./install/data2/postgresql.conf
	echo "log_checkpoints = on" >> ./install/data2/postgresql.conf
	echo "max_wal_size = 48MB" >> ./install/data2/postgresql.conf
	echo "min_wal_size = 32MB" >> ./install/data2/postgresql.conf
	echo "wal_level = logical" >> ./install/data2/postgresql.conf
	echo "wal_keep_segments = 64" >> ./install/data2/postgresql.conf
	echo "max_wal_senders = 10" >> ./install/data2/postgresql.conf
	echo "max_replication_slots = 10" >> ./install/data2/postgresql.conf

	echo "max_worker_processes = 10" >> ./install/data2/postgresql.conf
	echo "shared_preload_libraries = 'pglogical'" >> ./install/data2/postgresql.conf
	echo "track_commit_timestamp = on" >> ./install/data2/postgresql.conf

	# echo "client_min_messages = debug3" >> ./install/data2/postgresql.conf
	# echo "log_min_messages = debug3" >> ./install/data2/postgresql.conf

	echo '' > ./install/data2/logfile

	echo 'local replication stas trust' >> ./install/data2/pg_hba.conf

	./install/bin/pg_ctl -sw -D ./install/data2 -l ./install/data2/logfile start
	./install/bin/createdb stas -p5433
}

make install > /dev/null

cd contrib/pglogical
make clean && make install
cd ../..
cd contrib/pglogical_output
make clean && make install
cd ../..

pkill -9 postgres
reinit_master
# reinit_master2

# ./install/bin/psql <<SQL
# 	CREATE EXTENSION pglogical;
# 	SELECT pglogical.create_node(
# 		node_name := 'provider1',
# 		dsn := 'port=5432 dbname=stas'
# 	);
# 	SELECT pglogical.replication_set_add_all_tables('default', ARRAY['public']);
# SQL

# ./install/bin/psql -p 5433 <<SQL
# 	CREATE EXTENSION pglogical;
# 	SELECT pglogical.create_node(
# 		node_name := 'subscriber1',
# 		dsn := 'port=5433 dbname=stas'
# 	);
# 	SELECT pglogical.create_subscription(
# 		subscription_name := 'subscription1',
# 		provider_dsn := 'port=5432 dbname=stas'
# 	);
# SQL

# ./install/bin/psql -c "SELECT 'init' FROM pg_create_logical_replication_slot('regression_slot', 'pglogical_output');"

# ./install/bin/psql <<SQL
# 	begin;
# 	insert into t values (42);
# 	prepare transaction 'hellyeah';
# 	rollback prepared 'hellyeah';
# SQL

# ./install/bin/psql <<SQL
# SELECT * FROM pg_logical_slot_peek_changes('regression_slot',
# 	NULL, NULL,
# 	'expected_encoding', 'UTF8',
# 	'min_proto_version', '1',
# 	'max_proto_version', '1',
# 	'startup_params_format', '1',
# 	'proto_format', 'json',
# 	'no_txinfo', 't');
# SQL







