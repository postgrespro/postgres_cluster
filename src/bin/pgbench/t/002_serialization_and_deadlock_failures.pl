use strict;
use warnings;

use Config;
use PostgresNode;
use TestLib;
use Test::More tests => 76;

use constant
{
	READ_UNCOMMITTED => 0,
	READ_COMMITTED   => 1,
	REPEATABLE_READ  => 2,
	SERIALIZABLE     => 3,
};

my @isolation_level_sql = (
	'read uncommitted',
	'read committed',
	'repeatable read',
	'serializable');
my @isolation_level_abbreviations = ('RUC', 'RC', 'RR', 'S');

# The keys of advisory locks for testing deadlock failures:
use constant
{
	TRANSACTION_BEGINS => 4,
	DEADLOCK_1         => 5,
	WAIT_PGBENCH_2     => 6,
	DEADLOCK_2         => 7,
	TRANSACTION_ENDS   => 8,
};

# Test concurrent update in table row with different default transaction
# isolation levels.
my $node = get_new_node('main');
$node->init;
$node->start;
$node->safe_psql('postgres',
    'CREATE UNLOGGED TABLE xy (x integer, y integer); '
  . 'INSERT INTO xy VALUES (1, 2);');

my $script_serialization = $node->basedir . '/pgbench_script_serialization';
append_to_file($script_serialization,
		"BEGIN;\n"
	  . "\\set delta random(-5000, 5000)\n"
	  . "UPDATE xy SET y = y + :delta "
	  . "WHERE x = 1 AND pg_advisory_lock(0) IS NOT NULL;\n"
	  . "END;\n"
	  . "SELECT pg_advisory_unlock_all();");

my $script_deadlocks1 = $node->basedir . '/pgbench_script_deadlocks1';
append_to_file($script_deadlocks1,
		"BEGIN;\n"
	  . "SELECT pg_advisory_unlock_all();\n"
	  . "SELECT pg_advisory_lock(" . TRANSACTION_BEGINS . ");\n"
	  . "SELECT pg_advisory_unlock(" . TRANSACTION_BEGINS . ");\n"
	  . "SELECT pg_advisory_lock(" . DEADLOCK_1 . ");\n"
	  . "SELECT pg_advisory_lock(" . WAIT_PGBENCH_2 . ");\n"
	  . "SELECT pg_advisory_lock(" . DEADLOCK_2 . ");\n"
	  . "END;\n"
	  . "SELECT pg_advisory_unlock_all();\n"
	  . "SELECT pg_advisory_lock(" . TRANSACTION_ENDS . ");\n"
	  . "SELECT pg_advisory_unlock_all();");

my $script_deadlocks2 = $node->basedir . '/pgbench_script_deadlocks2';
append_to_file($script_deadlocks2,
		"BEGIN;\n"
	  . "SELECT pg_advisory_unlock_all();\n"
	  . "SELECT pg_advisory_lock(" . TRANSACTION_BEGINS . ");\n"
	  . "SELECT pg_advisory_unlock(" . TRANSACTION_BEGINS . ");\n"
	  . "SELECT pg_advisory_lock(" . DEADLOCK_2 . ");\n"
	  . "SELECT pg_advisory_lock(" . DEADLOCK_1 . ");\n"
	  . "END;\n"
	  . "SELECT pg_advisory_unlock_all();\n"
	  . "SELECT pg_advisory_lock(" . TRANSACTION_ENDS . ");\n"
	  . "SELECT pg_advisory_unlock_all();");

