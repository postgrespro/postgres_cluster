#!/usr/bin/perl
use strict;
no warnings;
use Test::More;
use DBI;
use Getopt::Long;

my $dbname;
my $username;
my $password;
my $host;
GetOptions ( "--host=s" => \$host,
    "--dbname=s" => \$dbname,
    "--username=s" => \$username,
    "--password=s" => \$password);
my $dbh = DBI->connect("dbi:Pg:dbname=$dbname; host=$host", "$username", "$password",
    {PrintError => 0});
ok($dbh->err == 0) or (print $DBI::errstr and BAIL_OUT);

my $query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or print $DBI::errstr;

$query = "SELECT schedule.create_job(\'abcdefghi\',
            \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createSimpleJobWithBadCron'')\');";
my $sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or (print $DBI::errstr and $dbh->disconnect() and BAIL_OUT);

$query = "SELECT schedule.create_job(\'*bcdefgh*\',
            \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createSimpleJobWithBadCron'')\');";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr;

$query = "SELECT schedule.create_job(\'* * * # *\',
            \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createSimpleJobWithBadCron'')\');";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr;

$query = "SELECT schedule.create_job(\' \',
            \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createSimpleJobWithBadCron'')\');";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr;

$query = "SELECT schedule.create_job(\'\',
            \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createSimpleJobWithBadCron'')\');";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr;

$dbh->disconnect();

done_testing();

