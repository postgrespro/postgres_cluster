n_shards=3
export PATH=~/postgres_cluster/dist/bin/:$PATH
ulimit -c unlimited
pkill -9 postgres
sleep 2
rm -fr master shard? *.log pg_worker_list.conf
for ((i=1;i<=n_shards;i++))
do    
    port=$((5432+i))
    echo "localhost $port" >> pg_worker_list.conf
    initdb shard$i
done
initdb master
cp pg_worker_list.conf master

echo Start shards

for ((i=1;i<=n_shards;i++))
do
    port=$((5432+i))
    sed "s/5432/$port/g" < postgresql.conf.pg_shard > shard$i/postgresql.conf
	echo "shared_preload_libraries = 'pg_dtm'" >> shard$i/postgresql.conf
    pg_ctl -D shard$i -l shard$i.log start
done

echo Start master
cp postgresql.conf.pg_shard master/postgresql.conf
echo "shared_preload_libraries = 'pg_shard'" >> master/postgresql.conf
pg_ctl -D master -l master.log start

sleep 5

for ((i=1;i<=n_shards;i++))
do
    port=$((5432+i))
	psql -p $port postgres -c "create extension pg_dtm"
done 

echo Done
