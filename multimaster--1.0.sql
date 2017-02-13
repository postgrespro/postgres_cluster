-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION multimaster" to load this file. \quit

CREATE FUNCTION mtm.start_replication() RETURNS void
AS 'MODULE_PATHNAME','mtm_start_replication'
LANGUAGE C;

CREATE FUNCTION mtm.stop_replication() RETURNS void
AS 'MODULE_PATHNAME','mtm_stop_replication'
LANGUAGE C;

CREATE FUNCTION mtm.stop_node(node integer, drop_slot bool default false) RETURNS void
AS 'MODULE_PATHNAME','mtm_stop_node'
LANGUAGE C;

CREATE FUNCTION mtm.add_node(conn_str text) RETURNS void
AS 'MODULE_PATHNAME','mtm_add_node'
LANGUAGE C;

-- Create replication slot for the node which was previously stopped 
CREATE FUNCTION mtm.recover_node(node integer) RETURNS void
AS 'MODULE_PATHNAME','mtm_recover_node'
LANGUAGE C;


CREATE FUNCTION mtm.get_snapshot() RETURNS bigint
AS 'MODULE_PATHNAME','mtm_get_snapshot'
LANGUAGE C;

CREATE FUNCTION mtm.get_csn(xid bigint) RETURNS bigint
AS 'MODULE_PATHNAME','mtm_get_csn'
LANGUAGE C;

CREATE FUNCTION mtm.get_last_csn() RETURNS bigint
AS 'MODULE_PATHNAME','mtm_get_last_csn'
LANGUAGE C;


CREATE TYPE mtm.node_state AS ("id" integer, "disabled" bool, "disconnected" bool, "catchUp" bool, "slotLag" bigint, "avgTransDelay" bigint, "lastStatusChange" timestamp, "oldestSnapshot" bigint, "SenderPid" integer, "SenderStartTime" timestamp, "ReceiverPid" integer, "ReceiverStartTime" timestamp, "connStr" text, "connectivityMask" bigint, "stalled" bool, "stopped" bool, "nHeartbeats" bigint);

CREATE FUNCTION mtm.get_nodes_state() RETURNS SETOF mtm.node_state
AS 'MODULE_PATHNAME','mtm_get_nodes_state'
LANGUAGE C;

CREATE TYPE mtm.cluster_state AS ("status" text, "disabledNodeMask" bigint, "disconnectedNodeMask" bigint, "catchUpNodeMask" bigint, "liveNodes" integer, "allNodes" integer, "nActiveQueries" integer, "nPendingQueries" integer, "queueSize" bigint, "transCount" bigint, "timeShift" bigint, "recoverySlot" integer,
"xidHashSize" bigint, "gidHashSize" bigint, "oldestXid" bigint, "configChanges" integer, "stalledNodeMask" bigint, "stoppedNodeMask" bigint, "lastStatusChange" timestamp);

CREATE TYPE mtm.trans_state AS ("status" text, "gid" text, "xid" bigint, "coordinator" integer, "gxid" bigint, "csn" timestamp, "snapshot" timestamp, "local" boolean, "prepared" boolean, "active" boolean, "twophase" boolean, "votingCompleted" boolean, "participants" bigint, "voted" bigint, "configChanges" integer);

CREATE FUNCTION mtm.get_trans_by_gid(git text) RETURNS mtm.trans_state
AS 'MODULE_PATHNAME','mtm_get_trans_by_gid'
LANGUAGE C;

CREATE FUNCTION mtm.get_trans_by_xid(xid bigint) RETURNS mtm.trans_state
AS 'MODULE_PATHNAME','mtm_get_trans_by_xid'
LANGUAGE C;

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

CREATE FUNCTION mtm.inject_2pc_error(stage integer) RETURNS void
AS 'MODULE_PATHNAME','mtm_inject_2pc_error'
LANGUAGE C;

CREATE FUNCTION mtm.check_deadlock(xid bigint) RETURNS boolean
AS 'MODULE_PATHNAME','mtm_check_deadlock'
LANGUAGE C;

CREATE TABLE IF NOT EXISTS mtm.local_tables(rel_schema text, rel_name text, primary key(rel_schema, rel_name));

