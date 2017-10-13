#!/usr/bin/perl
use strict;
no warnings;
use Test::More;
use DBI;
use Getopt::Long;
use Data::Dumper;

require 't/_json.pm';
my $dbh = require 't/_connect.pl';
ok($dbh->err == 0) or (print $DBI::errstr and BAIL_OUT);

my $query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

my %data = (
	name =>  "Test 1",
    cron =>  "* * * * *",
    commands =>  [
		"SELECT pg_sleep(120)",
		"INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createjobMaxRunTime'')"
	],
    run_as =>  "tester",
    use_same_transaction =>  "true",
    max_run_time => "00:01:00"
);

$query = q[SELECT schedule.create_job('].t::json(\%data).q[')];
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();


sleep 130;
$query = "SELECT count(*) FROM test_results";
$sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

my $result = $sth->fetchrow_array() and $sth->finish();
ok ($result == 0) or print "Count $result != 0\n";

if($result != 0)
{
	my $data = $dbh->selectall_arrayref('select * from test_results', { Slice => {} });
	print Data::Dumper::Dumper($data);
}

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

