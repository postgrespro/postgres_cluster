use strict;
use warnings;

use Cluster;
use TestLib;
use Test::More tests => 2;

use DBI;
use DBD::Pg ':async';

sub query_row
{
	my ($dbi, $sql, @keys) = @_;
	my $sth = $dbi->prepare($sql) || die;
	$sth->execute(@keys) || die;
	my $ret = $sth->fetchrow_array || undef;
	diag("query_row('$sql') -> $ret\n");
	return $ret;
}

sub query_exec
{
	my ($dbi, $sql) = @_;
	my $rv = $dbi->do($sql) || die;
	diag("query_exec('$sql') = $rv\n");
	return $rv;
}

sub query_exec_async
{
	my ($dbi, $sql) = @_;
	my $rv = $dbi->do($sql, {pg_async => PG_ASYNC}) || die;
	diag("query_exec_async('$sql')\n");
	return $rv;
}

my $cluster = new Cluster(2);

$cluster->init();
$cluster->configure();
$cluster->start();

my ($rc, $out, $err);
sleep(10);

$cluster->psql(0, 'postgres', "create table t(k int primary key, v text)");
$cluster->psql(0, 'postgres', "insert into t values (1, 'hello'), (2, 'world')");

my @conns = map { DBI->connect('DBI:Pg:' . $_->connstr()) } @{$cluster->{nodes}};

query_exec($conns[0], "begin");
query_exec($conns[1], "begin");

query_exec($conns[0], "update t set v = 'asd' where k = 1");
query_exec($conns[1], "update t set v = 'bsd'");

query_exec($conns[0], "update t set v = 'bar' where k = 2");
query_exec($conns[1], "update t set v = 'foo'");

query_exec_async($conns[0], "commit");
query_exec_async($conns[1], "commit");

my $timeout = 5;
while ($timeout > 0)
{
	my $r0 = $conns[0]->pg_ready();
	my $r1 = $conns[1]->pg_ready();
	if ($r0 && $r1) {
		last;
	}
	diag("queries still running: [0]=$r0 [1]=$r1\n");
	sleep(1);
}

if ($timeout > 0)
{
	diag("queries finished\n");

	my $succeeded = 0;
	$succeeded++ if $conns[0]->pg_result();
	$succeeded++ if $conns[1]->pg_result();

	pass("queries finished");
}
else
{
	diag("queries timed out\n");
	$conns[0]->pg_cancel() unless $conns[0]->pg_ready();
	$conns[1]->pg_cancel() unless $conns[1]->pg_ready();

	fail("queries timed out");
}

query_row($conns[0], "select * from t where k = 1");

ok($cluster->stop('fast'), "cluster stops");
1;
