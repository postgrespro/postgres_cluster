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

my $query = "SELECT schedule.clean_log();";
$dbh->do($query);
ok($dbh->err == 0) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

$query = "SELECT schedule.create_job(\'{ \"name\": \"Test\",
    \"cron\": \"* * * * *\",
    \"command\": \"aaaaaaaaaa;\",
    \"run_as\": \"tester\",
    \"onrollback\": \"SELECT 1\"
    }\'
    );";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
my $job_id = $sth->fetchrow_array() and $sth->finish();

sleep 120;
$query = "SELECT message FROM schedule.get_log() WHERE cron=$job_id AND status=\'error\' ORDER BY cron DESC LIMIT 1";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

my $errorstr = $sth->fetchrow_array() and $sth->finish();
ok($errorstr eq "error in command #1: syntax error at or near \"aaaaaaaaaa\"") or print $DBI::errstr . "\n";

$query = "SELECT schedule.deactivate_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT schedule.drop_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();


sleep 70;
$query = "SELECT schedule.create_job(\'{ \"name\": \"Test\",
    \"cron\": \"* * * * *\",
    \"command\": \"SELECT * FROM abc;\",
    \"run_as\": \"tester\",
    \"onrollback\": \"SELECT 1\"
    }\'
    );";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
$job_id = $sth->fetchrow_array() and $sth->finish();

sleep 120;
$query = "SELECT message FROM schedule.get_log() WHERE cron=$job_id AND status=\'error\' ORDER BY cron DESC LIMIT 1";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

$errorstr = $sth->fetchrow_array() and $sth->finish();
ok($errorstr eq "error in command #1: relation \"abc\" does not exist") or print $DBI::errstr . "\n";

$query = "SELECT schedule.deactivate_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT schedule.drop_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

sleep 70;
$query = "SELECT schedule.create_job(\'{ \"name\": \"Test\",
    \"cron\": \"* * * * *\",
    \"command\": \"SELECT test1();\",
    \"run_as\": \"tester\",
    \"onrollback\": \"SELECT 1;\"
    }\'
    );";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);
$job_id = $sth->fetchrow_array() and $sth->finish();

sleep 120;
$query = "SELECT message FROM schedule.get_log() WHERE cron=$job_id AND status=\'error\' ORDER BY cron DESC LIMIT 1";
my $sth = $dbh->prepare($query);
ok($sth->execute()) or (print $DBI::errstr . "\n" and $dbh->disconnect() and BAIL_OUT);

$errorstr = $sth->fetchrow_array() and $sth->finish();
ok($errorstr eq "error in command #1: function test1() does not exist") or print $DBI::errstr . "\n";

$query = "SELECT schedule.deactivate_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

$query = "SELECT schedule.drop_job(?)";
$sth = $dbh->prepare($query);
$sth->bind_param(1, $job_id);
ok($sth->execute(), $dbh->errstr) or print $DBI::errstr . "\n";
$sth->finish();

$dbh->disconnect();

done_testing();

