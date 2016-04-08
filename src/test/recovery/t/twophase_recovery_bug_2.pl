use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 1;

# Setup master node
my $node_master = get_new_node("master");
$node_master->init(allows_streaming => 1);
$node_master->append_conf('postgresql.conf', qq(
max_prepared_transactions = 10
));
$node_master->start;
$node_master->backup('master_backup');
$node_master->psql('postgres', "create table t(id int)");

# Setup slave node
my $node_slave = get_new_node('slave');
$node_slave->init_from_backup($node_master, 'master_backup', has_streaming => 1);
$node_slave->start;

my $psql_out = '';
my $psql_rc = '';

$node_master->psql('postgres', "
	begin;
	create table t2(id int);
	prepare transaction 'x';
");
sleep 2; # wait for changes to arrive on slave
$node_slave->teardown_node;

#$node_slave->stop;

$node_master->psql('postgres',"commit prepared 'x'");
$node_slave->start;
$node_slave->psql('postgres',"select count(*) from pg_prepared_xacts", stdout => \$psql_out);
is($psql_out, '0', "Commit prepared on master while slave is down.");

