use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 2;
use IPC::Run qw(start finish);
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
		max_prepared_transactions = 1000
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
		multimaster.ignore_tables_without_pk = true
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

diag("sleeping 10");
sleep(10);

diag("preparing the tables");
if ($nodes[0]->psql('postgres', "create table t (k int primary key, v int)"))
{
	BAIL_OUT('failed to create t');
}

if ($nodes[0]->psql('postgres', "insert into t (select generate_series(0, 999), 0)"))
{
	BAIL_OUT('failed to fill t');
}

if ($nodes[0]->psql('postgres', "create table reader_log (v int)"))
{
	BAIL_OUT('failed to create reader_log');
}

sub reader
{
	my ($node, $inref, $outref) = @_;

	my $clients = 1;
	my $jobs = 1;
	my $seconds = 30;
	my $tps = 10;
	my @argv = (
		'pgbench',
		'-n',
		-c => $clients,
		-j => $jobs,
		-T => $seconds,
		-h => $node->host(),
		-p => $node->port(),
		-f => 'tests/writer.pgb',
		-R => $tps,
		'postgres',
	);

	diag("running[" . getcwd() . "]: " . join(' ', @argv));

	return start(\@argv, $inref, $outref);
}

sub writer
{
	my ($node, $inref, $outref) = @_;

	my $clients = 10;
	my $jobs = 10;
	my $seconds = 30;
	my @argv = (
		'pgbench',
		'-n',
		-c => $clients,
		-j => $jobs,
		-T => $seconds,
		-h => $node->host(),
		-p => $node->port(),
		-f => 'tests/reader.pgb',
		'postgres',
	);

	diag("running[" . getcwd() . "]: " . join(' ', @argv));

	return start(\@argv, $inref, $outref);
}

diag("starting benches");
my $in = '';
my $out = '';
my @benches = ();
foreach my $node (@nodes)
{
	push(@benches, writer($node, \$in, \$out));
	push(@benches, reader($node, \$in, \$out));
}

diag("finishing benches");
foreach my $bench (@benches)
{
	finish($bench) || BAIL_OUT("pgbench exited with $?");
}
diag($out);

diag("checking readers' logs");

($rc, $out, $err) = $nodes[0]->psql('postgres', "select count(*) from reader_log where v != 0;");
is($out, 0, "there is nothing except zeros in reader_log");

($rc, $out, $err) = $nodes[0]->psql('postgres', "select count(*) from reader_log where v = 0;");
isnt($out, 0, "there are some zeros in reader_log");
