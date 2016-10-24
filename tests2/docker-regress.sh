#!/bin/sh

cd /pg/src/src/test/regress

cat /etc/hosts

./pg_regress --use-existing \
    --schedule=parallel_schedule \
    --host=node1 \
    --user=postgres \
    --dlpath=/pg/src/src/test/regress/

cat regression.diffs
