start segment: 0x4

-master non-2pc:
    last segment: 0x1b
    recovery time per 16 wal files, seconds:
        11.8
    average total recovery time (1.5M transactions): 17.0s

-master 2pc:
    last segment: 0x44
    recovery time per 16 wal files, seconds:
        142s
    average total recovery time (1.5M transactions): 568s

-patched 2pc:
    last segment: 0x44
    recovery time per 16 wal files, seconds:
        5.3
    average total recovery time (1.5M transactions): 21.2s

-patched2 2pc:
    last segment: 0x44
    recovery time per 16 wal files, seconds:
        5.2
    average total recovery time (1.5M transactions): 20.8s



------------------------


pkill -9 postgres
rm -rf tmp_install
make install



./tmp_install/bin/initdb -D ./tmp_install/data1
echo 'local   replication     stas                                trust' >> ./tmp_install/data1/pg_hba.conf
echo 'host   replication     all              ::1/128           trust' >> ./tmp_install/data1/pg_hba.conf
echo 'host    all             all             ::1/128                 trust' >> ./tmp_install/data1/pg_hba.conf
echo 'max_wal_senders = 10' >> ./tmp_install/data1/postgresql.conf
echo 'wal_level = replica' >> ./tmp_install/data1/postgresql.conf
echo 'max_prepared_transactions = 100' >> tmp_install/data1/postgresql.conf 
./tmp_install/bin/pg_ctl -w -D ./tmp_install/data1 -l logfile start
createdb
pgbench -i


mkdir ./tmp_install/wals
./tmp_install/bin/pg_receivexlog -D ./tmp_install/wals


./tmp_install/bin/pg_basebackup -D ./tmp_install/data_bb -h localhost
echo 'port = 5433' >> ./tmp_install/data_bb/postgresql.conf
echo "restore_command = 'cp /Users/stas/code/postgres_cluster/tmp_install/wals/%f \"%p\"'" > ./tmp_install/data_bb/recovery.conf





rm -rf ./tmp_install/data_bb_1/ && cp -R ./tmp_install/data_bb/ ./tmp_install/data_bb_1/
./tmp_install/bin/postgres -D tmp_install/data_bb_1






sudo dtrace -x ustackframes=100 -n 'profile-99 /execname == "postgres" && arg1/ {@[ustack()] = count(); } tick-10s { exit(0); }' -o out.stacks
./stackcollapse.pl out.stacks > out.folded
./flamegraph.pl out.folded > out.svg




         0.001  \set aid random(1, 100000 * :scale)
         0.001  \set bid random(1, 1 * :scale)
         0.000  \set tid random(1, 10 * :scale)
         0.000  \set delta random(-5000, 5000)
         0.096  BEGIN;
         0.235  UPDATE pgbench_accounts SET abalance = abalance + :delta WHERE aid = :aid;
         0.178  SELECT abalance FROM pgbench_accounts WHERE aid = :aid;
         0.155  INSERT INTO pgbench_history (tid, bid, aid, delta, mtime) VALUES (:tid, :bid, :aid, :delta, CURRENT_TIMESTAMP);
         0.408  END;



------------
non-2pc:
------------


pgbench -t 150000 -c 10 -P 1 -r -N


./tmp_install/bin/postgres -D tmp_install/data_bb_1
2017-01-20 17:12:41.180 +03 [36495] LOG:  database system was interrupted; last known up at 2017-01-20 17:02:55 +03
2017-01-20 17:12:41.222 +03 [36495] LOG:  starting archive recovery
2017-01-20 17:12:41.246 +03 [36495] LOG:  restored log file "000000010000000000000003" from archive
2017-01-20 17:12:41.252 +03 [36495] LOG:  redo starts at 0/30000D0
2017-01-20 17:12:41.252 +03 [36495] LOG:  consistent recovery state reached at 0/30001D8
2017-01-20 17:12:41.278 +03 [36495] LOG:  restored log file "000000010000000000000004" from archive
2017-01-20 17:12:41.457 +03 [36495] LOG:  restored log file "000000010000000000000005" from archive
2017-01-20 17:12:42.566 +03 [36495] LOG:  restored log file "000000010000000000000006" from archive
2017-01-20 17:12:43.699 +03 [36495] LOG:  restored log file "000000010000000000000007" from archive
2017-01-20 17:12:44.856 +03 [36495] LOG:  restored log file "000000010000000000000008" from archive
2017-01-20 17:12:46.077 +03 [36495] LOG:  restored log file "000000010000000000000009" from archive
2017-01-20 17:12:47.322 +03 [36495] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-20 17:12:48.533 +03 [36495] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-20 17:12:49.227 +03 [36495] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-20 17:12:49.972 +03 [36495] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-20 17:12:51.162 +03 [36495] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-20 17:12:52.379 +03 [36495] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-20 17:12:53.488 +03 [36495] LOG:  restored log file "000000010000000000000010" from archive
2017-01-20 17:12:54.715 +03 [36495] LOG:  restored log file "000000010000000000000011" from archive
2017-01-20 17:12:55.943 +03 [36495] LOG:  restored log file "000000010000000000000012" from archive
2017-01-20 17:12:57.190 +03 [36495] LOG:  restored log file "000000010000000000000013" from archive
2017-01-20 17:12:58.422 +03 [36495] LOG:  restored log file "000000010000000000000014" from archive
2017-01-20 17:12:59.589 +03 [36495] LOG:  restored log file "000000010000000000000015" from archive
2017-01-20 17:13:00.804 +03 [36495] LOG:  restored log file "000000010000000000000016" from archive
2017-01-20 17:13:02.005 +03 [36495] LOG:  restored log file "000000010000000000000017" from archive
2017-01-20 17:13:03.220 +03 [36495] LOG:  restored log file "000000010000000000000018" from archive
2017-01-20 17:13:04.439 +03 [36495] LOG:  restored log file "000000010000000000000019" from archive
2017-01-20 17:13:05.594 +03 [36495] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-20 17:13:06.829 +03 [36495] LOG:  restored log file "00000001000000000000001B" from archive
cp: /Users/stas/code/postgres_cluster/tmp_install/wals/00000001000000000000001C: No such file or directory
2017-01-20 17:13:08.007 +03 [36495] LOG:  redo done at 0/1BFFFF88
2017-01-20 17:13:08.007 +03 [36495] LOG:  last completed transaction was at log time 2017-01-20 17:11:16.998605+03
2017-01-20 17:13:08.024 +03 [36495] LOG:  restored log file "00000001000000000000001B" from archive
cp: /Users/stas/code/postgres_cluster/tmp_install/wals/00000002.history: No such file or directory
2017-01-20 17:13:08.034 +03 [36495] LOG:  selected new timeline ID: 2
cp: /Users/stas/code/postgres_cluster/tmp_install/wals/00000001.history: No such file or directory
2017-01-20 17:13:08.064 +03 [36495] LOG:  archive recovery complete
2017-01-20 17:13:08.266 +03 [36495] LOG:  MultiXact member wraparound protections are now enabled
2017-01-20 17:13:08.274 +03 [36494] LOG:  database system is ready to accept connections
2017-01-20 17:13:08.275 +03 [36530] LOG:  autovacuum launcher started
^C2017-01-20 17:13:33.779 +03 [36494] LOG:  received fast shutdown request
2017-01-20 17:13:33.779 +03 [36494] LOG:  aborting any active transactions
2017-01-20 17:13:33.779 +03 [36530] LOG:  autovacuum launcher shutting down
2017-01-20 17:13:33.780 +03 [36497] LOG:  shutting down
2017-01-20 17:13:33.812 +03 [36494] LOG:  database system is shut down

