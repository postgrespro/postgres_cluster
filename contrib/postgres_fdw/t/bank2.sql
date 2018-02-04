\set id random(1, 10000)

BEGIN;
WITH upd AS (UPDATE accounts SET amount = amount - 1 WHERE id = 2*:id RETURNING *)
    INSERT into local_transactions SELECT now() FROM upd;
UPDATE accounts SET amount = amount + 1 WHERE id = (2*:id + 2);
COMMIT;
