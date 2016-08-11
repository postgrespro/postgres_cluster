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
    conn_str="$conn_str${sep}dbname=postgres host=localhost port=$port sslmode=disable"
	raft_conn_str="$raft_conn_str${sep}${i}:localhost:$raft_port"
    sep=","
    initdb node$i
done

#echo Start DTM
#~/postgres_cluster/contrib/arbiter/bin/arbiter -r 0.0.0.0:5431 -i 0 -d dtm 2> dtm.log &
#sleep 2
echo "Starting nodes..."

echo Start nodes
for ((i=1;i<=n_nodes;i++))
do
    port=$((5431+i))
    sed "s/5432/$port/g" < postgresql.conf.mm > node$i/postgresql.conf
    echo "multimaster.conn_strings = '$conn_str'" >> node$i/postgresql.conf
    echo "multimaster.node_id = $i" >> node$i/postgresql.conf

    cp pg_hba.conf node$i
    pg_ctl -w -D node$i -l node$i.log start
done

# sleep 5
# psql -c "create extension multimaster;" postgres

echo Done
