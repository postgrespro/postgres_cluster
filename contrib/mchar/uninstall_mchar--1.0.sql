SET search_path = public;
BEGIN;

DROP FUNCTION mchar_pattern_fixed_prefix(internal, internal, internal);
DROP FUNCTION mchar_greaterstring(internal);
DROP TYPE MCHAR CASCADE;
DROP TYPE MVARCHAR CASCADE;

COMMIT;
