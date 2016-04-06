use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 1;

# Setup master node
my $node_master = get_new_node("Candie");
$node_master->init(allows_streaming => 1);
$node_master->append_conf('postgresql.conf', qq(
max_prepared_transactions = 10
));
$node_master->start;
$node_master->backup('master_backup');
$node_master->psql('postgres', "create table t(id int)");

# Setup master node
my $node_slave = get_new_node('Django');
$node_slave->init_from_backup($node_master, 'master_backup', has_streaming => 1);
$node_slave->start;

my $psql_out = '';
my $psql_rc = '';

$node_master->psql('postgres', "
	begin;
	insert into t values(0);
	create table t1(id int);
	insert into t1 values(1);
	create table t2(id int);
	insert into t2 values(2);
	savepoint s1;
	drop table t1;
	select * from t for update;
	select * from t2 for share;
	prepare transaction 'x';
");
sleep 2;
$node_slave->teardown_node;
$node_master->psql('postgres',"commit prepared 'x'");
$node_slave->start;
$node_slave->psql('postgres',"select count(*) from pg_prepared_xacts", stdout => \$psql_out);

is($psql_out, '0', "Commit prepared on master while slave is down.");


