-- DROP existent 

DROP EXTENSION IF EXISTS pgpro_scheduler;
DROP TABLE IF EXISTS result1;
DROP TABLE IF EXISTS result2;
DROP TABLE IF EXISTS result3;
DROP TABLE IF EXISTS result5;
DROP TABLE IF EXISTS result6;
DROP TABLE IF EXISTS result7;

-- CREATE SCHEMA

CREATE EXTENSION pgpro_scheduler;

CREATE TABLE result1 (
	time_mark	timestamp,
	text		text
);

CREATE TABLE result2 (
	time_mark	timestamp,
	n			int,
	text		text
);

CREATE TABLE result3 (
	time_mark	timestamp,
	n			int,
	text		text
);

CREATE TABLE result5 (
	time_mark	timestamp,
	text		text
);

CREATE TABLE result6 (
	time_mark	timestamp,
	text		text
);

CREATE TABLE result7 (
	time_mark	timestamp,
	n			int,
	text		text,
	status		text
);

GRANT SELECT ON result1 TO robot;
GRANT SELECT ON result2 TO robot;
GRANT SELECT ON result3 TO robot;
GRANT SELECT ON result5 TO robot;
GRANT SELECT ON result6 TO robot;
GRANT SELECT ON result7 TO robot;

GRANT INSERT ON result1 TO robot;
GRANT INSERT ON result2 TO robot;
GRANT INSERT ON result3 TO robot;
GRANT INSERT ON result5 TO robot;
GRANT INSERT ON result6 TO robot;
GRANT INSERT ON result7 TO robot;

-- CREATE HELPER FUNCTIONS

CREATE OR REPLACE FUNCTION random_finish() RETURNS TEXT
AS $BODY$
DECLARE
	result INTEGER;
BEGIN
	EXECUTE 'select (random() * 100)::int % 2' INTO result;
	IF result = 1 THEN 
		RETURN 'done returned from function';
	ELSE
		RAISE  EXCEPTION 'random failure of transaction';
	END IF;

	RETURN 'miracle happend';
END
$BODY$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_next_time() RETURNS TIMESTAMP WITH TIME ZONE
AS $BODY$
DECLARE
	result INTEGER;
DECLARE
	state text;
BEGIN
	EXECUTE 'show schedule.transaction_state' INTO state;	
	INSERT INTO result5 (time_mark, text) values (now(), 'transaction result: ' || state);
	RETURN now() + '00:05:15'::interval;
END
$BODY$ LANGUAGE plpgsql;


-- CREATE JOBS

SELECT schedule.create_job(
	'{
		"name": "Test #1: every minute",
		"cron": "* * * * *",
		"command": "insert into result1 (time_mark, text) values (now(), ''result of job'')",
		"run_as": "robot"
	}'
);

SELECT schedule.create_job(
	'{
		"name": "Test #2: every 15 minute, not sngl transaction",
		"cron": "*/15 * * * *",
		"cron": "* * * * *",
		"commands": [
			"insert into result2 (time_mark, n, text) values (now(), 1, ''start job'')",
			"insert into result2 (time_mark, n, text) values (now(), 2, random_finish())"
		],
		"run_as": "robot",
		"use_same_transaction": "false"
	}'
);

SELECT schedule.create_job(
	'{
		"name": "Test #3: 2/3 minute, sngl transaction",
		"cron": "*/15 * * * *",
		"cron": "* * * * *",
		"commands": [
			"insert into result3 (time_mark, n, text) values (now(), 1, ''start job'')",
			"insert into result3 (time_mark, n, text) values (now(), 2, random_finish())"
		],
		"run_as": "robot",
		"use_same_transaction": "true"
	}'
);

SELECT schedule.create_job(
	'{
		"name": "Test #4: sleep 160 timeout 120",
		"cron": "* * * * *",
		"command": "select pg_sleep(160)",
		"run_as": "robot",
		"use_same_transaction": "true",
		"max_run_time": "120 seconds"
	}'
);

SELECT schedule.create_job(
	'{
		"name": "Test #5: writes nexttime  in 05:15 min",
		"cron": "* * * * *",
		"command": "INSERT into result5 (time_mark, text) values (now(), random_finish())",
		"next_time_statement": "select get_next_time()",
		"run_as": "robot"
	}'
);

SELECT schedule.create_job(
	jsonb_build_object(
		'name', 'Test #6: timearray',
		'command', 'insert into result6 (time_mark, text) values (now(), ''result date job'')',
		'dates',
		jsonb_build_array(
			now() + '5 minutes'::interval,
			now() + '15 minutes'::interval,
			now() + '25 minutes'::interval,
			now() + '35 minutes'::interval,
			now() + '45 minutes'::interval,
			now() + '55 minutes'::interval,
			now() + '65 minutes'::interval,
			now() + '75 minutes'::interval,
			now() + '85 minutes'::interval
		),
		'run_as', 'robot'
	)
);

SELECT schedule.create_job(
	'{
		"name": "Test #7: on rollback",
		"cron": "*/10 * * * *",
		"commands": [
			"insert into result7 (time_mark, n, text) values (now(),1, ''start'')",
			"insert into result7 (time_mark, n, text) values (now(), 2, random_finish())"
		],
		"onrollback": "insert into result7 (time_mark,n, text) values (now(),3,  ''on rollback'')",
		"run_as": "robot"
	}'
);
