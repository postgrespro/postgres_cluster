create user __temp_robot;
create user __temp_root WITH  SUPERUSER;

SET SESSION AUTHORIZATION __temp_root;

select schedule.create_job(
    '{
        "name": "Test @reboot",
        "cron": "@reboot",
        "command": "show all",
        "run_as": "__temp_robot"
    }'
);

select schedule.create_job(
    '{
        "name": "Test 1",
        "cron": "* * * * *",
        "command": "select ''this is every minute job''",
        "run_as": "__temp_robot",
        "last_start_available": "2 hours"
    }'
);

select schedule.create_job(
    '{
        "name": "Test 2 4/4 2/4 * * *",
        "cron": "4/4 2/4 * * *",
        "command": "select pg_sleep(10)",
        "run_as": "__temp_robot"
    }'
);

select schedule.create_job(
	'{
		"name": "Test 3",
		"cron": "23 1 * * THU,SUN",
		"command": "select ''ok'' as ok"
	}'
);

select id, node, name, rule, do_sql, same_transaction, postpone, retry from schedule.cron order by id;

RESET SESSION AUTHORIZATION;
drop user __temp_root;
drop user __temp_robot;
