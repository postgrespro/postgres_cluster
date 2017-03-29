#!/bin/sh

# this script assumes that postgres and test_decodong is installed
# (srcdir)/tmp_install

rm -rf tmp_install/data1
./tmp_install/bin/initdb -D ./tmp_install/data1
./tmp_install/bin/pg_ctl -w -D ./tmp_install/data1 -l logfile start
./tmp_install/bin/createdb regression

cat >> ./tmp_install/data1/postgresql.conf <<-CONF
    wal_level=logical
    max_replication_slots=4
    max_prepared_transactions=20
    shared_preload_libraries='test_decoding'
    wal_sender_timeout=600000
CONF
./tmp_install/bin/pg_ctl -w -D ./tmp_install/data1 -l logfile restart

python3 repconsumer.py > xlog_decoded &
REPCONSUMER_PID=$!

sleep 3

cd src/test/regress

# ./pg_regress --inputdir=. --bindir='../../../tmp_install/bin' --dlpath=. --schedule=./parallel_schedule --use-existing

./pg_regress --inputdir=. --bindir='../../../tmp_install/bin' --dlpath=. --schedule=./serial_schedule --use-existing

cd ../../..

kill $REPCONSUMER_PID
./tmp_install/bin/pg_ctl -D ./tmp_install/data1 -l logfile stop
