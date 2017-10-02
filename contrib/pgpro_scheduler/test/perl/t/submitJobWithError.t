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

$query = "SELECT schedule.submit_job(\'SELECT * FROM test1\');";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();

sleep 30;
$query = "SELECT error FROM schedule.all_job_status WHERE id=$job_id";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

my $errorstr = $sth->fetchrow_array() and $sth->finish();
ok($errorstr eq "relation \"test1\" does not exist") or print $DBI::errstr . "\n";

$dbh->disconnect();

done_testing();

