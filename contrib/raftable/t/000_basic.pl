use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 10;

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
	my @cfg = ();
	for (my $i = 0; $i < $nodenum; $i++)
	{
		$nodesref->{$i} = get_new_node();
		my $extranode = get_new_node(); # just to find an extra port for raft
		push @cfg, "$i:127.0.0.1:${\$extranode->port}"
	}
	return $nodesref, join(',', @cfg);
}

sub init_nodes
{
	my ($nodesref, $cfg) = @_;
	print("cfg = $cfg\n");
	while (my ($id, $node) = each(%$nodesref))
	{
		$node->init;
		$node->append_conf("postgresql.conf", qq(
shared_preload_libraries = raftable
raftable.id = $id
raftable.peers = '$cfg'
));
	}
}

sub start_nodes
{
	my $nodesref = shift;
	while (my ($id, $node) = each(%$nodesref))
	{
		$node->start;
		$node->psql('postgres', "create extension raftable;");
	}
}

my ($nodesref, $cfg) = create_nodes(3);
init_nodes($nodesref, $cfg);
start_nodes($nodesref);

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

$able->psql('postgres', "select raftable('hello', '$tests{hello}', $timeout_ms);");
$baker->psql('postgres', "select raftable('and', '$tests{and}', $timeout_ms);");
$charlie->psql('postgres', "select raftable('goodbye', '$tests{goodbye}', $timeout_ms);");
$baker->stop;
$able->psql('postgres', "select raftable('world', '$tests{world}', $timeout_ms);");

$baker->start;
sleep(5);
while (my ($key, $value) = each(%tests))
{
	my ($rc, $stdout, $stderr) = $baker->psql('postgres', "select raftable('$key');");
	is($rc, 0, "Baker returns '$key'");
	is($stdout, $value, "Baker has the proper value for '$key'");
}

my $ok = 1;
my ($rc, $stdout, $stderr) = $baker->psql('postgres', "select raftable();");
is($rc, 0, "Baker returns everything");
while ($stdout =~ /\((\w+),(\w+)\)/g)
{
	if (!exists $tests{$1}) { $ok = 0; last; }
	my $val = delete $tests{$1};
	if ($val ne $2) { $ok = 0; last; }
}
if (keys %tests > 0) { $ok = 0; }
is($ok, 1, "Baker has the proper value for everything");

exit(0);