sub test_pgbench_default_transaction_isolation_level_and_serialization_failures
{
	my ($isolation_level) = @_;

	my $isolation_level_sql = $isolation_level_sql[$isolation_level];
	my $isolation_level_abbreviation =
	  $isolation_level_abbreviations[$isolation_level];

	local $ENV{PGPORT} = $node->port;
	my ($h_psql, $in_psql, $out_psql);
	my ($h_pgbench, $in_pgbench, $out_pgbench, $err_pgbench);

	# Open a psql session, run a parallel transaction and aquire an advisory
	# lock:
	print "# Starting psql\n";
	$h_psql = IPC::Run::start [ 'psql' ], \$in_psql, \$out_psql;

	$in_psql =
	  "begin transaction isolation level " . $isolation_level_sql . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /BEGIN/;

	$in_psql =
		"update xy set y = y + 1 "
	  . "where x = 1 and pg_advisory_lock(0) is not null;\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /UPDATE 1/;

	# Start pgbench:
	my @command = (
		qw(pgbench --no-vacuum --transactions 1 --debug),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_serialization);
	print "# Running: " . join(" ", @command) . "\n";
	$h_pgbench = IPC::Run::start \@command, \$in_pgbench, \$out_pgbench,
	  \$err_pgbench;

	# Wait until pgbench also tries to acquire the same advisory lock:
	do
	{
		$in_psql =
			"select * from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = 0::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /1 row/);

	# In psql, commit the transaction, release advisory locks and end the
	# session:
	$in_psql = "end;\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /COMMIT/;

	$in_psql = "select pg_advisory_unlock_all();\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_unlock_all/;

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
		qr{default transaction isolation level: $isolation_level_sql},
		"concurrent update: "
	  . $isolation_level_sql
	  . ": check default isolation level");

	like($out_pgbench,
		qr{processed: 1/1},
		"concurrent update: "
	  . $isolation_level_sql
	  . ": check processed transactions");

	my ($regex);

	if ($isolation_level == READ_UNCOMMITTED ||
		$isolation_level == READ_COMMITTED)
	{
		$regex = qr{^((?!number of failures)(.|\n))*$};
	}
	else
	{
		$regex = qr{number of failures: [1-9]\d* \([1-9]\d*\.\d* %\)};
	}

	like($out_pgbench,
		$regex,
		"concurrent update: $isolation_level_sql: check failures");

	if ($isolation_level == READ_UNCOMMITTED ||
		$isolation_level == READ_COMMITTED)
	{
		$regex =
			qr{^((?!client 0 got a serialization failure \(try 1/1\))(.|\n))*$};
	}
	else
	{
		$regex = qr{client 0 got a serialization failure \(try 1/1\)};
	}

	like($err_pgbench,
		$regex,
		"concurrent update: $isolation_level_sql: check serialization failure");
}

