use strict;
use warnings;
use Cluster;
use TestLib;
use Test::More tests => 4;

my $cluster = new Cluster(3);
$cluster->init();
$cluster->configure();
$cluster->start();
sleep(10);

########################################################
# Check data integrity before and after recovery
########################################################

$cluster->pgbench(1, ('-i', -s => '10') );
$cluster->pgbench(0, ('-n','-N', -T => '4') );
$cluster->pgbench(1, ('-n','-N', -T => '4') );
$cluster->pgbench(2, ('-n','-N', -T => '4') );


my $hash0; my $hash1; my $hash2; my $oldhash;
my $hash_query = "
select
    md5('(' || string_agg(aid::text || ', ' || abalance::text , '),(') || ')')
from
    (select * from pgbench_accounts order by aid) t;";

$cluster->{nodes}->[2]->stop('fast');
sleep(3);

$cluster->pgbench(0, ('-n','-N', -T => '4') );
$cluster->pgbench(1, ('-n','-N', -T => '4') );

$cluster->psql(0, 'postgres', $hash_query, stdout => \$hash0);
$cluster->psql(1, 'postgres', $hash_query, stdout => \$hash1);
# $cluster->psql(2, 'postgres', $hash_query, stdout => \$hash2);

is( ($hash0 == $hash1) , 1, "Check that hash is the same before recovery");
$oldhash = $hash0;

$cluster->{nodes}->[2]->start;
sleep(10);

$cluster->psql(0, 'postgres', $hash_query, stdout => \$hash0);
$cluster->psql(1, 'postgres', $hash_query, stdout => \$hash1);
$cluster->psql(2, 'postgres', $hash_query, stdout => \$hash2);

note("$oldhash, $hash0, $hash1, $hash2");
is( (($hash0 == $hash1) and ($hash1 == $hash2) and ($oldhash == $hash0)) , 1, "Check that hash is the same after recovery");

########################################################
# Check start after all nodes were disconnected
########################################################

$cluster->psql(0, 'postgres', "create extension multimaster;
	create table if not exists t(k int primary key, v int);");

$cluster->psql(0, 'postgres', "insert into t values(1, 10);");
$cluster->psql(1, 'postgres', "insert into t values(2, 20);");
$cluster->psql(2, 'postgres', "insert into t values(3, 30);");
sleep(2);

my $sum0; my $sum1; my $sum2;

$cluster->{nodes}->[1]->stop('fast');
$cluster->{nodes}->[2]->stop('fast');

sleep(5);
$cluster->{nodes}->[1]->start;
# try to start node3 right here?
sleep(5);
$cluster->{nodes}->[2]->start;
sleep(5);

$cluster->psql(0, 'postgres', "select sum(v) from t;", stdout => \$sum0);
$cluster->psql(1, 'postgres', "select sum(v) from t;", stdout => \$sum1);
$cluster->psql(2, 'postgres', "select sum(v) from t;", stdout => \$sum2);
is( (($sum0 == 60) and ($sum1 == $sum0) and ($sum2 == $sum0)) , 1, "Check that nodes are working and sync");

########################################################
# Check recovery during some load
########################################################

$cluster->pgbench(0, ('-i', -s => '10') );
$cluster->pgbench(0, ('-N', -T => '1') );
$cluster->pgbench(1, ('-N', -T => '1') );
$cluster->pgbench(2, ('-N', -T => '1') );

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
$cluster->psql(0, 'postgres', $hash_query, stdout => \$hash0);
$cluster->psql(1, 'postgres', $hash_query, stdout => \$hash1);
$cluster->psql(2, 'postgres', $hash_query, stdout => \$hash2);
note("$hash0, $hash1, $hash2");
is( (($hash0 == $hash1) and ($hash1 == $hash2)) , 1, "Check that hash is the same");

# $cluster->psql(0, 'postgres', "select sum(abalance) from pgbench_accounts;", stdout => \$sum0);
# $cluster->psql(1, 'postgres', "select sum(abalance) from pgbench_accounts;", stdout => \$sum1);
# $cluster->psql(2, 'postgres', "select sum(abalance) from pgbench_accounts;", stdout => \$sum2);

# note("Sums: $sum0, $sum1, $sum2");
# is($sum2, $sum0, "Check that sum_2 == sum_0");
# is($sum2, $sum1, "Check that sum_2 == sum_1");

if ($sum2 == '') {
	sleep(3600);
}

$cluster->{nodes}->[0]->stop('fast');
$cluster->{nodes}->[1]->stop('fast');
$cluster->{nodes}->[2]->stop('fast');
