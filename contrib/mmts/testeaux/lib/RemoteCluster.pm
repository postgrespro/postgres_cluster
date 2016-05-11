package RemoteCluster;

use strict;
use warnings;
use Data::Dumper;
use Net::OpenSSH;
use Cwd;
use RemoteNode;

sub new
{
	my ($class, $config_fname) = @_;
	open(my $config, '<', $config_fname);
	my @nodes = ();

	# Parse connection options from ssh_config
	my $node_cfg;
	foreach (<$config>)
	{
   		if (/^Host (.+)/)
		{
			if ($node_cfg->{'host'}){
				push(@nodes, new RemoteNode($node_cfg->{'host'}, $node_cfg->{'cfg'}));
				$node_cfg = {};
			}
			$node_cfg->{'host'} = $1;
		}
		elsif (/\s*([^\s]+)\s*([^\s]+)\s*/)
		{
			$node_cfg->{'cfg'}->{$1} = $2;
		}
	}
	push(@nodes, new RemoteNode($node_cfg->{'host'}, $node_cfg->{'cfg'}));

	# print Dumper(@nodes);

	my $self = {
		nnodes => scalar @nodes,
		nodes => \@nodes,
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
		$node->init;
	}
}

sub configure
{
	my ($self) = @_;
	my $nodes = $self->{nodes};

	my $connstr = join(', ', map { $_->connstr('postgres') } @$nodes);
	my $raftpeers = join(',', map { join(':', $_->{_id}, $_->{_host}, 6666) } @$nodes);

	foreach my $node (@$nodes)
	{
		my $id = $node->{_id};
		my $host = $node->{_host};
		my $pgport = $node->{_port};
		#my $raftport = $node->{raftport};

		$node->append_conf("postgresql.conf", qq(
			listen_addresses = '$host'
			port = 5432
			max_prepared_transactions = 200
			max_connections = 200
			max_worker_processes = 100
			wal_level = logical
			fsync = off	
			max_wal_senders = 10
			wal_sender_timeout = 0
			max_replication_slots = 10

			tcp_keepalives_idle = 2
			tcp_keepalives_interval = 1
			tcp_keepalives_count = 2

			shared_preload_libraries = 'raftable,multimaster'
			multimaster.workers = 10
			multimaster.queue_size = 10485760 # 10mb
			multimaster.node_id = $id
			multimaster.conn_strings = '$connstr'
			multimaster.use_raftable = false
			multimaster.ignore_tables_without_pk = true
			multimaster.twopc_min_timeout = 60000
			multimaster.twopc_prepare_ratio = 1000 #%

			raftable.id = $id
			raftable.peers = '$raftpeers'
		));

		$node->append_conf("pg_hba.conf", qq(
			local replication all trust
			host replication all 0.0.0.0/0 trust
			host replication all ::1/0 trust
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
		$node->stop();
	}
}

sub psql
{
	my ($self, $index, @args) = @_;
	my $node = $self->{nodes}->[$index];
	return $node->psql(@args);
}

# XXX: test
#my $cluster = new RemoteCluster('ssh-config');
# $cluster->init;
# $cluster->configure;
# $cluster->start;
# 
1;
