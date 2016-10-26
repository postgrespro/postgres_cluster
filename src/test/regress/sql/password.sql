--
-- Tests for password verifiers
--

-- Tests for GUC password_encryption
SET password_encryption = 'novalue'; -- error
SET password_encryption = true; -- ok
SET password_encryption = 'md5'; -- ok
SET password_encryption = 'plain'; -- ok
SET password_encryption = 'scram'; -- ok

-- consistency of password entries
SET password_encryption = 'plain';
CREATE ROLE regress_passwd1 PASSWORD 'role_pwd1';
SET password_encryption = 'md5';
CREATE ROLE regress_passwd2 PASSWORD 'role_pwd2';
SET password_encryption = 'on';
CREATE ROLE regress_passwd3 PASSWORD 'role_pwd3';
SET password_encryption = 'scram';
CREATE ROLE regress_passwd4 PASSWORD 'role_pwd4';
SET password_encryption = 'plain';
CREATE ROLE regress_passwd5 PASSWORD NULL;
-- check list of created entries
SELECT rolname, rolpassword
    FROM pg_authid
    WHERE rolname LIKE 'regress_passwd%'
    ORDER BY rolname, rolpassword;

-- Rename a role
ALTER ROLE regress_passwd3 RENAME TO regress_passwd3_new;
-- md5 entry should have been removed
SELECT rolname, rolpassword
    FROM pg_authid
    WHERE rolname LIKE 'regress_passwd3_new'
    ORDER BY rolname, rolpassword;
ALTER ROLE regress_passwd3_new RENAME TO regress_passwd3;

-- ENCRYPTED and UNENCRYPTED passwords
ALTER ROLE regress_passwd1 UNENCRYPTED PASSWORD 'foo'; -- unencrypted
ALTER ROLE regress_passwd2 UNENCRYPTED PASSWORD 'md5deaeed29b1cf796ea981d53e82cd5856'; -- encrypted with MD5
ALTER ROLE regress_passwd3 ENCRYPTED PASSWORD 'foo'; -- encrypted with MD5
ALTER ROLE regress_passwd4 ENCRYPTED PASSWORD 'md5deaeed29b1cf796ea981d53e82cd5856'; -- encrypted with MD5
SELECT rolname, rolpassword
    FROM pg_authid
    WHERE rolname LIKE 'regress_passwd%'
    ORDER BY rolname, rolpassword;

-- PASSWORD val USING protocol
ALTER ROLE regress_passwd1 PASSWORD 'foo' USING 'non_existent';
ALTER ROLE regress_passwd1 PASSWORD 'md5deaeed29b1cf796ea981d53e82cd5856' USING 'plain'; -- ok, as md5
ALTER ROLE regress_passwd2 PASSWORD 'foo' USING 'plain'; -- ok, as plain
ALTER ROLE regress_passwd3 PASSWORD 'md5deaeed29b1cf796ea981d53e82cd5856' USING 'scram'; -- ok, as md5
ALTER ROLE regress_passwd4 PASSWORD 'kfSJjF3tdoxDNA==:4096:c52173111c7354ca17c66ba570e230ccec51c15c9f510b998d28297f723af5fa:a55cacd2a24bc2673c3d4266b8b90fa58231a674ae1b08e02236beba283fc2d5' USING 'plain'; -- ok, as scram
SELECT rolname, rolpassword
    FROM pg_authid
    WHERE rolname LIKE 'regress_passwd%'
    ORDER BY rolname, rolpassword;

DROP ROLE regress_passwd1;
DROP ROLE regress_passwd2;
DROP ROLE regress_passwd3;
DROP ROLE regress_passwd4;
DROP ROLE regress_passwd5;

-- all entries should have been removed
SELECT rolname, rolpassword
    FROM pg_authid
    WHERE rolname LIKE 'regress_passwd%'
    ORDER BY rolname, rolpassword;
