use strict;
use warnings;

use lib 'lib';
use RemoteCluster;
use IPC::Run;

my $cluster = new RemoteCluster('ssh-config');
$cluster->init;
$cluster->configure;
$cluster->start;
sleep 10;

$cluster->{nodes}->[0]->psql('postgres','create extension if not exists multimaster');
$cluster->{nodes}->[0]->psql('postgres','select * from mtm.get_nodes_state();');

print("Initializing database\n");
IPC::Run::run (
	'pgbench',
	'-i',
	-h => $cluster->{nodes}->[0]->{_host},
	-U => $cluster->{nodes}->[0]->{_user},
	'postgres'
);

my $out = '';
my $err = '';
my $in = '';

my @pgb_params = (
	'pgbench',
	-c => 4,
	-T => 30,
	-P => 1,
	-h => $cluster->{nodes}->[0]->{_host},
	-U => $cluster->{nodes}->[0]->{_user},
	'postgres'
);



my $pgbench1 = IPC::Run::start @pgb_params; #, \$in, \$out, \$err;

sleep 5;
$cluster->{nodes}->[2]->net_deny_in;
$cluster->{nodes}->[2]->net_deny_out;
sleep 10;
$cluster->{nodes}->[2]->net_allow;

IPC::Run::finish($pgbench1);

print("---stdout--\n$out\n");
print("---stderr--\n$err\n");




