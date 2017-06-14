#!/usr/bin/perl
use strict;
no warnings;
use Test::Harness;
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

print "Prepare test enviroment\n";
my $dbh = DBI->connect("dbi:Pg:dbname=$dbname; host=$host", "$username", "$password",
    {PrintError => 1});
if($dbh->err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

my $query = "DROP TABLE IF EXISTS test_results;";
$dbh->do($query);
if($dbh->err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

$query = "CREATE TABLE test_results( time_mark timestamp, commentary text );";
$dbh->do($query);
if($dbh->err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

$query = "DROP ROLE IF EXISTS tester;";
$dbh->do($query);
if($dbh->err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

$query = "CREATE ROLE tester;";
$dbh->do($query);
if($DBI::err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

$query = "GRANT INSERT ON test_results TO tester;";
$dbh->do($query);
if($dbh->err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

$dbh->disconnect();

print "Run tests\n";
my @db_param = ["--host=$host", "--dbname=$dbname", "--username=$username", "--password=$password"];
my %args = (
    verbosity => 1,
    test_args => @db_param
);
my $harness = TAP::Harness->new( \%args );
my @tests = glob( 't/*.t' );
$harness->runtests(@tests );






