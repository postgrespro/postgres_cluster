#!/bin/sh

./install/bin/pg_ctl -D ./install/data1 stop
./install/bin/pg_ctl -D ./install/data2 stop

rm -rf install

make install

cd contrib/pg_xtm/

make clean
make
make install

cd ../..

./install/bin/initdb -D ./install/data1
./install/bin/initdb -D ./install/data2

sed -i '' 's/#port =.*/port = 5433/' ./install/data2/postgresql.conf

sed -i '' 's/#fsync =.*/fsync = off/' ./install/data1/postgresql.conf
sed -i '' 's/#fsync =.*/fsync = off/' ./install/data2/postgresql.conf

echo 'dtm.node_id = 0' >> ./install/data1/postgresql.conf
echo 'dtm.node_id = 1' >> ./install/data2/postgresql.conf

echo 'autovacuum = off' >> ./install/data1/postgresql.conf
echo 'autovacuum = off' >> ./install/data2/postgresql.conf

./install/bin/pg_ctl -D ./install/data1 -l ./install/data1/log start
./install/bin/pg_ctl -D ./install/data2 -l ./install/data2/log start


cd contrib/pg_xtm/dtmd
make clean
make
rm -rf /tmp/clog/*
./bin/dtmd