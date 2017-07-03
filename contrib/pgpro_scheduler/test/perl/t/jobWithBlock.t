#!/usr/bin/perl
use strict;
no warnings;
use Test::More;
use DBI;
use Getopt::Long;
use Time::localtime;

my $dbh = require 't/_connect.pl';
ok($dbh->err == 0) or (print $DBI::errstr and BAIL_OUT);

my $query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or (print $DBI::errstr and $dbh->disconnect() and BAIL_OUT);

$query = "SELECT schedule.create_job(NULL, '');";
my $sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err == 0) or (print $DBI::errstr and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();
$sth->finish();

$query = "SELECT schedule.set_job_attributes(?, \'{ \"name\": \"Test\",
    \"cron\": \"* * * * *\",
    \"commands\": [\"INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''jobNextTime'')\",
                    \"INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''jobNextTime'')\"],
    \"run_as\": \"tester\",
    \"use_same_transaction\": \"true\",
    \"next_time_statement\": \"SELECT now() + interval ''2 minute'';\"
    }\')";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute()) or (print $DBI::errstr and $dbh->disconnect() and BAIL_OUT);
$sth->finish();

my $tm;

$tm = localtime();
while ($tm->sec != 0)
{
    $tm = localtime();
}

$query = "VACUUM FULL ANALYZE;";
my $sth = $dbh->prepare($query);
$sth->execute();

sleep 60;
$query = "SELECT count(*) FROM test_results";
$sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr and $dbh->disconnect() and BAIL_OUT);

my $result = $sth->fetchrow_array() and $sth->finish();
ok ($result > 1) or print "Count <= 1\n";

my $query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or (print $DBI::errstr and $dbh->disconnect() and BAIL_OUT);

sleep 120;
$query = "SELECT count(*) FROM test_results";
$sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr and $dbh->disconnect() and BAIL_OUT);

my $result = $sth->fetchrow_array() and $sth->finish();
ok ($result > 1) or print "Count <= 1\n";

$query = "SELECT schedule.deactivate_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute()) or print $DBI::errstr ;
$sth->finish();

$query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or print $DBI::errstr;

$query = "SELECT schedule.drop_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute()) or print $DBI::errstr;
$sth->finish();

$dbh->disconnect();

done_testing();

