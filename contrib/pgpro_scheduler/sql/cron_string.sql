create user __temp_robot;
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

select * from schedule.cron order by id;

drop user __temp_robot;
