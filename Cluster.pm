package Cluster;

use strict;
use warnings;

use Proc::ProcessTable;
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
			log_statement = none
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
            default_transaction_isolation = 'repeatable read'
			max_replication_slots = 10
			shared_preload_libraries = 'raftable,multimaster'
			multimaster.workers = 10
			multimaster.queue_size = 10485760 # 10mb
			multimaster.node_id = $id
			multimaster.conn_strings = '$connstr'
			multimaster.use_raftable = false
			multimaster.heartbeat_recv_timeout = 1000
			multimaster.heartbeat_send_timeout = 250
			multimaster.max_nodes = 3
			multimaster.ignore_tables_without_pk = true
			multimaster.twopc_min_timeout = 2000
            log_line_prefix = '%t: '
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

sub stopnode
{
	my ($node, $mode) = @_;
	return 1 unless defined $node->{_pid};
	$mode = 'fast' unless defined $mode;
	my $name   = $node->name;
	diag("stopping $name ${mode}ly");

	if ($mode eq 'kill') {
		killtree($node->{_pid});
		return 1;
	}

	my $pgdata = $node->data_dir;
	my $ret = TestLib::system_log('pg_ctl', '-D', $pgdata, '-m', 'fast', 'stop');
	my $pidfile = $node->data_dir . "/postmaster.pid";
	diag("unlink $pidfile");
	unlink $pidfile;
	$node->{_pid} = undef;
	$node->_update_pid;

	if ($ret != 0) {
		diag("$name failed to stop ${mode}ly");
		return 0;
	}

	return 1;
}

sub stopid
{
	my ($self, $idx, $mode) = @_;
	return stopnode($self->{nodes}->[$idx]);
}

sub killtree
{
	my $root = shift;
	diag("killtree $root\n");

	my $t = new Proc::ProcessTable;

	my %parent = ();
	#my %cmd = ();
	foreach my $p (@{$t->table}) {
		$parent{$p->pid} = $p->ppid;
	#	$cmd{$p->pid} = $p->cmndline;
	}

	if (!defined $root) {
		return;
	}
	my @queue = ($root);
	my @killist = ();

	while (scalar @queue) {
		my $victim = shift @queue;
		while (my ($pid, $ppid) = each %parent) {
			if ($ppid == $victim) {
				push @queue, $pid;
			}
		}
		diag("SIGSTOP to $victim");
		kill 'STOP', $victim;
		unshift @killist, $victim;
	}

	diag("SIGKILL to " . join(' ', @killist));
	kill 'KILL', @killist;
	#foreach my $victim (@killist) {
	#	print("kill $victim " . $cmd{$victim} . "\n");
	#}
}

sub stop
{
	my ($self, $mode) = @_;
	my $nodes = $self->{nodes};
	$mode = 'fast' unless defined $mode;

	diag("Dumping logs:");
	foreach my $node (@$nodes) {
		diag("##################################################################");
		diag($node->{_logfile});
		diag("##################################################################");
		my $filename = $node->{_logfile};
		open my $fh, '<', $filename or die "error opening $filename: $!";
		my $data = do { local $/; <$fh> };
		diag($data);
		diag("##################################################################\n\n");
	}

	my $ok = 1;
	diag("stopping cluster ${mode}ly");
	
	foreach my $node (@$nodes) {
		if (!stopnode($node, $mode)) {
			$ok = 0;
			if (!stopnode($node, 'kill')) {
				my $name = $node->name;
				BAIL_OUT("failed to kill $name");
			}
		}
	}
	sleep(2);
	return $ok;
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
