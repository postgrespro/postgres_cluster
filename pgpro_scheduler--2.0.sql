\echo Use "CREATE EXTENSION pgpro_scheduler" to load this file. \quit

CREATE SCHEMA IF NOT EXISTS schedule;
SET search_path TO schedule;

CREATE TYPE job_status AS ENUM ('working', 'done', 'error');

CREATE TABLE at_jobs_submitted(
   id SERIAL PRIMARY KEY,
   node text,
   name text,
   comments text,
   at timestamp with time zone,
   do_sql text,
   params text[],
   depends_on bigint[],
   executor text,
   owner text,
   last_start_available timestamp with time zone,
   attempt bigint default 0,
   resubmit_limit bigint default 100,
   postpone interval,
   max_run_time	interval,
   submit_time timestamp with time zone default now()
);
CREATE INDEX at_jobs_submitted_node_at_idx on at_jobs_submitted (node,  at);

CREATE TABLE at_jobs_process(
	start_time timestamp with time zone default now()
) INHERITS (at_jobs_submitted);

CREATE INDEX at_jobs_process_node_at_idx on at_jobs_process (node,  at);

CREATE TABLE at_jobs_done(
	status boolean,
	reason text,
	done_time timestamp with time zone default now()
) INHERITS (at_jobs_process);

CREATE INDEX at_jobs_done_node_at_idx on at_jobs_done (node,  at);

CREATE TABLE cron(
   id SERIAL PRIMARY KEY,
   node text,
   name text,
   comments text,
   rule jsonb,
   next_time_statement text,
   do_sql text[],
   same_transaction boolean DEFAULT false,
   onrollback_statement text,
   active boolean DEFAULT true,
   broken boolean DEFAULT false,
   executor text,
   owner text,
   postpone interval,
   retry integer default 0,
   max_run_time	interval,
   max_instances integer default 1,
   start_date timestamp with time zone,
   end_date timestamp with time zone,
   reason text,
   _next_exec_time timestamp with time zone
);

CREATE TABLE at(
   start_at timestamp with time zone,
   last_start_available timestamp with time zone,
   retry integer,
   cron integer REFERENCES cron (id),
   node text,
   started timestamp with time zone,
   active boolean
);
CREATE INDEX at_cron_start_at_idx on at (cron, start_at);

CREATE TABLE log(
   start_at timestamp with time zone,
   last_start_available timestamp with time zone,
   retry integer,
   cron integer,
   node text,
   started timestamp with time zone,
   finished timestamp with time zone,
   status boolean,
   message text
);
CREATE INDEX log_cron_idx on log (cron);
CREATE INDEX log_cron_start_at_idx on log (cron, node, start_at);

---------------
--   TYPES   --
---------------

CREATE TYPE cron_rec AS(
	id integer,				-- job record id
	node text,				-- node name
	name text,				-- name of the job
	comments text,			-- comment on job
	rule jsonb,				-- rule of schedule
	commands text[],		-- sql commands to execute
	run_as text,			-- name of the executor user
	owner text,				-- name of the owner user
	start_date timestamp with time zone,	-- left bound of execution time window 
							-- unbound if NULL
	end_date timestamp with time zone,		-- right bound of execution time window
							-- unbound if NULL
	use_same_transaction boolean,	-- if true sequence of command executes 
									-- in a single transaction
	last_start_available interval,	-- time interval while command could 
									-- be executed if it's impossible 
									-- to start it at scheduled time
	max_run_time interval,	-- time interval - max execution time when 
							-- elapsed - sequence of queries will be aborted
	onrollback text,		-- statement to be executed on ROLLBACK
	max_instances int, 		-- the number of instances run at the same time
	next_time_statement text,	-- statement to be executed to calculate 
								-- next execution time
	active boolean,			-- job can be scheduled 
	broken boolean			-- if job is broken
);

CREATE TYPE cron_job AS(
	cron integer,			-- job record id
	node text,				-- node name 
	scheduled_at timestamp with time zone,	-- scheduled job time
	name text,				-- job name
	comments text,			-- job comments
	commands text[],		-- sql commands to execute
	run_as text,			-- name of the executor user
	owner text,				-- name of the owner user
	use_same_transaction boolean,	-- if true sequence of command executes
									-- in a single transaction
	started timestamp with time zone,		-- time when job started
	last_start_available timestamp with time zone,	-- time untill job must be started
	finished timestamp with time zone,		-- time when job finished
	max_run_time interval,	-- max execution time
	onrollback text,		-- statement on ROLLBACK
	next_time_statement text,	-- statement to calculate next start time
	max_instances int,		-- the number of instances run at the same time
	status job_status,	-- status of job
	message text			-- error message if one
);

