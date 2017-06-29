SET search_path TO schedule;

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
	FROM schedule.at_jobs_done where owner = session_user
		UNION 
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, attempt, 
		resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled, start_time, 
		NULL as is_success, NULL as error, NULL as done_time,
		'processing'::job_at_status_t status
	FROM ONLY schedule.at_jobs_process where owner = session_user
		UNION
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, attempt, 
		resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled, 
		NULL as start_time, NULL as is_success, NULL as error,
		NULL as done_time,
		'submitted'::job_at_status_t status
	FROM ONLY schedule.at_jobs_submitted where owner = session_user;

CREATE VIEW all_job_status AS 
	SELECT 
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, owner,
		attempt, resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled,
		start_time, status as is_success, reason as error, done_time,
		'done'::job_at_status_t status
	FROM schedule.at_jobs_done 
		UNION 
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, owner,
		attempt, resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled, start_time,
		NULL as is_success, NULL as error, NULL as done_time,
		'processing'::job_at_status_t status
	FROM ONLY schedule.at_jobs_process 
		UNION
	SELECT
		id, node, name, comments, at as run_after,
		do_sql as query, params, depends_on, executor as run_as, owner,
		attempt, resubmit_limit, postpone as max_wait_interval,
		max_run_time as max_duration, submit_time, canceled,
		NULL as start_time, NULL as is_success, NULL as error,
		NULL as done_time,
		'submitted'::job_at_status_t status
	FROM ONLY schedule.at_jobs_submitted;

GRANT SELECT ON schedule.job_status TO public;

RESET search_path;
