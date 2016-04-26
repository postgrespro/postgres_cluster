package RemoteCluster;

use strict;
use warnings;
use Data::Dumper;
use Net::OpenSSH;
use Cwd;

sub new
{
	my ($class, $config_fname) = @_;
	open(my $config, '<', $config_fname);
	my @config_lines = <$config>;
	my @nodes = ();

	# Parse connection options from ssh_config
	my $node;
	foreach (@config_lines)
	{
   		if (/^Host (.+)/)
		{
			if ($node->{'host'}){
				push(@nodes, $node);
				$node = {};
			}
			$node->{'host'} = $1;
		}
		elsif (/\s*([^\s]+)\s*([^\s]+)\s*/)
		{
			$node->{'cfg'}->{$1} = $2;
		}
	}
	push(@nodes, $node);

	# print Dumper(@nodes);

	my $self = {
		nnodes => scalar @nodes,
		nodes => \@nodes,
	};

	bless $self, $class;
	return $self;
}

sub run
{
	my ($self, $node_id, $cmd) = @_;
	my $node = $self->{nodes}[$node_id];
	my $opts = $node->{cfg};

	print "===\n";
	print Dumper($opts);
	print "===\n";

	my $ssh = Net::OpenSSH->new(
		$opts->{HostName},
		port => $opts->{Port},
		user => $opts->{User},
		key_path => $opts->{IdentityFile} =~ /"([^"]*)"/,
		master_opts => [-o => "StrictHostKeyChecking=no"]
	);

	my @ls = $ssh->capture($cmd);

	print Dumper(@ls);

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

sub detach 
{
	my ($self) = @_;
	my $nodes = $self->{nodes};

	foreach my $node (@$nodes)
	{
		delete $node->{_pid};
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
			wal_level = logical
			fsync = off	
			max_wal_senders = 10
			wal_sender_timeout = 0
			max_replication_slots = 10
			shared_preload_libraries = 'raftable,multimaster'
			multimaster.workers = 10
			multimaster.queue_size = 10485760 # 10mb
			multimaster.node_id = $id
			multimaster.conn_strings = '$connstr'
			multimaster.use_raftable = true
			multimaster.ignore_tables_without_pk = true
			multimaster.twopc_min_timeout = 60000
			raftable.id = $id
			raftable.peers = '$raftpeers'
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
		$node->stop();
	}
}

sub psql
{
	my ($self, $index, @args) = @_;
	my $node = $self->{nodes}->[$index];
	return $node->psql(@args);
}


my $cluster = new RemoteCluster('ssh-config');

print $cluster->{'nnodes'} . "\n";

$cluster->run(1, 'ls -la');

1;




