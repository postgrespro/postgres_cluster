./dtmbench  \
-c "dbname=regression host=localhost port=5432 sslmode=disable" \
-c "dbname=regression host=localhost port=5433 sslmode=disable" \
-c "dbname=regression host=localhost port=5434 sslmode=disable" \
-n 1000 -a 1000 -w 10 -r 1 $*
