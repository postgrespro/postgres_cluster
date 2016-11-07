/* contrib/pg_buffercache/pg_buffercache--1.2--1.3.sql */

-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION pg_buffercache(text) UPDATE TO '1.3'" to load this file. \quit

ALTER FUNCTION pg_buffercache_pages() PARALLEL SAFE;