> 65.594 - 47.322
=> 18.27199999999999


2017-01-20 17:17:05.452 +03 [36680] LOG:  restored log file "000000010000000000000004" from archive
2017-01-20 17:17:05.588 +03 [36680] LOG:  restored log file "000000010000000000000005" from archive
2017-01-20 17:17:06.516 +03 [36680] LOG:  restored log file "000000010000000000000006" from archive
2017-01-20 17:17:07.637 +03 [36680] LOG:  restored log file "000000010000000000000007" from archive
2017-01-20 17:17:08.778 +03 [36680] LOG:  restored log file "000000010000000000000008" from archive
2017-01-20 17:17:09.976 +03 [36680] LOG:  restored log file "000000010000000000000009" from archive
2017-01-20 17:17:11.176 +03 [36680] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-20 17:17:12.629 +03 [36680] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-20 17:17:13.349 +03 [36680] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-20 17:17:14.123 +03 [36680] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-20 17:17:15.348 +03 [36680] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-20 17:17:16.578 +03 [36680] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-20 17:17:17.704 +03 [36680] LOG:  restored log file "000000010000000000000010" from archive
2017-01-20 17:17:18.928 +03 [36680] LOG:  restored log file "000000010000000000000011" from archive
2017-01-20 17:17:20.177 +03 [36680] LOG:  restored log file "000000010000000000000012" from archive
2017-01-20 17:17:21.416 +03 [36680] LOG:  restored log file "000000010000000000000013" from archive
2017-01-20 17:17:22.622 +03 [36680] LOG:  restored log file "000000010000000000000014" from archive
2017-01-20 17:17:23.793 +03 [36680] LOG:  restored log file "000000010000000000000015" from archive
2017-01-20 17:17:25.021 +03 [36680] LOG:  restored log file "000000010000000000000016" from archive
2017-01-20 17:17:26.234 +03 [36680] LOG:  restored log file "000000010000000000000017" from archive
2017-01-20 17:17:27.459 +03 [36680] LOG:  restored log file "000000010000000000000018" from archive
2017-01-20 17:17:28.665 +03 [36680] LOG:  restored log file "000000010000000000000019" from archive
2017-01-20 17:17:29.813 +03 [36680] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-20 17:17:31.033 +03 [36680] LOG:  restored log file "00000001000000000000001B" from archive

> 29.813  - 11.176 
=> 18.637


2017-01-20 17:20:53.225 +03 [36797] LOG:  restored log file "000000010000000000000004" from archive
2017-01-20 17:20:53.369 +03 [36797] LOG:  restored log file "000000010000000000000005" from archive
2017-01-20 17:20:54.292 +03 [36797] LOG:  restored log file "000000010000000000000006" from archive
2017-01-20 17:20:55.381 +03 [36797] LOG:  restored log file "000000010000000000000007" from archive
2017-01-20 17:20:56.552 +03 [36797] LOG:  restored log file "000000010000000000000008" from archive
2017-01-20 17:20:58.003 +03 [36797] LOG:  restored log file "000000010000000000000009" from archive
2017-01-20 17:20:59.225 +03 [36797] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-20 17:21:00.400 +03 [36797] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-20 17:21:01.073 +03 [36797] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-20 17:21:01.824 +03 [36797] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-20 17:21:03.055 +03 [36797] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-20 17:21:04.251 +03 [36797] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-20 17:21:05.375 +03 [36797] LOG:  restored log file "000000010000000000000010" from archive
2017-01-20 17:21:06.588 +03 [36797] LOG:  restored log file "000000010000000000000011" from archive
2017-01-20 17:21:07.806 +03 [36797] LOG:  restored log file "000000010000000000000012" from archive
2017-01-20 17:21:09.013 +03 [36797] LOG:  restored log file "000000010000000000000013" from archive
2017-01-20 17:21:10.224 +03 [36797] LOG:  restored log file "000000010000000000000014" from archive
2017-01-20 17:21:11.381 +03 [36797] LOG:  restored log file "000000010000000000000015" from archive
2017-01-20 17:21:12.588 +03 [36797] LOG:  restored log file "000000010000000000000016" from archive
2017-01-20 17:21:13.803 +03 [36797] LOG:  restored log file "000000010000000000000017" from archive
2017-01-20 17:21:15.140 +03 [36797] LOG:  restored log file "000000010000000000000018" from archive
2017-01-20 17:21:16.424 +03 [36797] LOG:  restored log file "000000010000000000000019" from archive
2017-01-20 17:21:17.580 +03 [36797] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-20 17:21:18.891 +03 [36797] LOG:  restored log file "00000001000000000000001B" from archive

