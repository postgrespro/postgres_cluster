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
ok($dbh->err == 0, $dbh->errstr) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

$query = "SELECT schedule.create_job(now() + interval \'1 minute\',
            ARRAY[\'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithDate'')\',
                    \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithDate'')\']);";
my $sth = $dbh->prepare($query);
ok($sth->execute(), $dbh->errstr) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();

sleep 70;
$query = "SELECT count(*) FROM test_results";
$sth = $dbh->prepare($query);
ok($sth->execute(), $dbh->errstr) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

my $result = $sth->fetchrow_array() and $sth->finish();
ok ($result > 1) or print "Count == 0\n";

$query = "SELECT schedule.deactivate_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n" ;
$sth->finish();

$query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0, $dbh->errstr) or print $DBI::errstr . "\n" ;

$query = "SELECT schedule.drop_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n" ;
$sth->finish();

$dbh->disconnect();

done_testing();

