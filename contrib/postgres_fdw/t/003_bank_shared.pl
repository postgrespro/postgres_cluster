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
	$node->safe_psql('postgres', qq[
		CREATE EXTENSION postgres_fdw;
		CREATE TABLE accounts(id integer primary key, amount integer);
	]);

	foreach my $neighbor (@shards)
	{
		next if ($neighbor eq $node);

		my $port = $neighbor->port;
		my $host = $neighbor->host;

		$node->safe_psql('postgres', qq[
			CREATE SERVER shard_$port FOREIGN DATA WRAPPER postgres_fdw
					options(dbname 'postgres', host '$host', port '$port');
			CREATE FOREIGN TABLE
					accounts_fdw(id integer, amount integer)
					server shard_$port options(table_name 'accounts');
			CREATE USER MAPPING for stas SERVER shard_$port
					options (user 'stas');
		]);
	}

}

$shard1->psql('postgres', "insert into accounts select id, 0 from generate_series(1, 20020) as id;");
$shard2->psql('postgres', "insert into accounts select id, 0 from generate_series(1, 20020) as id;");


# diag("shard1: @{[$shard1->connstr('postgres')]}");
# diag("shard1: @{[$shard2->connstr('postgres')]}");
# sleep(3600);


###############################################################################
# pgbench scripts
###############################################################################

my $bank = File::Temp->new();
append_to_file($bank, q{
	\set id random(1, 20000)
	BEGIN;
	UPDATE accounts SET amount = amount - 1 WHERE id = :id;
	UPDATE accounts SET amount = amount + 1 WHERE id = (:id + 1);
	UPDATE accounts_fdw SET amount = amount - 1 WHERE id = :id;
	UPDATE accounts_fdw SET amount = amount + 1 WHERE id = (:id + 1);
	COMMIT;
});

###############################################################################
# Concurrent global transactions
###############################################################################

my ($hashes, $hash1, $hash2);
my $stability_errors = 0;
my $selects = 0;
my $seconds = 30;

my $pgb_handle1 = $shard1->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );
# my $pgb_handle2 = $shard2->pgbench_async(-n, -c => 5, -T => $seconds, -f => $bank, 'postgres' );
my $started = time();
while (time() - $started < $seconds)
{
	foreach my $node ($shard1, $shard2)
	{
		($hash1, $_, $hash2) = split "\n", $node->safe_psql('postgres', qq[
			begin isolation level repeatable read;
			select md5(array_agg((t.*)::text)::text) from (select * from accounts order by id) as t;
			select pg_sleep(1);
			select md5(array_agg((t.*)::text)::text) from (select * from accounts_fdw order by id) as t;
			commit;
		]);

		if ($hash1 ne $hash2)
		{
			diag("oops");
			$stability_errors++;
		}
		elsif ($hash1 eq '' or $hash2 eq '')
		{
			die;
		}
		else
		{
			diag("got hash $hash1");
			$selects++;
		}
	}
}

$shard1->pgbench_await($pgb_handle1);
# $shard2->pgbench_await($pgb_handle2);

die "no real queries happend" unless ( $selects > 0 );

is($stability_errors, 0, 'snapshot is stable during concurrent global and local transactions');

$shard1->stop;
$shard2->stop;
