/* pg_tsparser/pg_tsparser--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_tsparser" to load this file. \quit

CREATE OR REPLACE FUNCTION tsparser_start(internal, int4)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION tsparser_nexttoken(internal, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION tsparser_end(internal)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION tsparser_lextype(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION tsparser_headline(internal, internal, tsquery)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE TEXT SEARCH PARSER tsparser (
	START    = tsparser_start,
	GETTOKEN = tsparser_nexttoken,
	END      = tsparser_end,
	HEADLINE = tsparser_headline,
	LEXTYPES = tsparser_lextype
);

COMMENT ON TEXT SEARCH PARSER tsparser IS 'parser for text search';
