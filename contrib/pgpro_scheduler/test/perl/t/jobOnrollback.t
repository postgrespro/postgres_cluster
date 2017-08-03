#!/usr/bin/perl
use strict;
no warnings;
use Test::More;
use DBI;
use Getopt::Long;

my $dbh = require 't/_connect.pl';
ok($dbh->err == 0) or (print $DBI::errstr and BAIL_OUT);

my $query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

$query = "SELECT schedule.create_job(\'{ \"name\": \"Test 1\",
    \"cron\": \"* * * * *\",
    \"commands\": [\"INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''jobOnrollback'')\",
                    \"INSERT INTO test_results (time_mark1, commentary1) VALUES(now(), ''jobOnrollback'')\"],
    \"run_as\": \"tester\",
    \"use_same_transaction\": \"true\",
    \"onrollback\": \"INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''rollback'')\"
    }\'
    );";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();

sleep 120;
$query = "SELECT count(*) FROM test_results WHERE commentary=\'rollback\'";
$sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

my $result = $sth->fetchrow_array() and $sth->finish();
ok ($result > 0) or print "Count == 0\n";

$query = "SELECT count(*) FROM test_results WHERE commentary=\'jobOnrollback\'";
$sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

$result = $sth->fetchrow_array() and $sth->finish();
ok ($result == 0) or print "Count != 0\n";

$query = "SELECT schedule.deactivate_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

$query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0, $dbh->errstr) or print $DBI::errstr . "\n";

$query = "SELECT schedule.drop_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

$dbh->disconnect();

done_testing();

