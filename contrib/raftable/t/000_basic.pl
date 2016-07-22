use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 4;

sub genstr
{
	my $len = shift;
	my @chars = ("A".."Z", "a".."z");
	my $string;
	$string .= $chars[rand @chars] for 1..$len;
	return $string;
}

sub create_nodes
{
	my $nodenum = shift;
	my $nodesref = {};
	my $cfgref = [];
	for (my $i = 0; $i < $nodenum; $i++)
	{
		$nodesref->{$i} = get_new_node();
		my $extranode = get_new_node(); # just to find an extra port for raft
		push @$cfgref, [$i, '127.0.0.1', ${\$extranode->port}]
	}
	return $nodesref, $cfgref;
}

sub init_nodes
{
	my ($nodesref, $cfgref) = @_;
	while (my ($id, $node) = each(%$nodesref))
	{
		$node->init;
		$node->append_conf("postgresql.conf", qq(
shared_preload_libraries = raftable
));
	}
}

sub start_nodes
{
	my ($nodesref, $cfgref) = @_;
	while (my ($id, $node) = each(%$nodesref))
	{
		$node->start;
		$node->psql('postgres', "create extension raftable;");

		while (my ($i, $cfg1ref) = each(@$cfgref))
		{
			my ($peerid, $host, $port) = @$cfg1ref;
			diag("select raftable_peer($peerid, '$host', $port);");
			$node->psql('postgres', "select raftable_peer($peerid, '$host', $port);");
		}
		$node->psql('postgres', "select raftable_start($id);");
	}
}

my ($nodesref, $cfgref) = create_nodes(3);
init_nodes($nodesref, $cfgref);
start_nodes($nodesref, $cfgref);

my $able = $nodesref->{0};
my $baker = $nodesref->{1};
my $charlie = $nodesref->{2};

my %tests = (
	hello => genstr(1),
	and => genstr(100),
	goodbye => genstr(1000),
	world => genstr(3000),
);

my $timeout_ms = 1000;

diag("settings hello");
$able->psql('postgres', "select raftable('hello', '$tests{hello}', $timeout_ms);");
diag("settings and");
$baker->psql('postgres', "select raftable('and', '$tests{and}', $timeout_ms);");
diag("settings goodbye");
$charlie->psql('postgres', "select raftable('goodbye', '$tests{goodbye}', $timeout_ms);");
diag("stopping baker");
$baker->stop;
diag("settings world");
$able->psql('postgres', "select raftable('world', '$tests{world}', $timeout_ms);");

diag("starting baker");
$baker->start;
while (my ($i, $cfg1ref) = each(@$cfgref))
{
	my ($peerid, $host, $port) = @$cfg1ref;
	diag("select raftable_peer($peerid, '$host', $port);");
	$baker->psql('postgres', "select raftable_peer($peerid, '$host', $port);");
}
diag("checking baker");
while (my ($key, $value) = each(%tests))
{
	my ($rc, $stdout, $stderr) = $baker->psql('postgres', "select raftable('$key', $timeout_ms);");
	is($stdout, $value, "Baker gets the proper value for '$key' from the leader");
}

exit(0);
