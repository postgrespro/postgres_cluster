-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION multimaster" to load this file. \quit

CREATE FUNCTION mtm.start_replication() RETURNS void
AS 'MODULE_PATHNAME','mtm_start_replication'
LANGUAGE C;

CREATE FUNCTION mtm.stop_replication() RETURNS void
AS 'MODULE_PATHNAME','mtm_stop_replication'
LANGUAGE C;

CREATE FUNCTION mtm.drop_node(node integer, drop_slot bool default false) RETURNS void
AS 'MODULE_PATHNAME','mtm_drop_node'
LANGUAGE C;

CREATE FUNCTION mtm.add_node(conn_str cstring) RETURNS void
AS 'MODULE_PATHNAME','mtm_add_node'
LANGUAGE C;

-- Create replication slot for the node which was previously dropped together with it's slot 
CREATE FUNCTION mtm.recover_node(node integer) RETURNS void
AS 'MODULE_PATHNAME','mtm_recover_node'
LANGUAGE C;


CREATE FUNCTION mtm.get_snapshot() RETURNS bigint
AS 'MODULE_PATHNAME','mtm_get_snapshot'
LANGUAGE C;


CREATE TYPE mtm.node_state AS ("id" integer, "disabled" bool, "disconnected" bool, "catchUp" bool, "slotLag" bigint, "avgTransDelay" bigint, "lastStatusChange" timestamp, "oldestSnapshot" bigint, "connStr" text);

CREATE FUNCTION mtm.get_nodes_state() RETURNS SETOF mtm.node_state
AS 'MODULE_PATHNAME','mtm_get_nodes_state'
LANGUAGE C;

CREATE TYPE mtm.cluster_state AS ("status" text, "disabledNodeMask" bigint, "disconnectedNodeMask" bigint, "catchUpNodeMask" bigint, "liveNodes" integer, "allNodes" integer, "nActiveQueries" integer, "nPendingQueries" integer, "queueSize" bigint, "transCount" bigint, "timeShift" bigint, "recoverySlot" integer,
"xidHashSize" bigint, "gidHashSize" bigint, "oldestXid" integer, "configChanges" integer);

CREATE FUNCTION mtm.get_cluster_state() RETURNS mtm.cluster_state 
AS 'MODULE_PATHNAME','mtm_get_cluster_state'
LANGUAGE C;

CREATE FUNCTION mtm.get_cluster_info() RETURNS SETOF mtm.cluster_state 
AS 'MODULE_PATHNAME','mtm_get_cluster_info'
LANGUAGE C;

CREATE FUNCTION mtm.make_table_local(relation regclass) RETURNS void
AS 'MODULE_PATHNAME','mtm_make_table_local'
LANGUAGE C;

CREATE FUNCTION mtm.dump_lock_graph() RETURNS text
AS 'MODULE_PATHNAME','mtm_dump_lock_graph'
LANGUAGE C;

CREATE FUNCTION mtm.poll_node(nodeId integer, noWait boolean default FALSE) RETURNS boolean
AS 'MODULE_PATHNAME','mtm_poll_node'
LANGUAGE C;

CREATE TABLE IF NOT EXISTS mtm.ddl_log (issued timestamp with time zone not null, query text);

CREATE TABLE IF NOT EXISTS mtm.local_tables(rel_schema text, rel_name text, primary key(rel_schema, rel_name));

