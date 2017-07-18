use strict;
use DBI;
use Getopt::Long;

my $dbname;
my $username;
my $password;
my $host;
my $port;
GetOptions (
			"--host=s" => \$host,
			"--port=s" => \$port,
            "--dbname=s" => \$dbname,
            "--username=s" => \$username,
            "--password=s" => \$password);

$dbname ||= '_pgpro_scheduler_test';

my $dsn = "dbi:Pg:dbname=$dbname";
$dsn .= ";host=".$host if $host;
$dsn .= ";port=".$port if $port;

DBI->connect($dsn, $username, $password, {PrintError => 1});

