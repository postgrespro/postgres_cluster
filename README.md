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
	значения. Это будет означать какую-то внутреннюю ошибку в работе
	планировщика.

## Управление

Управление работой планировщика задач осуществляется через переменные 
PostgreSQL, которые описаны в предыдущем разделе.

Например, у вас существует свежая инсталляция PostgreSQL с установленным 
расширением планировщика. И вам требуется запустить планировщик на двух
базах database1 и database2. При этом вы хотите что бы планировщик для 
базы database1 мог исполнять 5 задач одновременно, а для базы database2 - 3.

В `$DATADIR/postgresql.conf` должна присутствовать строка:

	shared_preload_libraries = 'pgpro_scheduler'

Далее в `psql` введите следующие команды:

	# ALTER SYSTEM SET schedule.enable = true;
	# ALTER SYSTEM SET schedule.database = 'database1,database2';
	# ALTER DATABASE database1 SET schedule.max_workers = 5;
	# ALTER DATABASE database2 SET schedule.max_workers = 3;
	# SELECT pg_reload_conf();

Если вам не нужны указания различных значений для разных баз данных, то все это 
можно занести в конфигурационный файл PostgreSQL и перечитать конфигурацию.
Перезапуска не требуется.

Пример записей в `$DATADIR/postgresql.conf`, если количество одновременно 
исполняемых задач в обоих базах одинаково:

	shared_preload_libraries = 'pgpro_scheduler'
	schedule.enable = on
	schedule.database = 'database1,database2'
	schedule.max_workers = 5

Планировщик задач работает с помощью Background Worker'ов. Поэтому должно быть 
правильно установленно значение переменной `max_worker_processes`. Минимальное 
значение переменной может быть расчитано по следующей формуле:

> **N<sub>min</sub> = 1 + N<sub>databases</sub> + MAX_WORKERS<sub>1</sub> + ... + MAX_WORKERS<sub>n</sub>**

Где:

*	**N<sub>min</sub>** - это минимальное значение переменной, которое
	требуется для работы конфигурации. Имейте в виду, что Background Workes'ы 
	могут требоваться для работы других систем, например, параллельных запросов.
*	**N<sub>databases</sub>** - это количество баз данных, для которых
	запускается планировщик.
*	**MAX_WORKERS<sub>n</sub>** - это значение переменной `schedule.max_workers`
	в контексте каждой базы данных, для которой запусткается планировщик.

## SQL Схема

При установке расширения создается SQL схема `schedule`. Все функции для
работы с планировщиком и служебные таблицы создаются в ней.

Прямой доступ к внутренним таблицам запрещен. Все управление осуществляется 
набором SQL функций, о котором будет рассказано далее.

## SQL Типы 

Планировщик определяет 2 SQL типа, которые он использует в качестве типов 
возвращаемых значений для своих функций.

**cron_rec** - используется для информации о записи задачи в таблице расписания.

	CREATE TYPE schedule.cron_rec AS(
		id integer,             -- идентификатор задачи
		node text,              -- имя узла, на котором она будет выполняться
		name text,              -- имя задачи
		comments text,          -- комментарий к задаче
		rule jsonb,             -- правила построения расписания
		commands text[],        -- sql комманды, которые будут выполненны
		run_as text,            -- имя пользователя, с которым будет выполняться
								-- задача
		owner text,             -- имя пользователя, который создал задачу
		start_date timestamp,   -- нижняя граница временного периода, во время
								-- которого допускается выполнение задачи
								-- граница считаеися открытой если значение NULL
		end_date timestamp,     -- верхняя граница временного периода, во время
								-- граница считаеися открытой если значение NULL
		use_same_transaction boolean,   -- если true, то набор команд будет 
										-- выполняться в одной транзакции
		last_start_available interval,  -- максимальное время, на которое может 
										-- быть отложен запуск задачи, если 
										-- нет свободных workers для ее
										-- выполнения во время по расписанию
		max_instances int,		-- максимальное количество копий задачи, которые
								-- могут быть запущенны одновременно
		max_run_time interval,  -- максимальное время выполнения задачи
		onrollback text,        -- SQL команда, которая будет выполнена в случае
								-- аварийного завершения транзакции
		next_time_statement text,   -- SQL команда, которая будет выполненна 
									-- после завершения основного набора SQL 
									-- команд, которая возвращает следующее
									-- время выполнения задачи
		active boolean,         -- true - если задача доступна для запуску по 
								-- расписанию
		broken boolean          -- true - задача имеет ошибки в конфигурации,
								-- которые не позволяют ее выполнять далее
	);

