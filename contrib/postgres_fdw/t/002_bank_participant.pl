use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 1;

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

$pgb_handle1 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );
$pgb_handle2 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );

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

