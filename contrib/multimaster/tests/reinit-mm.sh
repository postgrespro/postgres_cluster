n_nodes=3
export PATH=~/postgres_cluster/dist/bin/:$PATH
ulimit -c unlimited
pkill -9 postgres
pkill -9 dtmd
rm -fr node? *.log dtm
mkdir dtm
conn_str=""
sep=""
for ((i=1;i<=n_nodes;i++))
do    
    port=$((5431+i))
    conn_str="$conn_str${sep}dbname=postgres host=127.0.0.1 port=$port sslmode=disable"
    sep=","
    initdb node$i
done

echo Start DTM
~/postgres_cluster/contrib/arbiter/bin/arbiter -r 0.0.0.0:5431 -i 0 -d dtm 2> dtm.log &
sleep 2

echo Start nodes
for ((i=1;i<=n_nodes;i++))
do
    port=$((5431+i))
    sed "s/5432/$port/g" < postgresql.conf.mm > node$i/postgresql.conf
    echo "multimaster.conn_strings = '$conn_str'" >> node$i/postgresql.conf
    echo "multimaster.node_id = $i" >> node$i/postgresql.conf
    cp pg_hba.conf node$i
    pg_ctl -D node$i -l node$i.log start
done

sleep 5
echo Initialize database schema
psql postgres -f init.sql

echo Done
