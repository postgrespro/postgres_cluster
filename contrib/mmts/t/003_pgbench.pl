use strict;
use warnings;

use Cluster;
use TestLib;
use Test::More tests => 3;
use IPC::Run qw(start finish);
use Cwd;

my $nnodes = 2;
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
	$cluster->bail_out_with_logs('failed to create t');
}

if ($cluster->psql(0, 'postgres', "insert into t (select generate_series(0, 999), 0)"))
{
	$cluster->bail_out_with_logs('failed to fill t');
}

if ($cluster->psql(0, 'postgres', "create table reader_log (v int)"))
{
	$cluster->bail_out_with_logs('failed to create reader_log');
}

sub reader
{
	my ($node, $inref, $outref) = @_;

	my $clients = 1;
	my $jobs = 1;
	my $seconds = 30;
	my $tps = 10;
	my @argv = (
		'pgbench',
		'-n',
		-c => $clients,
		-j => $jobs,
		-T => $seconds,
		-h => $node->host(),
		-p => $node->port(),
		-f => 'tests/writer.pgb',
		-R => $tps,
		'postgres',
	);

	diag("running[" . getcwd() . "]: " . join(' ', @argv));

	return start(\@argv, $inref, $outref);
}

sub writer
{
	my ($node, $inref, $outref) = @_;

	my $clients = 5;
	my $jobs = 1;
	my $seconds = 30;
	my @argv = (
		'pgbench',
		'-n',
		-c => $clients,
		-j => $jobs,
		-T => $seconds,
		-h => $node->host(),
		-p => $node->port(),
		-f => 'tests/reader.pgb',
		'postgres',
	);

	diag("running[" . getcwd() . "]: " . join(' ', @argv));

	return start(\@argv, $inref, $outref);
}

diag("starting benches");
$in = '';
$out = '';
my @benches = ();
foreach my $node (@{$cluster->{nodes}})
{
	push(@benches, writer($node, \$in, \$out));
	push(@benches, reader($node, \$in, \$out));
}

diag("finishing benches");
foreach my $bench (@benches)
{
	finish($bench) || $cluster->bail_out_with_logs("pgbench exited with $?");
}
diag($out);

diag("checking readers' logs");

($rc, $out, $err) = $cluster->psql(0, 'postgres', "select count(*) from reader_log where v != 0;");
is($out, 0, "there is nothing except zeros in reader_log");

($rc, $out, $err) = $cluster->psql(0, 'postgres', "select count(*) from reader_log where v = 0;");
isnt($out, 0, "there are some zeros in reader_log");

ok($cluster->stop('fast'), "cluster stops");
1;
