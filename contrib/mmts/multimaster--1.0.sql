-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION multimaster" to load this file. \quit

-- check that multimaster shared library is really loaded
DO $$
BEGIN
    IF strpos(current_setting('shared_preload_libraries'), 'multimaster') = 0 THEN
        RAISE EXCEPTION 'Multimaster must be loaded via shared_preload_libraries. Refusing to proceed.';
    END IF;
END
$$;

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

CREATE TYPE mtm.node_state AS ("id" integer, "enabled" bool, "connected" bool, "slot_active" bool, "stopped" bool, "catchUp" bool, "slotLag" bigint, "avgTransDelay" bigint, "lastStatusChange" timestamp, "oldestSnapshot" bigint, "SenderPid" integer, "SenderStartTime" timestamp, "ReceiverPid" integer, "ReceiverStartTime" timestamp, "connStr" text, "connectivityMask" bigint, "nHeartbeats" bigint);

CREATE FUNCTION mtm.get_nodes_state() RETURNS SETOF mtm.node_state
AS 'MODULE_PATHNAME','mtm_get_nodes_state'
LANGUAGE C;

CREATE TYPE mtm.cluster_state AS ("id" integer, "status" text, "disabledNodeMask" bigint, "disconnectedNodeMask" bigint, "catchUpNodeMask" bigint, "liveNodes" integer, "allNodes" integer, "nActiveQueries" integer, "nPendingQueries" integer, "queueSize" bigint, "transCount" bigint, "timeShift" bigint, "recoverySlot" integer,
"xidHashSize" bigint, "gidHashSize" bigint, "oldestXid" bigint, "configChanges" integer, "stalledNodeMask" bigint, "stoppedNodeMask" bigint, "deadNodeMask" bigint, "lastStatusChange" timestamp);

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

CREATE FUNCTION mtm.collect_cluster_info() RETURNS SETOF mtm.cluster_state 
AS 'MODULE_PATHNAME','mtm_collect_cluster_info'
LANGUAGE C;

CREATE FUNCTION mtm.make_table_local(relation regclass) RETURNS void
AS 'MODULE_PATHNAME','mtm_make_table_local'
LANGUAGE C;

CREATE FUNCTION mtm.broadcast_table(srcTable regclass, dstNodesMask bigint) RETURNS void
AS 'MODULE_PATHNAME','mtm_broadcast_table'
LANGUAGE C;

CREATE FUNCTION mtm.copy_table(srcTable regclass, dstNode integer) RETURNS void
AS 'MODULE_PATHNAME','mtm_copy_table'
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

CREATE FUNCTION mtm.referee_poll(xid bigint) RETURNS bigint
AS 'MODULE_PATHNAME','mtm_referee_poll'
LANGUAGE C;

CREATE TABLE IF NOT EXISTS mtm.local_tables(rel_schema text, rel_name text, primary key(rel_schema, rel_name));

CREATE OR REPLACE FUNCTION mtm.alter_sequences() RETURNS boolean AS 
$$
DECLARE
    seq_class record;
    seq_tuple record;
    node_id int;
    max_nodes int;
    new_start bigint;
    altered boolean := false;
BEGIN
    select current_setting('multimaster.max_nodes') into max_nodes;
    select id, "allNodes" into node_id from mtm.get_cluster_state();
    FOR seq_class IN
        SELECT '"' || ns.nspname || '"."' || seq.relname || '"' as seqname FROM pg_namespace ns,pg_class seq WHERE seq.relkind = 'S' and seq.relnamespace=ns.oid
    LOOP
            EXECUTE 'select * from ' || seq_class.seqname INTO seq_tuple;
            IF seq_tuple.increment_by != max_nodes THEN
                altered := true;
                RAISE NOTICE 'Altering step for sequence % to %.', seq_tuple.sequence_name, max_nodes;
                EXECUTE 'ALTER SEQUENCE ' || seq_class.seqname || ' INCREMENT BY ' || max_nodes || ';';
            END IF;
            IF (seq_tuple.last_value % max_nodes) != node_id THEN
                altered := true;
                new_start := (seq_tuple.last_value / max_nodes + 1)*max_nodes + node_id;
                RAISE NOTICE 'Altering start for sequence % to %.', seq_tuple.sequence_name, new_start;
                EXECUTE 'ALTER SEQUENCE ' || seq_class.seqname || ' RESTART WITH ' || new_start || ';';
            END IF;
    END LOOP;
    IF altered = false THEN
        RAISE NOTICE 'All found sequnces have proper params.';
    END IF;
    RETURN true;
END
$$
LANGUAGE plpgsql;

-- select mtm.alter_sequences();
