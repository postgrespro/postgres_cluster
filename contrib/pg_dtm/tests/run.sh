./dtmbench  \
-c "dbname=postgres host=localhost user=knizhnik port=5432 sslmode=disable" \
-c "dbname=postgres host=localhost user=knizhnik port=5433 sslmode=disable" \
-c "dbname=postgres host=localhost user=knizhnik port=5434 sslmode=disable" \
-n 1000 -a 1000 -w 10 -r 1 $*
