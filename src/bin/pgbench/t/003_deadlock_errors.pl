use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 18;

# Test concurrent deadlock updates in table with different default transaction
# isolation levels.
my $node = get_new_node('main');
$node->init;
$node->start;
$node->safe_psql('postgres',
	    'CREATE UNLOGGED TABLE xy (x integer, y integer); '
	  . 'INSERT INTO xy VALUES (1, 2), (2, 3);');

my $script1 = $node->basedir . '/pgbench_script1';
append_to_file($script1, "\\set delta1 random(-5000, 5000)\n");
append_to_file($script1, "\\set delta2 random(-5000, 5000)\n");
append_to_file($script1, "BEGIN;");
append_to_file($script1, "UPDATE xy SET y = y + :delta1 WHERE x = 1;");
append_to_file($script1, "UPDATE xy SET y = y + :delta2 WHERE x = 2;");
append_to_file($script1, "END;");

my $script2 = $node->basedir . '/pgbench_script2';
append_to_file($script2, "\\set delta1 random(-5000, 5000)\n");
append_to_file($script2, "\\set delta2 random(-5000, 5000)\n");
append_to_file($script2, "BEGIN;");
append_to_file($script2, "UPDATE xy SET y = y + :delta2 WHERE x = 2;");
append_to_file($script2, "UPDATE xy SET y = y + :delta1 WHERE x = 1;");
append_to_file($script2, "END;");

# Test deadlock transactions with Read committed default isolation level:
$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RC --file), $script1, qw(--file), $script2],
	qr{processed: 50/50},
	'concurrent deadlock update: Read Committed: check processed transactions');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RC --file), $script1, qw(--file), $script2],
	qr{deadlock failures: [1-9]\d* \([1-9]\d*\.\d* %\)},
	'concurrent deadlock update: Read Committed: check deadlock failures');

# Test deadlock transactions with Repeatable read default isolation level:
$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RR --file), $script1, qw(--file), $script2],
	qr{processed: 50/50},
	'concurrent deadlock update: Repeatable Read: check processed transactions');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RR --file), $script1, qw(--file), $script2],
	qr{deadlock failures: [1-9]\d* \([1-9]\d*\.\d* %\)},
	'concurrent deadlock update: Repeatable Read: check deadlock failures');

# Test deadlock transactions with Serializable default isolation level:
$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=S --file), $script1, qw(--file), $script2],
	qr{processed: 50/50},
	'concurrent deadlock update: Serializable: check processed transactions');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=S --file), $script1, qw(--file), $script2],
	qr{deadlock failures: [1-9]\d* \([1-9]\d*\.\d* %\)},
	'concurrent deadlock update: Serializable: check deadlock failures');
