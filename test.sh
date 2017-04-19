pkill -9 postgres
rm -rf tmp_install
make install
ulimit -c unlimited

./tmp_install/bin/initdb -D ./tmp_install/data1

cat <<-CONF >> ./tmp_install/data1/pg_hba.conf
    local   replication     stas                                trust
    host   replication     all              ::1/128           trust
    host    all             all             ::1/128                 trust
    host   replication     all              127.0.0.1/32           trust
    host    all             all             127.0.0.1/32                 trust
CONF

cat <<-CONF >> ./tmp_install/data1/postgresql.conf
    max_wal_senders = 10
    wal_level = logical
    max_prepared_transactions = 100
CONF

./tmp_install/bin/pg_ctl -w -D ./tmp_install/data1 -l logfile start
createdb -h localhost
pgbench -i -h localhost


###############################################################################

./tmp_install/bin/initdb -D ./tmp_install/data2

cat <<-CONF >> ./tmp_install/data2/pg_hba.conf
    local   replication     stas                                trust
    host   replication     all              ::1/128           trust
    host    all             all             ::1/128                 trust
    host   replication     all              127.0.0.1/32           trust
    host    all             all             127.0.0.1/32                 trust
CONF

cat <<-CONF >> ./tmp_install/data2/postgresql.conf
    max_wal_senders = 10
    wal_level = logical
    max_prepared_transactions = 100
    port = 5433
CONF

./tmp_install/bin/pg_ctl -w -D ./tmp_install/data2 -l logfile2 start
createdb -h localhost -p 5433
./tmp_install/bin/pg_dump -s | psql -p 5433
# pgbench -i -h localhost -p 5433


psql -c $'CREATE PUBLICATION mypub FOR TABLE pgbench_accounts, pgbench_branches, pgbench_history, pgbench_tellers;'
psql -c $'CREATE SUBSCRIPTION mysub CONNECTION \'dbname=stas host=127.0.0.1\' PUBLICATION mypub;' -p 5433