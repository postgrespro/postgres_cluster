###############################################################################
# Test of proper transaction isolation.
###############################################################################

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 1;
use DBI();
use DBD::Pg();

sub query_row
{
	my ($dbi, $sql, @keys) = @_;
	my $sth = $dbi->prepare($sql) || die;
	$sth->execute(@keys) || die;
	my $ret = $sth->fetchrow_array;
	print "query_row('$sql') -> $ret \n";
	return $ret;
}

sub query_exec
{
	my ($dbi, $sql) = @_;
	print "query_exec('$sql')\n";
	my $rv = $dbi->do($sql) || die;
	return $rv;
}

sub PostgresNode::psql_ok {
	my ($self, $sql, $comment) = @_;

	$self->command_ok(['psql', '-A', '-t', '--no-psqlrc',
		'-d', $self->connstr, '-c', $sql], $comment);
}

sub PostgresNode::psql_fails {
	my ($self, $sql, $comment) = @_;

	$self->command_ok(['psql', '-A', '-t', '--no-psqlrc',
		'-d', $self->connstr, '-c', $sql], $comment);
}

###############################################################################
# Setup nodes
###############################################################################

# Setup first node
my $node1 = get_new_node("node1");
$node1->init;
$node1->append_conf('postgresql.conf', qq(
max_prepared_transactions = 10
shared_preload_libraries = 'pg_tsdtm'
));
$node1->start;

# Setup second node
my $node2 = get_new_node("node2");
$node2->init;
$node2->append_conf('postgresql.conf', qq(
max_prepared_transactions = 10
shared_preload_libraries = 'pg_tsdtm'
));
$node2->start;

$node1->psql('postgres', "create extension pg_tsdtm;");
$node1->psql('postgres', "create table t(u int primary key, v int)");
$node1->psql('postgres', "insert into t (select generate_series(0, 9), 0)");

$node2->psql('postgres', "create extension pg_tsdtm;");
$node2->psql('postgres', "create table t(u int primary key, v int)");
$node2->psql('postgres', "insert into t (select generate_series(0, 9), 0)");

# we need two connections to each node (run two simultameous global tx)
my $conn11 = DBI->connect('DBI:Pg:' . $node1->connstr('postgres'));
my $conn21 = DBI->connect('DBI:Pg:' . $node2->connstr('postgres'));
my $conn12 = DBI->connect('DBI:Pg:' . $node1->connstr('postgres'));
my $conn22 = DBI->connect('DBI:Pg:' . $node2->connstr('postgres'));

sub count_total
{
	my ($c1, $c2) = @_;

	query_exec($c1, "begin");
	query_exec($c2, "begin");

	my $snapshot = query_row($c1, "select dtm_extend()");
	query_row($c2, "select dtm_access($snapshot)");

	my $sum1 = query_row($c1, "select sum(v) from t");
	my $sum2 = query_row($c2, "select sum(v) from t");

	query_exec($c1, "commit");
	query_exec($c2, "commit");

	my $tot = $sum1 + $sum2;

	print "total = $tot\n";
	return $tot;
}

###############################################################################
# Sanity check on dirty reads
###############################################################################

my $gtid1 = "gtx1";

# start global tx
query_exec($conn11, "begin transaction");
query_exec($conn21, "begin transaction");
my $snapshot = query_row($conn11, "select dtm_extend('$gtid1')");
query_exec($conn21, "select dtm_access($snapshot, '$gtid1')");

# transfer some amount of integers to different node
query_exec($conn11, "update t set v = v - 10 where u=1");
my $intermediate_total = count_total($conn12, $conn22);
query_exec($conn21, "update t set v = v + 10 where u=2");

# commit our global tx
query_exec($conn11, "prepare transaction '$gtid1'");
query_exec($conn21, "prepare transaction '$gtid1'");
query_exec($conn11, "select dtm_begin_prepare('$gtid1')");
query_exec($conn21, "select dtm_begin_prepare('$gtid1')");
my $csn = query_row($conn11, "select dtm_prepare('$gtid1', 0)");
query_exec($conn21, "select dtm_prepare('$gtid1', $csn)");
query_exec($conn11, "select dtm_end_prepare('$gtid1', $csn)");
query_exec($conn21, "select dtm_end_prepare('$gtid1', $csn)");
query_exec($conn11, "commit prepared '$gtid1'");
query_exec($conn21, "commit prepared '$gtid1'");

is($intermediate_total, 0, "Check for absence of dirty reads");

