# logical replication of 2PC test
use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 14;

# Initialize publisher node
my $node_publisher = get_new_node('publisher');
$node_publisher->init(allows_streaming => 'logical');
$node_publisher->append_conf(
        'postgresql.conf', qq(
        max_prepared_transactions = 10
	));
$node_publisher->start;

# Create subscriber node
my $node_subscriber = get_new_node('subscriber');
$node_subscriber->init(allows_streaming => 'logical');
$node_subscriber->append_conf(
	'postgresql.conf', qq(
	max_prepared_transactions = 10
	logical_replication_2pc = always
));
$node_subscriber->start;
# Create some pre-existing content on publisher
$node_publisher->safe_psql('postgres', "CREATE TABLE tab_full (a int PRIMARY KEY)");
$node_publisher->safe_psql('postgres',
	"INSERT INTO tab_full SELECT generate_series(1,10)");
$node_publisher->safe_psql('postgres', "CREATE TABLE tab_full2 (x text)");
$node_publisher->safe_psql('postgres',
	"INSERT INTO tab_full2 VALUES ('a'), ('b'), ('b')");

# Setup structure on subscriber
$node_subscriber->safe_psql('postgres', "CREATE TABLE tab_full (a int PRIMARY KEY)");
$node_subscriber->safe_psql('postgres', "CREATE TABLE tab_full2 (x text)");

# Setup logical replication
my $publisher_connstr = $node_publisher->connstr . ' dbname=postgres';
$node_publisher->safe_psql('postgres', "CREATE PUBLICATION pub");
$node_publisher->safe_psql('postgres',
"ALTER PUBLICATION pub ADD TABLE tab_full, tab_full2"
);

my $appname = 'sub';
$node_subscriber->safe_psql('postgres',
"CREATE SUBSCRIPTION sub CONNECTION '$publisher_connstr application_name=$appname' PUBLICATION pub"
);

# Wait for subscriber to finish initialization
my $caughtup_query =
"SELECT pg_current_wal_lsn() <= replay_lsn FROM pg_stat_replication WHERE application_name = '$appname';";
$node_publisher->poll_query_until('postgres', $caughtup_query)
  or die "Timed out while waiting for subscriber to catch up";

# Also wait for initial table sync to finish
my $synced_query =
"SELECT count(1) = 0 FROM pg_subscription_rel WHERE srsubstate NOT IN ('r', 's');";
$node_subscriber->poll_query_until('postgres', $synced_query)
  or die "Timed out while waiting for subscriber to synchronize data";

# check that 2PC gets replicated to subscriber
$node_publisher->safe_psql('postgres',
	"BEGIN;INSERT INTO tab_full VALUES (11);PREPARE TRANSACTION 'test_prepared_tab_full';");

$node_publisher->poll_query_until('postgres', $caughtup_query)
  or die "Timed out while waiting for subscriber to catch up";

# check that transaction is in prepared state on subscriber
my $result =
   $node_subscriber->safe_psql('postgres', "SELECT count(*) FROM pg_prepared_xacts where gid = 'test_prepared_tab_full';");
   is($result, qq(1), 'transaction is prepared on subscriber');

# check that 2PC gets committed on subscriber
$node_publisher->safe_psql('postgres',
	"COMMIT PREPARED 'test_prepared_tab_full';");

$node_publisher->poll_query_until('postgres', $caughtup_query)
  or die "Timed out while waiting for subscriber to catch up";

# check that transaction is committed on subscriber
$result =
   $node_subscriber->safe_psql('postgres', "SELECT count(*) FROM tab_full where a = 11;");
   is($result, qq(1), 'Row inserted via 2PC has committed on subscriber');
$result =
   $node_subscriber->safe_psql('postgres', "SELECT count(*) FROM pg_prepared_xacts where gid = 'test_prepared_tab_full';");
   is($result, qq(0), 'transaction is committed on subscriber');

