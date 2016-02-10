###############################################################################
# Test of proper transaction isolation.
# Based on Martin Kleppmann's set of tests (https://github.com/ept/hermitage)
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

my $conn1 = DBI->connect('DBI:Pg:' . $node1->connstr('postgres'));
my $conn2 = DBI->connect('DBI:Pg:' . $node2->connstr('postgres'));

sub count_total
{
	# my ($c1, $c2) = @_;
	my $c1 = DBI->connect('DBI:Pg:' . $node1->connstr('postgres'));
	my $c2 = DBI->connect('DBI:Pg:' . $node2->connstr('postgres'));

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
# Check for dirty reads
###############################################################################

my $gtid = "tx1";

query_exec($conn1, "begin transaction");
query_exec($conn2, "begin transaction");
my $snapshot = query_row($conn1, "select dtm_extend('$gtid')");
query_exec($conn2, "select dtm_access($snapshot, '$gtid')");
query_exec($conn1, "update t set v = v - 10 where u=1");

my $intermediate_total = count_total();

query_exec($conn2, "update t set v = v + 10 where u=2");
query_exec($conn1, "prepare transaction '$gtid'");
query_exec($conn2, "prepare transaction '$gtid'");
query_exec($conn1, "select dtm_begin_prepare('$gtid')");
query_exec($conn2, "select dtm_begin_prepare('$gtid')");
my $csn = query_row($conn1, "select dtm_prepare('$gtid', 0)");
query_exec($conn2, "select dtm_prepare('$gtid', $csn)");
query_exec($conn1, "select dtm_end_prepare('$gtid', $csn)");
query_exec($conn2, "select dtm_end_prepare('$gtid', $csn)");
query_exec($conn1, "commit prepared '$gtid'");
query_exec($conn2, "commit prepared '$gtid'");

is($intermediate_total, 0, "Check for absence of dirty reads");

























