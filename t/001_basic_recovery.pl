use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 2;
use DBI;
use DBD::Pg ':async';

###############################################################################
# Aux routines
###############################################################################

sub PostgresNode::inet_connstr {
   my ($self, $dbname) = @_;
   my $pgport = $self->port;
   my $pghost = '127.0.0.1';
   my $pgdata = $self->data_dir;

   if (!defined($dbname))
   {
   	return "port=$pgport host=$pghost";
   }
   return "port=$pgport host=$pghost dbname=$dbname";
}

###############################################################################
# Setup nodes
###############################################################################

my $nnodes = 3;
my @nodes = ();
my $pgconf_common = qq(
	listen_addresses = '127.0.0.1'
	max_prepared_transactions = 10
	max_worker_processes = 10
	max_wal_senders = 10
	max_replication_slots = 10
	wal_level = logical
	shared_preload_libraries = 'multimaster'
	multimaster.workers=4
	multimaster.queue_size=10485760 # 10mb
);

# Init nodes
for (my $i=0; $i < $nnodes; $i++) {
   push(@nodes, get_new_node("node$i"));
   $nodes[$i]->init;
}

# Collect conn info
my $mm_connstr = join(', ', map { "${ \$_->inet_connstr('postgres') }" } @nodes);

# Configure and start nodes
for (my $i=0; $i < $nnodes; $i++) {
   $nodes[$i]->append_conf('postgresql.conf', $pgconf_common);
   $nodes[$i]->append_conf('postgresql.conf', qq(
      multimaster.node_id = @{[ $i + 1 ]}
      multimaster.conn_strings = '$mm_connstr'
      #multimaster.arbiter_port = ${ \$nodes[0]->port }
   ));
   $nodes[$i]->append_conf('pg_hba.conf', qq(
   	host replication all 127.0.0.1/32 trust
   ));
   $nodes[$i]->start;
}

###############################################################################
# Wait until nodes are up
###############################################################################

my $psql_out;
# XXX: change to poll_untill
sleep(7);

###############################################################################
# Replication check
###############################################################################

$nodes[0]->psql('postgres', "
	create extension multimaster;
	create table if not exists t(k int primary key, v int);
	insert into t values(1, 10);
");

$nodes[1]->psql('postgres', "select v from t where k=1;", stdout => \$psql_out);
is($psql_out, '10', "Check sanity while all nodes are up.");

###############################################################################
# Isolation regress checks
###############################################################################

# we can call pg_regress here

###############################################################################
# Work after node stop
###############################################################################

$nodes[2]->teardown_node;

# $nodes[0]->poll_query_until('postgres',
#   "select disconnected = true from mtm.get_nodes_state() where id=3;")
#   or die "Timed out while waiting for node to disconnect";

$nodes[0]->psql('postgres', "
	insert into t values(2, 20);
");

$nodes[1]->psql('postgres', "select v from t where k=2;", stdout => \$psql_out);
is($psql_out, '20', "Check that we can commit after one node disconnect.");

















