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

$query = "SELECT schedule.submit_job(\'INSERT INTO test_results
            (time_mark, commentary) VALUES(now(), ''resubmitJob''); SELECT schedule.resubmit();\');";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();

sleep 10;
$query = "SELECT count(*) FROM test_results;";
$sth = $dbh->prepare($query);
ok($sth->execute(), $dbh->errstr) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

my $result = $sth->fetchrow_array() and $sth->finish();
ok ($result > 1) or print "Count == 0\n";

$query = "SELECT schedule.cancel_job($job_id);";
$sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
$result = $sth->fetchrow_array() and $sth->finish();
ok($result == 1) or print "Error cancel_job $job_id" . "\n";

$query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or print $DBI::errstr . "\n";

sleep 10;
$query = "SELECT count(*) FROM test_results;";
$sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

$result = $sth->fetchrow_array() and $sth->finish();
ok ($result == 0) or print "Count != 0\n";
$sth->finish();

$query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or print $DBI::errstr . "\n";

$dbh->disconnect();

done_testing();

