CREATE OR REPLACE FUNCTION shared_ispell_init(internal)
	RETURNS internal
	AS 'MODULE_PATHNAME', 'dispell_init'
	LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION shared_ispell_lexize(internal,internal,internal,internal)
	RETURNS internal
	AS 'MODULE_PATHNAME', 'dispell_lexize'
	LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION shared_ispell_reset()
	RETURNS void
	AS 'MODULE_PATHNAME', 'dispell_reset'
	LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION shared_ispell_mem_used()
	RETURNS integer
	AS 'MODULE_PATHNAME', 'dispell_mem_used'
	LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION shared_ispell_mem_available()
	RETURNS integer
	AS 'MODULE_PATHNAME', 'dispell_mem_available'
	LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION shared_ispell_dicts( OUT dict_name VARCHAR, OUT affix_name VARCHAR, OUT words INT, OUT affixes INT, OUT bytes INT)
    RETURNS SETOF record
    AS 'MODULE_PATHNAME', 'dispell_list_dicts'
    LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION shared_ispell_stoplists( OUT stop_name VARCHAR, OUT words INT, OUT bytes INT)
    RETURNS SETOF record
    AS 'MODULE_PATHNAME', 'dispell_list_stoplists'
    LANGUAGE C IMMUTABLE;

CREATE TEXT SEARCH TEMPLATE shared_ispell (
    INIT = shared_ispell_init,
    LEXIZE = shared_ispell_lexize
);

/*
CREATE TEXT SEARCH DICTIONARY czech_shared (
	TEMPLATE = shared_ispell,
	DictFile = czech,
	AffFile = czech,
	StopWords = czech
);

CREATE TEXT SEARCH CONFIGURATION public.czech_shared ( COPY = pg_catalog.simple );

ALTER TEXT SEARCH CONFIGURATION czech_shared
    ALTER MAPPING FOR asciiword, asciihword, hword_asciipart,
                      word, hword, hword_part
    WITH czech_shared;
*/