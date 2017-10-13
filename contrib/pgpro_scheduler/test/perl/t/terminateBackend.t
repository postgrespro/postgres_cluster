#!/usr/bin/perl
use strict;
no warnings;
use Test::More tests => 15;
use DBI;
use Getopt::Long;
use Data::Dumper;

my $dbh = require 't/_connect.pl';
ok($dbh->err == 0, 'connect') or  BAIL_OUT($DBI::errstr);

my $query = "DELETE FROM test_results;";
$dbh->do($query);
ok($dbh->err == 0, 'clean up test_results') or BAIL_OUT($DBI::errstr);

my $query = "DELETE FROM task_info;";
$dbh->do($query);
ok($dbh->err == 0, 'clean up task_info') or BAIL_OUT($DBI::errstr);

one_task_do('sleeper', 'select pg_sleep(200)');
one_task_do('writer',
'DO
$do$
BEGIN
	WHILE true LOOP
		INSERT INTO test_results (commentary) SELECT md5(random()::text) from generate_series(1, 1000000) s(i);
		DELETE FROM test_results;
	END LOOP;
END
$do$', qq[, 'max_run_time', '120 seconds'] );


$dbh->disconnect();
done_testing();

sub one_task_do
{
	my $name = shift;
	my $sql_part = shift;
	my $add_to_task = shift;

	$add_to_task ||= '';

	$query = "SELECT schedule.create_job(
		jsonb_build_object(
			'name', '$name' ,
			'cron', '* * * * *',
			'commands', jsonb_build_array( 
				'insert into task_info values ( pg_backend_pid(), ''$name'')',
				'$sql_part',
				'update task_info set finished = true where pid = pg_backend_pid()'
			),
			'max_instances', 1$add_to_task
    	)
	)";
	my $sth = $dbh->prepare($query);
	ok($sth->execute(), "create $name task") or BAIL_OUT($DBI::errstr);

	my $job_id = $sth->fetchrow_array();
	$sth->finish();

	my $pid = wait_for_task_to_begin($dbh, $name, 120);

	ok($pid > 0, 'find '.$name.' task started') or BAIL_OUT("failed to await task '$name' for 120s");

	$query = "SELECT pg_terminate_backend(?)";
	$sth = $dbh->prepare($query);
	$sth->bind_param(1, $pid);
	ok($sth->execute(), "terminate $name job") or BAIL_OUT($DBI::errstr);
	$sth->finish();

	$sth = $dbh->prepare('UPDATE task_info SET vanished = now() where pid = ?');
	ok($sth->execute($pid), "set $name task vanished") or BAIL_OUT(print $DBI::errstr);
	$sth->finish;

	ok(find_job_exited($dbh, $job_id), "find $name  exit job") or BAIL_OUT("Cannot find job $job_id exited");

	$sth = $dbh->prepare('SELECT schedule.deactivate_job(?)');
	ok($sth->execute($job_id), "deactivate $name job") or BAIL_OUT("Cannot deactivate $name job");

}

sub wait_for_task_to_begin
{
	my $db  = shift;
	my $name = shift;
	my $how_long = shift;

	my $iter = $how_long;
	my $sth1 = $db->prepare('SELECT pid from task_info where name = ? and vanished is  null  and finished = false limit 1');

	while($iter-- > 0)
	{
		if($sth1->execute($name))
		{
			my $pid = $sth1->fetchrow_array() and $sth1->finish();
			return $pid if $pid;
		}
		else
		{
			die $DBI::errstr;
		}
		sleep(1);
	}
	return 0;
}

sub  find_job_exited
{
	my $d = shift;
	my $id = shift;

	my $n = 20;

	while($n-- > 0)
	{
		my $s = $d->prepare('SELECT message from schedule.log where cron = ? and status = false');
		if($s->execute($id))
		{
			my @data = $s->fetchrow_array() and $s->finish();
			if(scalar(@data))
			{
				if($data[0] =~ /^Executor died unexpectedly/)
				{
					return 1;
				}
				else
				{
					return 0;
				}
			}
		}
		else
		{
			die $DBI::errstr;
		}
		sleep(1);
	}

	return 0;
}