# check that 2PC gets replicated to subscriber
$node_publisher->safe_psql('postgres',
	"BEGIN;INSERT INTO tab_full VALUES (12);PREPARE TRANSACTION 'test_prepared_tab_full';");

$node_publisher->poll_query_until('postgres', $caughtup_query)
  or die "Timed out while waiting for subscriber to catch up";

# check that transaction is in prepared state on subscriber
$result =
   $node_subscriber->safe_psql('postgres', "SELECT count(*) FROM pg_prepared_xacts where gid = 'test_prepared_tab_full';");
   is($result, qq(1), 'transaction is prepared on subscriber');

# check that 2PC gets aborted on subscriber
$node_publisher->safe_psql('postgres',
	"ROLLBACK PREPARED 'test_prepared_tab_full';");

$node_publisher->poll_query_until('postgres', $caughtup_query)
  or die "Timed out while waiting for subscriber to catch up";

# check that transaction is aborted on subscriber
$result =
   $node_subscriber->safe_psql('postgres', "SELECT count(*) FROM tab_full where a = 12;");
   is($result, qq(0), 'Row inserted via 2PC is not present on subscriber');

$result =
   $node_subscriber->safe_psql('postgres', "SELECT count(*) FROM pg_prepared_xacts where gid = 'test_prepared_tab_full';");
   is($result, qq(0), 'transaction is aborted on subscriber');