---------------
-- FUNCTIONS --
---------------

------------------------------
-- -- AT EXECUTOR FUNCTIONS --
------------------------------

CREATE FUNCTION get_self_id()
  RETURNS bigint 
  AS 'MODULE_PATHNAME', 'get_self_id'
  LANGUAGE C IMMUTABLE;

CREATE FUNCTION resubmit(run_after interval default NULL)
  RETURNS bigint 
  AS 'MODULE_PATHNAME', 'resubmit'
  LANGUAGE C IMMUTABLE;

CREATE FUNCTION submit_job(
	query text,
	params text[] default NULL,
	run_after timestamp with time zone default NULL,
	node text default NULL,
	max_duration interval default NULL,
	max_wait_interval interval default NULL,
	run_as text default NULL,
	depends_on bigint[] default NULL,
	name text default NULL,
	comments text default NULL,
	resubmit_limit bigint default 100
) RETURNS bigint AS
$BODY$
DECLARE
	last_avail timestamp with time zone;
	executor text;
	rec record;
	job_id bigint;
BEGIN
	IF query IS NULL THEN
		RAISE EXCEPTION 'there is no ''query'' parameter';
	END IF;

	IF run_after IS NULL AND depends_on IS NULL THEN
		run_after := now();
	END IF;
	IF run_after IS NOT NULL AND depends_on IS NOT NULL THEN
		RAISE EXCEPTION 'conflict in start time'
			USING HINT = 'you cannot use ''run_after'' and ''depends_on'' parameters at the same time';
	END IF;

	IF max_wait_interval IS NOT NULL AND run_after IS NOT NULL THEN
		last_avail := run_after + max_wait_interval;
	ELSE
		last_avail := NULL;
	END IF;

	IF node IS NULL THEN
		node := 'master';
	END IF;

	IF run_as IS NOT NULL AND run_as <> session_user THEN
		executor := run_as;
		BEGIN
			SELECT * INTO STRICT rec FROM pg_roles WHERE rolname = executor;
			EXCEPTION
				WHEN NO_DATA_FOUND THEN
			RAISE EXCEPTION 'there is no such user %', executor;
			SET SESSION AUTHORIZATION executor; 
			RESET SESSION AUTHORIZATION;
		END;
	ELSE
		executor := session_user;
	END IF;

	INSERT INTO at_jobs_submitted
		(node, at, do_sql, owner, executor, name, comments, max_run_time,
		 postpone, last_start_available, depends_on, params,
		 attempt, resubmit_limit)
	VALUES
		(node, run_after, query, session_user, executor,  name, comments,
		 max_duration, max_wait_interval, last_avail, depends_on, params,
		 0, resubmit_limit)
	RETURNING id INTO job_id;

	RETURN job_id;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

------------------------------------
-- -- SHEDULER EXECUTOR FUNCTIONS --
------------------------------------

CREATE FUNCTION onlySuperUser() RETURNS boolean  AS
$BODY$
DECLARE
	is_superuser boolean;
BEGIN
	EXECUTE 'SELECT rolsuper FROM pg_roles WHERE rolname = session_user'
	INTO is_superuser;
		IF NOT is_superuser THEN
			RAISE EXCEPTION 'access denied';
	END IF;
	RETURN TRUE;
END
$BODY$  LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION on_cron_update() RETURNS TRIGGER
AS $BODY$
DECLARE
  cron_id INTEGER;
BEGIN
  cron_id := NEW.id; 
  IF NOT NEW.active OR NEW.broken OR NEW.rule <> OLD.rule OR NEW.postpone <> OLD.postpone  THEN
     DELETE FROM at WHERE cron = cron_id AND active = false;
  END IF;
  RETURN OLD;
END
$BODY$  LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION on_cron_delete() RETURNS TRIGGER
AS $BODY$
DECLARE
  cron_id INTEGER;
BEGIN
  cron_id := OLD.id; 
  DELETE FROM at WHERE cron = cron_id;
  RETURN OLD;
END
$BODY$  LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION _is_job_editable(jobId integer) RETURNS boolean AS
$BODY$
DECLARE
   is_superuser boolean;
   job record;
BEGIN
   BEGIN
      SELECT * INTO STRICT job FROM cron WHERE id = jobId;
      EXCEPTION
         WHEN NO_DATA_FOUND THEN
	    RAISE EXCEPTION 'there is no such job with id %', jobId;
         WHEN TOO_MANY_ROWS THEN
	    RAISE EXCEPTION 'there are more than one job with id %', jobId;
   END;	
   EXECUTE 'SELECT rolsuper FROM pg_roles WHERE rolname = session_user'
      INTO is_superuser;
   IF is_superuser THEN
      RETURN true;
   END IF;
   IF job.owner = session_user THEN
      RETURN true;
   END IF;

   RETURN false;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION _possible_args() RETURNS jsonb AS
