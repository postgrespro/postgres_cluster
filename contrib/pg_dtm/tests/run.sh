./dtmbench  \
-C "dbname=postgres host=localhost port=5432 sslmode=disable" \
-C "dbname=postgres host=localhost port=5433 sslmode=disable" \
-n 1000 -a 1010 -d 1000 -w 10 -r 1 -i $*


./dtmbench  \
-C "dbname=postgres host=localhost port=5432 sslmode=disable" \
-C "dbname=postgres host=localhost port=5433 sslmode=disable" \
-n 30000 -a 1010 -d 1000 -w 10 -r 1 $*
