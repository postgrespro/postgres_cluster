use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 3;

my $shard1 = get_new_node("shard1");
$shard1->init;
$shard1->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
	postgres_fdw.use_global_snapshots = on
	global_snapshot_defer_time = 15
	track_global_snapshots = on
    default_transaction_isolation = 'REPEATABLE READ'
));
$shard1->start;

my $shard2 = get_new_node("shard2");
$shard2->init;
$shard2->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
	postgres_fdw.use_global_snapshots = on
	global_snapshot_defer_time = 15
	track_global_snapshots = on
	default_transaction_isolation = 'REPEATABLE READ'
));
$shard2->start;

###############################################################################
# Prepare nodes
###############################################################################

my @shards = ($shard1, $shard2);

foreach my $node (@shards)
{
	$node->safe_psql('postgres', qq[
		CREATE EXTENSION postgres_fdw;
		CREATE TABLE accounts(id integer primary key, amount integer);
		CREATE TABLE accounts_local() inherits(accounts);
		CREATE TABLE global_transactions(tx_time timestamp);
		CREATE TABLE local_transactions(tx_time timestamp);
	]);

	foreach my $neighbor (@shards)
	{
		next if ($neighbor eq $node);

		my $port = $neighbor->port;
		my $host = $neighbor->host;

		$node->safe_psql('postgres', qq[
			CREATE SERVER shard_$port FOREIGN DATA WRAPPER postgres_fdw
					options(dbname 'postgres', host '$host', port '$port');
			CREATE FOREIGN TABLE accounts_fdw_$port() inherits (accounts)
					server shard_$port options(table_name 'accounts_local');
			CREATE USER MAPPING for CURRENT_USER SERVER shard_$port;
		]);
	}
}

$shard1->psql('postgres', "insert into accounts_local select 2*id-1, 0 from generate_series(1, 10010) as id;");
$shard2->psql('postgres', "insert into accounts_local select 2*id,   0 from generate_series(1, 10010) as id;");

###############################################################################
# pgbench scripts
###############################################################################

my $bank = File::Temp->new();
append_to_file($bank, q{
	\set id random(1, 20000)
	BEGIN;
	WITH upd AS (UPDATE accounts SET amount = amount - 1 WHERE id = :id RETURNING *)
		INSERT into global_transactions SELECT now() FROM upd;
	UPDATE accounts SET amount = amount + 1 WHERE id = (:id + 1);
	COMMIT;
});

###############################################################################
# Helpers
###############################################################################

sub count_and_delete_rows
{
	my ($node, $table) = @_;
	my $count;

	$count = $node->safe_psql('postgres',"select count(*) from $table");
	$node->safe_psql('postgres',"delete from $table");
	diag($node->name, ": completed $count transactions");
	return $count;
}

###############################################################################
# Concurrent global transactions
###############################################################################

my ($err, $rc);
my $started;
my $seconds = 30;
my $selects;
my $total = '0';
my $oldtotal = '0';
my $isolation_errors = 0;
my $i;


my ($pgb_handle1, $pgb_handle2);

$pgb_handle1 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );
$pgb_handle2 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );

$started = time();
$selects = 0;
$i = 0;
while (time() - $started < $seconds)
{
	my $shard = $shard1;
	foreach my $shard (@shards)
	{
		$total = $shard->safe_psql('postgres', "select sum(amount) from accounts");
		if ( ($total ne $oldtotal) and ($total ne '') )
		{
			$isolation_errors++;
			$oldtotal = $total;
			diag("$i: Isolation error. Total = $total");
		}
		if ($total ne '') { $selects++; }
	}
	$i++;
}

$shard1->pgbench_await($pgb_handle1);
$shard2->pgbench_await($pgb_handle2);

# sanity check
diag("completed $selects selects");
die "no actual transactions happend" unless ( $selects > 0 &&
	count_and_delete_rows($shard1, 'global_transactions') > 0 &&
	count_and_delete_rows($shard2, 'global_transactions') > 0);

is($isolation_errors, 0, 'isolation between concurrent global transaction');

###############################################################################
# And do the same after soft restart
###############################################################################

$shard1->restart;
$shard2->restart;
$shard1->poll_query_until('postgres', "select 't'")
	or die "Timed out waiting for shard1 to became online";
$shard2->poll_query_until('postgres', "select 't'")
	or die "Timed out waiting for shard2 to became online";

$seconds = 15;
$pgb_handle1 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );
$pgb_handle2 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );

$started = time();
$selects = 0;
$i = 0;

while (time() - $started < $seconds)
{
	my $shard = $shard1;
	foreach my $shard (@shards)
	{
		$total = $shard->safe_psql('postgres', "select sum(amount) from accounts");
		if ( ($total ne $oldtotal) and ($total ne '') )
		{
			$isolation_errors++;
			$oldtotal = $total;
			diag("$i: Isolation error. Total = $total");
		}
		if ($total ne '') { $selects++; }
	}
	$i++;
}

$shard1->pgbench_await($pgb_handle1);
$shard2->pgbench_await($pgb_handle2);

# sanity check
diag("completed $selects selects");
die "no actual transactions happend" unless ( $selects > 0 &&
	count_and_delete_rows($shard1, 'global_transactions') > 0 &&
	count_and_delete_rows($shard2, 'global_transactions') > 0);

is($isolation_errors, 0, 'isolation between concurrent global transaction after restart');

###############################################################################
# And do the same after hard restart
###############################################################################

$shard1->teardown_node;
$shard2->teardown_node;
$shard1->start;
$shard2->start;
$shard1->poll_query_until('postgres', "select 't'")
	or die "Timed out waiting for shard1 to became online";
$shard2->poll_query_until('postgres', "select 't'")
	or die "Timed out waiting for shard2 to became online";


$seconds = 15;
$pgb_handle1 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );
$pgb_handle2 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );

$started = time();
$selects = 0;
$i = 0;

while (time() - $started < $seconds)
{
	my $shard = $shard1;
	foreach my $shard (@shards)
	{
		$total = $shard->safe_psql('postgres', "select sum(amount) from accounts");
		if ( ($total ne $oldtotal) and ($total ne '') )
		{
			$isolation_errors++;
			$oldtotal = $total;
			diag("$i: Isolation error. Total = $total");
		}
		if ($total ne '') { $selects++; }
	}
	$i++;
}

$shard1->pgbench_await($pgb_handle1);
$shard2->pgbench_await($pgb_handle2);

# sanity check
diag("completed $selects selects");
die "no actual transactions happend" unless ( $selects > 0 &&
	count_and_delete_rows($shard1, 'global_transactions') > 0 &&
	count_and_delete_rows($shard2, 'global_transactions') > 0);

is($isolation_errors, 0, 'isolation between concurrent global transaction after hard restart');
