go run *.go  \
-C "dbname=postgres port=5432 sslmode=disable" \
-C "dbname=postgres port=5433 sslmode=disable" \
-g -w 8 -r 1 -n 1000 -a 1000 -i
