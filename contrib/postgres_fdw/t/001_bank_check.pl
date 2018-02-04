use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 3;

my $master = get_new_node("master");
$master->init;
$master->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
	log_checkpoints = true
	postgres_fdw.use_tsdtm = on
));
$master->start;

my $shard1 = get_new_node("shard1");
$shard1->init;
$shard1->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
));
$shard1->start;

my $shard2 = get_new_node("shard2");
$shard2->init;
$shard2->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
));
$shard2->start;

###############################################################################
# Prepare nodes
###############################################################################

$master->psql('postgres', "CREATE EXTENSION postgres_fdw");
$master->psql('postgres', "CREATE TABLE accounts(id integer primary key, amount integer)");
$master->psql('postgres', "CREATE TABLE global_transactions(tx_time timestamp)");

foreach my $node ($shard1, $shard2)
{
	my $port = $node->port;
	my $host = $node->host;

	$node->psql('postgres', "CREATE TABLE accounts(id integer primary key, amount integer)");

	$master->psql('postgres', "CREATE SERVER shard_$port FOREIGN DATA WRAPPER postgres_fdw options(dbname 'postgres', host '$host', port '$port')");
	$master->psql('postgres', "CREATE FOREIGN TABLE accounts_fdw_$port() inherits (accounts) server shard_$port options(table_name 'accounts')");
	$master->psql('postgres', "CREATE USER MAPPING for stas SERVER shard_$port options (user 'stas')");
}

$shard1->psql('postgres', "insert into accounts select 2*id-1, 0 from generate_series(1, 10010) as id;");
$shard1->psql('postgres', "CREATE TABLE local_transactions(tx_time timestamp)");

$shard2->psql('postgres', "insert into accounts select 2*id, 0 from generate_series(1, 10010) as id;");
$shard2->psql('postgres', "CREATE TABLE local_transactions(tx_time timestamp)");

$master->pgbench(-n, -c => 20, -t => 30, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );

###############################################################################
# Helpers
###############################################################################

sub count_and_delete_rows
{
	my ($node, $table) = @_;
	my ($rc, $count, $err);

	($rc, $count, $err) = $node->psql('postgres',"select count(*) from $table",
									  on_error_die => 1);

	die "count_rows: $err" if ($err ne '');

	$node->psql('postgres',"delete from $table", on_error_die => 1);

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


my $pgb_handle;

$pgb_handle = $master->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );

$started = time();
$selects = 0;
while (time() - $started < $seconds)
{
	($rc, $total, $err) = $master->psql('postgres', "select sum(amount) from accounts");
	if ( ($total ne $oldtotal) and ($total ne '') )
	{
		$isolation_errors++;
		$oldtotal = $total;
		diag("Isolation error. Total = $total");
	}
	if (($err eq '') and ($total ne '') ) { $selects++; }
}

$master->pgbench_await($pgb_handle);

# sanity check
diag("completed $selects selects");
die "no actual transactions happend" unless ( $selects > 0 &&
	count_and_delete_rows($master, 'global_transactions') > 0);

is($isolation_errors, 0, 'isolation between concurrent global transaction');

###############################################################################
# Concurrent global and local transactions
###############################################################################

my ($pgb_handle1, $pgb_handle2, $pgb_handle3);

# global txses
$pgb_handle1 = $master->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );

# concurrent local
$pgb_handle2 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank1.sql", 'postgres' );
$pgb_handle3 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank2.sql", 'postgres' );

$started = time();
$selects = 0;
$oldtotal = 0;
while (time() - $started < $seconds)
{
	($rc, $total, $err) = $master->psql('postgres', "select sum(amount) from accounts");
	if ( ($total ne $oldtotal) and ($total ne '') )
	{
		$isolation_errors++;
		$oldtotal = $total;
		diag("Isolation error. Total = $total");
	}
	if (($err eq '') and ($total ne '') ) { $selects++; }
}

diag("selects = $selects");
$master->pgbench_await($pgb_handle1);
$shard1->pgbench_await($pgb_handle2);
$shard2->pgbench_await($pgb_handle3);

diag("completed $selects selects");
die "" unless ( $selects > 0 &&
	count_and_delete_rows($master, 'global_transactions') > 0 &&
	count_and_delete_rows($shard1, 'local_transactions') > 0 &&
	count_and_delete_rows($shard2, 'local_transactions') > 0);

is($isolation_errors, 0, 'isolation between concurrent global and local transactions');


###############################################################################
# Snapshot stability
###############################################################################

my ($hashes, $hash1, $hash2);
my $stability_errors = 0;
my $stable;

# global txses
$pgb_handle1 = $master->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );
# concurrent local
$pgb_handle2 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank1.sql", 'postgres' );
$pgb_handle3 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank2.sql", 'postgres' );

$selects = 0;
$started = time();
while (time() - $started < $seconds)
{
	foreach my $node ($master, $shard1, $shard2)
	{
		($hash1, $_, $hash2) = split "\n", $node->safe_psql('postgres', qq[
			begin isolation level repeatable read;
			select md5(array_agg((t.*)::text)::text) from (select * from accounts order by id) as t;
			select pg_sleep(1);
			select md5(array_agg((t.*)::text)::text) from (select * from accounts order by id) as t;
			commit;
		]);

		if ($hash1 ne $hash2)
		{
			$stability_errors++;
		}
		elsif ($hash1 eq '' or $hash2 eq '')
		{
			die;
		}
		else
		{
			$selects++;
		}
	}
}

$master->pgbench_await($pgb_handle1);
$shard1->pgbench_await($pgb_handle2);
$shard2->pgbench_await($pgb_handle3);

die "" unless ( $selects > 0 &&
	count_and_delete_rows($master, 'global_transactions') > 0 &&
	count_and_delete_rows($shard1, 'local_transactions') > 0 &&
	count_and_delete_rows($shard2, 'local_transactions') > 0);

is($stability_errors, 0, 'snapshot is stable during concurrent global and local transactions');

$master->stop;
$shard1->stop;
$shard2->stop;
