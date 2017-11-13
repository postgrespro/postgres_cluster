#!/bin/sh

cd /pg/src/src/test/regress

psql -U postgres regression <<-SQL 
    ALTER DATABASE "postgres" SET lc_messages TO 'C';
    ALTER DATABASE "postgres" SET lc_monetary TO 'C';
    ALTER DATABASE "postgres" SET lc_numeric TO 'C';
    ALTER DATABASE "postgres" SET lc_time TO 'C';
    ALTER DATABASE "postgres" SET timezone_abbreviations TO 'Default';
SQL

./pg_regress --use-existing \
    --schedule=serial_schedule \
    --host=node1 \
    --user=postgres

STATUS=$?

if [ -f "regression.diffs" ]
then
	cat regression.diffs
fi

exit $STATUS