sub test_pgbench_serialization_failures_retry
{
	my ($isolation_level) = @_;

	my $isolation_level_sql = $isolation_level_sql[$isolation_level];
	my $isolation_level_abbreviation =
	  $isolation_level_abbreviations[$isolation_level];

	local $ENV{PGPORT} = $node->port;
	my ($h_psql, $in_psql, $out_psql);
	my ($h_pgbench, $in_pgbench, $out_pgbench, $err_pgbench);

	# Open a psql session, run a parallel transaction and aquire an advisory
	# lock:
	print "# Starting psql\n";
	$h_psql = IPC::Run::start [ 'psql' ], \$in_psql, \$out_psql;

	$in_psql =
	  "begin transaction isolation level " . $isolation_level_sql . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /BEGIN/;

	$in_psql =
		"update xy set y = y + 1 "
	  . "where x = 1 and pg_advisory_lock(0) is not null;\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /UPDATE 1/;

	# Start pgbench:
	my @command = (
		qw(pgbench --no-vacuum --transactions 1 --debug --max-tries 2),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_serialization);
	print "# Running: " . join(" ", @command) . "\n";
	$h_pgbench = IPC::Run::start \@command, \$in_pgbench, \$out_pgbench,
	  \$err_pgbench;

	# Wait until pgbench also tries to acquire the same advisory lock:
	do
	{
		$in_psql =
			"select * from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = 0::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /1 row/);

	# In psql, commit the transaction, release advisory locks and end the
	# session:
	$in_psql = "end;\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /COMMIT/;

	$in_psql = "select pg_advisory_unlock_all();\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_unlock_all/;

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
		qr{processed: 1/1},
		"concurrent update with retrying: "
	  . $isolation_level_sql
	  . ": check processed transactions");

	like($out_pgbench,
		qr{^((?!number of failures)(.|\n))*$},
		"concurrent update with retrying: "
	  . $isolation_level_sql
	  . ": check failures");

	my $pattern =
		"client 0 sending UPDATE xy SET y = y \\+ (-?\\d+) "
	  . "WHERE x = 1 AND pg_advisory_lock\\(0\\) IS NOT NULL;\n"
	  . "(client 0 receiving\n)+"
	  . "client 0 got a serialization failure \\(try 1/2\\)\n"
	  . "client 0 sending END;\n"
	  . "\\g2+"
	  . "client 0 repeats the failed transaction \\(try 2/2\\)\n"
	  . "client 0 sending BEGIN;\n"
	  . "\\g2+"
	  . "client 0 executing \\\\set delta\n"
	  . "client 0 sending UPDATE xy SET y = y \\+ \\g1 "
	  . "WHERE x = 1 AND pg_advisory_lock\\(0\\) IS NOT NULL;";

	like($err_pgbench,
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
	my ($h_psql, $in_psql, $out_psql);
	my ($h1, $in1, $out1, $err1);
	my ($h2, $in2, $out2, $err2);

	# Open a psql session and aquire an advisory lock:
	print "# Starting psql\n";
	$h_psql = IPC::Run::start [ 'psql' ], \$in_psql, \$out_psql;

	$in_psql =
		"select pg_advisory_lock(" . WAIT_PGBENCH_2 . ") "
	  . "as pg_advisory_lock_" . WAIT_PGBENCH_2 . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_lock_@{[ WAIT_PGBENCH_2 ]}/;

	# Run the first pgbench:
	my @command1 = (
		qw(pgbench --no-vacuum --transactions 1 --debug),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks1);
	print "# Running: " . join(" ", @command1) . "\n";
	$h1 = IPC::Run::start \@command1, \$in1, \$out1, \$err1;

	# Wait until the first pgbench also tries to acquire the same advisory lock:
	do
	{
		$in_psql =
			"select case count(*) "
		  . "when 0 then '" . WAIT_PGBENCH_2 . "_zero' "
		  . "else '" . WAIT_PGBENCH_2 . "_not_zero' end "
		  . "from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = "
		  . WAIT_PGBENCH_2
		  . "::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /@{[ WAIT_PGBENCH_2 ]}_not_zero/);

	# Run the second pgbench:
	my @command2 = (
		qw(pgbench --no-vacuum --transactions 1 --debug),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks2);
	print "# Running: " . join(" ", @command2) . "\n";
	$h2 = IPC::Run::start \@command2, \$in2, \$out2, \$err2;

	# Wait until the second pgbench tries to acquire the lock held by the first
	# pgbench:
	do
	{
		$in_psql =
			"select case count(*) "
		  . "when 0 then '" . DEADLOCK_1 . "_zero' "
		  . "else '" . DEADLOCK_1 . "_not_zero' end "
		  . "from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = "
		  . DEADLOCK_1
		  . "::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /@{[ DEADLOCK_1 ]}_not_zero/);

	# In the psql session, release the lock that the first pgbench is waiting
	# for and end the session:
	$in_psql =
		"select pg_advisory_unlock(" . WAIT_PGBENCH_2 . ") "
	  . "as pg_advisory_unlock_" . WAIT_PGBENCH_2 . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_unlock_@{[ WAIT_PGBENCH_2 ]}/;

	$in_psql = "\\q\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() while length $in_psql;

	$h_psql->finish();

	# Get results from all pgbenches:
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
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": pgbench 1: check processed transactions");
	like($out2,
		qr{processed: 1/1},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": pgbench 2: check processed transactions");

	# The first or second pgbench should get a deadlock error
	like($out1 . $out2,
		qr{number of failures: 1 \(100\.000 %\)},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": check failures");

	like($err1 . $err2,
		qr{client 0 got a deadlock failure \(try 1/1\)},
		"concurrent deadlock update: "
	  . $isolation_level_sql
	  . ": check deadlock failure");
}

sub test_pgbench_deadlock_failures_retry
{
	my ($isolation_level) = @_;

	my $isolation_level_sql = $isolation_level_sql[$isolation_level];
	my $isolation_level_abbreviation =
	  $isolation_level_abbreviations[$isolation_level];

	local $ENV{PGPORT} = $node->port;
	my ($h_psql, $in_psql, $out_psql);
	my ($h1, $in1, $out1, $err1);
	my ($h2, $in2, $out2, $err2);

	# Open a psql session and aquire an advisory lock:
	print "# Starting psql\n";
	$h_psql = IPC::Run::start [ 'psql' ], \$in_psql, \$out_psql;

	$in_psql =
		"select pg_advisory_lock(" . WAIT_PGBENCH_2 . ") "
	  . "as pg_advisory_lock_" . WAIT_PGBENCH_2 . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_lock_@{[ WAIT_PGBENCH_2 ]}/;

	# Run the first pgbench:
	my @command1 = (
		qw(pgbench --no-vacuum --transactions 1 --debug --max-tries 2),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks1);
	print "# Running: " . join(" ", @command1) . "\n";
	$h1 = IPC::Run::start \@command1, \$in1, \$out1, \$err1;

	# Wait until the first pgbench also tries to acquire the same advisory lock:
	do
	{
		$in_psql =
			"select case count(*) "
		  . "when 0 then '" . WAIT_PGBENCH_2 . "_zero' "
		  . "else '" . WAIT_PGBENCH_2 . "_not_zero' end "
		  . "from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = "
		  . WAIT_PGBENCH_2
		  . "::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /@{[ WAIT_PGBENCH_2 ]}_not_zero/);

	# Run the second pgbench:
	my @command2 = (
		qw(pgbench --no-vacuum --transactions 1 --debug --max-tries 2),
		"--default-isolation-level",
		$isolation_level_abbreviation,
		"--file",
		$script_deadlocks2);
	print "# Running: " . join(" ", @command2) . "\n";
	$h2 = IPC::Run::start \@command2, \$in2, \$out2, \$err2;

	# Wait until the second pgbench tries to acquire the lock held by the first
	# pgbench:
	do
	{
		$in_psql =
			"select case count(*) "
		  . "when 0 then '" . DEADLOCK_1 . "_zero' "
		  . "else '" . DEADLOCK_1 . "_not_zero' end "
		  . "from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = "
		  . DEADLOCK_1
		  . "::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /@{[ DEADLOCK_1 ]}_not_zero/);

	# In the psql session, acquire the locks which pgbenches will wait for:
	# - pgbench with a deadlock failure will wait for the lock after the start
	#   of its retried transaction
	# - pgbench without a deadlock failure will wait for the lock after the end
	#   of its sucessuful transaction

	$in_psql =
		"select pg_advisory_lock(" . TRANSACTION_BEGINS . ") "
	  . "as pg_advisory_lock_" . TRANSACTION_BEGINS . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_lock_@{[ TRANSACTION_BEGINS ]}/;

	$in_psql =
		"select pg_advisory_lock(" . TRANSACTION_ENDS . ") "
	  . "as pg_advisory_lock_" . TRANSACTION_ENDS . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_lock_@{[ TRANSACTION_ENDS ]}/;

	# In the psql session, release the lock that the first pgbench is waiting
	# for:
	$in_psql =
		"select pg_advisory_unlock(" . WAIT_PGBENCH_2 . ") "
	  . "as pg_advisory_unlock_" . WAIT_PGBENCH_2 . ";\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_unlock_@{[ WAIT_PGBENCH_2 ]}/;

	# Wait until pgbenches try to acquire the locks held by the psql session:
	do
	{
		$in_psql =
			"select case count(*) "
		  . "when 0 then '" . TRANSACTION_BEGINS . "_zero' "
		  . "else '" . TRANSACTION_BEGINS . "_not_zero' end "
		  . "from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = "
		  . TRANSACTION_BEGINS
		  . "::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /@{[ TRANSACTION_BEGINS ]}_not_zero/);

	do
	{
		$in_psql =
			"select case count(*) "
		  . "when 0 then '" . TRANSACTION_ENDS . "_zero' "
		  . "else '" . TRANSACTION_ENDS . "_not_zero' end "
		  . "from pg_locks where "
		  . "locktype = 'advisory' and "
		  . "objsubid = 1 and "
		  . "((classid::bigint << 32) | objid::bigint = "
		  . TRANSACTION_ENDS
		  . "::bigint) and "
		  . "not granted;\n";
		print "# Running in psql: " . join(" ", $in_psql);
		$h_psql->pump() while length $in_psql;
	} while ($out_psql !~ /@{[ TRANSACTION_ENDS ]}_not_zero/);

	# In the psql session, release advisory locks and end the session:
	$in_psql = "select pg_advisory_unlock_all() as pg_advisory_unlock_all;\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() until $out_psql =~ /pg_advisory_unlock_all/;

	$in_psql = "\\q\n";
	print "# Running in psql: " . join(" ", $in_psql);
	$h_psql->pump() while length $in_psql;

	$h_psql->finish();

	# Get results from all pgbenches:
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

	# The first or second pgbench should get a deadlock error which was retried:
	like($out1 . $out2,
		qr{^((?!number of failures)(.|\n))*$},
		"concurrent deadlock update with retrying: "
	  . $isolation_level_sql
	  . ": check failures");

	my $pattern =
		"client 0 sending SELECT pg_advisory_lock\\((\\d)\\);\n"
	  . "(client 0 receiving\n)+"
	  . "(|client 0 sending SELECT pg_advisory_lock\\(" . WAIT_PGBENCH_2 . "\\);\n)"
	  . "\\g2*"
	  . "client 0 sending SELECT pg_advisory_lock\\((\\d)\\);\n"
	  . "\\g2+"
	  . "client 0 got a deadlock failure \\(try 1/2\\)\n"
	  . "client 0 sending END;\n"
	  . "\\g2+"
	  . "client 0 repeats the failed transaction \\(try 2/2\\)\n"
	  . "client 0 sending BEGIN;\n"
	  . "\\g2+"
	  . "client 0 sending SELECT pg_advisory_unlock_all\\(\\);\n"
	  . "\\g2+"
	  . "client 0 sending SELECT pg_advisory_lock\\(" . TRANSACTION_BEGINS . "\\);\n"
	  . "\\g2+"
	  . "client 0 sending SELECT pg_advisory_unlock\\(" . TRANSACTION_BEGINS . "\\);\n"
	  . "\\g2+"
	  . "client 0 sending SELECT pg_advisory_lock\\(\\g1\\);\n"
	  . "\\g2+"
	  . "\\g3"
	  . "\\g2*"
	  . "client 0 sending SELECT pg_advisory_lock\\(\\g4\\);\n";

	like($err1 . $err2,
		qr{$pattern},
		"concurrent deadlock update with retrying: "
	  . $isolation_level_sql
	  . ": check the retried transaction");
}

test_pgbench_default_transaction_isolation_level_and_serialization_failures(
	READ_UNCOMMITTED);
test_pgbench_default_transaction_isolation_level_and_serialization_failures(
	READ_COMMITTED);
test_pgbench_default_transaction_isolation_level_and_serialization_failures(
	REPEATABLE_READ);
test_pgbench_default_transaction_isolation_level_and_serialization_failures(
	SERIALIZABLE);

test_pgbench_serialization_failures_retry(REPEATABLE_READ);
test_pgbench_serialization_failures_retry(SERIALIZABLE);

test_pgbench_deadlock_failures(READ_UNCOMMITTED);
test_pgbench_deadlock_failures(READ_COMMITTED);
test_pgbench_deadlock_failures(REPEATABLE_READ);
test_pgbench_deadlock_failures(SERIALIZABLE);

test_pgbench_deadlock_failures_retry(READ_UNCOMMITTED);
test_pgbench_deadlock_failures_retry(READ_COMMITTED);
test_pgbench_deadlock_failures_retry(REPEATABLE_READ);
test_pgbench_deadlock_failures_retry(SERIALIZABLE);
