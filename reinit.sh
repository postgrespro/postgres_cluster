#!/bin/sh

# This script assumes that it is placed in postgres repository root,
# and postgres configured with --prefix=$PG_REPO_ROOT/install

reinit_master() {
	rm -rf install/data

	./install/bin/initdb -A trust -D ./install/data

	echo "max_prepared_transactions = 100" >> ./install/data/postgresql.conf
	echo "shared_buffers = 512MB" >> ./install/data/postgresql.conf
	# echo "fsync = off" >> ./install/data/postgresql.conf
	echo "log_checkpoints = on" >> ./install/data/postgresql.conf
	echo "max_wal_size = 48MB" >> ./install/data/postgresql.conf
	echo "min_wal_size = 32MB" >> ./install/data/postgresql.conf
	echo "wal_level = hot_standby" >> ./install/data/postgresql.conf
	echo "wal_keep_segments = 64" >> ./install/data/postgresql.conf
	echo "max_wal_senders = 2" >> ./install/data/postgresql.conf
	echo "max_replication_slots = 2" >> ./install/data/postgresql.conf

	echo '---- test ----' >> ./install/logfile

	echo "local replication `whoami` trust" >> ./install/data/pg_hba.conf

	./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile start
}

reinit_slave() {
	rm -rf install/data_slave

	./install/bin/pg_basebackup -D ./install/data_slave/ -R

	echo "port = 5433" >> ./install/data_slave/postgresql.conf
	echo "hot_standby = on" >> ./install/data_slave/postgresql.conf

	echo '---- test ----' >> ./install/slave_logfile

	./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/slave_logfile start


	echo "synchronous_standby_names = '*'" >> ./install/data/postgresql.conf
	./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile restart
}

postinit(){
	./install/bin/createdb `whoami`
	./install/bin/psql -c "create table t(id int);"
}

make install > /dev/null
echo > ./install/logfile
echo > ./install/slave_logfile


cat <<MSG
###############################################################################
# Check that we can commit and abort after soft restart.
# Here checkpoint happens before shutdown and no WAL replay will not occur
# during start. So code should re-create memory state from files.
###############################################################################
MSG

pkill -9 postgres
reinit_master >> /dev/null
postinit >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
	begin;
	insert into t values (43);
	prepare transaction 'y';
SQL
./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile restart
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
postinit >> /dev/null
psql <<SQL
	checkpoint;
	select * from pg_current_xlog_location();
	begin;
	insert into t values (42);
	prepare transaction 'x';
	select * from pg_current_xlog_location();
	begin;
	insert into t values (43);
	prepare transaction 'y';
SQL
pkill -9 postgres
echo '--- kill -9 ---' >> ./install/logfile
./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile start
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
postinit >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	savepoint s;
	insert into t values (43);
	prepare transaction 'x';
	commit prepared 'x';
SQL
pkill -9 postgres
./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile start
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
postinit >> /dev/null
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
postinit >> /dev/null
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
echo "master init"
reinit_slave >> /dev/null
echo "slave init"
postinit >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	prepare transaction 'x';
	--select * from pg_current_xlog_location();
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
postinit >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	insert into t values (100);
SQL
./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile stop
./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/slave_logfile restart
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
postinit >> /dev/null
psql <<SQL
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	insert into t values (100);
SQL
./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile stop
kill -9 `cat install/data_slave/postmaster.pid | head -n 1`
./install/bin/pg_ctl -sw -D ./install/data_slave -l ./install/slave_logfile start
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
postinit >> /dev/null
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
./install/bin/pg_ctl -sw -D ./install/data -l ./install/logfile start
psql <<SQL
	commit prepared 'x';
SQL



# check for prescan with pgbench










