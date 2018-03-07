use strict;
use warnings;
use File::Basename;
use File::Spec::Functions 'catfile';

use PostgresNode;
use TestLib;
use Test::More tests => 1;

my $master = get_new_node("master");
$master->init;
$master->append_conf('postgresql.conf', qq(
	max_prepared_transactions = 30
	log_checkpoints = true
	track_global_snapshots = on
	postgres_fdw.use_global_snapshots = on
	postgres_fdw.use_twophase = on
));
$master->start;

my $shard1 = get_new_node("shard1");
$shard1->init;
$shard1->append_conf('postgresql.conf', qq(
	track_global_snapshots = on
	max_prepared_transactions = 30
	log_checkpoints = true
));
$shard1->start;

my $shard2 = get_new_node("shard2");
$shard2->init;
$shard2->append_conf('postgresql.conf', qq(
	track_global_snapshots = on
	max_prepared_transactions = 30
	log_checkpoints = true
));
$shard2->start;

###############################################################################

$master->safe_psql('postgres', "CREATE EXTENSION postgres_fdw");
$master->safe_psql('postgres', "CREATE TABLE accounts(id integer primary key, amount integer)");

foreach my $node ($shard1, $shard2)
{
	my $port = $node->port;
	my $host = $node->host;

	$node->safe_psql('postgres', "CREATE TABLE accounts(id integer primary key, amount integer)");

	$master->safe_psql('postgres', "CREATE SERVER shard_$port FOREIGN DATA WRAPPER postgres_fdw options(dbname 'postgres', host '$host', port '$port')");
	$master->safe_psql('postgres', "CREATE FOREIGN TABLE accounts_fdw_$port() inherits (accounts) server shard_$port options(table_name 'accounts')");
	$master->safe_psql('postgres', "CREATE USER MAPPING for CURRENT_USER SERVER shard_$port");

	# diag("done $host $port");
}

$shard1->safe_psql('postgres', "insert into accounts select 2*id-1, 0 from generate_series(1, 10010) as id;");
$shard2->safe_psql('postgres', "insert into accounts select 2*id, 0 from generate_series(1, 10010) as id;");

# diag( $master->connstr() );
# sleep(3600);

###############################################################################

my ($err, $rc);
my $seconds = 30;
my $total = '0';
my $oldtotal = '0';
my $isolation_error = 0;

my $pgb_path = catfile(dirname(__FILE__), "bank.pgb");
$master->pgbench(-n, -c => 20, -t => 30, -f => "$pgb_path", 'postgres' );
my $pgb_handle = $master->pgbench_async(-n, -c => 5, -T => $seconds, -f => "$pgb_path", 'postgres' );

my $started = time();
while (time() - $started < $seconds)
{
	($rc, $total, $err) = $master->safe_psql('postgres', "select sum(amount) from accounts");
	if ( ($total ne $oldtotal) and ($total ne '') )
	{
		$isolation_error = 1;
		$oldtotal = $total;
		diag("Isolation error. Total = $total");
	}
	# diag("Total = $total");
}

$master->pgbench_await($pgb_handle);

is($isolation_error, 0, 'check proper isolation');

$master->stop;
$shard1->stop;
$shard2->stop;
