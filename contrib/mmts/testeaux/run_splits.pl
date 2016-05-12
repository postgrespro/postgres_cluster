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

my $pgbench1 = IPC::Run::start (
	'pgbench',
	-c => 4,
	-T => 30,
	-P => 1,
	-h => $cluster->{nodes}->[0]->{_host},
	-U => $cluster->{nodes}->[0]->{_user},
	'postgres'
);

sleep 5;
$cluster->{nodes}->[0]->net_deny_in;
sleep 10;
$cluster->{nodes}->[0]->net_allow;


IPC::Run::finish($pgbench1);
# IPC::Run::run @pgbench_init






