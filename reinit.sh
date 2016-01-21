#!/bin/sh

reinit_master() {
	rm -rf install/data

	./install/bin/initdb -A trust -D ./install/data

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

	./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile start
	./install/bin/createdb stas
	./install/bin/psql -c "create table t(id int);"
}

reinit_slave() {
	rm -rf install/data_slave

	./install/bin/pg_basebackup -D ./install/data_slave/ -R

	echo "port = 5433" >> ./install/data_slave/postgresql.conf
	echo "hot_standby = on" >> ./install/data_slave/postgresql.conf

	echo '' > ./install/data_slave/logfile

	./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/data_slave/logfile start
}

make install > /dev/null


cat <<MSG
###############################################################################
# Check that we can commit and abort after soft restart.
# Here checkpoint happens before shutdown and no WAL replay will not occur
# during start. So code should re-create memory state from files.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
	begin;
	insert into t values (43);
	prepare transaction 'y';
SQL
./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile restart
psql <<SQL
	commit prepared 'x';
	rollback prepared 'y';
SQL



cat <<MSG
###############################################################################
# Check that we can commit and abort after hard restart.
# On startup WAL replay will re-create memory for global transactions that 
# happend after last checkpoint and stored. After that  
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
	begin;
	insert into t values (43);
	prepare transaction 'y';
SQL
pkill -9 postgres
./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile start
psql <<SQL
	commit prepared 'x';
	rollback prepared 'y';
SQL


cat <<MSG
###############################################################################
# Check that WAL replay will cleanup it's memory state and release locks while 
# replaying commit.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
	commit prepared 'x';
SQL
pkill -9 postgres
./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile start
psql <<SQL
	begin;
	insert into t values (42);

	-- This prepare can fail due to 2pc identifier or locks conflict if replay
	-- didn't clean proc, gxact and locks on commit.
	prepare transaction 'x';
SQL



cat <<MSG
###############################################################################
# Check that we can commit while running active sync slave and that there is no
# active prepared transaction on slave after that.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
reinit_slave >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
	commit prepared 'x';
SQL
echo "Following list should be empty:"
psql -tc 'select * from pg_prepared_xacts;' -p 5433


cat <<MSG
###############################################################################
# The same as in previous case, but let's force checkpoint on slave between
# prepare and commit.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
reinit_slave >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
SQL
psql -p 5433 <<SQL
	checkpoint;
SQL
psql <<SQL
	commit prepared 'x';
SQL
echo "Following list should be empty:"
psql -tc 'select * from pg_prepared_xacts;' -p 5433


cat <<MSG
###############################################################################
# Check that we can commit transaction on promoted slave.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
reinit_slave >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
SQL
kill -9 `cat install/data/postmaster.pid | head -n 1`
./install/bin/pg_ctl promote -D ./install/data_slave
sleep 1 # is there a clever way to wait for promotion?
psql -p 5433 <<SQL
	commit prepared 'x';
SQL


cat <<MSG
###############################################################################
# Check that we restore prepared xacts after slave soft restart while master is
# down.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
reinit_slave >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	insert into t values (100);
SQL
./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile stop
./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/data_slave/logfile restart
echo "Following list should contain transaction 'x':"
psql -p5433 <<SQL
	select * from pg_prepared_xacts;
SQL


cat <<MSG
###############################################################################
# Check that we restore prepared xacts after slave hard restart while master is
# down.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
reinit_slave >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	insert into t values (100);
SQL
./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile stop
kill -9 `cat install/data_slave/postmaster.pid | head -n 1`
./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/data_slave/logfile start
echo "Following list should contain transaction 'x':"
psql -p5433 <<SQL
	select * from pg_prepared_xacts;
SQL


cat <<MSG
###############################################################################
# Check that we can replay several tx with same name.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
	commit prepared 'x';
	begin;
	insert into t values (42);
	prepare transaction 'x';
SQL
pkill -9 postgres
./install/bin/pg_ctl -sw -D ./install/data -l ./install/data/logfile start
psql <<SQL
	commit prepared 'x';
SQL


