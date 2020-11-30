# logical replication of 2PC test
use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 2;

# Initialize node
my $node_logical = get_new_node('logical');
$node_logical->init(allows_streaming => 'logical');
$node_logical->append_conf(
        'postgresql.conf', qq(
        max_prepared_transactions = 10
));
$node_logical->start;

# Create some pre-existing content on logical
$node_logical->safe_psql('postgres', "CREATE TABLE tab (a int PRIMARY KEY)");
$node_logical->safe_psql('postgres',
	"INSERT INTO tab SELECT generate_series(1,10)");
$node_logical->safe_psql('postgres',
	"SELECT 'init' FROM pg_create_logical_replication_slot('regression_slot', 'test_decoding');");

# This test is specifically for testing concurrent abort while logical decode is
# ongoing. The decode-delay value will allow for each change decode to sleep for
# those many seconds. We will fire off a ROLLBACK from another session when this
# delayed decode is ongoing. That will stop decoding immediately and the next
# pg_logical_slot_get_changes call should show only a few records decoded from
# the entire two phase transaction

# consume all changes so far
#$node_logical->safe_psql('postgres', "SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1', 'twophase-decoding', '1', 'twophase-decode-with-catalog-changes', '1');");

$node_logical->safe_psql('postgres', "
    BEGIN;
    INSERT INTO tab VALUES (11);
    INSERT INTO tab VALUES (12);
    PREPARE TRANSACTION 'test_prepared_tab';");

# start decoding the above with decode-delay in the background.
my $logical_connstr = $node_logical->connstr . ' dbname=postgres';

# decode now, it should only decode 1 INSERT record and should include
# an ABORT entry because of the ROLLBACK below
system_log("psql -d \"$logical_connstr\" -c \"SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1', 'twophase-decoding', '1', 'twophase-decode-with-catalog-changes', '1', 'decode-delay', '3');\" \&");

# sleep for a little while (shorter than decode-delay)
$node_logical->safe_psql('postgres', "select pg_sleep(1)");

# rollback the prepared transaction whose first record is being decoded
# after sleeping for decode-delay time
$node_logical->safe_psql('postgres', "ROLLBACK PREPARED 'test_prepared_tab';");

# wait for decoding to stop
$node_logical->psql('postgres', "select pg_sleep(4)");

# consume any remaining changes
$node_logical->safe_psql('postgres', "SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1', 'twophase-decoding', '1', 'twophase-decode-with-catalog-changes', '1');");

# check for occurrence of log about stopping decoding
my $output_file = slurp_file($node_logical->logfile());
my $abort_str = "stopping decoding of test_prepared_tab ";
like($output_file, qr/$abort_str/, "ABORT found in server log");

# Check that commit prepared is decoded properly on immediate restart
$node_logical->safe_psql('postgres', "
    BEGIN;
    INSERT INTO tab VALUES (11);
    INSERT INTO tab VALUES (12);
    PREPARE TRANSACTION 'test_prepared_tab';");
# consume changes
$node_logical->safe_psql('postgres', "SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1', 'twophase-decoding', '1', 'twophase-decode-with-catalog-changes', '1');");
$node_logical->stop('immediate');
$node_logical->start;

# commit post the restart
$node_logical->safe_psql('postgres', "COMMIT PREPARED 'test_prepared_tab';");
$node_logical->safe_psql('postgres', "SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1', 'twophase-decoding', '1', 'twophase-decode-with-catalog-changes', '1');");

# check inserts are visible
my $result = $node_logical->safe_psql('postgres', "SELECT count(*) FROM tab where a IN (11,12);");
is($result, qq(2), 'Rows inserted via 2PC are visible on restart');

$node_logical->safe_psql('postgres', "SELECT pg_drop_replication_slot('regression_slot');");
$node_logical->stop('fast');
