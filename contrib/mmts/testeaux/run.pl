use strict;
use warnings;
use Cluster;
use TestLib;
use DBI;
use File::Temp ();

$File::Temp::KEEP_ALL = 1;

my $cluster = new Cluster(3);
$cluster->init;
$cluster->configure;
$cluster->start;
$cluster->detach;

