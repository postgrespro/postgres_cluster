
. ./paths.sh
U=`whoami`
remoteSrvName=fdwremote

pkill -9 postgres || true
ulimit -c unlimited

rm master.log
rm slave.log
rm -rf PGDATA_Master
mkdir PGDATA_Master
rm -rf PGDATA_Slave
mkdir PGDATA_Slave

initdb -D PGDATA_Master --locale=C
initdb -D PGDATA_Slave --locale=C
	
echo "shared_preload_libraries = 'postgres_fdw, repeater'" >> PGDATA_Master/postgresql.conf
echo "repeater.host = 'localhost'" >> PGDATA_Master/postgresql.conf
echo "repeater.port = 5433" >> PGDATA_Master/postgresql.conf
echo "repeater.fdwname = '$remoteSrvName'" >> PGDATA_Master/postgresql.conf

pg_ctl -c -o "-p 5433" -D PGDATA_Slave -l slave.log start
pg_ctl -c -o "-p 5432" -D PGDATA_Master -l master.log start
createdb -p 5433 $U
createdb -p 5432 $U

psql -p 5432 -c "CREATE EXTENSION postgres_fdw;"
psql -p 5432 -c "create server $remoteSrvName foreign data wrapper postgres_fdw options (port '5433', use_remote_estimate 'on');"
psql -p 5432 -c "create user mapping for current_user server $remoteSrvName;"

# Prepare plan execution
psql -p 5432 -c "CREATE EXTENSION repeater;"

psql -p 5433 -c "CREATE TABLE t2 (id Serial, b INT, PRIMARY KEY(id));"
psql -p 5433 -c "DROP TABLE t2;"
