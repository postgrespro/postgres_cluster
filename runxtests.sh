iterations=${1:-10}
pkill -9 python
pkill -9 postgres
succeed=0
failed=0
SECONDS=0
for ((i=0; i<$iterations; i++))
do
	if make xcheck
	then
		((succeed++))
	else
		((failed++))
		exit
	fi
done
echo "Elapsed time for $iterations iterations: $SECONDS seconds ($succeed succeed, $failed failed)"
