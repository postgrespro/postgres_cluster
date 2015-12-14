go run perf/*.go  \
-C "dbname=postgres port=5432 sslmode=disable" \
-C "dbname=postgres port=5433 sslmode=disable" \
-g -w 64 -r 1 -n 10000 -a 10000 -i

go run perf/*.go  \
-C "dbname=postgres port=5432 sslmode=disable" \
-C "dbname=postgres port=5433 sslmode=disable" \
-g -w 64 -r 1 -n 10000 -a 10000
