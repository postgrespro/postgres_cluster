# pgpro_scheduler - PostgreSQL extension for job scheduling

pgpro_scheduler allows to schedule jobs execution and control their activity
in PostgreSQL database.

The job is the set of SQL commands. Schedule table could be described as a
crontab-like string or as a JSON object. It's possible to use combination
of both methods for scheduling settings.

Each job could calculate its next start time. The set of SQL commands 
could be executed in the same transaction or each command could be executed in
individual one. It's possible to set SQL statement to be executed on 
failure of main job transaction.

## Installation

pgpro_scheduler is a regular PostgreSQL extension and requires no prerequisites.

Before build extension from the source make sure that the environment variable 
`PATH` includes path to `pg_config` utility. Also make sure that you have 
developer version of PostgreSQL installed or PostgrteSQL was built from 
source code.

Install extension as follows:

	$ cd pgpro_scheduler
	$ make USE_PGXS=1
	$ sudo make USE_PGXS=1 install
	$ psql <DBNAME> -c "CREATE EXTENSION pgpro_scheduler"

## Configuration

The extension defines a number of PostgreSQL variables (GUC). This variables 
help to handle scheduler configuration.

*	**schedule.enabled** - boolean, if scheduler is enabled in this system.
	Default value: false. 
* 	**schedule.database** - text, list of database names on which scheduler 
	is enabled. Database names should be separated by comma.
	Default value: empty string.
*	**schedule.scheme** - text, the `scheme` name where scheduler store its
	tables and functions. To change this value restart required. Normally
	you should not change this variable but it could be useful if you 
	want run scheduled jobs on hot-standby database. So you can define 
	foreign data wrapper on master system to wrap default scheduler schema
	to another and use it on replica. Default value: schedule.
*	**schedule.nodename** - text, node name of this instance. 
	Default value is `master`. You should not change or use it if you run
	single server configuration. But it is necessary to change this name 
	if you run scheduler on hot-standby database.
*	**schedule.max_workers** - integer, max number of simultaneously running
	jobs for one database. Default value: 2.
*	**schedule.transaction_state** - text, this is internal variable.
	This variable contains state of executed job. This variable was designed 
	to use with a next job start time calculation procedure.
	Possible values are:
	*	**success** - transaction has finished successfully 
	*	**failure** - transaction has failed to finish
	*	**running** - transaction is in progress
	*	**undefined** - transaction has not started yet

	The last two values normally should not appear inside the user procedure. If
	you got them probably it indicates an internal scheduler error.

## Management

You could manage scheduler work by means of PostgreSQL variables described
above.

For example, you have a fresh PostgreSQL installation with scheduler extension
installed. You are going to use scheduler with databases called 'database1' and 
'database2'. You want 'database1' be capable to run 5 jobs in parallel and
'database2' - 3.

Put the following string to your `postgresql.conf`:

	shared_preload_libraries = 'pgpro_scheduler'

Then start `psql` and execute the following commands:

	# ALTER SYSTEM SET schedule.enabled = true;
	# ALTER SYSTEM SET schedule.database = 'database1,database2';
	# ALTER DATABASE database1 SET schedule.max_workers = 5;
	# ALTER DATABASE database2 SET schedule.max_workers = 3;
	# SELECT pg_reload_conf();

If you do not need the different values in `max_workers` you could store 
the same in configuration file. Then ask server to reread configuration. There 
is no need to restart.

Here is an example of `postgresql.conf`:

	shared_preload_libraries = 'pgpro_scheduler'
	schedule.enabled = on
	schedule.database = 'database1,database2'
	schedule.max_workers = 5

The scheduler is designed as background worker which dynamically starts 
another bgworkers. That's why you should care about proper value in
`max_worker_processes` variable. The minimal acceptable value 
could be calculated using the following formula:

> **N<sub>min</sub> = 1 + N<sub>databases</sub> + MAX_WORKERS<sub>1</sub> + ... + MAX_WORKERS<sub>n</sub>**

where:

*	**N<sub>min</sub>** - the minimal acceptable amount of bgworkers in the
	system. Consider the fact that other systems need to start background 
	workers too. E.g. parallel queries. So you need to adjust the value to
	their  needs either.
*	**N<sub>databases</sub>** - the number of databases scheduler works with
*	**MAX_WORKERS<sub>n</sub>** - the value of `schedule.max_workers`
	variable in context of each database

## SQL Scheme

The extension uses SQL scheme `schedule` to store its internal tables and
functions. Direct access to tables is forbidden. All manipulations should
be performed by means of functions defined by extension.

## SQL Types 

The scheduler defines 2 SQL types and use them as types for return values 
for some of its functions.

### cron_rec

