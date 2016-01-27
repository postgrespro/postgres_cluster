echo Insert with 1 index
./insbench -c "dbname=postgres host=localhost port=5432 sslmode=disable" -q -C -x 0
echo Insert with 9 indexex
./insbench -c "dbname=postgres host=localhost port=5432 sslmode=disable" -q -C -x 8
echo Insert with 9 concurrently update partial indexes
./insbench -c "dbname=postgres host=localhost port=5432 sslmode=disable" -q -C -x 8 -u 1
echo Insert with 9 frozen partial indexes
./insbench -c "dbname=postgres host=localhost port=5432 sslmode=disable" -q -C -x 8 -u 100