$BODY$
BEGIN 
   RETURN json_build_object(
      'node', 'node name (default: master)',
      'name', 'job name',
      'comments', 'some comments on job',
      'cron', 'cron string rule',
      'rule', 'jsonb job schedule',
      'command', 'sql command to execute',
      'commands', 'sql commands to execute, text[]',
      'run_as', 'user to execute command(s)',
      'start_date', 'begin of period while command could be executed, could be NULL',
      'end_date', 'end of period while command could be executed, could be NULL',
      'date', 'Exact date when command will be executed',
      'dates', 'Set of exact dates when comman will be executed',
      'use_same_transaction', 'if set of commans should be executed within the same transaction',
      'last_start_available', 'for how long could command execution be postponed in  format of interval type' ,
      'max_run_time', 'how long job could be executed, NULL - infinite',
      'max_instances', 'the number of instances run at the same time',
      'onrollback', 'statement to be executed after rollback if one occured',
      'next_time_statement', 'statement to be executed last to calc next execution time'
   );
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;


CREATE FUNCTION _get_excess_keys(params jsonb) RETURNS text[] AS
$BODY$
DECLARE
   excess text[];
   possible jsonb;
   key record;
BEGIN
   possible := _possible_args();

   FOR key IN SELECT * FROM  jsonb_object_keys(params) AS name LOOP
      IF NOT possible?key.name THEN
         EXECUTE 'SELECT array_append($1, $2)'
         INTO excess
         USING excess, key.name;
      END IF;
   END LOOP;

   RETURN excess;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION _string_or_null(str text) RETURNS text AS
$BODY$
BEGIN
   IF lower(str) = 'null' OR str = '' THEN
      RETURN 'NULL';
   END IF;
   RETURN quote_literal(str);
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION _get_cron_from_attrs(params jsonb) RETURNS jsonb AS
$BODY$
DECLARE
	dates text[];
	cron jsonb;
	clean_cron jsonb;
	N integer;
	name text;
BEGIN

	IF params?'cron' THEN 
		EXECUTE 'SELECT cron2jsontext($1::cstring)::jsonb' 
			INTO cron
			USING params->>'cron';
	ELSIF params?'rule' THEN
		cron := params->'rule';
	ELSIF NOT params?'date' AND NOT params?'dates' THEN
		RAISE  EXCEPTION 'There is no information about job''s schedule'
			USING HINT = 'Use ''cron'' - cron string, ''rule'' - json to set schedule rules or ''date'' and ''dates'' to set exact date(s)';
	END IF;

	IF cron IS NOT NULL THEN
		IF cron?'date' THEN
			dates := _get_array_from_jsonb(dates, cron->'date');
		END IF;
		IF cron?'dates' THEN
			dates := _get_array_from_jsonb(dates, cron->'dates');
		END IF;
	END IF;

	IF params?'date' THEN
		dates := _get_array_from_jsonb(dates, params->'date');
	END IF;
	IF params?'dates' THEN
		dates := _get_array_from_jsonb(dates, params->'dates');
	END IF;
	N := array_length(dates, 1);
	
	IF N > 0 THEN
		EXECUTE 'SELECT array_agg(lll) FROM (SELECT distinct(date_trunc(''min'', unnest::timestamp with time zone)) as lll FROM unnest($1) ORDER BY date_trunc(''min'', unnest::timestamp with time zone)) as Z'
			INTO dates USING dates;
		cron := COALESCE(cron, '{}'::jsonb) || json_build_object('dates', array_to_json(dates))::jsonb;
	END IF;
	
	clean_cron := '{}'::jsonb;
	FOR name IN SELECT * FROM unnest('{dates, crontab, onstart, days, hours, wdays, months, minutes}'::text[])
	LOOP
		IF cron?name THEN
			clean_cron := jsonb_set(clean_cron, array_append('{}'::text[], name), cron->name);
		END IF;
	END LOOP;
	RETURN clean_cron;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION _get_array_from_jsonb(dst text[], value jsonb) RETURNS text[] AS
$BODY$
DECLARE
	vtype text;
