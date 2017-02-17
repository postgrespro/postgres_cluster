use strict;
use warnings;

use Cluster;
use TestLib;
use Test::More tests => 2;
use IPC::Run qw(start finish);
use Cwd;

my $nnodes = 2;
my $nclients = 2;
my $nkeys = $nnodes * $nclients;
my $cluster = new Cluster($nnodes);

$cluster->init();
$cluster->configure();
$cluster->start();

my ($rc, $in, $out, $err);

diag("sleeping 10");
sleep(10);

diag("preparing the tables");
if ($cluster->psql(0, 'postgres', "create table t (k int primary key, v int)"))
{
	BAIL_OUT('failed to create t');
}

if ($cluster->psql(0, 'postgres', "insert into t (select generate_series(0, $nkeys - 1), 0)"))
{
	BAIL_OUT('failed to fill t');
}

sub appender
{
	my ($appender_id, $clients, $seconds, $node, $inref, $outref) = @_;

	my @argv = (
		'pgbench',
		'-n',
		-c => $clients,
		-j => $clients,
		-T => $seconds,
		-h => $node->host(),
		-p => $node->port(),
		-D => "appender_id=$appender_id",
		-D => "clients=$clients",
		-f => 'tests/appender.pgb',
		'postgres',
	);

	diag("running[" . getcwd() . "]: " . join(' ', @argv));

	return start(\@argv, $inref, $outref);
}

sub state_dump
{
	my $state = shift;

	diag("<<<<<");
	while (my ($key, $value) = each(%{$state}))
	{
		diag("$key -> $value");
	}
	diag(">>>>>");
}

sub state_leq
{
	my ($a, $b) = @_;

	while (my ($key, $value) = each(%{$a}))
	{
		if (!exists($b->{$key}))
		{
			diag("b has no key $key\n");
			return 0;
		}

		if ($b->{$key} < $value)
		{
			diag($b->{$key} . " < $value\n");
			return 0;
		}
	}

	return 1;
}

sub parse_state
{
	my $str = shift;
	my $state = {};

	while ($str =~ /(\d+)\|(\d+)/g)
	{
		$state->{$1} = $2;
	}

	return $state;
}

diag("starting appenders");
diag("starting benches");
$in = '';
$out = '';
my @appenders = ();
my $appender_id = 0;
my $seconds = 30;
foreach my $node (@{$cluster->{nodes}})
{
	push(@appenders, appender($appender_id, $nclients, $seconds, $node, \$in, \$out));
	$appender_id++;
}

my $selects = 0;
my $anomalies = 0;
my $started = time();
my $node_id = 0;
my $state_a = undef;
my $state_b = undef;
my $out_a = '';
my $out_b = '';
while (time() - $started < $seconds)
{
	$node_id = ($node_id + 1) % $nnodes;
	$state_a = $state_b;
	$out_a = $out_b;
	($rc, $out, $err) = $cluster->psql($node_id, 'postgres', "select * from t;");
	$selects++;
	$state_b = parse_state($out);
	$out_b = $out;
	if (defined $state_a)
	{
		if (!state_leq($state_a, $state_b) && !state_leq($state_a, $state_b))
		{
			diag("cross anomaly detected:\n===a\n$out_a\n+++b\n$out_b\n---\n");
			$anomalies++;
		}
	}
}

diag("finishing benches");
foreach my $appender (@appenders)
{
	finish($appender) || BAIL_OUT("pgbench exited with $?");
}

is($anomalies, 0, "no cross anomalies after $selects selects");

ok($cluster->stop('fast'), "cluster stops");
1;
