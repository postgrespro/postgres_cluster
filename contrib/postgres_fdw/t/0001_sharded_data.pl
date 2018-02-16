use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 1;

# my $master = get_new_node("master");
# $master->init;
# $master->append_conf('postgresql.conf', qq(
# 	max_prepared_transactions = 30
# 	log_checkpoints = true
# 	postgres_fdw.use_tsdtm = on
# ));
# $master->start;

my $shard1 = get_new_node("shard1");
$shard1->init;
$shard1->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
	postgres_fdw.use_tsdtm = on
));
$shard1->start;

my $shard2 = get_new_node("shard2");
$shard2->init;
$shard2->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
	postgres_fdw.use_tsdtm = on
));
$shard2->start;

###############################################################################
# Prepare nodes
###############################################################################

my @shards = ($shard1, $shard2);

foreach my $node (@shards)
{
	$node->safe_psql('postgres', "CREATE EXTENSION postgres_fdw");
	$node->safe_psql('postgres', "CREATE TABLE accounts(id integer primary key, amount integer)");
	$node->safe_psql('postgres', "CREATE TABLE accounts_local() inherits(accounts)");
	$node->safe_psql('postgres', "CREATE TABLE global_transactions(tx_time timestamp)");
	$node->safe_psql('postgres', "CREATE TABLE local_transactions(tx_time timestamp)");

	foreach my $neighbor (@shards)
	{
		next if ($neighbor eq $node);

		my $port = $neighbor->port;
		my $host = $neighbor->host;

		$node->safe_psql('postgres', "CREATE SERVER shard_$port FOREIGN DATA WRAPPER postgres_fdw options(dbname 'postgres', host '$host', port '$port')");
		$node->safe_psql('postgres', "CREATE FOREIGN TABLE accounts_fdw_$port() inherits (accounts) server shard_$port options(table_name 'accounts_local')");
		$node->safe_psql('postgres', "CREATE USER MAPPING for stas SERVER shard_$port options (user 'stas')");
	}

}

diag("\n");
diag( $shard1->connstr('postgres'), "\n" );
diag( $shard2->connstr('postgres'), "\n" );

$shard1->psql('postgres', "insert into accounts_local select 2*id-1, 0 from generate_series(1, 10010) as id;");
$shard2->psql('postgres', "insert into accounts_local select 2*id,   0 from generate_series(1, 10010) as id;");

diag("\n");
diag( $shard1->connstr('postgres'), "\n" );
diag( $shard2->connstr('postgres'), "\n" );

#sleep(6000);

$shard1->pgbench(-n, -c => 20, -t => 30, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );
$shard2->pgbench(-n, -c => 20, -t => 30, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );

diag("\n");
diag( $shard1->connstr('postgres'), "\n" );
diag( $shard2->connstr('postgres'), "\n" );
# sleep(3600);

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


my ($pgb_handle1, $pgb_handle2);

$pgb_handle1 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );
$pgb_handle2 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$TestLib::log_path/../../t/bank.sql", 'postgres' );

$started = time();
$selects = 0;
my $i = 1;
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
		$i++;
	}
}

$shard1->pgbench_await($pgb_handle1);
$shard2->pgbench_await($pgb_handle2);

# sanity check
diag("completed $selects selects");
die "no actual transactions happend" unless ( $selects > 0 &&
	count_and_delete_rows($shard1, 'global_transactions') > 0 &&
	count_and_delete_rows($shard2, 'global_transactions') > 0);

is($isolation_errors, 0, 'isolation between concurrent global transaction');