BEGIN
	IF value IS NULL THEN
		RETURN dst;
	END IF;

	EXECUTE 'SELECT jsonb_typeof($1)'
		INTO vtype
		USING value;
	IF vtype = 'string' THEN
		-- EXECUTE 'SELECT array_append($1, jsonb_set(''{"a":""}''::jsonb, ''{a}'', $2)->>''a'')'
		EXECUTE 'SELECT array_append($1, $2->>0)'
			INTO dst
			USING dst, value;
	ELSIF vtype = 'array' THEN
		EXECUTE 'SELECT $1 || array_agg(value)::text[] from jsonb_array_elements_text($2)'
			INTO dst
			USING dst, value;
	ELSE
		RAISE EXCEPTION 'The value could be only ''string'' or ''array'' type';
	END IF;

	RETURN dst;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION _get_commands_from_attrs(params jsonb) RETURNS text[] AS
$BODY$
DECLARE
	commands text[];
	N integer;
BEGIN
	N := 0;
	IF params?'command' THEN
		commands := _get_array_from_jsonb(commands, params->'command');
	END IF;

	IF params?'commands' THEN
		commands := _get_array_from_jsonb(commands, params->'commands');
	END IF;

	N := array_length(commands, 1);
	IF N is NULL or N = 0 THEN
		RAISE EXCEPTION 'There is no information about what job to execute'
			USING HINT = 'Use ''command'' or ''commands'' key to transmit information';
   END IF;

   RETURN commands;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;


CREATE FUNCTION _get_executor_from_attrs(params jsonb) RETURNS text AS
$BODY$
DECLARE
   rec record;
   executor text;
BEGIN
   IF params?'run_as' AND  params->>'run_as' <> session_user THEN
      executor := params->>'run_as';
      BEGIN
         SELECT * INTO STRICT rec FROM pg_roles WHERE rolname = executor;
         EXCEPTION
            WHEN NO_DATA_FOUND THEN
	       RAISE EXCEPTION 'there is no such user %', executor;
         SET SESSION AUTHORIZATION executor; 
         RESET SESSION AUTHORIZATION;
      END;
   ELSE
      executor := session_user;
   END IF;

   RETURN executor;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;
   

CREATE FUNCTION create_job(params jsonb) RETURNS integer AS
$BODY$
DECLARE 
   cron jsonb;
   commands text[];
   orb_statement text;
   start_date timestamp with time zone;
   end_date timestamp with time zone;
   executor text;
   owner text;
   max_run_time interval;
   excess text[];
   job_id integer;
   v_same_transaction boolean;
   v_next_time_statement text;
   v_postpone interval;
   v_onrollback text;
   name text;
   comments text;
   node text;
   mi int;
BEGIN
   EXECUTE 'SELECT _get_excess_keys($1)'
      INTO excess
      USING params;
   IF array_length(excess,1) > 0 THEN
      RAISE WARNING 'You used excess keys in params: %.', array_to_string(excess, ', ');
   END IF;

   cron := _get_cron_from_attrs(params);
   commands := _get_commands_from_attrs(params);
   executor := _get_executor_from_attrs(params);
   node := 'master';
   mi := 1;

   IF params?'start_date' THEN
      start_date := (params->>'start_date')::timestamp with time zone;
   END IF;

   IF params?'end_date' THEN
      end_date := (params->>'end_date')::timestamp with time zone;
   END IF;

   IF params?'name' THEN
      name := params->>'name';
   END IF;

   IF params?'comments' THEN
      name := params->>'comments';
   END IF;

   IF params?'max_run_time' THEN
      max_run_time := (params->>'max_run_time')::interval;
   END IF;

   IF params?'last_start_available' THEN
      v_postpone := (params->>'last_start_available')::interval;
   END IF;

   IF params?'use_same_transaction' THEN
      v_same_transaction := (params->>'use_same_transaction')::boolean;
   ELSE
      v_same_transaction := false;
   END IF;

   IF params?'onrollback' THEN
      v_onrollback := params->>'onrollback';
   END IF;

   IF params?'next_time_statement' THEN
      v_next_time_statement := params->>'next_time_statement';
   END IF;

   IF params?'node' AND params->>'node' IS NOT NULL THEN
      node := params->>'node';
   END IF;

   IF params?'max_instances' AND params->>'max_instances' IS NOT NULL AND (params->>'max_instances')::int > 1 THEN
      mi := (params->>'max_instances')::int;
   END IF;

   INSERT INTO cron
     (node, rule, do_sql, owner, executor,start_date, end_date, name, comments,
      max_run_time, same_transaction, active, onrollback_statement,
	  next_time_statement, postpone, max_instances)
     VALUES
     (node, cron, commands, session_user, executor, start_date, end_date, name,
      comments, max_run_time, v_same_transaction, true,
      v_onrollback, v_next_time_statement, v_postpone, mi)
     RETURNING id INTO job_id;

   RETURN job_id;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION create_job(cron text, command text, node text DEFAULT NULL) RETURNS integer AS
