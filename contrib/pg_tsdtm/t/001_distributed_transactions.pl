###############################################################################
# Test of proper transaction isolation.
# Based on Martin Kleppmann tests, https://github.com/ept/hermitage
###############################################################################

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 2;
use DBI;
use DBD::Pg ':async';

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
	my $rv = $dbi->do($sql) || die;
	print "query_exec('$sql')\n";
	return $rv;
}

sub query_exec_async
{
	my ($dbi, $sql) = @_;
	my $rv = $dbi->do($sql, {pg_async => PG_ASYNC}) || die;
	print "query_exec('$sql')\n";
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

$node1->psql('postgres', "create extension pg_tsdtm");
$node1->psql('postgres', "create table t(id int primary key, v int)");
$node1->psql('postgres', "insert into t values(1, 10)");

$node2->psql('postgres', "create extension pg_tsdtm");
$node2->psql('postgres', "create table t(id int primary key, v int)");
$node2->psql('postgres', "insert into t values(2, 20)");

# we need two connections to each node (run two simultameous global tx)
my $conn1a = DBI->connect('DBI:Pg:' . $node1->connstr('postgres'));
my $conn2a = DBI->connect('DBI:Pg:' . $node2->connstr('postgres'));
my $conn1b = DBI->connect('DBI:Pg:' . $node1->connstr('postgres'));
my $conn2b = DBI->connect('DBI:Pg:' . $node2->connstr('postgres'));

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

sub start_global
{
	my ($gtid, $c1, $c2) = @_;

	query_exec($c1, "begin transaction");
	query_exec($c2, "begin transaction");
	my $snapshot = query_row($c1, "select dtm_extend('$gtid')");
	query_exec($c2, "select dtm_access($snapshot, '$gtid')");
}

sub commit_global
{
	my ($gtid, $c1, $c2) = @_;

	query_exec($c1, "prepare transaction '$gtid'");
	query_exec($c2, "prepare transaction '$gtid'");
	query_exec($c1, "select dtm_begin_prepare('$gtid')");
	query_exec($c2, "select dtm_begin_prepare('$gtid')");
	my $csn = query_row($c1, "select dtm_prepare('$gtid', 0)");
	query_exec($c2, "select dtm_prepare('$gtid', $csn)");
	query_exec($c1, "select dtm_end_prepare('$gtid', $csn)");
	query_exec($c2, "select dtm_end_prepare('$gtid', $csn)");
	query_exec($c1, "commit prepared '$gtid'");
	query_exec($c2, "commit prepared '$gtid'");
}

###############################################################################
# Sanity check for dirty reads
###############################################################################

start_global("gtx1", $conn1a, $conn2a);

query_exec($conn1a, "update t set v = v - 10 where id=1");

my $intermediate_total = count_total($conn1b, $conn2b);

query_exec($conn2a, "update t set v = v + 10 where id=2");

commit_global("gtx1", $conn1a, $conn2a);

is($intermediate_total, 30, "Check for absence of dirty reads");

###############################################################################
# G0
###############################################################################

my $fail = 0;
$node1->psql('postgres', "update t set v = 10 where id = 2");
$node2->psql('postgres', "update t set v = 20 where id = 2");

start_global("gtx2a", $conn1a, $conn2a);
start_global("gtx2b", $conn1b, $conn2b);

query_exec($conn1a, "update t set v = 11 where id = 1");
query_exec_async($conn1b, "update t set v = 12 where id = 1");

# last update should be locked
$fail = 1 if $conn1b->pg_ready != 0;

query_exec($conn2a, "update t set v = 21 where id = 2");
commit_global("gtx2a", $conn1a, $conn2a);

# here transaction can continue
$conn1b->pg_result;

my $v1 = query_row($conn1a, "select v from t where id = 1");
my $v2 = query_row($conn2a, "select v from t where id = 2");

# we shouldn't see second's tx data
$fail = 1 if $v1 != 11 or $v2 != 21;

query_exec($conn2b, "update t set v = 22 where id = 2");
commit_global("gtx2b", $conn1b, $conn2b);

$v1 = query_row($conn1a, "select v from t where id = 1");
$v2 = query_row($conn2a, "select v from t where id = 2");

$fail = 1 if $v1 != 12 or $v2 != 22;

is($fail, 0, "Global transactions prevents Write Cycles (G0)");











