# Test state_3pc -- persistent state change of prepared xact.

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 6;

my $psql_out = '';
my $psql_rc  = '';

my $node = get_new_node("london");
$node->init(allows_streaming => 1);
$node->append_conf(
	'postgresql.conf', qq(
	max_prepared_transactions = 10
	log_checkpoints = true
));
$node->start;

# Create table we'll use in the test transactions
$node->safe_psql('postgres', "CREATE TABLE t_precommit_tbl (id int, msg text)");

###############################################################################
# Check ordinary restart
###############################################################################

$node->safe_psql(
	'postgres', "
	BEGIN;
	INSERT INTO t_precommit_tbl VALUES (1, 'x');
	PREPARE TRANSACTION 'xact_prepared';

	BEGIN;
	INSERT INTO t_precommit_tbl VALUES (2, 'x');
	PREPARE TRANSACTION 'xact_precommitted';

	SELECT pg_precommit_prepared('xact_precommitted', 'precommitted');");
$node->psql(
	'postgres',
	"SELECT count(*) FROM pg_prepared_xacts WHERE state3pc = 'precommitted'",
	stdout => \$psql_out);
is($psql_out, '1',
   "Check that state3pc is set");
$node->stop;
$node->start;

$node->psql(
	'postgres',
	"SELECT count(*) FROM pg_prepared_xacts WHERE state3pc = 'precommitted'",
	stdout => \$psql_out);
is($psql_out, '1',
	"Check that state3pc preserved during reboot");

###############################################################################
# Check crash/recover
###############################################################################

$node->safe_psql(
	'postgres', "
	BEGIN;
	INSERT INTO t_precommit_tbl VALUES (3, 'x');
	PREPARE TRANSACTION 'xact_prepared_2';

	BEGIN;
	INSERT INTO t_precommit_tbl VALUES (4, 'x');
	PREPARE TRANSACTION 'xact_precommitted_2';

	SELECT pg_precommit_prepared('xact_precommitted_2', 'precommitted');");
$node->stop("immediate");
$node->start;


$node->psql(
	'postgres',
	"SELECT count(*) FROM pg_prepared_xacts WHERE state3pc = 'precommitted'",
	stdout => \$psql_out);
is($psql_out, '2',
	"Check that state3pc preserved during after crash");

###############################################################################
# Check 'P ckpt 3PC ckpt reboot' sequence -- exorcize interactions with checkpointer
# Also add some DDL to make two phase files more complex
###############################################################################
$node->safe_psql(
	'postgres', "
	BEGIN;
	INSERT INTO t_precommit_tbl VALUES (5, 'x');
	CREATE TABLE apples (i int primary key, t text);
	CREATE TABLE pears (i int primary key, t text);
	PREPARE TRANSACTION 'xact_precommitted_3';

	CHECKPOINT;

	SELECT pg_precommit_prepared('xact_precommitted_3', 'precommitted');");
$node->psql(
	'postgres',
	"SELECT count(*) FROM pg_prepared_xacts WHERE state3pc = 'precommitted'",
	stdout => \$psql_out);
is($psql_out, '3',
   "Check that state3pc is set after P ckpt");

$node->safe_psql('postgres', "CHECKPOINT");
$node->psql(
	'postgres',
	"SELECT count(*) FROM pg_prepared_xacts WHERE state3pc = 'precommitted'",
	stdout => \$psql_out);
is($psql_out, '3',
   "Check that state3pc is set after P ckpt 3pc ckpt");

$node->stop("immediate");
$node->start;

$node->psql(
	'postgres',
	"SELECT count(*) FROM pg_prepared_xacts WHERE state3pc = 'precommitted'",
	stdout => \$psql_out);
is($psql_out, '3',
   "Check that state3pc is preserved after 'P ckpt 3PC ckpt reboot'");
