SELECT * FROM pt;
SELECT * FROM rt;
SELECT * FROM st;
SELECT count(*) FROM pt;
SELECT count(*) FROM pt,rt;
SELECT count(*) FROM pt,rt,st;
SELECT count(*) FROM pt,rt WHERE pt.id=rt.id;
SELECT count(*) FROM pt,rt,st WHERE pt.id=rt.id and rt.id=st.id;
SELECT count(*) FROM pt,rt,st WHERE pt.id=rt.id and rt.id=st.payload;
SELECT count(*) FROM pt,rt,st WHERE pt.id=rt.payload and rt.id=st.payload;
SELECT count(*) FROM pt group by pt.id;
SELECT count(*) FROM pt group by pt.id + 1;

-- Big table test
INSERT INTO t1 (payload) (SELECT * FROM generate_series(1, 10000));

SET enable_seqscan = off;
SET enable_indexscan = off;
SET enable_indexonlyscan = off;
SET enable_tidscan = off;
SET enable_bitmapscan = on;

explain SELECT count(*) FROM t1 WHERE id < 100;
SELECT count(*) FROM t1 WHERE id < 100;

SET enable_bitmapscan = off;
SET enable_indexscan = on;
explain SELECT count(*) FROM t1 WHERE id < 100;
SELECT count(*) FROM t1 WHERE id < 100;

SET enable_seqscan = on;
SET enable_indexscan = off;
explain SELECT count(*) FROM t1 WHERE id < 100;
SELECT count(*) FROM t1 WHERE id < 100;

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_indexonlyscan = on;
explain SELECT count(*) FROM t1 WHERE id < 100;
SELECT count(*) FROM t1 WHERE id < 100;
