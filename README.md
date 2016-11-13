# pgpro_scheduler - расширение PostgreSQL для управления расписанием задач

pgpro_scheduler это планировщик задач для СУБД PostgreSQL, который позволяет
планировать исполнение задач в базе и контроллировать их исполнение.

Задачи это наборы SQL команд. Расписание выполнения задач задается либо строкой
cron, либо указанием конкретных дат запуска, либо JSON объектом, в котором
указывается в какие дни часы и минуты задача должна быть запущена. Возможна
комбинация методов описания расписания.

Каждая задача имеет возможность для вычисления времени следующего своего
запуска. Набор SQL команд в задаче может обрабатываться в разных транзакциях, 
по транзакции на команду, или в одной. В последнем случае имеется возможность 
задания SQL конманды, которая будет выполняться в случае аварийного завершения 
транзакции.

## Installation

pgpro_scheduler это расширение PostgreSQL и не тербует никаких специальных
пререквизитов.

Перед сборкой расширения из исходного кода убедитесь, что переменная
окружения PATH содержит путь к команде `pg_config`. Так же убедитесь, 
что у вас установлена версия PostgresSQL для разработчиков или PostgreSQL
собран из исходного кода.

Процедура установки выглядит следующим образом:

	$ cd pgpro_scheduler
	$ make USE_PGXS=1
	$ sudo make USE_PGXS=1 install
	$ psql <DBNAME> -c "CREATE EXTNESION pgpro_scheduler"

## Конфигурация

Расширение определяет ряд переменных в PostgreSQL (GUC), которые позволяют 
управлять его конфигурацией.

*	**schedule.enable** - двоичная переменная, которая поределяет разрешено ли 
	выполнение расширения. По умолчанию: false. 
* 	**schedule.database** - строковая переменная, указывает с какими базам может
	работать расширение. По умолчанию - пустая строка.
*	**schedule.nodename** - строковая переменная, содержит название узла.
	По умолчанию - master. Если расширение используется на одной машине,
	то переменная не имеет смысла.
*	**schedule.max_workers** - целочисленная переменная, содержит максимальное
	количество одновременно работающих задач для одной базы. По умолчанию - 2.
*	**schedule.transaction_state** - строковая переменная, устанавливается
	расширением в процессе работы. По умолчанию - undefined. Переменная
	используется для определения статуса завершения транзакции при вычислении
	следующего времени выполнения задачи. Возможные значения:
	*	**success** - транзакция завершилась успешно
	*	**failure** - транзакция завершилась аварийно
	*	**running** - транзакция в процессе исполнения
	*	**undefined** - транзакция не началась
	Последние два значения не должны попадать в процедуру определения следующего
	значения. Это будет означать какую-то внутреннюю ошибку в работе расширения.

## Управление

Управление работой планировщика задач осуществляется через переменные 
PostgreSQL, которые описаны в предыдущем разделе.

Например, у вас существует свежая инсталляция PostgreSQL с установленным 
расширением планировщика. И вам требуется запустить планировщик на двух
базах database1 и database2. При этом вы хотите что бы планировщик для 
базы database1 мог исполнять 5 задач одновременно, а для базы database2 - 3.

В $DATADIR/postgresql.conf должна присутствовать строка:

	shared_preload_libraries = 'pgpro_scheduler'

Далее в psql введите следующие команды:

	ALTER SYSTEM SET schedule.enable = true;
	ALTER SYSTEM SET schedule.database = 'database1,database2';
	ALTER DATABASE database1 SET schedule.max_workers = 5;
	ALTER DATABASE database2 SET schedule.max_workers = 3;
	SELECT pg_reload_conf();

Если вам не нужны указания различных значений для разных баз данных, то все это 
можно занести в конфигурационный файл PostgreSQL и перечитать конфигурацию.
Перезапуска не требуется.

Пример записей в $DATADIR/postgresql.conf, если количество одновременно 
исполняемых задач в обоих базах одинаково:

	shared_preload_libraries = 'pgpro_scheduler'
	schedule.enable = on
	schedule.database = 'database1,database2'
	schedule.max_workers = 5

Планировщик задач работает с помощью Background Worker'ов. Поэтому должно быть 
правильно установленно значение переменной `max_worker_processes`. Минимальное 
значение переменной может быть расчитано по следующей формуле:

	N<sub>min</sub> = 1 + N<sub>databases</sub> + MAX_WORKERS<sub>1</sub> + ... + MAX_WORKERS<sub>n</sub>

