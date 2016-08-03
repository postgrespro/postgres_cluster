#!/bin/bash
set -e

mkdir -p "$PGDATA"
chmod 700 "$PGDATA"
chown -R postgres "$PGDATA"

chmod g+s /run/postgresql
chown -R postgres /run/postgresql

initdb

cat >> "$PGDATA/pg_hba.conf" <<-EOF
	host all all 0.0.0.0/0 trust
	local replication all trust
	host replication all 0.0.0.0/0 trust
EOF

cat >> "$PGDATA/postgresql.conf" <<-EOF
	listen_addresses = '*'
	unix_socket_directories = ''
	port = 5432
	max_prepared_transactions = 200
	max_connections = 200
	max_worker_processes = 100
	wal_level = logical
	fsync = off	
    log_line_prefix = '%t: '
	max_wal_senders = 10
	wal_sender_timeout = 0
	max_replication_slots = 10
	shared_preload_libraries = 'raftable,multimaster'
	multimaster.workers = 10
	multimaster.queue_size = 10485760 # 10mb
	multimaster.node_id = $NODEID
	multimaster.conn_strings = '$CONNS'
	multimaster.use_raftable = true
	multimaster.ignore_tables_without_pk = true
	raftable.id = $NODEID
	raftable.peers = '$PEERS'
EOF

"$@"
