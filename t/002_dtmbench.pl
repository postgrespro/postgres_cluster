use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 1;

my %allocated_ports = ();
sub allocate_ports
{
	my @allocated_now = ();
	my ($host, $ports_to_alloc) = @_;

	while ($ports_to_alloc > 0)
	{
		my $port = int(rand() * 16384) + 49152;
		next if $allocated_ports{$port};
		diag("checking for port $port\n");
		if (!TestLib::run_log(['pg_isready', '-h', $host, '-p', $port]))
		{
			$allocated_ports{$port} = 1;
			push(@allocated_now, $port);
			$ports_to_alloc--;
		}
	}

	return @allocated_now;
}

my $nnodes = 2;
my @nodes = ();

diag("creating nodes");
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

diag("mm_connstr = $mm_connstr\n");
diag("raft_peers = $raft_peers\n");

diag("initting and configuring nodes");
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
		max_prepared_transactions = 200
        max_connections = 200
		max_worker_processes = 100
		wal_level = logical
		fsync = off	
		max_wal_senders = 10
		wal_sender_timeout = 0
		max_replication_slots = 10
		shared_preload_libraries = 'raftable,multimaster'
		multimaster.workers = 10
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

diag("starting nodes");
foreach my $node (@nodes)
{
	$node->start();
}

my ($rc, $out, $err);

diag("sleeping");
sleep(10);

my @argv = ('dtmbench');
foreach my $node (@nodes)
{
	push(@argv, '-c', $node->connstr('postgres'));
}
push(@argv, '-n', 1000, '-a', 1000, '-w', 10, '-r', 1);

diag("running dtmbench -i");
if (!TestLib::run_log([@argv, '-i']))
{
	BAIL_OUT("dtmbench -i failed");
}

diag("running dtmbench");
if (!TestLib::run_log(\@argv, '>', \$out))
{
	fail("dtmbench failed");
}
elsif ($out =~ /Wrong sum/)
{
	fail("inconsistency detected");
}
else
{
	pass("all consistent during dtmbench");
}
