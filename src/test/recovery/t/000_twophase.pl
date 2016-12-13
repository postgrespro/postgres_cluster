# Tests dedicated to two-phase commit in recovery
use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 13;

# Setup master node
my $node_master = get_new_node("master");
$node_master->init(allows_streaming => 1);
$node_master->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 10
));
$node_master->start;
$node_master->backup('master_backup');
$node_master->psql('postgres', "create table t(id int)");

# Setup master node
my $node_slave = get_new_node('slave');
$node_slave->init_from_backup($node_master, 'master_backup', has_streaming => 1);
$node_slave->start;

# Switch to synchronous replication
$node_master->append_conf('postgresql.conf', qq(
	synchronous_standby_names = '*'
));
$node_master->psql('postgres', "select pg_reload_conf()");

my $psql_out = '';
my $psql_rc = '';

###############################################################################
# Check that we can commit and abort tx after soft restart.
# Here checkpoint happens before shutdown and no WAL replay will occur at next
# startup. In this case postgres re-create shared-memory state from twophase
# files.
###############################################################################

$node_master->psql('postgres', "
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	begin;
	insert into t values (142);
	savepoint s1;
	insert into t values (143);
	prepare transaction 'y';");
$node_master->stop;
$node_master->start;

$psql_rc = $node_master->psql('postgres', "commit prepared 'x'");
is($psql_rc, '0', 'Commit prepared transaction after restart.');

$psql_rc = $node_master->psql('postgres', "rollback prepared 'y'");
is($psql_rc, '0', 'Rollback prepared transaction after restart.');

###############################################################################
# Check that we can commit and abort after hard restart.
# At next startup, WAL replay will re-create shared memory state for prepared
# transaction using dedicated WAL records.
###############################################################################

$node_master->psql('postgres', "
	checkpoint;
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	begin;
	insert into t values (142);
	savepoint s1;
	insert into t values (143);
	prepare transaction 'y';");
$node_master->teardown_node;
$node_master->start;

$psql_rc = $node_master->psql('postgres', "commit prepared 'x'");
is($psql_rc, '0', 'Commit prepared tx after teardown.');

$psql_rc = $node_master->psql('postgres', "rollback prepared 'y'");
is($psql_rc, '0', 'Rollback prepared transaction after teardown.');

###############################################################################
# Check that WAL replay can handle several transactions with same name GID.
###############################################################################

$node_master->psql('postgres', "
	checkpoint;
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	commit prepared 'x';
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';");
$node_master->teardown_node;
$node_master->start;

$psql_rc = $node_master->psql('postgres', "commit prepared 'x'");
is($psql_rc, '0', 'Replay several transactions with same GID.');

###############################################################################
# Check that WAL replay cleans up its shared memory state and releases locks
# while replaying transaction commits.
###############################################################################

$node_master->psql('postgres', "
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	commit prepared 'x';");
$node_master->teardown_node;
$node_master->start;
$psql_rc = $node_master->psql('postgres', "begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	-- This prepare can fail due to conflicting GID or locks conflicts if
	-- replay did not fully cleanup its state on previous commit.
	prepare transaction 'x';");
is($psql_rc, '0', "Cleanup of shared memory state for 2PC commit");

$node_master->psql('postgres', "commit prepared 'x'");

###############################################################################
# Check that WAL replay will cleanup its shared memory state on running slave.
###############################################################################

$node_master->psql('postgres', "
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';
	commit prepared 'x';");
$node_slave->psql('postgres', "select count(*) from pg_prepared_xacts;",
	  stdout => \$psql_out);
is($psql_out, '0',
   "Cleanup of shared memory state on running standby without checkpoint.");

###############################################################################
# Same as in previous case, but let's force checkpoint on slave between
# prepare and commit to use on-disk twophase files.
###############################################################################

$node_master->psql('postgres', "
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';");
$node_slave->psql('postgres', "checkpoint;");
$node_master->psql('postgres', "commit prepared 'x';");
$node_slave->psql('postgres', "select count(*) from pg_prepared_xacts;",
	  stdout => \$psql_out);
is($psql_out, '0',
   "Cleanup of shared memory state on running standby after checkpoint.");

###############################################################################
# Check that prepared transactions can be committed on promoted slave.
###############################################################################

$node_master->psql('postgres', "
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';");
$node_master->teardown_node;
$node_slave->promote;
$node_slave->poll_query_until('postgres', "SELECT pg_is_in_recovery() <> true");

$psql_rc = $node_slave->psql('postgres', "commit prepared 'x';");
is($psql_rc, '0', "Restore of prepared transaction on promoted slave.");

# change roles
($node_master, $node_slave) = ($node_slave, $node_master);
$node_slave->enable_streaming($node_master);
$node_slave->append_conf('recovery.conf', qq(
recovery_target_timeline='latest'
));
$node_slave->start;

###############################################################################
# Check that prepared transactions are replayed after soft restart of standby
# while master is down. Since standby knows that master is down it uses
# different code path on start to be sure that the status of transactions is
# consistent.
###############################################################################

$node_master->psql('postgres', "
	begin;
	insert into t values (42);
	savepoint s1;
	insert into t values (43);
	prepare transaction 'x';");
$node_master->stop;
$node_slave->restart;
$node_slave->promote;
$node_slave->poll_query_until('postgres', "SELECT pg_is_in_recovery() <> true");

$node_slave->psql('postgres', "select count(*) from pg_prepared_xacts",
	  stdout => \$psql_out);
is($psql_out, '1',
   "Restore prepared transactions from files with master down.");

# restore state
($node_master, $node_slave) = ($node_slave, $node_master);
$node_slave->enable_streaming($node_master);
$node_slave->append_conf('recovery.conf', qq(
recovery_target_timeline='latest'
));
$node_slave->start;
$node_master->psql('postgres', "commit prepared 'x'");

###############################################################################
# Check that prepared transactions are correctly replayed after slave hard
# restart while master is down.
###############################################################################

$node_master->psql('postgres', "
	begin;
	insert into t values (242);
	savepoint s1;
	insert into t values (243);
	prepare transaction 'x';
	");
$node_master->stop;
$node_slave->teardown_node;
$node_slave->start;
$node_slave->promote;
$node_slave->poll_query_until('postgres',
	  "SELECT pg_is_in_recovery() <> true");

$node_slave->psql('postgres', "select count(*) from pg_prepared_xacts",
	  stdout => \$psql_out);
is($psql_out, '1',
   "Restore prepared transactions from records with master down.");

# restore state
($node_master, $node_slave) = ($node_slave, $node_master);
$node_slave->enable_streaming($node_master);
$node_slave->append_conf('recovery.conf', qq(
recovery_target_timeline='latest'
));
$node_slave->start;
$node_master->psql('postgres', "commit prepared 'x'");


###############################################################################
# Check for a lock confcict between prepared tx with DDL inside and replay of
# XLOG_STANDBY_LOCK wal record.
###############################################################################

$node_master->psql('postgres', "
	begin;
	create table t2(id int);
	savepoint s1;
	insert into t2 values (42);
	prepare transaction 'x';
	-- checkpoint will issue XLOG_STANDBY_LOCK that can conflict with lock
	-- held by 'create table' statement
	checkpoint;
	commit prepared 'x';");

$node_slave->psql('postgres', "select count(*) from pg_prepared_xacts",
	  stdout => \$psql_out);
is($psql_out, '0', "Replay prepared transaction with DDL.");


###############################################################################
# Check that replay will correctly set SUBTRANS and properly andvance nextXid
# so it won't conflict with savepoint xids.
###############################################################################

$node_master->psql('postgres', "
	begin;
	delete from t;
	insert into t values (43);
	savepoint s1;
	insert into t values (43);
	savepoint s2;
	insert into t values (43);
	savepoint s3;
	insert into t values (43);
	savepoint s4;
	insert into t values (43);
	savepoint s5;
	insert into t values (43);
	prepare transaction 'x';
	checkpoint;");

$node_master->stop;
$node_master->start;
$node_master->psql('postgres', "
	-- here we can get xid of previous savepoint if nextXid
	-- wasn't properly advanced
	begin;
	insert into t values (142);
	abort;
	commit prepared 'x';");

$node_master->psql('postgres', "select count(*) from t",
	  stdout => \$psql_out);
is($psql_out, '6', "Check nextXid handling for prepared subtransactions");