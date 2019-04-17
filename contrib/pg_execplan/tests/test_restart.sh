export PGDATABASE=test_base
psql -p 5432 -c "SELECT * FROM pt;"
pg_ctl -D PGDATA_n1 stop
pg_ctl -w -c -o "-p 5433" -D PGDATA_n1 -l n1.log start
psql -p 5432 -c "SELECT * FROM pt;"
