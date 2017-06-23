use strict;
use warnings;

use Config;
use PostgresNode;
use TestLib;
use Test::More tests => 15;

use constant
{
	READ_COMMITTED  => 0,
	REPEATABLE_READ => 1,
	SERIALIZABLE    => 2,
};

my @isolation_level_sql = ('read committed', 'repeatable read', 'serializable');
my @isolation_level_abbreviations = ('RC', 'RR', 'S');

# Test concurrent update in table row with different default transaction
# isolation levels.
my $node = get_new_node('main');
$node->init;
$node->start;
$node->safe_psql('postgres',
    'CREATE UNLOGGED TABLE xy (x integer, y integer); '
  . 'INSERT INTO xy VALUES (1, 2);');

my $script = $node->basedir . '/pgbench_script';
append_to_file($script, "\\set delta random(-5000, 5000)\n");
append_to_file($script, "UPDATE xy SET y = y + :delta WHERE x = 1;");

sub test_pgbench
{
	my ($isolation_level) = @_;

	my $isolation_level_sql = $isolation_level_sql[$isolation_level];
	my $isolation_level_abbreviation =
	  $isolation_level_abbreviations[$isolation_level];

	local $ENV{PGPORT} = $node->port;
	my ($h_psql, $in_psql, $out_psql);
	my ($h_pgbench, $in_pgbench, $out_pgbench, $stderr);

	# Open the psql session and run the parallel transaction:
	print "# Starting psql\n";
	$h_psql = IPC::Run::start [ 'psql' ], \$in_psql, \$out_psql;

	$in_psql =
	  "begin transaction isolation level " . $isolation_level_sql . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /BEGIN/;

	$in_psql = "update xy set y = y + 1 where x = 1;\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /UPDATE 1/;

	# Start pgbench:
	my @command = (
		qw(pgbench --no-vacuum --default-isolation-level),
		$isolation_level_abbreviation,
		"--file",
		$script);
	print "# Running: " . join(" ", @command) . "\n";
	$h_pgbench = IPC::Run::start \@command, \$in_pgbench, \$out_pgbench,
	  \$stderr;

	# Let pgbench run the update command in the transaction:
	sleep 10;

	# In psql, commit the transaction and end the session:
	$in_psql = "end;\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /COMMIT/;

	$in_psql = "\\q\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() while length $in_psql;

	$h_psql->finish();

	# Get pgbench results
	$h_pgbench->pump() until length $out_pgbench;
	$h_pgbench->finish();

	# On Windows, the exit status of the process is returned directly as the
	# process's exit code, while on Unix, it's returned in the high bits
	# of the exit code (see WEXITSTATUS macro in the standard <sys/wait.h>
	# header file). IPC::Run's result function always returns exit code >> 8,
	# assuming the Unix convention, which will always return 0 on Windows as
	# long as the process was not terminated by an exception. To work around
	# that, use $h->full_result on Windows instead.
	my $result =
	    ($Config{osname} eq "MSWin32")
	  ? ($h_pgbench->full_results)[0]
	  : $h_pgbench->result(0);

	# Check pgbench results
	ok(!$result, "@command exit code 0");
	is($stderr,  '', "@command no stderr");

	like($out_pgbench,
		qr{default transaction isolation level: $isolation_level_sql},
		"concurrent update: $isolation_level_sql: check default isolation level");

	like($out_pgbench,
		qr{processed: 10/10},
		"concurrent update: $isolation_level_sql: check processed transactions");

	my $regex =
		($isolation_level == READ_COMMITTED)
	  ? qr{serialization failures: 0 \(0\.000 %\)}
	  : qr{serialization failures: [1-9]\d* \([1-9]\d*\.\d* %\)};

	like($out_pgbench,
		$regex,
		"concurrent update: $isolation_level_sql: check serialization failures");
}

test_pgbench(READ_COMMITTED);
test_pgbench(REPEATABLE_READ);
test_pgbench(SERIALIZABLE);
