#ifndef RAFT_H
#define RAFT_H

#define NOBODY -1

typedef struct raft_data_t *raft_t;
typedef struct raft_msg_data_t *raft_msg_t;

typedef int raft_bool_t;

typedef struct raft_update_t {
	int len;
	char *data;
	void *userdata; /* use this to track which query caused this update */
} raft_update_t;

/* --- Callbacks --- */

/*
 * This should be a function that applies an 'update' to the state machine.
 * 'snapshot' is true if 'update' contains a snapshot. 'userdata' is the
 * userdata that raft was configured with.
 */
typedef void (*raft_applier_t)(void *userdata, raft_update_t update, raft_bool_t snapshot);

/*
 * This should be a function that makes a snapshot of the state machine. Used
 * for raft log compaction. 'userdata' is the userdata that raft was configured
 * with.
 */
typedef raft_update_t (*raft_snapshooter_t)(void *userdata);

/* --- Configuration --- */

typedef struct raft_config_t {
	int peernum_max;

	int heartbeat_ms;
	int election_ms_min;
	int election_ms_max;

	int log_len;

	int chunk_len;
	int msg_len_max;

	void *userdata; /* this will get passed to applier() and snapshooter() */
	raft_applier_t applier;
	raft_snapshooter_t snapshooter;
} raft_config_t;

/*
 * Initialize a raft instance. Returns NULL on failure.
 */
raft_t raft_init(raft_config_t *config);

/*
 * Add a peer named 'id'. 'self' should be true, if that peer is this instance.
 * Only one peer should have 'self' == true.
 */
raft_bool_t raft_peer_up(raft_t r, int id, char *host, int port, raft_bool_t self);

/*
 * Returns the number of entried applied by the leader.
 */
int raft_progress(raft_t r);

/*
 * Remove a previously added peer named 'id'.
 */
raft_bool_t raft_peer_down(raft_t r, int id);

/* --- Log Actions --- */

/*
 * Emit an 'update'. Returns the log index if emitted successfully, or -1
 * otherwise.
 */
int raft_emit(raft_t r, raft_update_t update);

/*
 * Checks whether an entry at 'index' has been applied by the peer named 'id'.
 */
raft_bool_t raft_applied(raft_t t, int id, int index);

/* --- Control --- */

/*
 * Note, that UDP socket and raft messages are exposed to the user. This gives
 * the user the opportunity to incorporate the socket with other sockets in
 * select() or poll(). Thus, the messages will be processed as soon as they
 * come, not as soon as we call raft_tick().
 */

/*
 * Perform various raft logic tied to time. Call this function once in a while
 * and pass the elapsed 'msec' from the previous call. This function will only
 * trigger time-related events, and will not receive and process messages (see
 * the note above).
 */
void raft_tick(raft_t r, int msec);

/*
 * Receive a raft message. Returns NULL if no message available.
 */
raft_msg_t raft_recv_message(raft_t r);

/*
 * Process the message.
 */
void raft_handle_message(raft_t r, raft_msg_t m);

/*
 * Create the raft socket.
 */
int raft_create_udp_socket(raft_t r);

/*
 * Returns true if this peer thinks it is the leader.
 */
raft_bool_t raft_is_leader(raft_t r);

/*
 * Returns the id of the current leader, or NOBODY if no leader.
 */
int raft_get_leader(raft_t r);

#endif
