#!/bin/sh
pkill -9 postgres
make install
rm -rf install/data
rm -rf install/data_slave
rm -rf /cores/*


# Node master
./install/bin/initdb -D ./install/data
echo "max_prepared_transactions = 100" >> ./install/data/postgresql.conf
echo "shared_buffers = 512MB" >> ./install/data/postgresql.conf
echo "fsync = off" >> ./install/data/postgresql.conf
echo "log_checkpoints = on" >> ./install/data/postgresql.conf
echo "max_wal_size = 48MB" >> ./install/data/postgresql.conf
echo "min_wal_size = 32MB" >> ./install/data/postgresql.conf
echo "wal_level = hot_standby" >> ./install/data/postgresql.conf
echo "wal_keep_segments = 64" >> ./install/data/postgresql.conf
echo "max_wal_senders = 2" >> ./install/data/postgresql.conf
echo "max_replication_slots = 2" >> ./install/data/postgresql.conf

echo '' > ./install/data/logfile
echo 'local replication stas trust' >> ./install/data/pg_hba.conf

./install/bin/pg_ctl -w -D ./install/data -l ./install/data/logfile start
./install/bin/createdb stas
./install/bin/psql -c "create table t(id int);"

./install/bin/pg_basebackup -D ./install/data_slave/ -R

# Node slave
echo "port = 5433" >> ./install/data_slave/postgresql.conf
echo "hot_standby = on" >> ./install/data_slave/postgresql.conf
echo '' > ./install/data_slave/logfile
./install/bin/pg_ctl -w -D ./install/data_slave -l ./install/data_slave/logfile start


