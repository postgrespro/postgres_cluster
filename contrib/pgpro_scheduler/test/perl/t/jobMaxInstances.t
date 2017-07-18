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
    \"commands\": [\"SELECT pg_sleep(120)\",
                    \"INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''jobMaxInstances'')\"],
    \"run_as\": \"tester\",
    \"use_same_transaction\": \"true\",
    \"max_instances\": 1
    }\'
    );";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();

sleep 180;
$query = "SELECT schedule.deactivate_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT message FROM schedule.get_log()
    WHERE cron=$job_id AND status=\'error\' AND message = 'max instances limit reached' LIMIT 1";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $errorstr = $sth->fetchrow_array() and $sth->finish();
ok($errorstr eq "max instances limit reached") or print $DBI::errstr . "\n";

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