> 60 + 17.580 - 59.225
=> 18.354999999999997






2pc_patched:
---------

\set aid random(1, 100000 * :scale)
\set bid random(1, 1 * :scale)
\set tid random(1, 10 * :scale)
\set delta random(-5000, 5000)
BEGIN;
UPDATE pgbench_accounts SET abalance = abalance + :delta WHERE aid = :aid;
SELECT abalance FROM pgbench_accounts WHERE aid = :aid;
INSERT INTO pgbench_history (tid, bid, aid, delta, mtime) VALUES (:tid, :bid, :aid, :delta, CURRENT_TIMESTAMP);
PREPARE TRANSACTION 'tx-:client_id';
COMMIT PREPARED 'tx-:client_id';


2017-01-20 18:55:33.015 +03 [42352] LOG:  restored log file "000000010000000000000004" from archive
2017-01-20 18:55:33.129 +03 [42352] LOG:  restored log file "000000010000000000000005" from archive
2017-01-20 18:55:33.471 +03 [42352] LOG:  restored log file "000000010000000000000006" from archive
2017-01-20 18:55:33.878 +03 [42352] LOG:  restored log file "000000010000000000000007" from archive
2017-01-20 18:55:34.323 +03 [42352] LOG:  restored log file "000000010000000000000008" from archive
2017-01-20 18:55:34.765 +03 [42352] LOG:  restored log file "000000010000000000000009" from archive
2017-01-20 18:55:35.234 +03 [42352] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-20 18:55:35.728 +03 [42352] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-20 18:55:36.208 +03 [42352] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-20 18:55:36.710 +03 [42352] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-20 18:55:37.201 +03 [42352] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-20 18:55:37.713 +03 [42352] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-20 18:55:38.219 +03 [42352] LOG:  restored log file "000000010000000000000010" from archive
2017-01-20 18:55:38.798 +03 [42352] LOG:  restored log file "000000010000000000000011" from archive
2017-01-20 18:55:39.301 +03 [42352] LOG:  restored log file "000000010000000000000012" from archive
2017-01-20 18:55:39.814 +03 [42352] LOG:  restored log file "000000010000000000000013" from archive
2017-01-20 18:55:40.311 +03 [42352] LOG:  restored log file "000000010000000000000014" from archive
2017-01-20 18:55:40.806 +03 [42352] LOG:  restored log file "000000010000000000000015" from archive
2017-01-20 18:55:41.312 +03 [42352] LOG:  restored log file "000000010000000000000016" from archive
2017-01-20 18:55:41.821 +03 [42352] LOG:  restored log file "000000010000000000000017" from archive
2017-01-20 18:55:42.318 +03 [42352] LOG:  restored log file "000000010000000000000018" from archive
2017-01-20 18:55:42.817 +03 [42352] LOG:  restored log file "000000010000000000000019" from archive
2017-01-20 18:55:43.342 +03 [42352] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-20 18:55:43.864 +03 [42352] LOG:  restored log file "00000001000000000000001B" from archive
2017-01-20 18:55:44.378 +03 [42352] LOG:  restored log file "00000001000000000000001C" from archive
2017-01-20 18:55:44.514 +03 [42352] LOG:  restored log file "00000001000000000000001D" from archive
2017-01-20 18:55:45.000 +03 [42352] LOG:  restored log file "00000001000000000000001E" from archive
2017-01-20 18:55:45.450 +03 [42352] LOG:  restored log file "00000001000000000000001F" from archive
2017-01-20 18:55:45.959 +03 [42352] LOG:  restored log file "000000010000000000000020" from archive
2017-01-20 18:55:46.461 +03 [42352] LOG:  restored log file "000000010000000000000021" from archive
2017-01-20 18:55:46.970 +03 [42352] LOG:  restored log file "000000010000000000000022" from archive
2017-01-20 18:55:47.481 +03 [42352] LOG:  restored log file "000000010000000000000023" from archive
2017-01-20 18:55:47.984 +03 [42352] LOG:  restored log file "000000010000000000000024" from archive
2017-01-20 18:55:48.511 +03 [42352] LOG:  restored log file "000000010000000000000025" from archive
2017-01-20 18:55:49.024 +03 [42352] LOG:  restored log file "000000010000000000000026" from archive
2017-01-20 18:55:49.549 +03 [42352] LOG:  restored log file "000000010000000000000027" from archive
2017-01-20 18:55:50.142 +03 [42352] LOG:  restored log file "000000010000000000000028" from archive
2017-01-20 18:55:50.736 +03 [42352] LOG:  restored log file "000000010000000000000029" from archive
2017-01-20 18:55:51.219 +03 [42352] LOG:  restored log file "00000001000000000000002A" from archive
2017-01-20 18:55:51.704 +03 [42352] LOG:  restored log file "00000001000000000000002B" from archive
2017-01-20 18:55:52.204 +03 [42352] LOG:  restored log file "00000001000000000000002C" from archive
2017-01-20 18:55:52.861 +03 [42352] LOG:  restored log file "00000001000000000000002D" from archive
2017-01-20 18:55:53.382 +03 [42352] LOG:  restored log file "00000001000000000000002E" from archive
2017-01-20 18:55:53.885 +03 [42352] LOG:  restored log file "00000001000000000000002F" from archive
2017-01-20 18:55:54.378 +03 [42352] LOG:  restored log file "000000010000000000000030" from archive
2017-01-20 18:55:54.860 +03 [42352] LOG:  restored log file "000000010000000000000031" from archive
2017-01-20 18:55:55.334 +03 [42352] LOG:  restored log file "000000010000000000000032" from archive
2017-01-20 18:55:55.868 +03 [42352] LOG:  restored log file "000000010000000000000033" from archive
2017-01-20 18:55:56.384 +03 [42352] LOG:  restored log file "000000010000000000000034" from archive
2017-01-20 18:55:56.892 +03 [42352] LOG:  restored log file "000000010000000000000035" from archive
2017-01-20 18:55:57.026 +03 [42352] LOG:  restored log file "000000010000000000000036" from archive
2017-01-20 18:55:57.571 +03 [42352] LOG:  restored log file "000000010000000000000037" from archive
2017-01-20 18:55:58.063 +03 [42352] LOG:  restored log file "000000010000000000000038" from archive
2017-01-20 18:55:58.561 +03 [42352] LOG:  restored log file "000000010000000000000039" from archive
2017-01-20 18:55:59.075 +03 [42352] LOG:  restored log file "00000001000000000000003A" from archive
2017-01-20 18:55:59.557 +03 [42352] LOG:  restored log file "00000001000000000000003B" from archive
2017-01-20 18:56:00.044 +03 [42352] LOG:  restored log file "00000001000000000000003C" from archive
2017-01-20 18:56:00.438 +03 [42352] LOG:  restored log file "00000001000000000000003D" from archive
2017-01-20 18:56:00.928 +03 [42352] LOG:  restored log file "00000001000000000000003E" from archive
2017-01-20 18:56:01.397 +03 [42352] LOG:  restored log file "00000001000000000000003F" from archive
2017-01-20 18:56:01.878 +03 [42352] LOG:  restored log file "000000010000000000000040" from archive
2017-01-20 18:56:02.349 +03 [42352] LOG:  restored log file "000000010000000000000041" from archive
2017-01-20 18:56:02.832 +03 [42352] LOG:  restored log file "000000010000000000000042" from archive
2017-01-20 18:56:03.311 +03 [42352] LOG:  restored log file "000000010000000000000043" from archive
2017-01-20 18:56:03.798 +03 [42352] LOG:  restored log file "000000010000000000000044" from archive




