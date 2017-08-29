use strict;
use warnings;

use Config;
use PostgresNode;
use TestLib;
use Test::More tests => 27;

use constant
{
	READ_COMMITTED  => 0,
	REPEATABLE_READ => 1,
	SERIALIZABLE    => 2,
};

my @isolation_level_sql = ('read committed', 'repeatable read', 'serializable');
my @isolation_level_abbreviations = ('RC', 'RR', 'S');

# Test concurrent deadlock updates in table with different default transaction
# isolation levels.
my $node = get_new_node('main');
$node->init;
$node->start;
$node->safe_psql('postgres',
	    'CREATE UNLOGGED TABLE xy (x integer, y integer); '
	  . 'INSERT INTO xy VALUES (1, 2), (2, 3);');

my $script1 = $node->basedir . '/pgbench_script1';
append_to_file($script1,
		"\\set delta1 random(-5000, 5000)\n"
	  . "\\set delta2 random(-5000, 5000)\n"
	  . "BEGIN;\n"
	  . "UPDATE xy SET y = y + :delta1 WHERE x = 1;\n"
	  . "SELECT pg_sleep(20);\n"
	  . "UPDATE xy SET y = y + :delta2 WHERE x = 2;\n"
	  . "END;");

my $script2 = $node->basedir . '/pgbench_script2';
append_to_file($script2,
		"\\set delta1 random(-5000, 5000)\n"
	  . "\\set delta2 random(-5000, 5000)\n"
	  . "BEGIN;\n"
	  . "UPDATE xy SET y = y + :delta2 WHERE x = 2;\n"
	  . "UPDATE xy SET y = y + :delta1 WHERE x = 1;\n"
	  . "END;");

sub test_pgbench
{
	my ($isolation_level) = @_;

	my $isolation_level_sql = $isolation_level_sql[$isolation_level];
	my $isolation_level_abbreviation =
	  $isolation_level_abbreviations[$isolation_level];

	local $ENV{PGPORT} = $node->port;

	my ($h1, $in1, $out1, $err1);
	my ($h2, $in2, $out2, $err2);

	# Run first pgbench
	my @command1 = (
		qw(pgbench --no-vacuum --transactions=1 --default-isolation-level),
		$isolation_level_abbreviation,
		"--file",
		$script1);
	print "# Running: " . join(" ", @command1) . "\n";
	$h1 = IPC::Run::start \@command1, \$in1, \$out1, \$err1;

	# Let pgbench run first update command in the transaction:
	sleep 10;

	# Run second pgbench
	my @command2 = (
		qw(pgbench --no-vacuum --transactions=1 --default-isolation-level),
		$isolation_level_abbreviation,
		"--file",
		$script2);
	print "# Running: " . join(" ", @command2) . "\n";
	$h2 = IPC::Run::start \@command2, \$in2, \$out2, \$err2;

	# Get all pgbench results
	$h1->pump() until length $out1;
	$h1->finish();

	$h2->pump() until length $out2;
	$h2->finish();

	# On Windows, the exit status of the process is returned directly as the
	# process's exit code, while on Unix, it's returned in the high bits
	# of the exit code (see WEXITSTATUS macro in the standard <sys/wait.h>
	# header file). IPC::Run's result function always returns exit code >> 8,
	# assuming the Unix convention, which will always return 0 on Windows as
	# long as the process was not terminated by an exception. To work around
	# that, use $h->full_result on Windows instead.
	my $result1 =
	    ($Config{osname} eq "MSWin32")
	  ? ($h1->full_results)[0]
	  : $h1->result(0);

	my $result2 =
	    ($Config{osname} eq "MSWin32")
	  ? ($h2->full_results)[0]
	  : $h2->result(0);

	# Check all pgbench results
	ok(!$result1, "@command1 exit code 0");
	ok(!$result2, "@command2 exit code 0");

	is($err1,  '', "@command1 no stderr");
	is($err2,  '', "@command2 no stderr");

	like($out1,
		qr{default transaction isolation level: $isolation_level_sql},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": pgbench 1: check default isolation level");
	like($out2,
		qr{default transaction isolation level: $isolation_level_sql},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": pgbench 2: check default isolation level");

	like($out1,
		qr{processed: 1/1},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": pgbench 1: check processed transactions");
	like($out2,
		qr{processed: 1/1},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": pgbench 2: check processed transactions");

	# First or second pgbench should get a deadlock error
	like($out1 . $out2,
		qr{deadlock failures: 1 \(100\.000 %\)},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": check deadlock failures");
}

test_pgbench(READ_COMMITTED);
test_pgbench(REPEATABLE_READ);
test_pgbench(SERIALIZABLE);