$BODY$
BEGIN
	RETURN create_job(json_build_object('cron', cron, 'command', command, 'node', node)::jsonb);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION create_job(dt timestamp with time zone, command text, node text DEFAULT NULL) RETURNS integer AS
$BODY$
BEGIN
	RETURN create_job(json_build_object('date', dt::text, 'command', command, 'node', node)::jsonb);
END
$BODY$
LANGUAGE plpgsql
	SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION create_job(dts timestamp with time zone[], command text, node text DEFAULT NULL) RETURNS integer AS
$BODY$
BEGIN
	RETURN create_job(json_build_object('dates', array_to_json(dts), 'command', command, 'node', node)::jsonb);
END
$BODY$
LANGUAGE plpgsql
	SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION create_job(cron text, commands text[], node text DEFAULT NULL) RETURNS integer AS
$BODY$
BEGIN
	RETURN create_job(json_build_object('cron', cron, 'commands', array_to_json(commands), 'node', node)::jsonb);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION create_job(dt timestamp with time zone, commands text[], node text DEFAULT NULL) RETURNS integer AS
$BODY$
BEGIN
	RETURN create_job(json_build_object('date', dt::text, 'commands', array_to_json(commands), 'node', node)::jsonb);
END
$BODY$
LANGUAGE plpgsql
	SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION create_job(dts timestamp with time zone[], commands text[], node text DEFAULT NULL) RETURNS integer AS
$BODY$
BEGIN
	RETURN create_job(json_build_object('dates', array_to_json(dts), 'commands', array_to_json(commands), 'node', node)::jsonb);
END
$BODY$
LANGUAGE plpgsql
	SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION set_job_attributes(jobId integer, attrs jsonb) RETURNS boolean AS
$BODY$
DECLARE
   job record;
   cmd text;
   excess text[];
BEGIN
   IF NOT _is_job_editable(jobId) THEN 
      RAISE EXCEPTION 'permission denied';
   END IF;
   EXECUTE 'SELECT _get_excess_keys($1)'
      INTO excess
      USING attrs;
   IF array_length(excess,1) > 0 THEN
      RAISE WARNING 'You used excess keys in params: %.', array_to_string(excess, ', ');
   END IF;

   EXECUTE 'SELECT * FROM cron WHERE id = $1'
      INTO job
      USING jobId;

   cmd := '';

   IF attrs?'cron' OR attrs?'date' OR attrs?'dates' OR attrs?'rule' THEN
      cmd := cmd || 'rule = ' ||
        quote_literal(_get_cron_from_attrs(attrs)) || '::jsonb, ';
   END IF;

   IF attrs?'command' OR attrs?'commands' THEN
      cmd := cmd || 'do_sql = ' ||
        quote_literal(_get_commands_from_attrs(attrs)) || '::text[], ';
   END IF;

   IF attrs?'run_as' THEN
      cmd := cmd || 'executor = ' ||
        quote_literal(_get_executor_from_attrs(attrs)) || ', ';
   END IF;

   IF attrs?'start_date' THEN
      cmd := cmd || 'start_date = ' ||
        _string_or_null(attrs->>'start_date') || '::timestamp with time zone, ';
   END IF;

   IF attrs?'end_date' THEN
      cmd := cmd || 'end_date = ' ||
        _string_or_null(attrs->>'end_date') || '::timestamp with time zone, ';
   END IF;

   IF attrs?'name' THEN
      cmd := cmd || 'name = ' ||
        _string_or_null(attrs->>'name') || ', ';
   END IF;

   IF attrs?'node' THEN
      cmd := cmd || 'node = ' ||
        _string_or_null(attrs->>'node') || ', ';
   END IF;

   IF attrs?'comments' THEN
      cmd := cmd || 'comments = ' ||
        _string_or_null(attrs->>'comments') || ', ';
   END IF;

   IF attrs?'max_run_time' THEN
      cmd := cmd || 'max_run_time = ' ||
        _string_or_null(attrs->>'max_run_time') || '::interval, ';
   END IF;

   IF attrs?'onrollback' THEN
      cmd := cmd || 'onrollback_statement = ' ||
        _string_or_null(attrs->>'onrollback') || ', ';
   END IF;

   IF attrs?'next_time_statement' THEN
      cmd := cmd || 'next_time_statement = ' ||
        _string_or_null(attrs->>'next_time_statement') || ', ';
   END IF;

   IF attrs?'use_same_transaction' THEN
      cmd := cmd || 'same_transaction = ' ||
        quote_literal(attrs->>'use_same_transaction') || '::boolean, ';
   END IF;

   IF attrs?'last_start_available' THEN
      cmd := cmd || 'postpone = ' ||
        _string_or_null(attrs->>'last_start_available') || '::interval, ';
   END IF; 

   IF attrs?'max_instances' AND attrs->>'max_instances' IS NOT NULL AND (attrs->>'max_instances')::int > 0 THEN
      cmd := cmd || 'max_instances = ' || (attrs->>'max_instances')::int || ', ';
   END IF;


   IF length(cmd) > 0 THEN
      cmd := substring(cmd from 0 for length(cmd) - 1);
   ELSE
      RETURN false;
   END IF;

   cmd := 'UPDATE cron SET ' || cmd || ' where id = $1';

   EXECUTE cmd
     USING jobId;

   RETURN true; 
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION set_job_attribute(jobId integer, name text, value anyarray) RETURNS boolean AS
$BODY$
BEGIN
   IF name <> 'dates' AND name <> 'commands' THEN
      RAISE EXCEPTION 'key % cannot have an array value. Only dates, commands allowed', name;
   END IF;

   RETURN set_job_attributes(jobId, json_build_object(name, array_to_json(value))::jsonb);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION set_job_attribute(jobId integer, name text, value text) RETURNS boolean AS
