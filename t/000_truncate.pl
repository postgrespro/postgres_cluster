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
	"-T $seconds",
	'-N',
	'-c 8',
	-h => $cluster->{nodes}->[0]->host(),
	-p => $cluster->{nodes}->[0]->port(),
	'postgres',
);
diag("running pgbench: " . join(' ', @bench_argv));
my $bench_run = start(\@bench_argv, $in, $out);
sleep(2);

my $started = time();
while (time() - $started < $seconds)
{
	($rc, $out, $err) = $cluster->psql(1, 'postgres', "truncate pgbench_history;");
	($rc, $out, $err) = $cluster->psql(1, 'postgres', "vacuum full");
	($rc, $out, $err) = $cluster->psql(0, 'postgres', "truncate pgbench_history;");
	($rc, $out, $err) = $cluster->psql(0, 'postgres', "vacuum full");
	sleep(0.5);
}

finish($bench_run) || $cluster->bail_out_with_logs("pgbench exited with $?");
sleep(1);
ok($cluster->stop('fast'), "cluster stops");
1;
