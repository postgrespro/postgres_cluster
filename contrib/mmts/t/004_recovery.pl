use strict;
use warnings;
use Cluster;
use TestLib;
use Test::More tests => 2;

my $cluster = new Cluster(3);
$cluster->init();
$cluster->configure();
$cluster->start();
sleep(10);

$cluster->psql(0, 'postgres', "create extension multimaster");
$cluster->pgbench(0, ('-i', -s => '10') );

# kill node while neighbour is under load
my $pgb_handle = $cluster->pgbench_async(1, ('-N', -T => '10') );
sleep(5);
$cluster->{nodes}->[2]->stop('fast');
$cluster->pgbench_await($pgb_handle);

# start node while neighbour is under load
$pgb_handle = $cluster->pgbench_async(0, ('-N', -T => '50') );
sleep(10);
$cluster->{nodes}->[2]->start;
$cluster->pgbench_await($pgb_handle);

# give it extra 10s to recover
sleep(10);

# check data identity
my $sum0;
my $sum1;
my $sum2;
$cluster->psql(0, 'postgres', "select sum(abalance) from pgbench_accounts;", stdout => \$sum0);
$cluster->psql(1, 'postgres', "select sum(abalance) from pgbench_accounts;", stdout => \$sum1);
$cluster->psql(2, 'postgres', "select sum(abalance) from pgbench_accounts;", stdout => \$sum2);

diag("Sums: $sum0, $sum1, $sum2");
is($sum2, $sum0, "Check that sum_2 == sum_0");
is($sum2, $sum1, "Check that sum_2 == sum_1");
