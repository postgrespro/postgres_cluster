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
	my $ret = $sth->fetchrow_array || undef;
	print "query_row('$sql') -> $ret \n";
	return $ret;
}

sub query_exec
{
	my ($dbi, $sql) = @_;
	my $rv = $dbi->do($sql) || die;
	print "query_exec('$sql') = $rv\n";
	return $rv;
}

sub query_exec_async
{
	my ($dbi, $sql) = @_;
	my $rv = $dbi->do($sql, {pg_async => PG_ASYNC}) || die;
	print "query_exec_async('$sql') = $rv\n";
	return $rv;
}

my %allocated_ports = ();
sub allocate_ports
{
	my @allocated_now = ();
	my ($host, $ports_to_alloc) = @_;

	while ($ports_to_alloc > 0)
	{
		my $port = int(rand() * 16384) + 49152;
		next if $allocated_ports{$port};
		print "# Checking for port $port\n";
		if (!TestLib::run_log(['pg_isready', '-h', $host, '-p', $port]))
		{
			$allocated_ports{$port} = 1;
			push(@allocated_now, $port);
			$ports_to_alloc--;
		}
	}

	return @allocated_now;
}

my $nnodes = 3;
my @nodes = ();

# Create nodes and allocate ports
foreach my $i (1..$nnodes)
{
	my $host = "127.0.0.1";
	my ($pgport, $raftport) = allocate_ports($host, 2);
	my $node = new PostgresNode("node$i", $host, $pgport);
	$node->{id} = $i;
	$node->{raftport} = $raftport;
	push(@nodes, $node);
}

my $mm_connstr = join(',', map { "${ \$_->connstr('postgres') }" } @nodes);
my $raft_peers = join(',', map { join(':', $_->{id}, $_->host, $_->{raftport}) } @nodes);

print("# mm_connstr = $mm_connstr\n");
print("# raft_peers = $raft_peers\n");

# Init and Configure
foreach my $node (@nodes)
{
	my $id = $node->{id};
	my $host = $node->host;
	my $pgport = $node->port;
	my $raftport = $node->{raftport};

	$node->init(hba_permit_replication => 0);
	$node->append_conf("postgresql.conf", qq(
		listen_addresses = '$host'
		unix_socket_directories = ''
		port = $pgport
		max_prepared_transactions = 10
		max_worker_processes = 10
		wal_level = logical
		fsync = off	
		max_wal_senders = 10
		wal_sender_timeout = 0
		max_replication_slots = 10
		shared_preload_libraries = 'raftable,multimaster'
		multimaster.workers = 4
		multimaster.queue_size = 10485760 # 10mb
		multimaster.node_id = $id
		multimaster.conn_strings = '$mm_connstr'
		multimaster.use_raftable = true
		raftable.id = $id
		raftable.peers = '$raft_peers'
	));

	$node->append_conf("pg_hba.conf", qq(
		local replication all trust
		host replication all 127.0.0.1/32 trust
		host replication all ::1/128 trust
	));
}

# Start
foreach my $node (@nodes)
{
	$node->start();
}

my ($rc, $out, $err);
sleep(10);

$nodes[0]->psql('postgres', "create table t(k int primary key, v text)");
$nodes[0]->psql('postgres', "insert into t values (1, 'hello'), (2, 'world')");

#sub space2semicol
#{
#	my $str = shift;
#	$str =~ tr/ /;/;
#	return $str;
#}
#
my @conns = map { DBI->connect('DBI:Pg:' . $_->connstr()) } @nodes;

query_exec($conns[0], "begin");
query_exec($conns[1], "begin");

query_exec($conns[0], "update t set v = 'asd' where k = 1");
query_exec($conns[1], "update t set v = 'bsd' where k = 2");

query_exec($conns[0], "update t set v = 'bar' where k = 2");
query_exec($conns[1], "update t set v = 'foo' where k = 1");

query_exec_async($conns[0], "commit");
query_exec_async($conns[1], "commit");

for my $i (1..2)
{
	($rc, $out, $err) = $nodes[$i]->psql('postgres', "select * from t");
	print(" rc[$i] = $rc\n");
	print("out[$i] = $out\n");
	print("err[$i] = $err\n");
}

#sleep(2);