2017-01-20 18:44:13.110 +03 [42169] LOG:  restored log file "000000010000000000000004" from archive
2017-01-20 18:44:13.218 +03 [42169] LOG:  restored log file "000000010000000000000005" from archive
2017-01-20 18:44:13.638 +03 [42169] LOG:  restored log file "000000010000000000000006" from archive
2017-01-20 18:44:14.148 +03 [42169] LOG:  restored log file "000000010000000000000007" from archive
2017-01-20 18:44:14.673 +03 [42169] LOG:  restored log file "000000010000000000000008" from archive
2017-01-20 18:44:15.276 +03 [42169] LOG:  restored log file "000000010000000000000009" from archive
2017-01-20 18:44:15.870 +03 [42169] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-20 18:44:16.436 +03 [42169] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-20 18:44:17.197 +03 [42169] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-20 18:44:17.776 +03 [42169] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-20 18:44:18.413 +03 [42169] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-20 18:44:19.006 +03 [42169] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-20 18:44:19.577 +03 [42169] LOG:  restored log file "000000010000000000000010" from archive
2017-01-20 18:44:20.206 +03 [42169] LOG:  restored log file "000000010000000000000011" from archive
2017-01-20 18:44:20.801 +03 [42169] LOG:  restored log file "000000010000000000000012" from archive
2017-01-20 18:44:21.320 +03 [42169] LOG:  restored log file "000000010000000000000013" from archive
2017-01-20 18:44:21.862 +03 [42169] LOG:  restored log file "000000010000000000000014" from archive
2017-01-20 18:44:22.389 +03 [42169] LOG:  restored log file "000000010000000000000015" from archive
2017-01-20 18:44:22.949 +03 [42169] LOG:  restored log file "000000010000000000000016" from archive
2017-01-20 18:44:23.489 +03 [42169] LOG:  restored log file "000000010000000000000017" from archive
2017-01-20 18:44:24.006 +03 [42169] LOG:  restored log file "000000010000000000000018" from archive
2017-01-20 18:44:24.517 +03 [42169] LOG:  restored log file "000000010000000000000019" from archive
2017-01-20 18:44:25.036 +03 [42169] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-20 18:44:25.564 +03 [42169] LOG:  restored log file "00000001000000000000001B" from archive
2017-01-20 18:44:26.093 +03 [42169] LOG:  restored log file "00000001000000000000001C" from archive
2017-01-20 18:44:26.235 +03 [42169] LOG:  restored log file "00000001000000000000001D" from archive
2017-01-20 18:44:26.728 +03 [42169] LOG:  restored log file "00000001000000000000001E" from archive
2017-01-20 18:44:27.181 +03 [42169] LOG:  restored log file "00000001000000000000001F" from archive
2017-01-20 18:44:27.706 +03 [42169] LOG:  restored log file "000000010000000000000020" from archive
2017-01-20 18:44:28.216 +03 [42169] LOG:  restored log file "000000010000000000000021" from archive
2017-01-20 18:44:28.731 +03 [42169] LOG:  restored log file "000000010000000000000022" from archive
2017-01-20 18:44:29.235 +03 [42169] LOG:  restored log file "000000010000000000000023" from archive
2017-01-20 18:44:29.752 +03 [42169] LOG:  restored log file "000000010000000000000024" from archive
2017-01-20 18:44:30.250 +03 [42169] LOG:  restored log file "000000010000000000000025" from archive
2017-01-20 18:44:30.766 +03 [42169] LOG:  restored log file "000000010000000000000026" from archive
2017-01-20 18:44:31.299 +03 [42169] LOG:  restored log file "000000010000000000000027" from archive
2017-01-20 18:44:31.771 +03 [42169] LOG:  restored log file "000000010000000000000028" from archive
2017-01-20 18:44:32.268 +03 [42169] LOG:  restored log file "000000010000000000000029" from archive
2017-01-20 18:44:32.775 +03 [42169] LOG:  restored log file "00000001000000000000002A" from archive
2017-01-20 18:44:33.308 +03 [42169] LOG:  restored log file "00000001000000000000002B" from archive
2017-01-20 18:44:33.814 +03 [42169] LOG:  restored log file "00000001000000000000002C" from archive
2017-01-20 18:44:34.306 +03 [42169] LOG:  restored log file "00000001000000000000002D" from archive
2017-01-20 18:44:34.806 +03 [42169] LOG:  restored log file "00000001000000000000002E" from archive
2017-01-20 18:44:35.311 +03 [42169] LOG:  restored log file "00000001000000000000002F" from archive
2017-01-20 18:44:35.794 +03 [42169] LOG:  restored log file "000000010000000000000030" from archive
2017-01-20 18:44:36.269 +03 [42169] LOG:  restored log file "000000010000000000000031" from archive
2017-01-20 18:44:36.773 +03 [42169] LOG:  restored log file "000000010000000000000032" from archive
2017-01-20 18:44:37.276 +03 [42169] LOG:  restored log file "000000010000000000000033" from archive
2017-01-20 18:44:37.771 +03 [42169] LOG:  restored log file "000000010000000000000034" from archive
2017-01-20 18:44:38.260 +03 [42169] LOG:  restored log file "000000010000000000000035" from archive
2017-01-20 18:44:38.388 +03 [42169] LOG:  restored log file "000000010000000000000036" from archive
2017-01-20 18:44:38.879 +03 [42169] LOG:  restored log file "000000010000000000000037" from archive
2017-01-20 18:44:39.383 +03 [42169] LOG:  restored log file "000000010000000000000038" from archive
2017-01-20 18:44:39.873 +03 [42169] LOG:  restored log file "000000010000000000000039" from archive
2017-01-20 18:44:40.366 +03 [42169] LOG:  restored log file "00000001000000000000003A" from archive
2017-01-20 18:44:40.861 +03 [42169] LOG:  restored log file "00000001000000000000003B" from archive
2017-01-20 18:44:41.354 +03 [42169] LOG:  restored log file "00000001000000000000003C" from archive
2017-01-20 18:44:41.755 +03 [42169] LOG:  restored log file "00000001000000000000003D" from archive
2017-01-20 18:44:42.260 +03 [42169] LOG:  restored log file "00000001000000000000003E" from archive
2017-01-20 18:44:42.742 +03 [42169] LOG:  restored log file "00000001000000000000003F" from archive
2017-01-20 18:44:43.238 +03 [42169] LOG:  restored log file "000000010000000000000040" from archive
2017-01-20 18:44:43.726 +03 [42169] LOG:  restored log file "000000010000000000000041" from archive
2017-01-20 18:44:44.218 +03 [42169] LOG:  restored log file "000000010000000000000042" from archive
2017-01-20 18:44:44.706 +03 [42169] LOG:  restored log file "000000010000000000000043" from archive
2017-01-20 18:44:45.208 +03 [42169] LOG:  restored log file "000000010000000000000044" from archive



