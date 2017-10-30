-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION referee" to load this file. \quit

CREATE TABLE IF NOT EXISTS referee.decision(key text primary key not null, node_id int);

CREATE OR REPLACE FUNCTION referee.get_winner(applicant_id int) RETURNS int AS
$$
DECLARE
    winner_id int;
BEGIN
    insert into referee.decision values ('winner', applicant_id);
    select node_id into winner_id from referee.decision where key = 'winner';
    return winner_id;
EXCEPTION WHEN others THEN
    select node_id into winner_id from referee.decision where key = 'winner';
    return winner_id;
END
$$
LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION referee.clean() RETURNS bool AS
$$
BEGIN
    delete from referee.decision where key = 'winner';
    return 'true';
END
$$
LANGUAGE plpgsql;
