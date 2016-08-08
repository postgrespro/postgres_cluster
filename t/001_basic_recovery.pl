use strict;
use warnings;
use Cluster;
use TestLib;
use Test::More tests => 4;

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
$cluster->{nodes}->[2]->teardown_node;

sleep(5); # Wait until failure of node will be detected

diag("inserting 2 on node 1");
my $ret = $cluster->psql(0, 'postgres', "insert into t values(2, 20);"); # this transaciton may fail
diag "tx1 status = $ret";

diag("inserting 3 on node 2");
my $ret = $cluster->psql(1, 'postgres', "insert into t values(3, 30);"); # this transaciton may fail
diag "tx2 status = $ret";

diag("inserting 4 on node 1 (can fail)");
my $ret = $cluster->psql(0, 'postgres', "insert into t values(4, 40);"); 
diag "tx1 status = $ret";

diag("inserting 5 on node 2 (can fail)");
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

$cluster->psql(0, 'postgres', "select mtm.poll_node(3);");

diag("inserting 6 on node 1 (can fail)");
$cluster->psql(0, 'postgres', "insert into t values(6, 60);"); 
diag("inserting 7 on node 2 (can fail)");
$cluster->psql(1, 'postgres', "insert into t values(7, 70);");

sleep(5); # Wait until recovery of node will be completed

$cluster->psql(0, 'postgres', "select * from mtm.get_cluster_state();");
$cluster->psql(1, 'postgres', "select * from mtm.get_cluster_state();");
$cluster->psql(2, 'postgres', "select * from mtm.get_cluster_state();");

diag("inserting 8 on node 1");
$cluster->psql(0, 'postgres', "insert into t values(8, 80);");
diag("inserting 9 on node 2");
$cluster->psql(1, 'postgres', "insert into t values(9, 90);");

diag("selecting from node 3");
$cluster->psql(2, 'postgres', "select v from t where k=8;", stdout => \$psql_out);
diag("selected");

is($psql_out, '80', "Check replication after failed node recovery.");

$cluster->psql(2, 'postgres', "select v from t where k=9;", stdout => \$psql_out);
diag("selected");

is($psql_out, '90', "Check replication after failed node recovery.");

$cluster->stop();
1;