-----------
2pc-master
-----------


2017-01-20 19:14:48.707 +03 [58215] LOG:  database system was interrupted; last known up at 2017-01-20 19:05:17 +03
2017-01-20 19:14:48.749 +03 [58215] LOG:  starting archive recovery
2017-01-20 19:14:48.799 +03 [58215] LOG:  restored log file "000000010000000000000003" from archive
2017-01-20 19:14:48.804 +03 [58215] LOG:  redo starts at 0/30000D0
2017-01-20 19:14:48.805 +03 [58215] LOG:  consistent recovery state reached at 0/30001A0
2017-01-20 19:14:48.825 +03 [58215] LOG:  restored log file "000000010000000000000004" from archive
2017-01-20 19:14:50.255 +03 [58215] LOG:  restored log file "000000010000000000000005" from archive
2017-01-20 19:14:57.682 +03 [58215] LOG:  restored log file "000000010000000000000006" from archive
2017-01-20 19:15:05.873 +03 [58215] LOG:  restored log file "000000010000000000000007" from archive
2017-01-20 19:15:13.835 +03 [58215] LOG:  restored log file "000000010000000000000008" from archive
2017-01-20 19:15:21.976 +03 [58215] LOG:  restored log file "000000010000000000000009" from archive
2017-01-20 19:15:30.263 +03 [58215] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-20 19:15:38.319 +03 [58215] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-20 19:15:46.189 +03 [58215] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-20 19:15:54.779 +03 [58215] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-20 19:16:03.105 +03 [58215] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-20 19:16:12.730 +03 [58215] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-20 19:16:22.178 +03 [58215] LOG:  restored log file "000000010000000000000010" from archive
2017-01-20 19:16:30.918 +03 [58215] LOG:  restored log file "000000010000000000000011" from archive
2017-01-20 19:16:40.575 +03 [58215] LOG:  restored log file "000000010000000000000012" from archive
2017-01-20 19:16:50.352 +03 [58215] LOG:  restored log file "000000010000000000000013" from archive
2017-01-20 19:16:59.941 +03 [58215] LOG:  restored log file "000000010000000000000014" from archive
2017-01-20 19:17:11.666 +03 [58215] LOG:  restored log file "000000010000000000000015" from archive
2017-01-20 19:17:20.490 +03 [58215] LOG:  restored log file "000000010000000000000016" from archive
2017-01-20 19:17:28.920 +03 [58215] LOG:  restored log file "000000010000000000000017" from archive
2017-01-20 19:17:38.462 +03 [58215] LOG:  restored log file "000000010000000000000018" from archive
2017-01-20 19:17:48.278 +03 [58215] LOG:  restored log file "000000010000000000000019" from archive
2017-01-20 19:17:57.842 +03 [58215] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-20 19:18:09.710 +03 [58215] LOG:  restored log file "00000001000000000000001B" from archive
2017-01-20 19:18:18.116 +03 [58215] LOG:  restored log file "00000001000000000000001C" from archive
2017-01-20 19:18:19.926 +03 [58215] LOG:  restored log file "00000001000000000000001D" from archive
2017-01-20 19:18:28.002 +03 [58215] LOG:  restored log file "00000001000000000000001E" from archive
2017-01-20 19:18:37.715 +03 [58215] LOG:  restored log file "00000001000000000000001F" from archive
2017-01-20 19:18:49.512 +03 [58215] LOG:  restored log file "000000010000000000000020" from archive
2017-01-20 19:18:57.924 +03 [58215] LOG:  restored log file "000000010000000000000021" from archive
2017-01-20 19:19:06.512 +03 [58215] LOG:  restored log file "000000010000000000000022" from archive
2017-01-20 19:19:14.711 +03 [58215] LOG:  restored log file "000000010000000000000023" from archive
2017-01-20 19:19:23.009 +03 [58215] LOG:  restored log file "000000010000000000000024" from archive
2017-01-20 19:19:31.854 +03 [58215] LOG:  restored log file "000000010000000000000025" from archive
2017-01-20 19:19:41.936 +03 [58215] LOG:  restored log file "000000010000000000000026" from archive
2017-01-20 19:19:51.507 +03 [58215] LOG:  restored log file "000000010000000000000027" from archive
2017-01-20 19:19:58.979 +03 [58215] LOG:  restored log file "000000010000000000000028" from archive
2017-01-20 19:20:08.850 +03 [58215] LOG:  restored log file "000000010000000000000029" from archive
2017-01-20 19:20:19.331 +03 [58215] LOG:  restored log file "00000001000000000000002A" from archive
2017-01-20 19:20:28.180 +03 [58215] LOG:  restored log file "00000001000000000000002B" from archive
2017-01-20 19:20:37.764 +03 [58215] LOG:  restored log file "00000001000000000000002C" from archive
2017-01-20 19:20:47.966 +03 [58215] LOG:  restored log file "00000001000000000000002D" from archive
2017-01-20 19:20:56.130 +03 [58215] LOG:  restored log file "00000001000000000000002E" from archive
2017-01-20 19:21:05.027 +03 [58215] LOG:  restored log file "00000001000000000000002F" from archive
2017-01-20 19:21:14.755 +03 [58215] LOG:  restored log file "000000010000000000000030" from archive
2017-01-20 19:21:25.207 +03 [58215] LOG:  restored log file "000000010000000000000031" from archive
2017-01-20 19:21:36.655 +03 [58215] LOG:  restored log file "000000010000000000000032" from archive
2017-01-20 19:21:45.702 +03 [58215] LOG:  restored log file "000000010000000000000033" from archive
2017-01-20 19:21:54.497 +03 [58215] LOG:  restored log file "000000010000000000000034" from archive
2017-01-20 19:22:04.305 +03 [58215] LOG:  restored log file "000000010000000000000035" from archive
2017-01-20 19:22:06.271 +03 [58215] LOG:  restored log file "000000010000000000000036" from archive
2017-01-20 19:22:15.816 +03 [58215] LOG:  restored log file "000000010000000000000037" from archive
2017-01-20 19:22:26.531 +03 [58215] LOG:  restored log file "000000010000000000000038" from archive
2017-01-20 19:22:36.417 +03 [58215] LOG:  restored log file "000000010000000000000039" from archive
2017-01-20 19:22:45.403 +03 [58215] LOG:  restored log file "00000001000000000000003A" from archive
2017-01-20 19:22:56.128 +03 [58215] LOG:  restored log file "00000001000000000000003B" from archive
2017-01-20 19:23:07.588 +03 [58215] LOG:  restored log file "00000001000000000000003C" from archive
2017-01-20 19:23:19.459 +03 [58215] LOG:  restored log file "00000001000000000000003D" from archive
2017-01-20 19:23:31.660 +03 [58215] LOG:  restored log file "00000001000000000000003E" from archive
2017-01-20 19:23:39.753 +03 [58215] LOG:  restored log file "00000001000000000000003F" from archive
2017-01-20 19:23:48.763 +03 [58215] LOG:  restored log file "000000010000000000000040" from archive
2017-01-20 19:23:58.009 +03 [58215] LOG:  restored log file "000000010000000000000041" from archive
2017-01-20 19:24:08.117 +03 [58215] LOG:  restored log file "000000010000000000000042" from archive
2017-01-20 19:24:18.102 +03 [58215] LOG:  restored log file "000000010000000000000043" from archive
2017-01-20 19:24:28.290 +03 [58215] LOG:  restored log file "000000010000000000000044" from archive
cp: /Users/stas/code/postgres_cluster/tmp_install/wals/000000010000000000000045: No such file or directory



