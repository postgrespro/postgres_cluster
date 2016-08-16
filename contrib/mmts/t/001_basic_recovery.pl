use strict;
use warnings;
use Cluster;
use TestLib;
use Test::More tests => 9;

my $cluster = new Cluster(3);
$cluster->init();
$cluster->configure();
$cluster->start();

###############################################################################
# Wait until nodes are up
###############################################################################

my $psql_out;
# XXX: create extension on start and poll_untill status is Online
sleep(10);

###############################################################################
# Replication check
###############################################################################

$cluster->psql(0, 'postgres', "
	create extension multimaster;
	create table if not exists t(k int primary key, v int);
	insert into t values(1, 10);");
$cluster->psql(1, 'postgres', "select v from t where k=1;", stdout => \$psql_out);
is($psql_out, '10', "Check replication while all nodes are up.");

###############################################################################
# Isolation regress checks
###############################################################################

# we can call pg_regress here

###############################################################################
# Work after node stop
###############################################################################

diag("stopping node 2");
if ($cluster->stopid(2, 'immediate')) {
	pass("node 2 stops");
} else {
	fail("node 2 stops");
	if (!$cluster->stopid(2, 'kill')) {
		my $name = $cluster->{nodes}->[2]->name;
		BAIL_OUT("failed to kill $name");
	}
}

sleep(5); # Wait until failure of node will be detected

diag("inserting 2 on node 0");
my $ret = $cluster->psql(0, 'postgres', "insert into t values(2, 20);"); # this transaciton may fail
diag "tx1 status = $ret";

diag("inserting 3 on node 1");
my $ret = $cluster->psql(1, 'postgres', "insert into t values(3, 30);"); # this transaciton may fail
diag "tx2 status = $ret";

diag("inserting 4 on node 0 (can fail)");
my $ret = $cluster->psql(0, 'postgres', "insert into t values(4, 40);"); 
diag "tx1 status = $ret";

diag("inserting 5 on node 1 (can fail)");
my $ret = $cluster->psql(1, 'postgres', "insert into t values(5, 50);"); 
diag "tx2 status = $ret";

diag("selecting");
$cluster->psql(1, 'postgres', "select v from t where k=4;", stdout => \$psql_out);
diag("selected");
is($psql_out, '40', "Check replication after node failure.");

###############################################################################
# Work after node start
###############################################################################

diag("starting node 2");
$cluster->{nodes}->[2]->start;

sleep(5); # Wait until node is started

diag("inserting 6 on node 0 (can fail)");
$cluster->psql(0, 'postgres', "insert into t values(6, 60);"); 
diag("inserting 7 on node 1 (can fail)");
$cluster->psql(1, 'postgres', "insert into t values(7, 70);");

diag("polling node 2");
for (my $poller = 0; $poller < 3; $poller++) {
	my $pollee = 2;
	ok($cluster->poll($poller, 'postgres', $pollee, 10, 1), "node $pollee is online according to node $poller");
}

diag("getting cluster state");
$cluster->psql(0, 'postgres', "select * from mtm.get_cluster_state();", stdout => \$psql_out);
diag("Node 1 status: $psql_out");
$cluster->psql(1, 'postgres', "select * from mtm.get_cluster_state();", stdout => \$psql_out);
diag("Node 2 status: $psql_out");
$cluster->psql(2, 'postgres', "select * from mtm.get_cluster_state();", stdout => \$psql_out);
diag("Node 3 status: $psql_out");

diag("inserting 8 on node 0");
$cluster->psql(0, 'postgres', "insert into t values(8, 80);");
diag("inserting 9 on node 1");
$cluster->psql(1, 'postgres', "insert into t values(9, 90);");

diag("selecting from node 2");
$cluster->psql(2, 'postgres', "select v from t where k=8;", stdout => \$psql_out);
diag("selected");

is($psql_out, '80', "Check replication after failed node recovery.");

$cluster->psql(2, 'postgres', "select v from t where k=9;", stdout => \$psql_out);
diag("selected");

is($psql_out, '90', "Check replication after failed node recovery.");

ok($cluster->stop('fast'), "cluster stops");
1;
