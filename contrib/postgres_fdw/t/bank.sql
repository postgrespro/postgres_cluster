\set id random(1, 20000)

BEGIN;
WITH upd AS (UPDATE accounts SET amount = amount - 1 WHERE id = :id RETURNING *)
    INSERT into global_transactions SELECT now() FROM upd;
-- separate this test with big amount of connections
--select pg_sleep(0.5*random());
UPDATE accounts SET amount = amount + 1 WHERE id = (:id + 1);
-- INSERT into global_transactions values(now());
COMMIT;
