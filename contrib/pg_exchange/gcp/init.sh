#!/bin/bash

# ------------------------------------------------------------------------------
#
# Initialization & create DB & start of a postgresql instance at first launch.
#
# ------------------------------------------------------------------------------

PGINSTALL=`pwd`/tmp_install/
PGDATA=GCP_PGDATA

export PATH=$PGINSTALL/bin:$PATH
export PGDATABASE=test

pkill -9 -e postgres || true
sleep 1

rm -rf $PGDATA
mkdir $PGDATA
rm -rf logfile.log || true

# Building project

make > /dev/null
make -C contrib > /dev/null
make install > /dev/null
make -C contrib install > /dev/null

initdb -D $PGDATA -E UTF8 --locale=C

echo "shared_preload_libraries = 'postgres_fdw, pg_exchange'" >> $PGDATA/postgresql.conf
echo "listen_addresses = '*'" >> $PGDATA/postgresql.conf
echo "host    all             all             0.0.0.0/0                 trust" >> $PGDATA/pg_hba.conf

pg_ctl -w -c -D $PGDATA -l logfile.log start
createdb

