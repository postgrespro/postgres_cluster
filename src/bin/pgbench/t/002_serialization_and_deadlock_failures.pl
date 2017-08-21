use strict;
use warnings;

use Config;
use PostgresNode;
use TestLib;
use Test::More tests => 66;

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
  . 'INSERT INTO xy VALUES (1, 2), (2, 3);');

my $script_serialization = $node->basedir . '/pgbench_script_serialization';
append_to_file($script_serialization,
		"BEGIN;\n"
	  . "\\set delta random(-5000, 5000)\n"
	  . "UPDATE xy SET y = y + :delta WHERE x = 1;\n"
	  . "END;");

my $script_deadlocks1 = $node->basedir . '/pgbench_script_deadlocks1';
append_to_file($script_deadlocks1,
		"BEGIN;\n"
	  . "\\set delta1 random(-5000, 5000)\n"
	  . "\\set delta2 random(-5000, 5000)\n"
	  . "UPDATE xy SET y = y + :delta1 WHERE x = 1;\n"
	  . "SELECT pg_sleep(20);\n"
	  . "UPDATE xy SET y = y + :delta2 WHERE x = 2;\n"
	  . "END;");

my $script_deadlocks2 = $node->basedir . '/pgbench_script_deadlocks2';
append_to_file($script_deadlocks2,
		"BEGIN;\n"
	  . "\\set delta1 random(-5000, 5000)\n"
	  . "\\set delta2 random(-5000, 5000)\n"
	  . "UPDATE xy SET y = y + :delta2 WHERE x = 2;\n"
	  . "UPDATE xy SET y = y + :delta1 WHERE x = 1;\n"
	  . "END;");

sub test_pgbench_default_transaction_isolation_level_and_serialization_failures
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
		$script_serialization);
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

sub test_pgbench_serialization_failures_retry
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
		qw(pgbench --no-vacuum --max-attempts 2 --debug),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_serialization);
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

	like($out_pgbench,
		qr{processed: 10/10},
		"concurrent update with retrying: "
	  . $isolation_level_sql
	  . ": check processed transactions");

	like($out_pgbench,
		qr{serialization failures: 0 \(0\.000 %\)},
		"concurrent update with retrying: "
	  . $isolation_level_sql
	  . ": check serialization failures");

	my $pattern =
		"client 0 sending UPDATE xy SET y = y \\+ (-?\\d+) WHERE x = 1;\n"
	  . "(client 0 receiving\n)+"
	  . "client 0 got a serialization failure \\(attempt 1/2\\)\n"
	  . "client 0 sending END;\n"
	  . "\\g2+"
	  . "client 0 repeats the failed transaction \\(attempt 2/2\\)\n"
	  . "client 0 sending BEGIN;\n"
	  . "\\g2+"
	  . "client 0 executing \\\\set delta\n"
	  . "client 0 sending UPDATE xy SET y = y \\+ \\g1 WHERE x = 1;";

	like($stderr,
		qr{$pattern},
		"concurrent update with retrying: "
	  . $isolation_level_sql
	  . ": check the retried transaction");
}

sub test_pgbench_deadlock_failures
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
		qw(pgbench --no-vacuum --transactions 1 --default-isolation-level),
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks1);
	print "# Running: " . join(" ", @command1) . "\n";
	$h1 = IPC::Run::start \@command1, \$in1, \$out1, \$err1;

	# Let pgbench run first update command in the transaction:
	sleep 10;

	# Run second pgbench
	my @command2 = (
		qw(pgbench --no-vacuum --transactions 1 --default-isolation-level),
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks2);
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

sub test_pgbench_deadlock_failures_retry
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
		qw(pgbench --no-vacuum --transactions 1 --max-attempts 2 --debug),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks1);
	print "# Running: " . join(" ", @command1) . "\n";
	$h1 = IPC::Run::start \@command1, \$in1, \$out1, \$err1;

	# Let pgbench run first update command in the transaction:
	sleep 10;

	# Run second pgbench
	my @command2 = (
		qw(pgbench --no-vacuum --transactions 1 --max-attempts 2 --debug),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks2);
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

	like($out1,
		qr{processed: 1/1},
		"concurrent deadlock update with retrying: "
	  . $isolation_level_sql
	  . ": pgbench 1: check processed transactions");
	like($out2,
		qr{processed: 1/1},
		"concurrent deadlock update with retrying: "
	  . $isolation_level_sql
	  . ": pgbench 2: check processed transactions");

	like($out1,
		qr{deadlock failures: 0 \(0\.000 %\)},
		"concurrent deadlock update with retrying: "
	  . $isolation_level_sql
	  . ": pgbench 1: check deadlock failures");
	like($out2,
		qr{deadlock failures: 0 \(0\.000 %\)},
		"concurrent deadlock update with retrying: "
	  . $isolation_level_sql
	  . ": pgbench 2: check deadlock failures");

	# First or second pgbench should get a deadlock error
	like($err1 . $err2,
		qr{client 0 got a deadlock failure \(attempt 1/2\)},
		"concurrent deadlock update with retrying: "
	  . $isolation_level_sql
	  . ": check deadlock failure");

	if ($isolation_level == READ_COMMITTED)
	{
		my $pattern =
			"client 0 sending UPDATE xy SET y = y \\+ (-?\\d+) WHERE x = (\\d);\n"
		  . "(client 0 receiving\n)+"
		  . "(|client 0 sending SELECT pg_sleep\\(20\\);\n)"
		  . "\\g3*"
		  . "client 0 sending UPDATE xy SET y = y \\+ (-?\\d+) WHERE x = (\\d);\n"
		  . "\\g3+"
		  . "client 0 got a deadlock failure \\(attempt 1/2\\)\n"
		  . "client 0 sending END;\n"
		  . "\\g3+"
		  . "client 0 repeats the failed transaction \\(attempt 2/2\\)\n"
		  . "client 0 sending BEGIN;\n"
		  . "\\g3+"
		  . "client 0 executing \\\\set delta1\n"
		  . "client 0 executing \\\\set delta2\n"
		  . "client 0 sending UPDATE xy SET y = y \\+ \\g1 WHERE x = \\g2;\n"
		  . "\\g3+"
		  . "\\g4"
		  . "\\g3*"
		  . "client 0 sending UPDATE xy SET y = y \\+ \\g5 WHERE x = \\g6;\n";

		like($err1 . $err2,
			qr{$pattern},
			"concurrent deadlock update with retrying: "
		  . $isolation_level_sql
		  . ": check the retried transaction");
	}
}

test_pgbench_default_transaction_isolation_level_and_serialization_failures(
	READ_COMMITTED);
test_pgbench_default_transaction_isolation_level_and_serialization_failures(
	REPEATABLE_READ);
test_pgbench_default_transaction_isolation_level_and_serialization_failures(
	SERIALIZABLE);

test_pgbench_serialization_failures_retry(REPEATABLE_READ);
test_pgbench_serialization_failures_retry(SERIALIZABLE);

test_pgbench_deadlock_failures(READ_COMMITTED);
test_pgbench_deadlock_failures(REPEATABLE_READ);
test_pgbench_deadlock_failures(SERIALIZABLE);

test_pgbench_deadlock_failures_retry(READ_COMMITTED);
test_pgbench_deadlock_failures_retry(REPEATABLE_READ);
test_pgbench_deadlock_failures_retry(SERIALIZABLE);
