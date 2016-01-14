./dtmbench  \
-c "dbname=postgres host=localhost port=5432 sslmode=disable" \
-n 1000 -a 100000 -w 10 -r 1 $*