Где:

*	**N<sub>min</sub>** - это минимальное значение переменной, которое
	требуется для работы конфигурации. Имейте в виду, что Background Workes'ы 
	могут требоваться для работы других систем, например, параллельных запросов.
*	**N<sub>databases</sub>** - это количество баз данных, для которых
	запускается планировщик.
*	**MAX_WORKERS<sub>n</sub>** - это значение переменной schedule.max_workers
	в контексте каждой базы данных, для которой запусткается планировщик.

## SQL Schema

The extention creates a `schedule` schema. All functions, types and tables of extension
are defined within this scheme. Direct access to the tables created is forbidden
to public. All actions should be done by means of sql interface functions.

## SQL Types

Extension defines two SQL types and uses them as types of return values 
in interface functions.

	CREATE TYPE schedule.cron_rec AS(
		id integer,             -- job record id
		node text,              -- name of node 
		name text,              -- name of the job
		comments text,          -- comment on job
		rule jsonb,             -- rule of schedule
		commands text[],        -- sql commands to execute
		run_as text,            -- name of the executor user
		owner text,             -- name of the owner user
		start_date timestamp,   -- left bound of execution time window 
								-- unbound if NULL
		end_date timestamp,     -- right bound of execution time window
								-- unbound if NULL
		use_same_transaction boolean,   -- if true sequence of command executes 
										-- in a single transaction
		last_start_available interval,  -- time interval while command could 
										-- be executed if it's impossible 
										-- to start it at scheduled time
		max_instances int,		-- the number of instances run at the same time
		max_run_time interval,  -- time interval - max execution time when 
								-- elapsed - sequence of queries will be aborted
		onrollback text,        -- statement to be executed on ROLLBACK
		next_time_statement text,   -- statement to be executed to calculate 
									-- next execution time
		active boolean,         -- is job executes at that moment
		broken boolean          -- if job is broken
	);

	CREATE TYPE schedule.cron_job AS(
		cron integer,           -- job record id
		node text,              -- name of node 
		scheduled_at timestamp, -- scheduled job time
		name text,              -- job name
		comments text,          -- job comments
		commands text[],        -- sql commands to execute
		run_as text,            -- name of the executor user
		owner text,             -- name of the owner user
		use_same_transaction boolean,	-- if true sequence of command executes
								-- in a single transaction
		started timestamp,      -- time when job started
		last_start_available timestamp,	-- time untill job must be started
		finished timestamp,     -- time when job finished
		max_run_time interval,  -- max execution time
		max_instances int,		-- the number of instances run at the same time
		onrollback text,        -- statement on ROLLBACK
		next_time_statement text,	-- statement to calculate next start time
		status text,             -- status of job: working, done, error 
		message text             -- error message if one
	);

## SQL Interface Functions

### schedule.create_job(text, text, text)

This function creates a job and sets it active.

Arguments:

* crontab string
* sql to execute
* node name, optional

Returns ID of created job.

### schedule.create_job(text, text[], text)

This function creates a job and sets it active.

Arguments:

* crontab string
* set of sqls to execute
* node name, optional

Returns ID of created job.

### schedule.create_job(timestamp, text, text)

This function creates a job and sets it active.

Arguments:

* execution time 
* sql to execute
* node name, optional

Returns ID of created job.

### schedule.create_job(timestamp, text[], text)

This function creates a job and sets it active.

Arguments:

* execution time
* set of sqls to execute
* node name, optional

Returns ID of created job.

### schedule.create_job(timestamp[], text, text)

This function creates a job and sets it active.

Arguments:

* set of execution times
* sql to execute
* node name, optional

Returns ID of created job.

### schedule.create_job(timestamp[], text[], text)

This function creates a job and sets it active.

Arguments:

* set of execution times
* set of sqls to execute
* node name, optional

Returns ID of created job.

### schedule.create_job(jsonb) 

This function creates a job and sets it active.

As input parameter it accepts jsonb object which describes job to be created.
It returns an integer - ID of created job.

Jsonb object can contains following keys:

