#!/bin/sh

if [ "$1" = 'postgres' ]; then
	mkdir -p "$PGDATA"
	mkdir -p /pg/src/src/test/regress/testtablespace

	# look specifically for PG_VERSION, as it is expected in the DB dir
	if [ ! -s "$PGDATA/PG_VERSION" ]; then
		initdb --nosync

		{ echo; echo "host all all 0.0.0.0/0 trust"; } >> "$PGDATA/pg_hba.conf"
		{ echo; echo "host replication all 0.0.0.0/0 trust"; } >> "$PGDATA/pg_hba.conf"

		# internal start of server in order to allow set-up using psql-client
		# does not listen on TCP/IP and waits until start finishes
		pg_ctl -D "$PGDATA" \
			-o "-c listen_addresses=''" \
			-w start

		: ${POSTGRES_USER:=postgres}
		: ${POSTGRES_DB:=$POSTGRES_USER}
		export POSTGRES_USER POSTGRES_DB

		if [ "$POSTGRES_DB" != 'postgres' ]; then
			psql -U `whoami` postgres <<-EOSQL
				CREATE DATABASE "$POSTGRES_DB" ;
			EOSQL
			echo
		fi

		if [ "$POSTGRES_USER" = `whoami` ]; then
			op='ALTER'
		else
			op='CREATE'
		fi

		psql -U `whoami` postgres <<-EOSQL
			$op USER "$POSTGRES_USER" WITH SUPERUSER PASSWORD '';
		EOSQL
		echo

		############################################################################

		# CONNSTRS="\
		# 	dbname=$POSTGRES_DB user=$POSTGRES_USER host=node1, \
		# 	dbname=$POSTGRES_DB user=$POSTGRES_USER host=node2, \
		# 	dbname=$POSTGRES_DB user=$POSTGRES_USER host=node3"


		cat <<-EOF >> $PGDATA/postgresql.conf
			listen_addresses='*' 
			max_prepared_transactions = 100
			synchronous_commit = on
			fsync = on
			wal_level = logical
			max_worker_processes = 30
			max_replication_slots = 10
			max_wal_senders = 10
			shared_preload_libraries = 'multimaster'
			default_transaction_isolation = 'repeatable read'
			log_line_prefix = '%m: '
			wal_writer_delay = 1ms
			# log_statement = all

			multimaster.max_nodes = 3
			multimaster.heartbeat_recv_timeout = 1100
			multimaster.heartbeat_send_timeout = 250
			multimaster.max_recovery_lag = 1GB
			multimaster.min_recovery_lag = 10kB
			multimaster.preserve_commit_order = off
		EOF

		if [ -n "$NODE_ID" ]; then
			echo "multimaster.node_id = $NODE_ID" >> $PGDATA/postgresql.conf
		fi

		if [ -n "$CONNSTRS" ]; then
			echo "multimaster.conn_strings = '$CONNSTRS'" >> $PGDATA/postgresql.conf
		fi

		if [ -n "$MAJOR" ]; then
			echo 'multimaster.major_node = on' >> $PGDATA/postgresql.conf
		fi

		if [ -n "$REFEREE" ]; then
			echo 'multimaster.referee = on' >> $PGDATA/postgresql.conf
		fi

		if [ -n "$REFEREE_CONNSTR" ]; then
			echo "multimaster.referee_connstring = '$REFEREE_CONNSTR'" >> $PGDATA/postgresql.conf
		fi

		cat $PGDATA/postgresql.conf

		pg_ctl -D "$PGDATA" -m fast -w stop
	fi
fi

exec "$@"
