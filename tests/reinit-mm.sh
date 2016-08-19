n_nodes=3
export PATH=~/code/postgres_cluster/install/bin/:$PATH
ulimit -c unlimited
pkill -9 postgres
pkill -9 arbiter

cd ~/code/postgres_cluster/contrib/mmts/
make clean && make install
cd ~/code/postgres_cluster/contrib/raftable/
make clean && make install
cd ~/code/postgres_cluster/contrib/mmts/tests


rm -fr node? *.log dtm
conn_str=""
sep=""
for ((i=1;i<=n_nodes;i++))
do    
    port=$((5431 + i))
	raft_port=$((6665 + i))
    conn_str="$conn_str${sep}dbname=regression user=stas host=localhost port=$port sslmode=disable"
	raft_conn_str="$raft_conn_str${sep}${i}:localhost:$raft_port"
    sep=","
    initdb node$i
    pg_ctl -w -D node$i -l node$i.log start
    createdb regression
    pg_ctl -w -D node$i -l node$i.log stop
done

echo "Starting nodes..."

echo Start nodes
for ((i=1;i<=n_nodes;i++))
do
    port=$((5431+i))

    cat <<SQL > node$i/postgresql.conf
        listen_addresses='*'
        port = '$port'
        max_prepared_transactions = 100
        synchronous_commit = on
        fsync = off
        wal_level = logical
        max_worker_processes = 15
        max_replication_slots = 10
        max_wal_senders = 10
        shared_preload_libraries = 'raftable,multimaster'
        default_transaction_isolation = 'repeatable read'
        log_checkpoints = on
        log_autovacuum_min_duration = 0
        multimaster.workers = 1
        multimaster.use_raftable = true
        multimaster.queue_size=52857600
        multimaster.ignore_tables_without_pk = 1
        multimaster.heartbeat_recv_timeout = 1000
        multimaster.heartbeat_send_timeout = 250
        multimaster.twopc_min_timeout = 400000
        multimaster.volkswagen_mode = 1

        multimaster.conn_strings = '$conn_str'
        multimaster.node_id = $i
        raftable.id = $i
        raftable.peers = '$raft_conn_str'
SQL
    cp pg_hba.conf node$i
    pg_ctl -w -D node$i -l node$i.log start
done

sleep 10
psql regression < ../../../regress.sql

echo Done
