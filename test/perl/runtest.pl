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
my $port;
GetOptions (
			"--host=s" => \$host,
			"--port=s" => \$port,
            "--dbname=s" => \$dbname,
            "--username=s" => \$username,
            "--password=s" => \$password);

$dbname ||= '_pgpro_scheduler_test';
$username ||= 'postgres';

my $adm_dsn = 'dbi:Pg:dbname=postgres';
my $dsn = "dbi:Pg:dbname=$dbname";


if($host)
{
	$adm_dsn += ";host=".$host;
	$dsn += ";host=".$host;
}
if($port)
{
	$adm_dsn += ";port=".$port;
	$dsn += ";port=".$port;
}

print "Prepare test enviroment\n";
my $dbh = DBI->connect($adm_dsn, $username, $password, {PrintError => 1});
if($dbh->err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

my @sqls = ( 
	"ALTER SYSTEM SET schedule.enabled=off",
	"SELECT pg_reload_conf()",
	"DROP DATABASE IF EXISTS $dbname",
	"CREATE DATABASE $dbname",
);

map { __do_sql($dbh, $_) } @sqls;
$dbh->disconnect();

$dbh = DBI->connect($dsn, $username, $password, {PrintError => 1});
if($dbh->err != 0){
    print $DBI::errstr . "\n";
    exit(-1);
}

my @sql2 = (
	"CREATE EXTENSION pgpro_scheduler",
	"ALTER DATABASE $dbname SET schedule.max_workers = 1",
	"ALTER SYSTEM SET schedule.database = '$dbname'",
	"ALTER SYSTEM SET schedule.enabled = on",
	"SELECT pg_reload_conf();",
	"CREATE TABLE test_results( time_mark timestamp, commentary text )",
	"DROP ROLE IF EXISTS tester",
	"CREATE ROLE tester",
	"GRANT INSERT ON test_results TO tester",
);
map { __do_sql($dbh, $_) } @sql2;
$dbh->disconnect();

print "Run tests\n";

my @db_param = ["--dbname=$dbname"];
push @db_param, "--host=$host" if $host;
push @db_param, "--port=$port" if $port;
push @db_param, "--username=$username" if $username;
push @db_param, "--password=$password" if $password;

my %args = (
    verbosity => 1,
    test_args => \@db_param
);
my $harness = TAP::Harness->new( \%args );
my @tests = glob( 't/*.t' );
$harness->runtests(@tests );


sub __do_sql
{
	my $dbh = shift;
	my $query = shift;

	print " -> $query\n";
	$dbh->do($query);
	if($dbh->err != 0)
	{
    	print STDERR "ON query: $query ".$DBI::errstr."\n";
		exit(-1);
	}
}