$BODY$
DECLARE
   attrs jsonb;
BEGIN
   IF name = 'dates' OR name = 'commands' THEN
      attrs := json_build_object(name, array_to_json(value::text[]));
   ELSE
      attrs := json_build_object(name, value);
   END IF;
   RETURN set_job_attributes(jobId, attrs);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION drop_job(jobId integer) RETURNS boolean AS
$BODY$
BEGIN
   IF NOT _is_job_editable(jobId) THEN 
      RAISE EXCEPTION 'permission denied';
   END IF;

   DELETE FROM cron WHERE id = jobId;

   RETURN true;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION deactivate_job(jobId integer) RETURNS boolean AS
$BODY$
BEGIN
   IF NOT _is_job_editable(jobId) THEN 
      RAISE EXCEPTION 'permission denied';
   END IF;

   UPDATE cron SET active = false WHERE id = jobId;

   RETURN true;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION activate_job(jobId integer) RETURNS boolean AS
$BODY$
BEGIN
   IF NOT _is_job_editable(jobId) THEN 
      RAISE EXCEPTION 'Permission denied';
   END IF;

   UPDATE cron SET active = true WHERE id = jobId;

   RETURN true;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION _make_cron_job(ii cron) RETURNS cron_job AS
$BODY$
DECLARE
	oo cron_job;
BEGIN
	oo.cron := ii.id;

	RETURN oo;
END
$BODY$
LANGUAGE plpgsql
	SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION _make_cron_rec(ii cron) RETURNS cron_rec AS
$BODY$
DECLARE
	oo cron_rec;
BEGIN
	oo.id := ii.id;
	oo.name := ii.name;
	oo.node := ii.node;
	oo.comments := ii.comments;
	oo.rule := ii.rule;
	oo.commands := ii.do_sql;
	oo.run_as := ii.executor;
	oo.owner := ii.owner;
	oo.start_date := ii.start_date;
	oo.end_date := ii.end_date;
	oo.use_same_transaction := ii.same_transaction;
	oo.last_start_available := ii.postpone;
	oo.max_run_time := ii.max_run_time;
	oo.onrollback := ii.onrollback_statement;
	oo.next_time_statement := ii.next_time_statement;
	oo.max_instances := ii.max_instances;
	oo.active := ii.active;
	oo.broken := ii.broken;

	RETURN oo;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;

CREATE FUNCTION clean_log() RETURNS INT  AS
$BODY$
DECLARE
	cnt integer;
BEGIN
    PERFORM onlySuperUser();

	WITH a AS (DELETE FROM log RETURNING 1)
		SELECT count(*) INTO cnt FROM a;

	RETURN cnt;
END
$BODY$
LANGUAGE plpgsql set search_path FROM CURRENT;

create FUNCTION get_job(jobId int) RETURNS cron_rec AS
$BODY$
DECLARE
	job cron;
BEGIN
	IF NOT _is_job_editable(jobId) THEN 
		RAISE EXCEPTION 'permission denied';
	END IF;
	EXECUTE 'SELECT * FROM cron WHERE id = $1'
		INTO job
		USING jobId;
	RETURN _make_cron_rec(job);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_cron() RETURNS SETOF cron_rec AS
$BODY$
DECLARE
	ii cron;
	oo cron_rec;
BEGIN
    PERFORM onlySuperUser();

	FOR ii IN SELECT * FROM cron LOOP
		oo := _make_cron_rec(ii);
		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_owned_cron() RETURNS SETOF cron_rec AS
