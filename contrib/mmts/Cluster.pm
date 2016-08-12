package Cluster;

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More;
use Cwd;

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

sub new
{
	my ($class, $nodenum) = @_;

	my $nodes = [];

	foreach my $i (1..$nodenum)
	{
		my $host = "127.0.0.1";
		my ($pgport, $raftport) = allocate_ports($host, 2);
		my $node = new PostgresNode("node$i", $host, $pgport);
		$node->{id} = $i;
		$node->{raftport} = $raftport;
		push(@$nodes, $node);
	}

	my $self = {
		nodenum => $nodenum,
		nodes => $nodes,
	};

	bless $self, $class;
	return $self;
}

sub init
{
	my ($self) = @_;
	my $nodes = $self->{nodes};

	foreach my $node (@$nodes)
	{
		$node->init(hba_permit_replication => 0);
	}
}

sub configure
{
	my ($self) = @_;
	my $nodes = $self->{nodes};

	my $connstr = join(',', map { "${ \$_->connstr('postgres') }" } @$nodes);
	my $raftpeers = join(',', map { join(':', $_->{id}, $_->host, $_->{raftport}) } @$nodes);

	foreach my $node (@$nodes)
	{
		my $id = $node->{id};
		my $host = $node->host;
		my $pgport = $node->port;
		my $raftport = $node->{raftport};

		$node->append_conf("postgresql.conf", qq(
			listen_addresses = '$host'
			unix_socket_directories = ''
			port = $pgport
			max_prepared_transactions = 200
			max_connections = 200
			max_worker_processes = 100
			max_parallel_degree = 0
			wal_level = logical
			fsync = off	
			max_wal_senders = 10
			wal_sender_timeout = 0
            default_transaction_isolation = 'repeatable read'
			max_replication_slots = 10
			shared_preload_libraries = 'raftable,multimaster'
			multimaster.workers = 10
			multimaster.queue_size = 10485760 # 10mb
			multimaster.node_id = $id
			multimaster.conn_strings = '$connstr'
			multimaster.use_raftable = true
			multimaster.heartbeat_recv_timeout = 1000
			multimaster.heartbeat_send_timeout = 250
			multimaster.ignore_tables_without_pk = true
			multimaster.twopc_min_timeout = 2000
		));

		$node->append_conf("pg_hba.conf", qq(
			local replication all trust
			host replication all 127.0.0.1/32 trust
			host replication all ::1/128 trust
		));
	}
}

sub start
{
	my ($self) = @_;
	my $nodes = $self->{nodes};

	foreach my $node (@$nodes)
	{
		$node->start();
	}
}

sub stop
{
	my ($self) = @_;
	my $nodes = $self->{nodes};

	foreach my $node (@$nodes)
	{
		$node->stop('fast');
	}
}

sub teardown
{
	my ($self) = @_;
	my $nodes = $self->{nodes};

	foreach my $node (@$nodes)
	{
		$node->teardown();
	}
}

sub psql
{
	my ($self, $index, @args) = @_;
	my $node = $self->{nodes}->[$index];
	return $node->psql(@args);
}

sub poll
{
	my ($self, $poller, $dbname, $pollee, $tries, $delay) = @_;
	my $node = $self->{nodes}->[$poller];
	for (my $i = 0; $i < $tries; $i++) {
		my $psql_out;
		my $pollee_plus_1 = $pollee + 1;
		$self->psql($poller, $dbname, "select mtm.poll_node($pollee_plus_1, true);", stdout => \$psql_out);
		if ($psql_out eq "t") {
			return 1;
		}
		my $tries_left = $tries - $i - 1;
		diag("$poller poll for $pollee failed [$tries_left tries left]");
		sleep($delay);
	}
	return 0;
}

1;
