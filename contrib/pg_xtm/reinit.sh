export PATH=/home/knizhnik/postgrespro/cluster_install/bin/:$PATH
ulimit -c unlimited
pkill -9 postgres
rm -fr node? *.log dtm/*
initdb node1
initdb node2
cp postgresql.conf.1 node1/postgresql.conf
cp postgresql.conf.2 node2/postgresql.conf
pg_ctl -D node1 -l node1.log start
pg_ctl -D node2 -l node2.log start
