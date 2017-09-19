use strict;
use warnings;
use PostgresNode;
use Cluster;
use TestLib;
use Test::More tests => 7;

my $cluster = new Cluster(3);
$cluster->init();
$cluster->configure();
$cluster->start();

# XXXX: delete all '-n' ?

# await online status
$cluster->{nodes}->[0]->poll_query_until('postgres', "select 't'");

# init
$cluster->pgbench(0, ('-i', -s => '10') );
$cluster->pgbench(0, ('-N', '-n', -T => '1') );
$cluster->pgbench(1, ('-N', '-n', -T => '1') );
$cluster->pgbench(2, ('-N', '-n', -T => '1') );
$cluster->psql(0, 'postgres', "create extension multimaster");


# ################################################################################
# # auto recovery
# ################################################################################
$cluster->{nodes}->[2]->stop('fast');
$cluster->pgbench(0, ('-N', '-n', -T => '1') );
$cluster->{nodes}->[2]->start;
$cluster->{nodes}->[2]->poll_query_until('postgres', "select 't'");
is($cluster->is_data_identic( (0,1,2) ), 1, "check auto recovery");

# ################################################################################
# # auto recovery with dead slots
# ################################################################################

# ## TBD

# ################################################################################
# # basebackup and add node
# ################################################################################

my $new_connstr;

$cluster->add_node();
$new_connstr = $cluster->{nodes}->[3]->{mmconnstr};
$cluster->psql(0, 'postgres', "SELECT mtm.add_node('$new_connstr')");
# await for comletion?
$cluster->{nodes}->[3]->start;
$cluster->{nodes}->[3]->poll_query_until('postgres', "select 't'");
$cluster->pgbench(0, ('-N', '-n', -T => '1') );
$cluster->pgbench(3, ('-N', '-n', -T => '1') );
is($cluster->is_data_identic( (0,1,2,3) ), 1, "basebackup and add node");

# ################################################################################
# # soft stop / resume
# ################################################################################

my ($stopped_out, $stopped_err);

$cluster->psql(0, 'postgres', "select mtm.stop_node(3,'f')");
# await for comletion?
$cluster->pgbench(0, ('-N', '-n', -T => '1') );
$cluster->pgbench(1, ('-N', '-n', -T => '1') );
$cluster->pgbench(3, ('-N', '-n', -T => '1') );
$cluster->{nodes}->[2]->psql('postgres', "select 't'",
                            stdout => \$stopped_out, stderr => \$stopped_err);
is($cluster->is_data_identic( (0,1,3) ), 1, "soft stop / resume");
print("::$stopped_out ::$stopped_err\n");
is($stopped_out eq '' && $stopped_err ne '', 1, "soft stop / resume");

$cluster->psql(0, 'postgres', "select mtm.resume_node(3)");
$cluster->{nodes}->[2]->poll_query_until('postgres', "select 't'");
$cluster->pgbench(2, ('-N', '-n', -T => '1') );
is($cluster->is_data_identic( (0,1,2,3) ), 1, "soft stop / resume");

################################################################################
# hard stop / basebackup / recover
################################################################################

diag('Stopping node with slot drop');
$cluster->psql(0, 'postgres', "select mtm.stop_node(3,'t')");
# await for comletion?
$cluster->{nodes}->[2]->stop('fast');

$cluster->pgbench(0, ('-N', '-n', -T => '1') );
$cluster->pgbench(1, ('-N', '-n', -T => '1') );
$cluster->pgbench(3, ('-N', '-n', -T => '1') );
is($cluster->is_data_identic( (0,1,3) ), 1, "hard stop / resume");

$cluster->psql(0, 'postgres', "select mtm.recover_node(3)");

# now we need to perform backup from live node
$cluster->add_node(port => $cluster->{nodes}->[2]->{_port},
    arbiter_port => $cluster->{nodes}->[2]->{arbiter_port},
    node_id => 3);

my $dd = $cluster->{nodes}->[4]->data_dir;
diag("preparing to start $dd");

$cluster->{nodes}->[4]->start;
$cluster->{nodes}->[4]->poll_query_until('postgres', "select 't'");

$cluster->pgbench(0, ('-N', '-n', -T => '1') );
$cluster->pgbench(1, ('-N', '-n', -T => '1') );
$cluster->pgbench(3, ('-N', '-n', -T => '1') );
$cluster->pgbench(4, ('-N', '-n', -T => '1') );
is($cluster->is_data_identic( (0,1,3,4) ), 1, "hard stop / resume");