# Check that commit prepared is decoded properly on crash restart
$node_publisher->safe_psql('postgres', "
    BEGIN;
    INSERT INTO tab_full VALUES (12);
    INSERT INTO tab_full VALUES (13);
    PREPARE TRANSACTION 'test_prepared_tab';");
$node_subscriber->stop('immediate');
$node_publisher->stop('immediate');
$node_publisher->start;
$node_subscriber->start;

# commit post the restart
$node_publisher->safe_psql('postgres', "COMMIT PREPARED 'test_prepared_tab';");
$node_publisher->poll_query_until('postgres', $caughtup_query)
  or die "Timed out while waiting for subscriber to catch up";

# check inserts are visible
$result = $node_subscriber->safe_psql('postgres', "SELECT count(*) FROM tab_full where a IN (11,12);");
is($result, qq(2), 'Rows inserted via 2PC are visible on the subscriber');

# TODO add test cases involving DDL. This can be added after we add functionality
# to replicate DDL changes to subscriber.

# check all the cleanup
$node_subscriber->safe_psql('postgres', "DROP SUBSCRIPTION sub");

$result = $node_subscriber->safe_psql('postgres',
	"SELECT count(*) FROM pg_subscription");
is($result, qq(0), 'check subscription was dropped on subscriber');

$result = $node_publisher->safe_psql('postgres',
	"SELECT count(*) FROM pg_replication_slots");
is($result, qq(0), 'check replication slot was dropped on publisher');

$result = $node_subscriber->safe_psql('postgres',
	"SELECT count(*) FROM pg_subscription_rel");
is($result, qq(0),
	'check subscription relation status was dropped on subscriber');

$result = $node_publisher->safe_psql('postgres',
	"SELECT count(*) FROM pg_replication_slots");
is($result, qq(0), 'check replication slot was dropped on publisher');

$result = $node_subscriber->safe_psql('postgres',
	"SELECT count(*) FROM pg_replication_origin");
is($result, qq(0), 'check replication origin was dropped on subscriber');

# Check that if xact was prepared before snap became consistent, but committed
# after we still get it.
diag("\npublisher listens on \"" . $node_publisher->connstr . "\"");
diag("subscriber listens on \"" . $node_subscriber->connstr . "\"");

$node_publisher->safe_psql('postgres', "truncate tab_full");
$node_subscriber->safe_psql('postgres', "truncate tab_full");
# start xact
my ($running_xact_stdin, $running_xact_stdout, $running_xact_stderr) = ('', '', '');
my $running_xact = IPC::Run::start(
	[
		'psql', '-X', '-qAt', '-v', 'ON_ERROR_STOP=1', '-f', '-', '-d',
		$node_publisher->connstr('postgres')
	],
	'<',
	\$running_xact_stdin,
	'>',
	\$running_xact_stdout,
	'2>',
	\$running_xact_stderr);
$running_xact_stdin .= q[
  begin;
  insert into tab_full values (42);
  select 42;
];
$running_xact->pump until $running_xact_stdout =~ qr/42$/m;
$running_xact_stdout = '';
$running_xact_stderr = '';

# start creating sub, it will hang
my ($create_sub_stdin, $create_sub_stdout, $create_sub_stderr) = ('', '', '');
my $create_sub = IPC::Run::start(
	[
		'psql', '-X', '-qAt', '-v', 'ON_ERROR_STOP=1', '-f', '-', '-d',
		$node_subscriber->connstr('postgres')
	],
	'<',
	\$create_sub_stdin,
	'>',
	\$create_sub_stdout,
	'2>',
	\$create_sub_stderr);
$create_sub_stdin .= qq[
  create subscription sub connection '$publisher_connstr application_name=$appname' publication pub with (copy_data=false);
  ];
## push input
pump $create_sub while length $create_sub_stdin;
## Wait till decoding of xl_running_xacts, not 100% reliable
sleep(3);

# start another xact
my ($running_xact2_stdin, $running_xact2_stdout, $running_xact2_stderr) = ('', '', '');
my $running_xact2 = IPC::Run::start(
	[
		'psql', '-X', '-qAt', '-v', 'ON_ERROR_STOP=1', '-f', '-', '-d',
		$node_publisher->connstr('postgres')
	],
	'<',
	\$running_xact2_stdin,
	'>',
	\$running_xact2_stdout,
	'2>',
	\$running_xact2_stderr);
$running_xact2_stdin .= q[
  begin;
  insert into tab_full values (43);
  select 42;
];
$running_xact2->pump until $running_xact2_stdout =~ qr/42$/m;
$running_xact2_stdout = '';
$running_xact2_stderr = '';

# finish 1st running xact: logical decoding will find initial starting point
$running_xact_stdin .= q[
  commit;
  select 42;
];
$running_xact->pump until $running_xact_stdout =~ qr/42$/m;
$running_xact_stdout = '';
$running_xact_stderr = '';

# PREPARE xact
my ($prepared_xact_stdin, $prepared_xact_stdout, $prepared_xact_stderr) = ('', '', '');
my $prepared_xact = IPC::Run::start(
	[
		'psql', '-X', '-qAt', '-v', 'ON_ERROR_STOP=1', '-f', '-', '-d',
		$node_publisher->connstr('postgres')
	],
	'<',
	\$prepared_xact_stdin,
	'>',
	\$prepared_xact_stdout,
	'2>',
	\$prepared_xact_stderr);
$prepared_xact_stdin .= q[
  begin; insert into tab_full values (44);
  prepare transaction 'decoding_start_test';
  select 42;
  ];
$prepared_xact->pump until $prepared_xact_stdout =~ qr/42$/m;
$prepared_xact_stdout = '';
$prepared_xact_stderr = '';

# finish 2nd running xact: logical decoding will find consistent snapshot
$running_xact2_stdin .= q[
  commit;
  select 42;
];
$running_xact2->pump until $running_xact2_stdout =~ qr/42$/m;
$running_xact2_stdout = '';
$running_xact2_stderr = '';

# ensure that sub is created
$create_sub_stdin .= qq[
  select 42;
  ];
## push input
pump $create_sub until $create_sub_stdout =~ qr/42$/m;

# commit prepared xact
$prepared_xact_stdin .= q[
  commit prepared 'decoding_start_test';
  select 42;
  ];
$prepared_xact->pump until $prepared_xact_stdout =~ qr/42$/m;
$prepared_xact_stdout = '';
$prepared_xact_stderr = '';

$node_publisher->poll_query_until('postgres',
								  "SELECT pg_current_wal_lsn() <= replay_lsn FROM pg_stat_replication WHERE application_name = 'sub';")
	or die "Timed out while waiting for subscriber to catch up";

# check that prepared xact (the only one replicated) was really done
my $res_sub = $node_subscriber->safe_psql('postgres', "SELECT sum(a) FROM tab_full");
is($res_sub, q(44), 'prepared xact replicated');

$running_xact->finish;
$running_xact2->finish;
$create_sub->finish;
$prepared_xact->finish;
$node_subscriber->safe_psql('postgres', "DROP SUBSCRIPTION sub");
$node_publisher->safe_psql('postgres', "DROP PUBLICATION pub");

# Now test that tablesync behaviour is sane.
diag("Testing tablesync");
$node_publisher->safe_psql('postgres', "CREATE TABLE apples (i int)");
$node_subscriber->safe_psql('postgres', "CREATE TABLE apples (i int)");
$node_publisher->safe_psql('postgres', "CREATE TABLE pears (i int)");
$node_subscriber->safe_psql('postgres', "CREATE TABLE pears (i int)");
# create a lot of pears
$node_publisher->safe_psql('postgres',
	"INSERT INTO pears SELECT generate_series(10000, 1000000); select txid_current();");

# create publication involving only apples...
$node_publisher->safe_psql('postgres',
						   "CREATE PUBLICATION pub FOR TABLE apples");

# set up replication and wait for it to start up
$node_subscriber->safe_psql('postgres',
							"create subscription sub connection '$publisher_connstr application_name=sub' publication pub;");
$node_subscriber->poll_query_until('postgres', $synced_query)
  or die "Timed out while waiting for subscriber to synchronize data";

# now, add another table, starting initial tablesync of large table
$node_publisher->safe_psql('postgres',
						   "ALTER PUBLICATION pub ADD TABLE pears");
$node_subscriber->safe_psql('postgres',
							"ALTER SUBSCRIPTION sub REFRESH PUBLICATION WITH (copy_data=true)");
# and while it is (hopefully) still going on, prepare some xact
# but before that, make sure tablesync can find consistent point
$node_publisher->safe_psql('postgres', "CHECKPOINT; CHECKPOINT;");
$node_publisher->safe_psql('postgres', "BEGIN; INSERT INTO apples VALUES (42); INSERT INTO pears VALUES (42); PREPARE TRANSACTION 'prepared_c';");
# and prepare another xact which we will abort
$node_publisher->safe_psql('postgres', "BEGIN; INSERT INTO apples VALUES (43); INSERT INTO pears VALUES (43); PREPARE TRANSACTION 'prepared_a';");
# and finish xacts
$node_publisher->safe_psql('postgres', "COMMIT PREPARED 'prepared_c';");
$node_publisher->safe_psql('postgres', "ROLLBACK PREPARED 'prepared_a';");

# replicate commit/abort and check sum
$node_publisher->poll_query_until('postgres',
								  "SELECT pg_current_wal_lsn() <= replay_lsn FROM pg_stat_replication WHERE application_name = 'sub';")
	or die "Timed out while waiting for subscriber to catch up";
my $res_pub = $node_publisher->safe_psql('postgres', "SELECT sum(i) FROM (SELECT i FROM apples UNION ALL SELECT i FROM pears) u");
$res_sub = $node_subscriber->safe_psql('postgres', "SELECT sum(i) FROM (SELECT i FROM apples UNION ALL SELECT i FROM pears) u");
is($res_pub, $res_sub, 'sum is the same');

$node_subscriber->safe_psql('postgres', "DROP SUBSCRIPTION sub");
$node_publisher->safe_psql('postgres', "DROP PUBLICATION pub");

$node_subscriber->stop('fast');
$node_publisher->stop('fast');
