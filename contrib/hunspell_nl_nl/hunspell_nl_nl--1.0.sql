/* contrib/hunspell_nl_nl/hunspell_nl_nl--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hunspell_nl_nl" to load this file. \quit

CREATE TEXT SEARCH DICTIONARY dutch_hunspell (
    TEMPLATE = ispell,
    DictFile = nl_nl,
    AffFile = nl_nl,
    StopWords = dutch
);

COMMENT ON TEXT SEARCH DICTIONARY dutch_hunspell IS 'hunspell dictionary for dutch language';

CREATE TEXT SEARCH CONFIGURATION dutch_hunspell (
    COPY = simple
);

COMMENT ON TEXT SEARCH CONFIGURATION dutch_hunspell IS 'hunspell configuration for dutch language';

ALTER TEXT SEARCH CONFIGURATION dutch_hunspell
    ALTER MAPPING FOR asciiword, asciihword, hword_asciipart,
        word, hword, hword_part
    WITH dutch_hunspell, dutch_stem;
