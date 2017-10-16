CURPATH=`pwd`
BASEDIR=$CURPATH/../../..
export PATH=$BASEDIR/tmp_install/usr/local/pgsql/bin/:$PATH
export DESTDIR=$BASEDIR/tmp_install

n_nodes=3
ulimit -c unlimited
pkill -9 postgres

cd $BASEDIR
make install

cd $BASEDIR/contrib/mmts
make clean && make install

cd $BASEDIR/contrib/mmts/tests

rm -fr node? *.log
conn_str=""
sep=""
for ((i=1;i<=n_nodes;i++))
do    
    port=$((5431 + i))
    arbiter_port=$((7000 + i))
    conn_str="$conn_str${sep}dbname=regression user=stas host=127.0.0.1 port=$port arbiter_port=$arbiter_port sslmode=disable"
    sep=","
    initdb node$i
    pg_ctl -w -D node$i -l node$i.log start
    createdb regression
    # psql regression -c "create table t(id int primary key, v int); insert into t values($i,$i);"
    pg_ctl -w -D node$i -l node$i.log stop
done

echo "Starting nodes..."

echo Start nodes
for ((i=1;i<=n_nodes;i++))
do
    port=$((5431+i))
    arbiter_port=$((7000 + i))

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
        shared_preload_libraries = 'multimaster'
        default_transaction_isolation = 'repeatable read'

        multimaster.workers = 1
        multimaster.heartbeat_recv_timeout = 2000
        multimaster.heartbeat_send_timeout = 250
        #multimaster.volkswagen_mode = 1
        multimaster.conn_strings = '$conn_str'
        multimaster.node_id = $i
        multimaster.max_nodes = 4
        multimaster.arbiter_port = $arbiter_port
        multimaster.min_2pc_timeout = 10000000
        multimaster.trans_spill_threshold = 100
        multimaster.monotonic_sequences = true
SQL
    cp pg_hba.conf node$i
    pg_ctl -w -D node$i -l node$i.log start
done

sleep 10
psql regression < regress.sql

echo Done
