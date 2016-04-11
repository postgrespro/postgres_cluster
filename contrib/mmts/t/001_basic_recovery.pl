use strict;
use warnings;
use Cluster;
use TestLib;
use Test::More tests => 3;
use DBI;
use DBD::Pg ':async';

my $cluster = new Cluster(3);
$cluster->init();
$cluster->configure();
$cluster->start();

###############################################################################
# Wait until nodes are up
###############################################################################

my $psql_out;
# XXX: create extension on start and poll_untill status is Online
sleep(5);

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

diag("sleeping 15");
sleep(15);

diag("inserting 2");
$cluster->psql(0, 'postgres', "insert into t values(2, 20);");
diag("selecting");
$cluster->psql(1, 'postgres', "select v from t where k=2;", stdout => \$psql_out);
diag("selected");
is($psql_out, '20', "Check replication after node failure.");

###############################################################################
# Work after node start
###############################################################################

diag("starting node 2");
$cluster->{nodes}->[2]->start;
diag("sleeping 10");
sleep(10); # XXX: here we can poll
diag("inserting 3");
$cluster->psql(0, 'postgres', "insert into t values(3, 30);");
diag("selecting");
$cluster->psql(2, 'postgres', "select v from t where k=3;", stdout => \$psql_out);
diag("selected");
is($psql_out, '30', "Check replication after failed node recovery.");


