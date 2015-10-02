#!/bin/sh
rm -rf test.log
failed=0
passed=0
for testname in $@; do
	if bin/${testname}-test >> test.log 2>&1; then
		echo "${testname} ok"
		passed=$((passed + 1))
	else
		echo "${testname} FAILED"
		failed=$((failed + 1))
	fi
done

echo "tests passed: $passed"
echo "tests failed: $failed"
if [ $failed -eq 0 ]; then
	rm -rf test.log
else
	echo "see test.log for details"
	exit 1
fi
