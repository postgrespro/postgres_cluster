DROP VIEW job_status;
DROP VIEW all_job_status;

ALTER TABLE at_jobs_submitted ALTER id TYPE bigint;
ALTER TABLE at_jobs_process ALTER id TYPE bigint;
ALTER TABLE at_jobs_done ALTER id TYPE bigint;

CREATE VIEW job_status AS 
	SELECT 
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, attempt, 
		resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled,
		start_time, status as is_success, reason as error, done_time,
		'done'::job_at_status_t status
	FROM @extschema@.at_jobs_done where owner = session_user
		UNION 
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, attempt, 
		resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled, start_time, 
		NULL as is_success, NULL as error, NULL as done_time,
		'processing'::job_at_status_t status
	FROM ONLY @extschema@.at_jobs_process where owner = session_user
		UNION
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, attempt, 
		resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled, 
		NULL as start_time, NULL as is_success, NULL as error,
		NULL as done_time,
		'submitted'::job_at_status_t status
	FROM ONLY @extschema@.at_jobs_submitted where owner = session_user;

CREATE VIEW all_job_status AS 
	SELECT 
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, owner,
		attempt, resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled,
		start_time, status as is_success, reason as error, done_time,
		'done'::job_at_status_t status
	FROM @extschema@.at_jobs_done 
		UNION 
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, owner,
		attempt, resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled, start_time,
		NULL as is_success, NULL as error, NULL as done_time,
		'processing'::job_at_status_t status
	FROM ONLY @extschema@.at_jobs_process 
		UNION
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, owner,
		attempt, resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled,
		NULL as start_time, NULL as is_success, NULL as error,
		NULL as done_time,
		'submitted'::job_at_status_t status
	FROM ONLY @extschema@.at_jobs_submitted;

GRANT SELECT ON @extschema@.job_status TO public;

DROP FUNCTION get_log();
DROP FUNCTION get_log(text);
DROP FUNCTION get_user_log();


CREATE INDEX ON cron (owner);
CREATE INDEX ON cron (executor);

--
-- show all scheduled jobs 
--
CREATE VIEW all_jobs_log AS 
	SELECT 
		coalesce(c.id, l.cron) as cron,
		c.node as node,
		l.start_at as scheduled_at,
		coalesce(c.name, '--DELETED--') as name,
		c.comments as comments,
		c.do_sql as commands,
		c.executor as run_as,
		c.owner as owner,
		c.same_transaction as use_same_transaction,
		l.started as started,
		l.last_start_available as last_start_available,
		l.finished as finished,
		c.max_run_time as max_run_time,
		c.onrollback_statement as onrollback,
		c.next_time_statement as next_time_statement,
		c.max_instances as max_instances,
		CASE WHEN l.status THEN
			'done'::@extschema@.job_status_t
		ELSE
			'error'::@extschema@.job_status_t
		END as status,
		l.message as message

	FROM @extschema@.log as l LEFT OUTER JOIN @extschema@.cron as c ON c.id = l.cron;

--
-- show scheduled jobs of session user
--

CREATE VIEW jobs_log AS 
	SELECT
		coalesce(c.id, l.cron) as cron,
		c.node as node,
		l.start_at as scheduled_at,
		coalesce(c.name, '--DELETED--') as name,
		c.comments as comments,
		c.do_sql as commands,
		c.executor as run_as,
		c.owner as owner,
		c.same_transaction as use_same_transaction,
		l.started as started,
		l.last_start_available as last_start_available,
		l.finished as finished,
		c.max_run_time as max_run_time,
		c.onrollback_statement as onrollback,
		c.next_time_statement as next_time_statement,
		c.max_instances as max_instances,
		CASE WHEN l.status THEN
			'done'::@extschema@.job_status_t
		ELSE
			'error'::@extschema@.job_status_t
		END as status,
		l.message as message
	FROM log as l, cron as c WHERE c.executor = session_user AND c.id = l.cron;


CREATE FUNCTION get_log(usename text) RETURNS 
	table(
		cron int,
		node text,
		scheduled_at timestamp with time zone,
		name text,
		comments text,
		commands text[],
		run_as text,
		owner text,
		use_same_transaction boolean,
		started timestamp with time zone,
		last_start_available timestamp with time zone,
		finished timestamp with time zone,
		max_run_time interval,
		onrollback text,
		next_time_statement text,
		max_instances integer,
		status @extschema@.job_status_t,
		message text
	)
AS
$BODY$
 	SELECT * FROM @extschema@.all_jobs_log where owner = usename;
$BODY$
LANGUAGE sql STABLE; 



CREATE FUNCTION get_log() RETURNS 
	table(
		cron int,
		node text,
		scheduled_at timestamp with time zone,
		name text,
		comments text,
		commands text[],
		run_as text,
		owner text,
		use_same_transaction boolean,
		started timestamp with time zone,
		last_start_available timestamp with time zone,
		finished timestamp with time zone,
		max_run_time interval,
		onrollback text,
		next_time_statement text,
		max_instances integer,
		status @extschema@.job_status_t,
		message text
	)
AS
$BODY$
 	SELECT * FROM @extschema@.all_jobs_log;
$BODY$
LANGUAGE sql STABLE; 


CREATE FUNCTION get_user_log() RETURNS 
	table(
		cron int,
		node text,
		scheduled_at timestamp with time zone,
		name text,
		comments text,
		commands text[],
		run_as text,
		owner text,
		use_same_transaction boolean,
		started timestamp with time zone,
		last_start_available timestamp with time zone,
		finished timestamp with time zone,
		max_run_time interval,
		onrollback text,
		next_time_statement text,
		max_instances integer,
		status @extschema@.job_status_t,
		message text
	)
AS
$BODY$
 	SELECT * FROM @extschema@.jobs_log;
$BODY$
LANGUAGE sql STABLE; 

GRANT SELECT ON @extschema@.jobs_log TO public;

ALTER TABLE @extschema@.at ADD PRIMARY KEY (start_at, cron);
ALTER TABLE @extschema@.log ADD PRIMARY KEY (start_at, cron);

