#!/bin/sh

cd /pg/src/src/test/regress

./pg_regress --use-existing \
    --schedule=parallel_schedule \
    --host=node1 \
    --user=postgres

STATUS=$?

# cat regression.diffs

exit $STATUS
