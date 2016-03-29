use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 5;

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

my $tries = 10;

$able->psql('postgres', "select raftable('hello', '$tests{hello}', $tries);");
$baker->psql('postgres', "select raftable('and', '$tests{and}', $tries);");
$charlie->psql('postgres', "select raftable('goodbye', '$tests{goodbye}', $tries);");
$baker->stop;
$able->psql('postgres', "select raftable('world', '$tests{world}', $tries);");

$baker->start;
sleep(5);
while (my ($key, $value) = each(%tests))
{
	my $o = $baker->psql('postgres', "select raftable('$key');");
	is($o, $value, "Baker has '$key'");
}

my $ok = 1;
my $o = $baker->psql('postgres', "select raftable();");
while ($o =~ /\((\w+),(\w+)\)/g)
{
	if (!exists $tests{$1}) { $ok = 0; last; }
	my $val = delete $tests{$1};
	if ($val ne $2) { $ok = 0; last; }
}
if (keys %tests > 0) { $ok = 0; }
is($ok, 1, "Baker has everything");

exit(0);