* **name** - job name;
* **node** - node name, default 'master';
* **comments** - some comments on job;
* **cron** - cron string rule;
* **rule** - jsonb job schedule;
* **command** - sql command to execute;
* **commands** - sql commands to execute, text[];
* **run\_as** - user to execute command(s);
* **start\_date** - begin of period while command could be executed, could be NULL;
* **end\_date** - end of period while command could be executed, could be NULL;
* **date** - Exact date when command will be executed;
* **dates** - Set of exact dates when comman will be executed;
* **use\_same\_transaction** - if set of commans should be executed within
the same transaction;
* **last\_start\_available** - for how long could command execution be
postponed in  format of interval type;
* **max\_run\_time** - how long task could be executed, NULL - infinite;
* **onrollback** - sql statement to be executed after rollback, if one occured;
* **next\_time\_statement** - sql statement to be executed last to calc next
execution time'.

The schedule of job could be set as crontab format string (key `cron`). Also
it could be set as a jsonb object (key `rule`).

The object may contain following keys:

* **minutes** - an array with values in range 0-59 - minutes
* **hours** -  an array with values in range 0-23 - hours 
* **days** - an array with value in range 1-31 - month days 
* **months** - an array with value in range 1-12 - months 
* **wdays** - an array with value in range 0-6 - week days, 0 - Sunday
* **onstart** - 1 or 0, by default 0, if set to 1 - run only once at start time 

Also schedule can be set as exact time or times to execute on. Use keys 
`date` for single time and `dates` as array for sequence of execution times.

SQL commands to be executed can be set over `command` or `commands` keys. The
second one must be an array.

`next_time_statement` executs after sql commands finished. It must return 
timestamp which will be considered as next execution time. If `pg_variables`
extension is installed this statment can access information about if 
execution of main commands was successful. Information is stored in 
variable `transaction` of package `pgpro_scheduler`.

	select pgv_get_text('pgpro_scheduler', 'transaction');

There are 3 available states:

* **success** - commands executed with no errors
* **failure** - commands commited some errors
* **processing** - commands are being proccessed. Actually if
next_time_statement receive this state that means something goes wrong inside
scheduler  

### schedule.set_job_attributes(integer, jsonb)

The function allows to edit settings of existing job. 

First argument is ID of existing job, second argument is jsonb object 
described in function `schedule.create_job`. Some of keys may be omitted.

Function returns boolean value - true on success and false on failure.

User can edit only jobs it owns unless it's a superuser.

### schedule.set_job_attribute(integer, text, text || anyarray)

The function allows to set exact property of existing job.

First argument is ID of existing job, second is name of the property - one 
of the keys described in function `schedule.create_job`.

Function returns boolean value - true on success and false on failure.

### schedule.deactivate_job(integer)

The function allows to set cron job inactive. The job is not to be deleted 
from cron table but its execution will be disabled.

The first argument is ID of existing job.

### schedule.activate_job(integer) 

The function allows to set cron job active. 

The first argument is ID of existing job.

### schedule.drop_job(jobId integer)

The function deletes cron job. 

The first argument is ID of existing job.

### schedule.get_job(int)

The function returns information about exact cron record.

It returns record of type `cron_rec`. See description above.

### schedule.get_user_owned_cron(text)

The function returns list of the jobs in cron table owned by user 
passed in first argument or by session user if no user passed.

It returns set of records of `cron_rec` type. See description above.

### schedule.get_user_cron(text)

The function returns list of the jobs in cron table which will be executed
as  user passed in first argument or by session user if no user passed.

It returns set of records of `cron_rec` type. See description above.

### schedule.get_user_active_jobs(text)

The function returns all jobs executed at the moment as user passed in first 
argument. If no user specified - session user used.

It returns set of records of `cron_job` type. See description above.

### schedule.get_active_jobs()

The function returns all jobs executed at the moment. Can be executed only
by superuser.

It returns set of records of `cron_job` type. See description above.

### schedule.get_log()

The function returns all jobs which was executed. Can be executed only
by superuser.

It returns set of records of `cron_job` type. See description above.

### schedule.get_user_log(text) 

The function returns all jobs which was executed as user passed in first
argument. If no user specified - session user used.

It returns set of records of `cron_job` type. See description above.

### schedule.clean_log()

The function deletes all records in log table. Can be executed only by 
superuser.

Returns number of records deleted.

## Internals

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

