use strict;
use warnings;
use Cluster;
use TestLib;
use DBI;
use File::Temp ();

#use DBD::Pg ':async';

#@{; eval { TestLib::end_av->object_2svref } || [] } = ();

$File::Temp::KEEP_ALL = 1;

my $cluster = new Cluster(3);
$cluster->init;
$cluster->configure;
$cluster->start;
$cluster->detach;


#sleep(3600);