2pc-patched2:

2017-01-23 10:55:20.002 +03 [78498] LOG:  consistent recovery state reached at 0/30001D8
2017-01-23 10:55:20.019 +03 [78498] LOG:  restored log file "000000010000000000000004" from archive
2017-01-23 10:55:20.100 +03 [78498] LOG:  restored log file "000000010000000000000005" from archive
2017-01-23 10:55:20.328 +03 [78498] LOG:  restored log file "000000010000000000000006" from archive
2017-01-23 10:55:20.597 +03 [78498] LOG:  restored log file "000000010000000000000007" from archive
2017-01-23 10:55:20.878 +03 [78498] LOG:  restored log file "000000010000000000000008" from archive
2017-01-23 10:55:21.167 +03 [78498] LOG:  restored log file "000000010000000000000009" from archive
2017-01-23 10:55:21.460 +03 [78498] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-23 10:55:21.757 +03 [78498] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-23 10:55:22.072 +03 [78498] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-23 10:55:22.368 +03 [78498] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-23 10:55:22.678 +03 [78498] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-23 10:55:23.000 +03 [78498] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-23 10:55:23.315 +03 [78498] LOG:  restored log file "000000010000000000000010" from archive
2017-01-23 10:55:23.622 +03 [78498] LOG:  restored log file "000000010000000000000011" from archive
2017-01-23 10:55:23.940 +03 [78498] LOG:  restored log file "000000010000000000000012" from archive
2017-01-23 10:55:24.282 +03 [78498] LOG:  restored log file "000000010000000000000013" from archive
2017-01-23 10:55:24.587 +03 [78498] LOG:  restored log file "000000010000000000000014" from archive
2017-01-23 10:55:24.884 +03 [78498] LOG:  restored log file "000000010000000000000015" from archive
2017-01-23 10:55:25.204 +03 [78498] LOG:  restored log file "000000010000000000000016" from archive
2017-01-23 10:55:25.509 +03 [78498] LOG:  restored log file "000000010000000000000017" from archive
2017-01-23 10:55:25.809 +03 [78498] LOG:  restored log file "000000010000000000000018" from archive
2017-01-23 10:55:26.107 +03 [78498] LOG:  restored log file "000000010000000000000019" from archive
2017-01-23 10:55:26.423 +03 [78498] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-23 10:55:26.748 +03 [78498] LOG:  restored log file "00000001000000000000001B" from archive
2017-01-23 10:55:27.064 +03 [78498] LOG:  restored log file "00000001000000000000001C" from archive
2017-01-23 10:55:27.156 +03 [78498] LOG:  restored log file "00000001000000000000001D" from archive
2017-01-23 10:55:27.480 +03 [78498] LOG:  restored log file "00000001000000000000001E" from archive
2017-01-23 10:55:27.806 +03 [78498] LOG:  restored log file "00000001000000000000001F" from archive
2017-01-23 10:55:28.197 +03 [78498] LOG:  restored log file "000000010000000000000020" from archive
2017-01-23 10:55:28.596 +03 [78498] LOG:  restored log file "000000010000000000000021" from archive
2017-01-23 10:55:28.994 +03 [78498] LOG:  restored log file "000000010000000000000022" from archive
2017-01-23 10:55:29.325 +03 [78498] LOG:  restored log file "000000010000000000000023" from archive
2017-01-23 10:55:29.659 +03 [78498] LOG:  restored log file "000000010000000000000024" from archive
2017-01-23 10:55:29.971 +03 [78498] LOG:  restored log file "000000010000000000000025" from archive
2017-01-23 10:55:30.297 +03 [78498] LOG:  restored log file "000000010000000000000026" from archive
2017-01-23 10:55:30.585 +03 [78498] LOG:  restored log file "000000010000000000000027" from archive
2017-01-23 10:55:30.921 +03 [78498] LOG:  restored log file "000000010000000000000028" from archive
2017-01-23 10:55:31.232 +03 [78498] LOG:  restored log file "000000010000000000000029" from archive
2017-01-23 10:55:31.540 +03 [78498] LOG:  restored log file "00000001000000000000002A" from archive
2017-01-23 10:55:31.855 +03 [78498] LOG:  restored log file "00000001000000000000002B" from archive
2017-01-23 10:55:32.167 +03 [78498] LOG:  restored log file "00000001000000000000002C" from archive
2017-01-23 10:55:32.480 +03 [78498] LOG:  restored log file "00000001000000000000002D" from archive
2017-01-23 10:55:32.803 +03 [78498] LOG:  restored log file "00000001000000000000002E" from archive
2017-01-23 10:55:33.126 +03 [78498] LOG:  restored log file "00000001000000000000002F" from archive
2017-01-23 10:55:33.435 +03 [78498] LOG:  restored log file "000000010000000000000030" from archive
2017-01-23 10:55:33.741 +03 [78498] LOG:  restored log file "000000010000000000000031" from archive
2017-01-23 10:55:34.076 +03 [78498] LOG:  restored log file "000000010000000000000032" from archive
2017-01-23 10:55:34.396 +03 [78498] LOG:  restored log file "000000010000000000000033" from archive
2017-01-23 10:55:34.683 +03 [78498] LOG:  restored log file "000000010000000000000034" from archive
2017-01-23 10:55:35.006 +03 [78498] LOG:  restored log file "000000010000000000000035" from archive
2017-01-23 10:55:35.109 +03 [78498] LOG:  restored log file "000000010000000000000036" from archive
2017-01-23 10:55:35.431 +03 [78498] LOG:  restored log file "000000010000000000000037" from archive
2017-01-23 10:55:35.753 +03 [78498] LOG:  restored log file "000000010000000000000038" from archive
2017-01-23 10:55:36.091 +03 [78498] LOG:  restored log file "000000010000000000000039" from archive
2017-01-23 10:55:36.404 +03 [78498] LOG:  restored log file "00000001000000000000003A" from archive
2017-01-23 10:55:36.714 +03 [78498] LOG:  restored log file "00000001000000000000003B" from archive
2017-01-23 10:55:37.042 +03 [78498] LOG:  restored log file "00000001000000000000003C" from archive
2017-01-23 10:55:37.350 +03 [78498] LOG:  restored log file "00000001000000000000003D" from archive
2017-01-23 10:55:37.683 +03 [78498] LOG:  restored log file "00000001000000000000003E" from archive
2017-01-23 10:55:38.023 +03 [78498] LOG:  restored log file "00000001000000000000003F" from archive
2017-01-23 10:55:38.340 +03 [78498] LOG:  restored log file "000000010000000000000040" from archive
2017-01-23 10:55:38.632 +03 [78498] LOG:  restored log file "000000010000000000000041" from archive
2017-01-23 10:55:38.890 +03 [78498] LOG:  restored log file "000000010000000000000042" from archive
2017-01-23 10:55:39.178 +03 [78498] LOG:  restored log file "000000010000000000000043" from archive
2017-01-23 10:55:39.487 +03 [78498] LOG:  restored log file "000000010000000000000044" from archive







