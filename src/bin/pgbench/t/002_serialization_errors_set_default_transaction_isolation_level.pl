use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 27;

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

# Test transactions with Read committed default isolation level:
$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RC --file), $script ],
	qr{default transaction isolation level: read committed},
	'concurrent update: Read Committed: check default isolation level');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RC --file), $script ],
	qr{processed: 50/50},
	'concurrent update: Read Committed: check processed transactions');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RC --file), $script ],
	qr{serialization failures: 0 \(0\.000 %\)},
	'concurrent update: Read Committed: check serialization failures');

# Test transactions with Repeatable read default isolation level:
$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RR --file), $script ],
	qr{default transaction isolation level: repeatable read},
	'concurrent update: Repeatable Read: check default isolation level');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RR --file), $script ],
	qr{processed: 50/50},
	'concurrent update: Repeatable Read: check processed transactions');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=RR --file), $script ],
	qr{serialization failures: [1-9]\d* \([1-9]\d*\.\d* %\)},
	'concurrent update: Repeatable Read: check serialization failures');

# Test transactions with Serializable default isolation level:
$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=S --file), $script ],
	qr{default transaction isolation level: serializable},
	'concurrent update: Serializable: check default isolation level');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=S --file), $script ],
	qr{processed: 50/50},
	'concurrent update: Serializable: check processed transactions');

$node->command_like(
	[   qw(pgbench --no-vacuum --client=5 --transactions=10
		  --default-isolation-level=S --file), $script ],
	qr{serialization failures: [1-9]\d* \([1-9]\d*\.\d* %\)},
	'concurrent update: Serializable: check serialization failures');
