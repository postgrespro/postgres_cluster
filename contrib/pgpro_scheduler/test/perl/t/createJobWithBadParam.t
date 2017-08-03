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
ok($dbh->err == 0) or print $DBI::errstr . "\n";

$query = "SELECT schedule.create_job(\'abcdefghi\',
            ARRAY[\'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\',
                    \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\']);";
my $sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT schedule.create_job(\'*bcdefgh*\',
            ARRAY[\'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\',
                    \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\']);";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT schedule.create_job(\'* * * # *\',
            ARRAY[\'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\',
                    \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\']);";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT schedule.create_job(\' \',
            ARRAY[\'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\',
                    \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\']);";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT schedule.create_job(\'\',
            ARRAY[\'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\',
                    \'INSERT INTO test_results (time_mark, commentary) VALUES(now(), ''createJobWithCron'')\']);";
$sth = $dbh->prepare($query);
$sth->execute();
ok($dbh->err != 0) or print $DBI::errstr . "\n";
$sth->finish();

$dbh->disconnect();

done_testing();

