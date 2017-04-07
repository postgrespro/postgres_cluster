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
    sed "s/5432/$port/g" < postgresql.conf.fdw > shard$i/postgresql.conf
	echo "shared_preload_libraries = 'pg_tsdtm'" >> shard$i/postgresql.conf
    pg_ctl -D shard$i -l shard$i.log start
done

echo Start master
cp postgresql.conf.fdw master/postgresql.conf
pg_ctl -D master -l master.log start

sleep 5

user=`whoami` 
psql postgres -c "CREATE EXTENSION postgres_fdw"
psql postgres -c "CREATE TABLE t(u integer primary key, v integer)"

for ((i=1;i<=n_shards;i++))
do
    port=$((5432+i))
	psql -p $port postgres -c "CREATE EXTENSION pg_tsdtm"
	psql -p $port postgres -c "CREATE TABLE t(u integer primary key, v integer)"

	psql postgres -c "CREATE SERVER shard$i FOREIGN DATA WRAPPER postgres_fdw options(dbname 'postgres', host 'localhost', port '$port')"
	psql postgres -c "CREATE FOREIGN TABLE t_fdw$i() inherits (t) server shard$i options(table_name 't')"
	psql postgres -c "CREATE USER MAPPING for $user SERVER shard$i options (user '$user')"
done