**cron_job** используется для информации о конкретном исполнении задачи.

	CREATE TYPE schedule.cron_job AS(
		cron integer,           -- идентификатор задачи
		node text,              -- имя узла, на котором она выполняться
		scheduled_at timestamp, -- запланированное время выполнения
		name text,              -- имя задачи
		comments text,          -- комментарий к задаче
		commands text[],        -- sql комманды для выполнения
		run_as text,            -- имя пользователя, из-под которого идет выполнение
		owner text,             -- имя пользователя, создавшего задачу
		use_same_transaction boolean,	-- если true, то набор команд 
								-- выполняется в одной транзакции
		started timestamp,      -- время, когда задача была запущена
		last_start_available timestamp,	-- время, до которого задача должна
								-- быть запцщена
		finished timestamp,     -- время, когда задача была завершена
		max_run_time interval,  -- время, за которое задача должна выполнится,
								-- иначе она будет аварийно остановлена
		max_instances int,		-- количество возможных одновременных сущностей
								-- задачи, которые могут работать одновременно
		onrollback text,        -- SQL, который будет выполнен при аварийном 
								-- завершении транзакции
		next_time_statement text,	-- SQL для вычисления следующего времени запуска
		status text,			-- статус задачи: working, done, error 
		message text			-- сообщение, это может быть сообщение об
								-- ошибке, так и какая-то служебная информация
	);

## Функции управления

### schedule.create_job(cron text, sql text, node text)

Создает задачу и делает ее активной.

Агрументы:

* **cron** - crontab-like строка для задания расписания выполнения
* **sql** - строка, SQL команда для выполнения
* **node** - название узла, опционально

Возвращает идентификатор созданной задачи.

### schedule.create_job(cron text, sqls text[], node text)

Создает задачу и делает ее активной.

Агрументы:

* **cron** - crontab-like строка для задания расписания выполнения
* **sqls** - набор SQL комманд для выполнения в виде массива строк
* **node** - название узла, опционально

Возвращает идентификатор созданной задачи.

### schedule.create_job(date timestamp with time zone, sql text, node text)

Создает задачу и делает ее активной.

Агрументы:

* **date** - время исполнения задачи
* **sql** - строка, SQL команда для выполнения
* **node** - название узла, опционально

Возвращает идентификатор созданной задачи.

### schedule.create_job(date timestamp with time zone, sqls text[], node text)

Создает задачу и делает ее активной.

Агрументы:

* **date** - время исполнения задачи
* **sqls** - набор SQL комманд для выполнения в виде массива строк
* **node** - название узла, опционально

Возвращает идентификатор созданной задачи.

### schedule.create_job(dates timestamp with time zone[], sql text, node text)

Создает задачу и делает ее активной.

Агрументы:

* **dates** - набор дат для выполнения комманды в виде массива
* **sql** - строка, SQL команда для выполнения
* **node** - название узла, опционально

Возвращает идентификатор созданной задачи.

### schedule.create_job(dates timestamp with time zone[], sqls text[], node text)

Создает задачу и делает ее активной.

Агрументы:

* **dates** - набор дат для выполнения комманды в виде массива
* **sqls** - набор SQL комманд для выполнения в виде массива строк
* **node** - название узла, опционально

Возвращает идентификатор созданной задачи.

### schedule.create_job(data jsonb) 

Создает задачу и делает ее активной.

Единтвенный принимаемый параметр является объектом JSONB, который содержит
информацию о создаваемой задаче.

JSONB объект может содержат следующие ключи, не все они являются обязательными:

*	**name** - имя задачи;
*	**node** - имя узла, на котором будет выполняться задача;
*	**comments** - коментарии к задаче;
*	**cron** - строка cron-like, для описания расписания выполнения;
*	**rule** - расписание в виде JSONB объекта (смотри далее);
*	**command** - SQL команда для выполнения;
*	**commands** - набор SQL команд для выполнения в виде массива;
*	**run\_as** - пользователь, с правами которого будет исполняться задача
*	**start\_date** - начало временного периода, во время которого возможен 
	запуск выполнения задачи, если не указано, то нижняя граница не определена;
