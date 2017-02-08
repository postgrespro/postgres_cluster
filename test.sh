pkill -9 postgres
rm -rf tmp_install
make install



./tmp_install/bin/initdb -D ./tmp_install/data1
echo 'local   replication     stas                                trust' >> ./tmp_install/data1/pg_hba.conf
echo 'host   replication     all              ::1/128           trust' >> ./tmp_install/data1/pg_hba.conf
echo 'host    all             all             ::1/128                 trust' >> ./tmp_install/data1/pg_hba.conf
echo 'host   replication     all              127.0.0.1/32           trust' >> ./tmp_install/data1/pg_hba.conf
echo 'host    all             all             127.0.0.1/32                 trust' >> ./tmp_install/data1/pg_hba.conf
echo 'max_wal_senders = 10' >> ./tmp_install/data1/postgresql.conf
echo 'wal_level = logical' >> ./tmp_install/data1/postgresql.conf
echo 'max_prepared_transactions = 100' >> tmp_install/data1/postgresql.conf 
./tmp_install/bin/pg_ctl -w -D ./tmp_install/data1 -l logfile start
createdb -h localhost
pgbench -i -h localhost




mkdir ./tmp_install/wals
./tmp_install/bin/pg_receivexlog -D ./tmp_install/wals



pgbench -j 2 -c 10 -f 2pc.pgb -P 1 -r -t 300000

# pgbench -j 2 -c 10 -M prepared -P 1 -N -r -t 150000






patched:
    -rw-------    1 stas  staff    16M Feb  8 11:28 000000010000000000000064
    -rw-------    1 stas  staff    16M Feb  8 11:28 000000010000000000000065.partial

    # select pg_current_xlog_location();
    pg_current_xlog_location 
    --------------------------
    0/65B68390


patched-2pc.pgb (190b):
    -rw-------    1 stas  staff    16M Feb  8 11:49 0000000100000000000000D7
    -rw-------    1 stas  staff    16M Feb  8 11:50 0000000100000000000000D8.partial
    pg_current_xlog_location 
    --------------------------
    0/D8B43E28



pgmaster (190b):
    -rw-------    1 stas  staff    16M Feb  8 12:08 0000000100000000000000B6
    -rw-------    1 stas  staff    16M Feb  8 12:08 0000000100000000000000B7.partial

     pg_current_xlog_location 
    --------------------------
    0/B7501578

pgmaster (6b):
    -rw-------    1 stas  staff    16M Feb  8 12:33 000000010000000000000094
    -rw-------    1 stas  staff    16M Feb  8 12:39 000000010000000000000095.partial

    stas=# select pg_current_xlog_location();
    pg_current_xlog_location 
    --------------------------
    0/9572CB28

patched-2pc.pgb (6b):
    -rw-------    1 stas  staff    16M Feb  8 13:24 000000010000000000000095
    -rw-------    1 stas  staff    16M Feb  8 13:30 000000010000000000000096.partial


    pg_current_xlog_location 
    --------------------------
    0/96C442E0