$BODY$
DECLARE
	ii cron;
	oo cron_rec;
BEGIN
	FOR ii IN SELECT * FROM cron WHERE owner = session_user LOOP
		oo := _make_cron_rec(ii);
		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;


CREATE FUNCTION get_owned_cron(usename text) RETURNS SETOF cron_rec AS
$BODY$
DECLARE
	ii cron;
	oo cron_rec;
BEGIN
	IF usename <> session_user THEN
    	PERFORM onlySuperUser();
	END IF;

	FOR ii IN SELECT * FROM cron WHERE owner = usename LOOP
		oo := _make_cron_rec(ii);
		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_user_owned_cron() RETURNS SETOF cron_rec AS
$BODY$
BEGIN
	RETURN QUERY  SELECT * from get_owned_cron();
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_user_owned_cron(usename text) RETURNS SETOF cron_rec AS
$BODY$
BEGIN
	RETURN QUERY SELECT * from  get_owned_cron(usename);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;



CREATE FUNCTION get_user_cron() RETURNS SETOF cron_rec AS
$BODY$
DECLARE
	ii cron;
	oo cron_rec;
BEGIN
	FOR ii IN SELECT * FROM cron WHERE executor = session_user LOOP
		oo := _make_cron_rec(ii);
		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_user_cron(usename text) RETURNS SETOF cron_rec AS
$BODY$
DECLARE
	ii cron;
	oo cron_rec;
BEGIN
	IF usename <> session_user THEN
    	PERFORM onlySuperUser();
	END IF;

	FOR ii IN SELECT * FROM cron WHERE executor = usename LOOP
		oo := _make_cron_rec(ii);
		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_user_active_jobs() RETURNS SETOF cron_job AS
$BODY$
DECLARE
	ii record;
	oo cron_job;
BEGIN
	FOR ii IN SELECT * FROM at as at, cron as cron WHERE cron.executor = session_user AND cron.id = at.cron AND at.active LOOP
		oo.cron = ii.id;
		oo.node = ii.node;
		oo.scheduled_at = ii.start_at;
		oo.name = ii.name;
		oo.comments= ii.comments;
		oo.commands = ii.do_sql;
		oo.run_as = ii.executor;
		oo.owner = ii.owner;
		oo.max_instances = ii.max_instances;
		oo.use_same_transaction = ii.same_transaction;
		oo.started = ii.started;
		oo.last_start_available = ii.last_start_available;
		oo.finished = NULL;
		oo.max_run_time = ii.max_run_time;
		oo.onrollback = ii.onrollback_statement;
		oo.next_time_statement = ii.next_time_statement;
		oo.message = NULL;
		oo.status = 'working';

		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_active_jobs(usename text) RETURNS SETOF cron_job AS
$BODY$
DECLARE
BEGIN
	RETURN QUERY  SELECT * FROM get_user_active_jobs(usename);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_active_jobs() RETURNS SETOF cron_job AS
$BODY$
DECLARE
	ii record;
	oo cron_job;
BEGIN
    PERFORM onlySuperUser();
	FOR ii IN SELECT * FROM at as at, cron as cron WHERE cron.id = at.cron AND at.active LOOP
		oo.cron = ii.id;
		oo.node = ii.node;
		oo.scheduled_at = ii.start_at;
		oo.name = ii.name;
		oo.comments= ii.comments;
		oo.commands = ii.do_sql;
		oo.run_as = ii.executor;
		oo.owner = ii.owner;
		oo.max_instances = ii.max_instances;
		oo.use_same_transaction = ii.same_transaction;
		oo.started = ii.started;
		oo.last_start_available = ii.last_start_available;
		oo.finished = NULL;
		oo.max_run_time = ii.max_run_time;
		oo.onrollback = ii.onrollback_statement;
		oo.next_time_statement = ii.next_time_statement;
		oo.message = NULL;
		oo.status = 'working';

		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_user_active_jobs(usename text) RETURNS SETOF cron_job AS
$BODY$
DECLARE
	ii record;
	oo cron_job;
BEGIN
	IF usename <> session_user THEN
    	PERFORM onlySuperUser();
	END IF;

	FOR ii IN SELECT * FROM at, cron WHERE cron.executor = usename AND cron.id = at.cron AND at.active LOOP
		oo.cron = ii.id;
		oo.node = ii.node;
		oo.scheduled_at = ii.start_at;
		oo.name = ii.name;
		oo.comments= ii.comments;
		oo.commands = ii.do_sql;
		oo.run_as = ii.executor;
		oo.max_instances = ii.max_instances;
		oo.owner = ii.owner;
		oo.use_same_transaction = ii.same_transaction;
		oo.started = ii.started;
		oo.last_start_available = ii.last_start_available;
		oo.finished = NULL;
		oo.max_run_time = ii.max_run_time;
		oo.onrollback = ii.onrollback_statement;
		oo.next_time_statement = ii.next_time_statement;
		oo.message = NULL;
		oo.status = 'working';

		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_log(usename text) RETURNS SETOF cron_job AS