*	**end\_date** - окончание временного периода, во время которого возможен
	запуск выполнения задачи, если не указано, то верхняя граница не определена;
*	**date** - конкретная дата запуска задачи;
*	**dates** - надор дат запуска задачи;
*	**use\_same\_transaction** - устанавливает будет ли набор команд выполняться
	в рамках одной транзакции. По умолчанию: true TODO ???
*	**last\_start\_available** - на какое количество времени может быть отложено
	выполнение задачи, если в момент запуска по расписанию уже запущено
	максимально возможное количество задач. Время задается в формате 
	типа `interval`. Например, '00:02:34' - две минуты тридцать четыре секунды.
	Если время не определено, то ожидание будет бесконечным. По умолчанию 
	время не определено;
*	**max\_run\_time** - определяет максимально возможное время выполнения 
	задачи. Время задается в формате типа `interval`. Если время не определено,
	то время исполнения не ограничено. По умолчанию время не определено;
*	**onrollback** - SQL команда, которая будет выполнена если транзакция 
	завершится аварийно. По умолчанию неопределена;
*	**next\_time\_statement** - SQL команда, которая будет выполнена для
	определения следующего времени записи задачи.

Правила для вычисления расписания выполнения задачи могут быть заданы в виде 
строки cron (ключ `cron`), а так же в виде JSONВ объекта (ключ `rule`).

Данный объект может сожержать следующие поля:

* **minutes** - минуты, целочисленный массив со значениями в диапазоне 0-59
* **hours** -  часы, целочисленный массив со значениями в диапазоне 0-23
* **days** - дни месяца, целочисленный массив со значениями в диапазоне 1-31
* **months** - месяцы, целочисленный массив со значениями в диапазоне 1-12
* **wdays** - дни недели, целочисленный массив со значениями в диапазоне 0-6,
	где 0 - Воскресенье
* **onstart** - целое число, со значением 0 или 1, если 1, то задает выполнение
	задачи на старте планировщика

Так же расписание может быть задано на конкретную дату или на набор конкретных
дат. Для этого используйте ключи `date` или `dates` соответственно.

Все вышеописанные методы задания расписания могут быть скомбинированны между 
собой. Но использование хотя бы одного из них обязательно.

Ключ `next_time_statement` используется для того, что бы вычислить слеюующее 
время выполнения задачи. Если он определен, то первое время выполнения задачи
будет рассчитано с помощью методов приведенных выще, а последующие запуски будут
поставленны в расписание в то время, которое вернет SQL команда, указанная 
в данном ключе. Команда должна возвращать запись, в первом поле которого 
должно сожержаться значение следующего времени запуска типа `timestamp with time
zone`. Если значение будет другого типа или выполнение данного SQL вызовет
ошибку, то задача будет помеченна как сломанная, и дальнейшее ее выполнение 
будет запрещено. 

SQL для вычисления следующего времени запускается в случае удачного и не
удачного завершения транзакции. О том как завершилась транзакция можно узнать 
из значения переменной PostgreSQL `schedule.transaction_state`.

Значение переменной может быть:

* **success** - транзакция завершилась успешно
* **failure** - транзакция завершилась с ошибкой
* **running** - транзакция в процессе выполнения
* **undefined** - неопределена 

Последние два значения не должны появляться внутри выполнения
`next_time_statement`. Если они появились там, то это скорее всего означает 
какую-то внутреннюю ошибку планировщика.

Сами SQL команды задаются либо ключом `command`, либо ключом `commands`. 
Первый это одна SQL команда, второй набор команд. На самом деле в ключ 
`command` можно передать несколько команд разделанных точкой с запятой. Тогда
они все исполнятся в одной транзакции. Предпочтительно для набора команд
использовать ключ `commands`, так как в сочетании с ключом
`use_same_transaction` вы можете задавать исполнение команд в одной транзакции
или выполнять каждую команду в отдельной транзакции, что позволит сохранить 
результат успешно выполненных команд, если последующая завершается с 
ошибкой. Так же в сообщении об ошибке будет более точная информация.

Возвращает идентификатор созданной задачи.

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

