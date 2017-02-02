CREATE EXTENSION pg_tsparser;

SELECT * FROM ts_token_type('tsparser');

SELECT * FROM ts_parse('tsparser', '345 qwe@efd.r '' http://www.com/ http://aew.werc.ewr/?ad=qwe&dw 1aew.werc.ewr/?ad=qwe&dw 2aew.werc.ewr http://3aew.werc.ewr/?ad=qwe&dw http://4aew.werc.ewr http://5aew.werc.ewr:8100/?  ad=qwe&dw 6aew.werc.ewr:8100/?ad=qwe&dw 7aew.werc.ewr:8100/?ad=qwe&dw=%20%32 +4.0e-10 qwe qwe qwqwe 234.435 455 5.005 teodor@stack.net teodor@123-stack.net 123_teodor@stack.net 123-teodor@stack.net qwe-wer asdf <fr>qwer jf sdjk<we hjwer <werrwe> ewr1> ewri2 <a href="qwe<qwe>">
/usr/local/fff /awdf/dwqe/4325 rewt/ewr wefjn /wqe-324/ewr gist.h gist.h.c gist.c. readline 4.2 4.2. 4.2, readline-4.2 readline-4.2. 234
<i <b> wow  < jqw <> qwerty');

-- Test text search configuration with parser
CREATE TEXT SEARCH CONFIGURATION english_ts (
    PARSER = tsparser
);

ALTER TEXT SEARCH CONFIGURATION english_ts
    ADD MAPPING FOR email, file, float, host, hword_numpart, int,
    numhword, numword, sfloat, uint, url, url_path, version
    WITH simple;

ALTER TEXT SEARCH CONFIGURATION english_ts
    ADD MAPPING FOR asciiword, asciihword, hword_asciipart,
    word, hword, hword_part
    WITH english_stem;

SELECT to_tsvector('english_ts', 'pg_trgm');