2017-01-23 10:58:21.817 +03 [78719] LOG:  restored log file "000000010000000000000004" from archive
2017-01-23 10:58:21.903 +03 [78719] LOG:  restored log file "000000010000000000000005" from archive
2017-01-23 10:58:22.136 +03 [78719] LOG:  restored log file "000000010000000000000006" from archive
2017-01-23 10:58:22.404 +03 [78719] LOG:  restored log file "000000010000000000000007" from archive
2017-01-23 10:58:22.684 +03 [78719] LOG:  restored log file "000000010000000000000008" from archive
2017-01-23 10:58:22.985 +03 [78719] LOG:  restored log file "000000010000000000000009" from archive
2017-01-23 10:58:23.282 +03 [78719] LOG:  restored log file "00000001000000000000000A" from archive
2017-01-23 10:58:23.588 +03 [78719] LOG:  restored log file "00000001000000000000000B" from archive
2017-01-23 10:58:23.889 +03 [78719] LOG:  restored log file "00000001000000000000000C" from archive
2017-01-23 10:58:24.230 +03 [78719] LOG:  restored log file "00000001000000000000000D" from archive
2017-01-23 10:58:24.544 +03 [78719] LOG:  restored log file "00000001000000000000000E" from archive
2017-01-23 10:58:24.856 +03 [78719] LOG:  restored log file "00000001000000000000000F" from archive
2017-01-23 10:58:25.170 +03 [78719] LOG:  restored log file "000000010000000000000010" from archive
2017-01-23 10:58:25.496 +03 [78719] LOG:  restored log file "000000010000000000000011" from archive
2017-01-23 10:58:25.831 +03 [78719] LOG:  restored log file "000000010000000000000012" from archive
2017-01-23 10:58:26.190 +03 [78719] LOG:  restored log file "000000010000000000000013" from archive
2017-01-23 10:58:26.537 +03 [78719] LOG:  restored log file "000000010000000000000014" from archive
2017-01-23 10:58:26.854 +03 [78719] LOG:  restored log file "000000010000000000000015" from archive
2017-01-23 10:58:27.184 +03 [78719] LOG:  restored log file "000000010000000000000016" from archive
2017-01-23 10:58:27.531 +03 [78719] LOG:  restored log file "000000010000000000000017" from archive
2017-01-23 10:58:27.844 +03 [78719] LOG:  restored log file "000000010000000000000018" from archive
2017-01-23 10:58:28.149 +03 [78719] LOG:  restored log file "000000010000000000000019" from archive
2017-01-23 10:58:28.479 +03 [78719] LOG:  restored log file "00000001000000000000001A" from archive
2017-01-23 10:58:28.810 +03 [78719] LOG:  restored log file "00000001000000000000001B" from archive
2017-01-23 10:58:29.133 +03 [78719] LOG:  restored log file "00000001000000000000001C" from archive
2017-01-23 10:58:29.228 +03 [78719] LOG:  restored log file "00000001000000000000001D" from archive
2017-01-23 10:58:29.531 +03 [78719] LOG:  restored log file "00000001000000000000001E" from archive
2017-01-23 10:58:30.032 +03 [78719] LOG:  restored log file "00000001000000000000001F" from archive
2017-01-23 10:58:30.423 +03 [78719] LOG:  restored log file "000000010000000000000020" from archive
2017-01-23 10:58:30.734 +03 [78719] LOG:  restored log file "000000010000000000000021" from archive
2017-01-23 10:58:31.064 +03 [78719] LOG:  restored log file "000000010000000000000022" from archive
2017-01-23 10:58:31.404 +03 [78719] LOG:  restored log file "000000010000000000000023" from archive
2017-01-23 10:58:31.746 +03 [78719] LOG:  restored log file "000000010000000000000024" from archive
2017-01-23 10:58:32.056 +03 [78719] LOG:  restored log file "000000010000000000000025" from archive
2017-01-23 10:58:32.371 +03 [78719] LOG:  restored log file "000000010000000000000026" from archive
2017-01-23 10:58:32.676 +03 [78719] LOG:  restored log file "000000010000000000000027" from archive
2017-01-23 10:58:33.014 +03 [78719] LOG:  restored log file "000000010000000000000028" from archive
2017-01-23 10:58:33.339 +03 [78719] LOG:  restored log file "000000010000000000000029" from archive
2017-01-23 10:58:33.660 +03 [78719] LOG:  restored log file "00000001000000000000002A" from archive
2017-01-23 10:58:33.976 +03 [78719] LOG:  restored log file "00000001000000000000002B" from archive
2017-01-23 10:58:34.293 +03 [78719] LOG:  restored log file "00000001000000000000002C" from archive
2017-01-23 10:58:34.609 +03 [78719] LOG:  restored log file "00000001000000000000002D" from archive
2017-01-23 10:58:34.931 +03 [78719] LOG:  restored log file "00000001000000000000002E" from archive
2017-01-23 10:58:35.250 +03 [78719] LOG:  restored log file "00000001000000000000002F" from archive
2017-01-23 10:58:35.570 +03 [78719] LOG:  restored log file "000000010000000000000030" from archive
2017-01-23 10:58:35.888 +03 [78719] LOG:  restored log file "000000010000000000000031" from archive
2017-01-23 10:58:36.213 +03 [78719] LOG:  restored log file "000000010000000000000032" from archive
2017-01-23 10:58:36.531 +03 [78719] LOG:  restored log file "000000010000000000000033" from archive
2017-01-23 10:58:36.824 +03 [78719] LOG:  restored log file "000000010000000000000034" from archive
2017-01-23 10:58:37.145 +03 [78719] LOG:  restored log file "000000010000000000000035" from archive
2017-01-23 10:58:37.245 +03 [78719] LOG:  restored log file "000000010000000000000036" from archive
2017-01-23 10:58:37.567 +03 [78719] LOG:  restored log file "000000010000000000000037" from archive
2017-01-23 10:58:37.886 +03 [78719] LOG:  restored log file "000000010000000000000038" from archive
2017-01-23 10:58:38.202 +03 [78719] LOG:  restored log file "000000010000000000000039" from archive
2017-01-23 10:58:38.515 +03 [78719] LOG:  restored log file "00000001000000000000003A" from archive
2017-01-23 10:58:38.836 +03 [78719] LOG:  restored log file "00000001000000000000003B" from archive
2017-01-23 10:58:39.162 +03 [78719] LOG:  restored log file "00000001000000000000003C" from archive
2017-01-23 10:58:39.490 +03 [78719] LOG:  restored log file "00000001000000000000003D" from archive
2017-01-23 10:58:39.813 +03 [78719] LOG:  restored log file "00000001000000000000003E" from archive
2017-01-23 10:58:40.137 +03 [78719] LOG:  restored log file "00000001000000000000003F" from archive
2017-01-23 10:58:40.436 +03 [78719] LOG:  restored log file "000000010000000000000040" from archive
2017-01-23 10:58:40.745 +03 [78719] LOG:  restored log file "000000010000000000000041" from archive
2017-01-23 10:58:41.022 +03 [78719] LOG:  restored log file "000000010000000000000042" from archive
2017-01-23 10:58:41.320 +03 [78719] LOG:  restored log file "000000010000000000000043" from archive
2017-01-23 10:58:41.632 +03 [78719] LOG:  restored log file "000000010000000000000044" from archive








