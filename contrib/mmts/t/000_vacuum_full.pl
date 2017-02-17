use strict;
use warnings;

use Cluster;
use TestLib;
use Test::More tests => 1;
use IPC::Run qw(start finish);
use Cwd;

my $nnodes = 2;
my $cluster = new Cluster($nnodes);

$cluster->init();
$cluster->configure();
$cluster->start();
diag("sleeping 10");
sleep(10);

my ($in, $out, $err, $rc);
my $seconds = 30;
$in = '';
$out = '';

my @init_argv = (
	'pgbench',
	'-i',
	-h => $cluster->{nodes}->[0]->host(),
	-p => $cluster->{nodes}->[0]->port(),
	'postgres',
);
diag("running pgbench init");
my $init_run = start(\@init_argv, $in, $out);
finish($init_run) || BAIL_OUT("pgbench exited with $?");

my @bench_argv = (
	'pgbench',
	'-T 30',
	'-N',
	'-c 8',
	-h => $cluster->{nodes}->[0]->host(),
	-p => $cluster->{nodes}->[0]->port(),
	'postgres',
);
diag("running pgbench: " . join(' ', @bench_argv));
my $bench_run = start(\@bench_argv, $in, $out);

my $started = time();
while (time() - $started < $seconds)
{
	($rc, $out, $err) = $cluster->psql(2, 'postgres', "vacuum full;");
	if ($rc != 0)
	{
		BAIL_OUT("vacuum full failed");
	}
}

finish($bench_run) || BAIL_OUT("pgbench exited with $?");
ok($cluster->stop('fast'), "cluster stops");
1;
