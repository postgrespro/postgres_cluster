use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 22;

# Test pg_xlogdump for timestamp output.

sub command_output
{
	my ($cmd, $expected_stdout, $test_name) = @_;
	my ($stdout, $stderr);
	print("# Running: " . join(" ", @{$cmd}) . "\n");
	my $result = IPC::Run::run $cmd, '>', \$stdout, '2>', \$stderr;
	ok($result, "@$cmd exit code 0");
	is($stderr, '', "@$cmd no stderr");
	return $stdout;
}

my $node = get_new_node('main');
$node->init;
$node->start;
my $pgdata = $node->data_dir;
my $xlogfilename0 = $node->safe_psql('postgres',
	"SELECT pg_xlogfile_name(pg_current_xlog_location())");
my $startseg = "$pgdata/pg_xlog/$xlogfilename0";
my $endseg = $startseg;
$node->command_like(
	[ 'pg_xlogdump', '-S', "$pgdata/pg_xlog/$xlogfilename0" ],
	qr/pg_xlogdump: start timestamp:/,
	'pg_xlogdump prints start timestamp');
$node->command_like(
	[ 'pg_xlogdump', '-S', "$pgdata/pg_xlog/$xlogfilename0" ],
	qr/start timestamp[^\n]*\n((?!end timestamp).)*$/m,
	'pg_xlogdump does not print end timestamp by default');
$node->command_fails(
	[ 'pg_xlogdump', '-E', "$pgdata/pg_xlog/$xlogfilename0" ],
	'pg_xlogdump requires -S argument for printing start/end timestamps');
$node->command_fails(
	[ 'pg_xlogdump', '-S', '-F', 'XXX', "$pgdata/pg_xlog/$xlogfilename0" ],
	'pg_xlogdump fails with invalid filter');
$node->command_like(
	[ 'pg_xlogdump', '-S', '-F', 'XLOG_XACT_COMMIT,XLOG_XACT_ABORT', "$pgdata/pg_xlog/$xlogfilename0" ],
	qr/pg_xlogdump: start timestamp:/,
	'pg_xlogdump accepts filter "XLOG_XACT_COMMIT,XLOG_XACT_ABORT"');

open CONF, ">>$pgdata/postgresql.conf";
print CONF "track_commit_timestamp = on\n";
close CONF;
$node->restart;

# create test tables and move to a new segment
$node->safe_psql('postgres',
	"CREATE TABLE test1 (a int); CREATE TABLE test2 (a int); SELECT pg_switch_xlog()");

# get initial commit_timestamp and logfile_name
$node->safe_psql('postgres',
	"INSERT INTO test1 VALUES (1)");
my $tx1_timestamp = $node->safe_psql('postgres',
	"SELECT pg_xact_commit_timestamp(xmin) FROM test1"
);
print("# transaction 1 commit timestamp: $tx1_timestamp\n");
my $xlogfilename1 = $node->safe_psql('postgres',
	"SELECT pg_xlogfile_name(pg_current_xlog_location())"
);

# produce some xlog segments
for (my $i = 0; $i < 10; $i++) {
	$node->safe_psql('postgres',
		"BEGIN; INSERT INTO test2 VALUES ($i); COMMIT; SELECT pg_switch_xlog()");
}

# get a segment from the middle of the sequence
my $xlogfilenameM = $node->safe_psql('postgres',
	"SELECT pg_xlogfile_name(pg_current_xlog_location())");

# produce last segment
$node->safe_psql('postgres',
	"SELECT pg_sleep(2); DELETE FROM test2; INSERT INTO test2 VALUES (1)");

# insert XLOG_XACT_ABORT to make sure that the default bahaviour is to show only COMMIT TIMESTAMP
	$node->safe_psql('postgres',
	"SELECT pg_sleep(2); BEGIN TRANSACTION; DELETE FROM test2; ROLLBACK TRANSACTION");

# get final logfile_name and commit_timestamp (and switch segment)
my $xlogfilenameN = $node->safe_psql('postgres',
	"SELECT pg_xlogfile_name(pg_current_xlog_location())");
$node->safe_psql('postgres',
	"SELECT pg_switch_xlog()");
my $tx2_timestamp = $node->safe_psql('postgres',
	"SELECT pg_xact_commit_timestamp(xmin) FROM test2"
);
print("# transaction N commit timestamp: $tx2_timestamp\n");

# remove a file from the middle to make sure that the modified pg_xlogdump reads only necessary files
unlink "$pgdata/pg_xlog/$xlogfilenameM";

# run pg_xlogdump to check it's output
my $xld_output = command_output(
	[ 'pg_xlogdump', '-S', '-E', "$pgdata/pg_xlog/$xlogfilename1", "$pgdata/pg_xlog/$xlogfilenameN" ]);
ok($xld_output =~ qr/pg_xlogdump: start timestamp: ([^,]+), lsn: (.*)/, "start timestamp and lsn found");
my ($startts, $startlsn) = ($1, $2);
ok($xld_output =~ qr/pg_xlogdump: end timestamp: ([^,]+), lsn: (.*)/, "end timestamp and lsn found");

# check commit timestamps for first and last commits
my ($endts, $endlsn) = ($1, $2);
my $timediff1 = $node->safe_psql('postgres',
	"SELECT EXTRACT(EPOCH FROM pg_xact_commit_timestamp(xmin) - \'$startts\'::timestamp) FROM test1");
ok($timediff1 >= 0 && $timediff1 < 1, "xlog start timestamp ($startts) equals to transaction 1 timestamp ($tx1_timestamp)");
my $timediff2 = $node->safe_psql('postgres',
	"SELECT EXTRACT(EPOCH FROM \'$endts\'::timestamp - pg_xact_commit_timestamp(xmin)) FROM test2");
ok($timediff2 >= 0 && $timediff2 < 1, "xlog end timestamp ($endts) equals to transaction N timestamp ($tx2_timestamp)");

# check lsns
my $lsndiff1 = $node->safe_psql('postgres',
	"SELECT \'$endlsn\'::pg_lsn - \'$startlsn\'::pg_lsn");
print("lsndiff1: $lsndiff1\n");
ok($lsndiff1 > 0, "xlog end lsn ($endlsn) greater than xlog start lsn ($startlsn)");
my $lsndiff2 = $node->safe_psql('postgres',
	"SELECT pg_current_xlog_location() - \'$endlsn\'::pg_lsn");
ok($lsndiff2 >= 0, "xlog current lsn greater than or equal to xlog end lsn");

# check search for non-existing record types
$node->command_like(
	[ 'pg_xlogdump', '-S', '-F', 'XLOG_RESTORE_POINT', "$pgdata/pg_xlog/$xlogfilename0" ],
	qr/xlog record with timestamp is not found/,
	'pg_xlogdump accepts filter XLOG_RESTORE_POINT and processes a segment without such records');
