# pgpro_scheduler internals

Extention creates 3 tables in schema `schedule`. They are not accessable
by public.

**schedule.cron** - table contains records to be scheduled to porces job.
Analog for crontab.

	CREATE TABLE schedule.cron(
		id SERIAL PRIMARY KEY,
		name text,			-- name of job
		node text,			-- name of node
		comments text,		-- comments on job
		rule jsonb,			-- json object with shedule, see description below
		next_time_statement text,	-- sql statement to be executed to 
									-- calculate next execution time
		do_sql text[],		-- SQL statements to be executed
		same_transaction boolean DEFAULT false,	-- if sequence in do_sql will be
												-- executed in one transaction
		onrollback_statement text,	-- sql statement to be executed after ROLLBACK
		active boolean DEFAULT true,	-- is job active
		broken boolean DEFAULT false,	-- is job broken 
		executor text,		-- name of executor user
		owner text,			-- neme of user who owns (created) job
		postpone interval,	-- on what time execution could be delayed if there
							-- are no free session to execute it in time
		retry integer default 0,	-- number of retrys if error
		max_run_time interval,	-- how long job can be processed
		max_instances integer default 1,	-- how much instances of the same
											-- job could be executed simultaneously
		start_date timestamp,	-- begin of time period within job can
								-- be performed, can be NULL
		end_date timestamp,		-- end of time period within job can
								-- be performed, can be NULL
		reason text				-- text reason why job marked as broken
	);

**schedule.at** - table  stores nearest jobs to be executed or
being executed  at the moment. Each record contains information about 
time the job must begin, reference to cron table, time of last start allowed 
(if specified), time of actual start (if being performed), state - waiting 
execution or executing.

	CREATE TABLE schedule.at(
		start_at timestamp,		-- time job will start
		last_start_available timestamp,	-- time last start allowed
		retry integer,			
		cron integer REFERENCES schedule.cron (id), -- cron table reference
		node text,
		started timestamp,		-- time of actual start
		active boolean			-- true - execution,  false - waiting
	);

**scedule.log** - table with job executed. When job has been performed 
it moved from **schedule.at** to this table, so tables has about the same
structure except this table has information about result of execution. 

	CREATE TABLE schedule.log(
		start_at timestamp,		-- time at job were to be started
		last_start_available timestamp,	-- time of last start available
		retry integer,
		cron integer,			-- reference to cron table
		node text,				-- reference to cron table node
		started timestamp,		-- time job has been started
		finished timestamp,		-- time job has been finished
		status boolean,			-- true - success, false - failure
		message text			-- error message
	);