$BODY$
BEGIN
 	RETURN QUERY SELECT * FROM get_user_log(usename);
END
$BODY$
LANGUAGE plpgsql
	SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_log() RETURNS SETOF cron_job AS
$BODY$
BEGIN
 	RETURN QUERY SELECT * FROM get_user_log('___all___');
END
$BODY$
LANGUAGE plpgsql
	SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_user_log() RETURNS SETOF cron_job AS
$BODY$
BEGIN
 	RETURN QUERY SELECT * FROM get_user_log(session_user);
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

CREATE FUNCTION get_user_log(usename text) RETURNS SETOF cron_job AS
$BODY$
DECLARE
	ii record;
	oo cron_job;
	sql_cmd text;
BEGIN
	IF usename <> session_user THEN
    	PERFORM onlySuperUser();
	END IF;

	IF usename = '___all___' THEN
		sql_cmd := 'SELECT * FROM log as l , cron as cron WHERE cron.id = l.cron';
	ELSE
		sql_cmd := 'SELECT * FROM log as l , cron as cron WHERE cron.executor = ''' || usename || ''' AND cron.id = l.cron';
	END IF;

	FOR ii IN EXECUTE sql_cmd LOOP
		IF ii.id IS NOT NULL THEN
			oo.cron = ii.id;
			oo.name = ii.name;
			oo.node = ii.node;
			oo.comments= ii.comments;
			oo.commands = ii.do_sql;
			oo.run_as = ii.executor;
			oo.owner = ii.owner;
			oo.use_same_transaction = ii.same_transaction;
			oo.max_instances = ii.max_instances;
			oo.max_run_time = ii.max_run_time;
			oo.onrollback = ii.onrollback_statement;
			oo.next_time_statement = ii.next_time_statement;
		ELSE
			oo.cron = ii.cron;
			oo.name = '-- DELETED --';
		END IF;
		oo.scheduled_at = ii.start_at;
		oo.started = ii.started;
		oo.last_start_available = ii.last_start_available;
		oo.finished = ii.finished;
		oo.message = ii.message;
		IF ii.status THEN
			oo.status = 'done';
		ELSE
			oo.status = 'error';
		END IF;

		RETURN NEXT oo;
	END LOOP;
	RETURN;
END
$BODY$
LANGUAGE plpgsql
   SECURITY DEFINER set search_path FROM CURRENT;

-- CREATE FUNCTION enable() RETURNS boolean AS 
-- $BODY$
-- DECLARE
-- 	value text;
-- BEGIN
-- 	EXECUTE 'show enabled' INTO value; 
-- 	IF value = 'on' THEN
-- 		RAISE NOTICE 'Scheduler already enabled';
-- 		RETURN false;
-- 	ELSE 
-- 		ALTER SYSTEM SET enabled = true;
-- 		SELECT pg_reload_conf();
-- 	END IF;
-- 	RETURN true;
-- END
-- $BODY$
-- LANGUAGE plpgsql;
-- 
-- CREATE FUNCTION disable() RETURNS boolean AS 
-- $BODY$
-- DECLARE
-- 	value text;
-- BEGIN
-- 	EXECUTE 'show enabled' INTO value; 
-- 	IF value = 'off' THEN
-- 		RAISE NOTICE 'Scheduler already disabled';
-- 		RETURN false;
-- 	ELSE 
-- 		ALTER SYSTEM SET enabled = false;
-- 		SELECT pg_reload_conf();
-- 	END IF;
-- 	RETURN true;
-- END
-- $BODY$
-- LANGUAGE plpgsql;

CREATE FUNCTION cron2jsontext(CSTRING)
  RETURNS text 
  AS 'MODULE_PATHNAME', 'cron_string_to_json_text'
  LANGUAGE C IMMUTABLE;

--------------
-- TRIGGERS --
--------------

CREATE TRIGGER cron_delete_trigger 
BEFORE DELETE ON cron 
   FOR EACH ROW EXECUTE PROCEDURE on_cron_delete();

CREATE TRIGGER cron_update_trigger 
AFTER UPDATE ON cron 
   FOR EACH ROW EXECUTE PROCEDURE on_cron_update();

-----------
-- GRANT --
-----------

GRANT USAGE ON SCHEMA schedule TO public;


RESET search_path;