This type describes information about the job to be scheduled.

	CREATE TYPE schedule.cron_rec AS(
		id integer,             -- job id
		node text,              -- node name to be executed on
		name text,              -- job name 
		comments text,          -- job's comment
		rule jsonb,             -- scheduling rules
		commands text[],        -- sql commands to be executed
		run_as text,            -- name of executor user
		owner text,             -- name of owner user
		start_date timestamp,   -- lower bound of execution window
								-- NULL if unbound
		end_date timestamp,     -- upper bound of execution window
								-- NULL if unbound
		use_same_transaction boolean,   -- if true the set of sql commands 
										-- will be executed in same transaction
		last_start_available interval,  -- max time till scheduled job 
										-- can wait execution if all allowed 
										-- workers are busy
		max_instances int,		-- max number of simultaneous running instances
								-- of this job
		max_run_time interval,  -- max execution time
		onrollback text,        -- SQL command to be performed on transaction
								-- failure
		next_time_statement text,   -- SQL command to execute on main 
									-- transaction end to calculate next 
									-- start time
		active boolean,         -- true - job could be scheduled
		broken boolean          -- true - job has errors in configutration
								-- that prevent it's further execution
	);

### cron_job

Type describes information about job scheduled execution

	CREATE TYPE schedule.cron_job AS(
		cron integer,           -- job id
		node text,              -- node name to be executed on
		scheduled_at timestamp, -- scheduled execution time
		name text,              -- job name
		comments text,          -- job comments
		commands text[],        -- sql commands to be executed
		run_as text,            -- name of executor user
		owner text,             -- name of owner user
		use_same_transaction boolean,	-- if true the set of sql commands
										-- will be executed in same transaction
		started timestamp,      -- timestamp of this job execution started
		last_start_available timestamp,	-- time untill job must be started
		finished timestamp,     -- timestamp of this job execution finished
		max_run_time interval,  -- max execution time
		max_instances int,		-- the number of instances run at the same time
		onrollback text,        -- statement on ROLLBACK
		next_time_statement text,	-- statement to calculate next start time
		status text,			-- status of this task: working, done, error 
		message text			-- error message
	);

## Functions

### schedule.create_job(cron text, sql text, node text)

Creates job and sets it active.

Arguments:

* **cron** - crontab-like string to set schedule 
* **sql** - SQL statement to execute
* **node** - node name, optional

Returns id of created job.

### schedule.create_job(cron text, sqls text[], node text)

Creates job and sets it active.

Arguments:

* **cron** - crontab-like string to set schedule 
* **sqls** - set of SQL statements to be executed
* **node** - node name, optional

Returns id of created job.

### schedule.create_job(date timestamp with time zone, sql text, node text)

Creates job and sets it active.

Arguments:

* **date** - exact date of execution
* **sql** - SQL statement to execute
* **node** - node name, optional

Returns id of created job.

### schedule.create_job(date timestamp with time zone, sqls text[], node text)

Creates job and sets it active.

Arguments:

* **date** - exec date of execution
* **sqls** - set of SQL statements to be executed
* **node** - node name, optional

Returns id of created job.

### schedule.create_job(dates timestamp with time zone[], sql text, node text)

Creates job and sets it active.

Arguments:

* **dates** - set of execution dates
* **sql** - SQL statement to execute
* **node** - node name, optional

Returns id of created job.

### schedule.create_job(dates timestamp with time zone[], sqls text[], node text)

Creates job and sets it active.

Arguments:

* **dates** - set of execution dates
* **sqls** - set of SQL statements to be executed
* **node** - node name, optional

Returns id of created job.

### schedule.create_job(data jsonb) 

Creates job and sets it active.

The only argument is a JSONB object with information about job. 

The object could contains the following keys, some of them could be omitted:

*	**name** - job name;
*	**node** - node name;
*	**comments** - job comments;
*	**cron** - cron-like string for scheduling settings;
*	**rule** - scheduling settings as JSONB object (see description later);
*	**command** - SQL statement to be executed;
*	**commands** - a set of SQL statements to be executed as an array;
*	**run\_as** - user to execute command(s);
*	**start\_date** -  begin of period while command can be executed,
	could be NULL;
*	**end\_date** - end of period while command can be executed,
	could be NULL;
*	**date** - exact date when command will be executed;
*	**dates** - set of exact dates when command will be executed;
*	**use\_same\_transaction** - if set of commands will be executed within
	the same transaction. Default: false;
*	**last\_start\_available** - for how long command execution could be
	postponed if maximum number of allowed workers reached at the scheduled 
	moment. Time set in format of `interval`. E.g. '00:02:34' - it is 
	possible to wait for 2 minutes 34 seconds. If time is NULL or not set 
	waits forever. Default: NULL;
*	**max\_run\_time** - for how long scheduled job can be executed. 
	Format: `interval`. If NULL or not set - there is no time limits.
	Default: NULL;
*	**onrollback** - SQL statement to be executed on ROLLBACK if main 
	transaction failure. Default: not defined;
*	**next\_time\_statement** - SQL statement to calculate next start time.

The rules of scheduling could be set as cron-like string (key `cron`) and 
also as JSONB object (key `rule`).

This object contains the following keys:

* **minutes** - minutes, array of integers in range 0-59
* **hours** -  hours, array of integers in range 0-23
* **days** - days of month, array of integers in range 1-31
* **months** - months, array of integers in range 1-12
* **wdays** - day of week, array of integers in range 0-6 where 0 - is Sunday
* **onstart** - integer with value  0 or 1, if value equals to 1 job will be
	executed on scheduler start only once

Also job could be scheduled on exact date or set of dates. Use keys `date` 
and `dates` keys accordingly.

All scheduling methods could be combined but the use of at least one of them is
mandatory.

Key `next_time_statement` may contain SQL statement to be executed 
after the main transaction to calculate next start time. If key is defined 
the first start time will be calculated by methods described above but 
successive start times will be derived from this statement. The statement 
must return record with the first field containing value of type
`timestamp with time zone`. If returning value be of the different type or
statement execution produce an error the job will be marked as broken and further
execution will be prohibited. 

This statement will be executed in spite of main transaction execution state.
It's possible to get state of main transaction form postgres variable
`schedule.transaction_state`.

The possible values are:

* **success** - transaction is successful
* **failure** - transaction is failed
* **running** - transaction is in progress
* **undefined** - undefined - transaction has not been started yet

The last two values should not appear as a value of the key
`next_time_statement` within user procedure.

SQL statement to be executed could be set in `command` or `commands` key.
The first one sets the single statement, the second - the set of statements.
In fact the first key could contains the set of commands in one string 
divided by semicolon. In this case they all be executed in single transaction
in spite of the value `use_same_transaction`. So for set of the statements 
is better to use key `commands` key as you get more control on execution.

Returns id of created job.

### schedule.set_job_attributes(job_id integer, data jsonb)

Edits properties of existed job

Arguments:

*	**job_id** - job id
*	**data** - JSONB object with properties to be edited. The description
	of keys and their structure could be found in function 
	`schedule.create_job` description.

The function returns boolean value:

* **true** - properties changed successfully
* **false** - properties unchanged

The user can edit properties of jobs it owns unless the user is superuser.

### schedule.set_job_attribute(job_id integer, name text, value text || anyarray)

Edits one property of existed job.

Arguments:

*	**job_id** - job id
*	**name** - property name
*	**value** - property value

The full list of the properties could be found in `schedule.create_job` 
function description. Some values are of array types. Their **value** could 
be passed as an array, but if the value could not be an array the exception
will be raised.

The function returns boolean value, true on success and false on failure.

The user can edit properties of jobs it owns unless the user is superuser.

### schedule.deactivate_job(job_id integer)

Deactivates job and suspends its further scheduling and execution.

Arguments:

*	**job_id** - job id

Returns true on success, false on failure.

### schedule.activate_job(integer) 

Activates job and starts its scheduling and execution.

Arguments:

*	**job_id** - job id

Returns true on success, false on failure.


### schedule.drop_job(jobId integer)

Deletes job.

Arguments:

*	**job_id** - job id

Returns true on success, false on failure.

### schedule.get_job(job_id integer)

Retrieves information about the job.

Arguments:

*	**job_id** - job id

The return value is of type `cron_rec`. Description of type could be found 
in  **SQL types** section.

### schedule.get_user_owned_cron(username text)

Retrieves job list owned by user.

Arguments:

*	**username** - user name, optional

Returns the set of records of type `cron_rec`. Records contain information
about jobs owned by user. If user name is omitted the session user name is 
used.

Retrieve jobs owned by another user is allowed only to superuser.

`cron_rec` type description can be found in  **SQL type** section.

### schedule.get_user_cron(username text)

Retrieves job list executed as user.

Arguments:

*	**username** - user name, optional

Returns the set of records of type `cron_rec`. Records contain information
about jobs executed as user. If user name is not specified session user is 
used.

Retrieve jobs executed as another user is allowed only to superuser.

Type `cron_rec` description can be found in  **SQL type** section.

### schedule.get_user_active_jobs(username text)

Returns jobs list executed in this very moment as user passed in arguments.

Arguments:

*	**username** - user name, optional

If user name is omitted the session user name used. To list jobs executed 
as another user allowed only to superuser.

Return value is set of records of type `cron_job`.
See the type description in **SQL types** section.

### schedule.get_active_jobs()

Returns list of jobs being executed at that very moment.
The function call allowed only to superuser.

Return value is set of records of type `cron_job`.
See the type description in **SQL types** section.

### schedule.get_log()

Returns list of all completed jobs. 
The function call allowed only to superuser.

Return value is set of records of type `cron_job`.
See the type description in **SQL types** section.

### schedule.get_user_log(username text) 

Returns list of completed jobs executed as user passed in arguments.

Arguments:

*	**username** - user name, optional

If username is omitted the session user name used.
Jobs executed as another users are accessible only to superuser.

Return value is set of records of type `cron_job`.
See the type description in **SQL types** section.

### schedule.clean_log()

Delete all records with information about completed jobs.
Can be called by superuser only.

Returns the number of records deleted.

