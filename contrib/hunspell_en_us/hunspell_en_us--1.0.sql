/* contrib/hunspell_en_us/hunspell_en_us--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hunspell_en_us" to load this file. \quit

CREATE TEXT SEARCH DICTIONARY english_hunspell (
    TEMPLATE = ispell,
    DictFile = en_us,
    AffFile = en_us,
    StopWords = english
);

COMMENT ON TEXT SEARCH DICTIONARY english_hunspell IS 'hunspell dictionary for english language';

CREATE TEXT SEARCH CONFIGURATION english_hunspell (
    COPY = simple
);

COMMENT ON TEXT SEARCH CONFIGURATION english_hunspell IS 'hunspell configuration for english language';

ALTER TEXT SEARCH CONFIGURATION english_hunspell
    ALTER MAPPING FOR asciiword, asciihword, hword_asciipart,
        word, hword, hword_part
    WITH english_hunspell, english_stem;
