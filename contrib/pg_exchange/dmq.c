/*----------------------------------------------------------------------------
 *
 * dmq.c
 *	  Distributed message queue.
 *
 *
 *	  Backend to remote backend messaging with memqueue-like interface.
 * COPY protocol is used as a transport to avoid unnecessary overhead of sql
 * parsing.
 *	  Sender is a custom bgworker that starts with postgres, can open multiple
 * remote connections and keeps open memory queue with each ordinary backend.
 * It's a sender responsibility to establish a connection with a remote
 * counterpart. Sender can send heartbeats to allow early detection of dead
 * connections. Also it can stubbornly try to reestablish dead connection.
 *	  Receiver is an ordinary backend spawned by a postmaster upon sender
 * connection request. As it first call sender will call dmq_receiver_loop()
 * function that will switch fe/be protocol to a COPY mode and enters endless
 * receiving loop.
 *
 * XXX: needs better PQerror reporting logic -- perhaps once per given Idle
 * connection.
 *
 * XXX: is there max size for a connstr?
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *----------------------------------------------------------------------------
 */
#include "postgres.h"

#include "dmq.h"
#include "logger.h"

#include "access/transam.h"
#include "libpq/libpq.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "executor/executor.h"
#include "utils/builtins.h"
#include "utils/timestamp.h"
#include "storage/shm_toc.h"
#include "storage/shm_mq.h"
#include "storage/ipc.h"
#include "tcop/tcopprot.h"
#include "utils/dynahash.h"
#include "utils/ps_status.h"

//#define DMQ_MQ_SIZE  ((Size) 65536)
#define DMQ_MQ_SIZE  ((Size) 1048576) /* 1 MB */
//#define DMQ_MQ_SIZE  ((Size) 8388608) /* 8 MB */

#define DMQ_MQ_MAGIC 0x646d71

// XXX: move to common
#define BIT_CHECK(mask, bit) (((mask) & ((int64)1 << (bit))) != 0)

/*
 * Shared data structures to hold current connections topology.
 * All that stuff can be moved to persistent tables to avoid hardcoded
 * size limits, but now it doesn't seems to be worth of troubles.
 */

#define DMQ_CONNSTR_MAX_LEN 1024

#define DMQ_MAX_SUBS_PER_BACKEND 100

typedef struct {
	bool			active;
	char			sender_name[DMQ_NAME_MAXLEN];
	char			receiver_name[DMQ_NAME_MAXLEN];
	char			connstr[DMQ_CONNSTR_MAX_LEN];
	int				ping_period;
	PGconn		   *pgconn;
	DmqConnState	state;
	int				pos;
} DmqDestination;

typedef struct
{
	char	stream_name[DMQ_NAME_MAXLEN];
	int		mask_pos;
	int		procno;
} DmqStreamSubscription;


/* Global state for dmq */
struct DmqSharedState
{
	LWLock		   *lock;

	/* sender stuff */
	pid_t			sender_pid;
	dsm_handle	    out_dsm;
	DmqDestination  destinations[DMQ_MAX_DESTINATIONS];

	/* receivers stuff */
	HTAB		   *subscriptions;
	int				n_receivers;
	struct
	{
		char		name[DMQ_NAME_MAXLEN];
		dsm_handle	dsm_h;
		int			procno;
		bool		active;
		pid_t		pid;
	} receivers[DMQ_MAX_RECEIVERS];

} *dmq_state;


/* Backend-local i/o queues. */
static struct
{
	shm_mq_handle  *mq_outh;
	int				n_inhandles;
	struct
	{
		dsm_segment	   *dsm_seg;
		shm_mq_handle  *mqh;
		char			name[DMQ_NAME_MAXLEN];
		int				mask_pos;
	} inhandles[DMQ_MAX_RECEIVERS];
} dmq_local;

/* Flags set by signal handlers */
static volatile sig_atomic_t got_SIGHUP = false;

static shmem_startup_hook_type PreviousShmemStartupHook;

dmq_receiver_hook_type dmq_receiver_start_hook;
dmq_receiver_hook_type dmq_receiver_stop_hook;

int dmq_heartbeat_timeout;
void dmq_sender_main(Datum main_arg);

PG_FUNCTION_INFO_V1(dmq_receiver_loop);

/*****************************************************************************
 *
 * Helpers
 *
 *****************************************************************************/

static double
dmq_now(void)
{
	instr_time cur_time;
	INSTR_TIME_SET_CURRENT(cur_time);
	return INSTR_TIME_GET_MILLISEC(cur_time);
}

/*****************************************************************************
 *
 * Initialization
 *
 *****************************************************************************/

/* SIGHUP: set flag to reload configuration at next convenient time */
static void
dmq_sighup_handler(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_SIGHUP = true;

	/* Waken anything waiting on the process latch */
	SetLatch(MyLatch);

	errno = save_errno;
}

/*
 * Set pointer to dmq shared state in all backends.
 */
static void
dmq_shmem_startup_hook(void)
{
	bool found;
	HASHCTL		hash_info;

	if (PreviousShmemStartupHook)
		PreviousShmemStartupHook();

	MemSet(&hash_info, 0, sizeof(hash_info));
	hash_info.keysize = DMQ_NAME_MAXLEN;
	hash_info.entrysize = sizeof(DmqStreamSubscription);

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

	dmq_state = ShmemInitStruct("dmq",
								sizeof(struct DmqSharedState),
								&found);
	if (!found)
	{
		int i;

		dmq_state->lock = &(GetNamedLWLockTranche("dmq"))->lock;
		dmq_state->out_dsm = DSM_HANDLE_INVALID;
		memset(dmq_state->destinations, '\0', sizeof(DmqDestination)*DMQ_MAX_DESTINATIONS);

		dmq_state->sender_pid = 0;
		dmq_state->n_receivers = 0;
		for (i = 0; i < DMQ_MAX_RECEIVERS; i++)
		{
			dmq_state->receivers[i].name[0] = '\0';
			dmq_state->receivers[i].dsm_h = DSM_HANDLE_INVALID;
			dmq_state->receivers[i].procno = -1;
			dmq_state->receivers[i].active = false;
		}
	}

	dmq_state->subscriptions = ShmemInitHash("dmq_stream_subscriptions",
								DMQ_MAX_SUBS_PER_BACKEND*MaxBackends,
								DMQ_MAX_SUBS_PER_BACKEND*MaxBackends,
								&hash_info,
								HASH_ELEM);

	LWLockRelease(AddinShmemInitLock);
}

static Size
dmq_shmem_size(void)
{
	Size	size = 0;

//	size = add_size(size, DMQ_MQ_SIZE * DMQ_MAX_DESTINATIONS * 2);
	size = add_size(size, sizeof(struct DmqSharedState));
	size = add_size(size, hash_estimate_size(DMQ_MAX_SUBS_PER_BACKEND*MaxBackends,
											 sizeof(DmqStreamSubscription)));
	return MAXALIGN(size);
}

/*
 * Register background worker and hooks.
 * library_name -  the name of a library in which the initial entry point for
 * the background worker should be sought. See background workers manual for
 * details.
 */
void
dmq_init(const char *library_name)
{
	BackgroundWorker worker;

	if (!process_shared_preload_libraries_in_progress)
		return;

	/* Reserve area for our shared state */
	RequestAddinShmemSpace(dmq_shmem_size());

	RequestNamedLWLockTranche("dmq", 1);

	/* Set up common data for all our workers */
	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = 5;
	worker.bgw_notify_pid = 0;
	worker.bgw_main_arg = 0;
	sprintf(worker.bgw_library_name, "%s", library_name);
	sprintf(worker.bgw_function_name, "dmq_sender_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "dmq-sender");
	snprintf(worker.bgw_type, BGW_MAXLEN, "dmq-sender");
	RegisterBackgroundWorker(&worker);

	/* Register shmem hooks */
	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = dmq_shmem_startup_hook;
}

static Size
dmq_toc_size()
{
	int i;
	shm_toc_estimator e;

	shm_toc_initialize_estimator(&e);
	for (i = 0; i < MaxBackends; i++)
		shm_toc_estimate_chunk(&e, DMQ_MQ_SIZE);
	shm_toc_estimate_keys(&e, MaxBackends);
	return shm_toc_estimate(&e);
}


/*****************************************************************************
 *
 * Sender
 *
 *****************************************************************************/

static int
fe_send(PGconn *conn, char *msg, size_t len)
{
	if (PQputCopyData(conn, msg, len) < 0)
		return -1;

	// XXX: move PQflush out of the loop?
	if (PQflush(conn) < 0)
		return -1;

	return 0;
}

static void
dmq_sender_at_exit(int status, Datum arg)
{
	int		i;

	LWLockAcquire(dmq_state->lock, LW_SHARED);
	for (i = 0; i < dmq_state->n_receivers; i++)
	{
		if (dmq_state->receivers[i].active &&
			dmq_state->receivers[i].pid > 0)
		{
			kill(dmq_state->receivers[i].pid, SIGTERM);
		}
	}
	LWLockRelease(dmq_state->lock);
}

static void
switch_destination_state(DmqDestinationId dest_id, DmqConnState state)
{
	DmqDestination *dest;

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	dest = &(dmq_state->destinations[dest_id]);
	Assert(dest->active);

	dest->state = state;
	LWLockRelease(dmq_state->lock);
}

void
dmq_sender_main(Datum main_arg)
{
	int		i;
	dsm_segment	   *seg;
	shm_toc		   *toc;
	shm_mq_handle **mq_handles;
	WaitEventSet   *set;
	DmqDestination	conns[DMQ_MAX_DESTINATIONS];

	double	prev_timer_at = dmq_now();

	on_shmem_exit(dmq_sender_at_exit, (Datum) 0);

	/* init this worker */
	pqsignal(SIGHUP, dmq_sighup_handler);
	pqsignal(SIGTERM, die);
	BackgroundWorkerUnblockSignals();

	/* setup queue receivers */
	seg = dsm_create(dmq_toc_size(), 0);
	dsm_pin_segment(seg);
	toc = shm_toc_create(DMQ_MQ_MAGIC, dsm_segment_address(seg),
						 dmq_toc_size());
	mq_handles = palloc(MaxBackends*sizeof(shm_mq_handle *));
	for (i = 0; i < MaxBackends; i++)
	{
		shm_mq	   *mq;
		mq = shm_mq_create(shm_toc_allocate(toc, DMQ_MQ_SIZE), DMQ_MQ_SIZE);
		shm_toc_insert(toc, i, mq);
		shm_mq_set_receiver(mq, MyProc);
		mq_handles[i] = shm_mq_attach(mq, seg, NULL);
	}

	for (i = 0; i < DMQ_MAX_DESTINATIONS; i++)
	{
		conns[i].active = false;
	}

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	dmq_state->sender_pid = MyProcPid;
	dmq_state->out_dsm = dsm_segment_handle(seg);
	LWLockRelease(dmq_state->lock);

	set = CreateWaitEventSet(CurrentMemoryContext, 15);
	AddWaitEventToSet(set, WL_POSTMASTER_DEATH, PGINVALID_SOCKET, NULL, NULL);
	AddWaitEventToSet(set, WL_LATCH_SET, PGINVALID_SOCKET, MyLatch, NULL);

	got_SIGHUP = true;

	for (;;)
	{
		WaitEvent	event;
		int			nevents;
		bool		wait = true;
		double		now_millisec;
		bool		timer_event = false;

		if (ProcDiePending)
			break;

		/*
		 * Read new connections from shared memory.
		 */
		if (got_SIGHUP)
		{
			got_SIGHUP = false;

			LWLockAcquire(dmq_state->lock, LW_SHARED);
			for (i = 0; i < DMQ_MAX_DESTINATIONS; i++)
			{
				DmqDestination *dest = &(dmq_state->destinations[i]);
				/* start connection for a freshly added destination */
				if (dest->active && !conns[i].active)
				{
					conns[i] = *dest;
					Assert(conns[i].pgconn == NULL);
					conns[i].state = Idle;
					dest->state = Idle;
					prev_timer_at = 0; /* do not wait for timer event */
				}
				/* close connection to deleted destination */
				else if (!dest->active && conns[i].active)
				{
					PQfinish(conns[i].pgconn);
					conns[i].active = false;
					conns[i].pgconn = NULL;
				}
			}
			LWLockRelease(dmq_state->lock);
		}

		/*
		 * Transfer data from backend queues to their remote counterparts.
		 */
		for (i = 0; i < MaxBackends; i++)
		{
			void	   *data;
			Size		len;
			shm_mq_result res;

			res = shm_mq_receive(mq_handles[i], &len, &data, true);
			if (res == SHM_MQ_SUCCESS)
			{
				DmqDestinationId conn_id;

				/* first byte is connection_id */
				conn_id = * (DmqDestinationId *) data;
				data = (char *) data + sizeof(DmqDestinationId);
				len -= sizeof(DmqDestinationId);
				Assert(0 <= conn_id && conn_id < DMQ_MAX_DESTINATIONS);

				if (conns[conn_id].state == Active)
				{
					int ret = fe_send(conns[conn_id].pgconn, data, len);

					if (ret < 0)
					{
						// Assert(PQstatus(conns[conn_id].pgconn) != CONNECTION_OK);
						conns[conn_id].state = Idle;
						switch_destination_state(conn_id, Idle);
						// DeleteWaitEvent(set, conns[conn_id].pos);

						mtm_log(DmqStateFinal,
								"[DMQ] failed to send message to %s: %s",
								conns[conn_id].receiver_name,
								PQerrorMessage(conns[conn_id].pgconn));
					}
					else
					{
						mtm_log(DmqTraceOutgoing,
								"[DMQ] sent message (l=%zu, m=%s) to %s",
								len, (char *) data, conns[conn_id].receiver_name);
					}
				}
				else
				{
					mtm_log(WARNING,
						 "[DMQ] dropping message (l=%zu, m=%s) to disconnected %s",
						 len, (char *) data, conns[conn_id].receiver_name);
				}

				wait = false;
			}
			else if (res == SHM_MQ_DETACHED)
			{
				shm_mq	   *mq = shm_mq_get_queue(mq_handles[i]);

				/*
				 * Overwrite old mq struct since mq api don't have a way to reattach
				 * detached queue.
				 */
				shm_mq_detach(mq_handles[i]);
				mq = shm_mq_create(mq, DMQ_MQ_SIZE);
				shm_mq_set_receiver(mq, MyProc);
				mq_handles[i] = shm_mq_attach(mq, seg, NULL);

				mtm_log(DmqTraceShmMq,
						"[DMQ] sender reattached shm_mq to procno %d", i);
			}
		}

		/*
		 * Generate timeout or socket events.
		 *
		 *
		 * XXX: here we expect that whole cycle takes less then 250-100 ms.
		 * Otherwise we can stuck with timer_event forever.
		 */
		now_millisec = dmq_now();
		if (now_millisec - prev_timer_at > 250)
		{
			prev_timer_at = now_millisec;
			timer_event = true;
		}
		else
		{
			nevents = WaitEventSetWait(set, wait ? 100 : 0, &event,
									   1, PG_WAIT_EXTENSION);
		}

		/*
		 * Handle timer event: reconnect previously broken connection or
		 * send hearbeats.
		 */
		if (timer_event)
		{
			uintptr_t		conn_id;

			for (conn_id = 0; conn_id < DMQ_MAX_DESTINATIONS; conn_id++)
			{
				if (!conns[conn_id].active)
					continue;

				/* Idle --> Connecting */
				if (conns[conn_id].state == Idle)
				{
					double		pqtime;

					if (conns[conn_id].pgconn)
						PQfinish(conns[conn_id].pgconn);

					pqtime = dmq_now();

					conns[conn_id].pgconn = PQconnectStart(conns[conn_id].connstr);
					mtm_log(DmqPqTiming, "[DMQ] [TIMING] pqs = %f ms", dmq_now() - pqtime);

					if (PQstatus(conns[conn_id].pgconn) == CONNECTION_BAD)
					{
						conns[conn_id].state = Idle;
						switch_destination_state(conn_id, Idle);

						mtm_log(DmqStateIntermediate,
								"[DMQ] failed to start connection with %s (%s): %s",
								conns[conn_id].receiver_name,
								conns[conn_id].connstr,
								PQerrorMessage(conns[conn_id].pgconn));
					}
					else
					{
						conns[conn_id].state = Connecting;
						switch_destination_state(conn_id, Connecting);
						conns[conn_id].pos = AddWaitEventToSet(set, WL_SOCKET_CONNECTED,
											PQsocket(conns[conn_id].pgconn),
											NULL, (void *) conn_id);

						mtm_log(DmqStateIntermediate,
								"[DMQ] switching %s from Idle to Connecting on '%s'",
								conns[conn_id].receiver_name,
								conns[conn_id].connstr);
					}
				}
				/* Heatbeat */
				else if (conns[conn_id].state == Active)
				{
					int ret = fe_send(conns[conn_id].pgconn, "H", 2);
					if (ret < 0)
					{
						conns[conn_id].state = Idle;
						switch_destination_state(conn_id, Idle);
						// DeleteWaitEvent(set, conns[conn_id].pos);
						// Assert(PQstatus(conns[i].pgconn) != CONNECTION_OK);

						mtm_log(DmqStateFinal,
								"[DMQ] failed to send heartbeat to %s: %s",
								conns[conn_id].receiver_name,
								PQerrorMessage(conns[conn_id].pgconn));
					}
				}
			}
		}
		/*
		 * Handle all the connection machinery: consequently go through
		 * Connecting --> Negotiating --> Active states.
		 */
		else if (nevents > 0 && event.events & WL_SOCKET_MASK)
		{
			uintptr_t conn_id = (uintptr_t) event.user_data;

			Assert(conns[conn_id].active);

			switch (conns[conn_id].state)
			{
				case Idle:
					Assert(false);
					break;

				/* Await for connection establishment and call dmq_receiver_loop() */
				case Connecting:
				{
					double		pqtime;
					PostgresPollingStatusType status;

					pqtime = dmq_now();
					status = PQconnectPoll(conns[conn_id].pgconn);
					mtm_log(DmqPqTiming, "[DMQ] [TIMING] pqp = %f ms", dmq_now() - pqtime);

					mtm_log(DmqStateIntermediate,
							"[DMQ] Connecting: PostgresPollingStatusType = %d on %s",
							status,
							conns[conn_id].receiver_name);

					if (status == PGRES_POLLING_READING)
					{
						ModifyWaitEvent(set, event.pos, WL_SOCKET_READABLE, NULL);
						mtm_log(DmqStateIntermediate,
								"[DMQ] Connecting: modify wait event to WL_SOCKET_READABLE on %s",
								conns[conn_id].receiver_name);
					}
					else if (status == PGRES_POLLING_WRITING)
					{
						ModifyWaitEvent(set, event.pos, WL_SOCKET_WRITEABLE, NULL);
						mtm_log(DmqStateIntermediate,
								"[DMQ] Connecting: modify wait event to WL_SOCKET_WRITEABLE on %s",
								conns[conn_id].receiver_name);
					}
					else if (status == PGRES_POLLING_OK)
					{
						char *sender_name = conns[conn_id].sender_name;
						char *query = psprintf("select dmq_receiver_loop('%s')",
											   sender_name);

						conns[conn_id].state = Negotiating;
						switch_destination_state(conn_id, Negotiating);
						ModifyWaitEvent(set, event.pos, WL_SOCKET_READABLE, NULL);
						PQsendQuery(conns[conn_id].pgconn, query);

						mtm_log(DmqStateIntermediate,
								"[DMQ] switching %s from Connecting to Negotiating",
								conns[conn_id].receiver_name);
					}
					else if (status == PGRES_POLLING_FAILED)
					{
						conns[conn_id].state = Idle;
						switch_destination_state(conn_id, Idle);
						DeleteWaitEvent(set, event.pos);

						mtm_log(DmqStateIntermediate,
								"[DMQ] failed to connect with %s (%s): %s",
								conns[conn_id].receiver_name,
								conns[conn_id].connstr,
								PQerrorMessage(conns[conn_id].pgconn));
					}
					else
						Assert(false);

					break;
				}

				/*
				 * Await for response to dmq_receiver_loop() call and switch to
				 * active state.
				 */
				case Negotiating:
					Assert(event.events & WL_SOCKET_READABLE);
					if (!PQconsumeInput(conns[conn_id].pgconn))
					{
						conns[conn_id].state = Idle;
						switch_destination_state(conn_id, Idle);
						DeleteWaitEvent(set, event.pos);

						mtm_log(DmqStateIntermediate,
								"[DMQ] failed to get handshake from %s: %s",
								conns[conn_id].receiver_name,
								PQerrorMessage(conns[i].pgconn));
					}
					if (!PQisBusy(conns[conn_id].pgconn))
					{
						conns[conn_id].state = Active;
						switch_destination_state(conn_id, Active);
						DeleteWaitEvent(set, event.pos);

						mtm_log(DmqStateFinal,
								"[DMQ] Connected to %s",
								conns[conn_id].receiver_name);
					}
					break;

				/* Do nothing and check that connection is still alive */
				case Active:
					Assert(event.events & WL_SOCKET_READABLE);
					if (!PQconsumeInput(conns[conn_id].pgconn))
					{
						conns[conn_id].state = Idle;
						switch_destination_state(conn_id, Idle);

						mtm_log(DmqStateFinal,
								"[DMQ] connection error with %s: %s",
								conns[conn_id].receiver_name,
								PQerrorMessage(conns[conn_id].pgconn));
					}
					break;
			}
		}
		else if (nevents > 0 && event.events & WL_LATCH_SET)
		{
			ResetLatch(MyLatch);
		}
		else if (nevents > 0 && event.events & WL_POSTMASTER_DEATH)
		{
			proc_exit(1);
		}

		CHECK_FOR_INTERRUPTS();
	}
	FreeWaitEventSet(set);

}



/*****************************************************************************
 *
 * Receiver stuff
 *
 *****************************************************************************/

static void
dmq_handle_message(StringInfo msg, shm_mq_handle **mq_handles, dsm_segment *seg)
{
	const char *stream_name;
	const char *body;
	const char *msgptr;
	int 		body_len;
	int			msg_len;
	bool		found;
	DmqStreamSubscription *sub;
	shm_mq_result res;

	/*
	 * Consume stream_name packed as a string and interpret rest of the data
	 * as message body with unknown format that we are going to send down to
	 * the subscribed backend.
	 */
	msg_len = msg->len - msg->cursor;
	msgptr = pq_getmsgbytes(msg, msg_len);
	stream_name = msgptr;
	body = msgptr + strlen(stream_name) + 1;
	body_len = msg_len - (body - msgptr);
	pq_getmsgend(msg);

	/*
	 * Stream name "H" is reserved for simple heartbeats without body. So no
	 * need to send somewhere.
	 */
	if (strcmp(stream_name, "H") == 0)
	{
		Assert(body_len == 0);
		return;
	}

	/*
	 * Find subscriber.
	 * XXX: we can cache that and re-read shared memory upon a signal, but
	 * likely that won't show any measurable speedup.
	 */
	LWLockAcquire(dmq_state->lock, LW_SHARED);
	sub = (DmqStreamSubscription *) hash_search(dmq_state->subscriptions,
												stream_name, HASH_FIND,
												&found);
	LWLockRelease(dmq_state->lock);

	if (!found)
	{
		mtm_log(WARNING,
			"[DMQ] subscription %s is not found (body = %s)",
			stream_name, body);
		return;
	}

	mtm_log(DmqTraceIncoming,
			"[DMQ] got message %s.%s, passing to %d", stream_name, body,
			sub->procno);

	/* and send it */
	res = shm_mq_send(mq_handles[sub->procno], msg_len, msgptr, false);
	if (res != SHM_MQ_SUCCESS)
	{
		mtm_log(WARNING, "[DMQ] can't send to queue %d", sub->procno);
	}
}

/*
 * recv_buffer can be as large as possible. It is critical for message passing
 * effectiveness.
 */
#define DMQ_RECV_BUFFER (8388608) /* 8 MB */
static char recv_buffer[DMQ_RECV_BUFFER];
static int  recv_bytes;
static int  read_bytes;

/*
 * _pq_have_full_message.
 *
 * Check if our recv buffer has fully received message. Also left-justify
 * message in buffer if it doesn't fit in buffer starting from current
 * position.
 *
 * Return 1 and fill given StringInfo if there is message and return 0
 * otherwise.
 */
static int
_pq_have_full_message(StringInfo s)
{

	/* Have we got message length header? */
	if (recv_bytes - read_bytes >= 4)
	{
		int len = pg_ntoh32( * (int *) (recv_buffer + read_bytes) );

		Assert(len < DMQ_RECV_BUFFER);

		if (read_bytes + len <= recv_bytes)
		{
			/* Have full message, wrap it as StringInfo and return */
			s->data = recv_buffer + read_bytes;
			s->maxlen = s->len = len;
			s->cursor = 4;
			read_bytes += len;
			return 1;
		}
		else if (read_bytes + len >= DMQ_RECV_BUFFER)
		{
			memmove(recv_buffer, recv_buffer + read_bytes,
									recv_bytes - read_bytes);
			recv_bytes -= read_bytes;
			read_bytes = 0;
		}
	}

	return 0;
}

/*
 * _pq_getmessage_if_avalable()
 *
 * Get pq message in non-blocking mode. This uses it own recv buffer instead
 * of pqcomm one, since they are private to pqcomm.c.
 *
 * Returns 0 when no full message are available, 1 when we got message and EOF
 * in case of connection problems.
 *
 * Caller should not wait on latch after we've got message -- there can be
 * several of them in our buffer.
 */
static int
_pq_getmessage_if_avalable(StringInfo s)
{
	int rc;

	/* Check if we have full messages after previous call */
	if (_pq_have_full_message(s) > 0)
		return 1;

	if (read_bytes > 0)
	{
		if (recv_bytes == read_bytes)
		{
			/* no partially read messages, so just start over */
			read_bytes = recv_bytes = 0;
		}
		else
		{
			/*
			 * Move data to the left in case we are near the buffer end.
			 * Case when message starts earlier in the buffer and spans
			 * past it's end handled separately down the lines.
			 */
			Assert(recv_bytes > read_bytes);
			if (recv_bytes > (DMQ_RECV_BUFFER - 1024))
			{
				memmove(recv_buffer, recv_buffer + read_bytes,
										recv_bytes - read_bytes);
				recv_bytes -= read_bytes;
				read_bytes = 0;
			}
		}
	}

	/* defuse secure_read() from blocking */
	MyProcPort->noblock = true;

	rc = secure_read(MyProcPort, recv_buffer + recv_bytes,
						DMQ_RECV_BUFFER - recv_bytes);

	if (rc < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			return EOF;
	}
	else if (rc == 0)
	{
		return EOF;
	}
	else
	{
		recv_bytes += rc;
		Assert(recv_bytes >= read_bytes && recv_bytes <= DMQ_RECV_BUFFER);

		mtm_log(DmqTraceIncoming, "dmq: got %d bytes", rc);

		/*
		 * Here we need to re-check for full message again, so the caller will know
		 * whether he should wait for event on socket.
		 */
		if (_pq_have_full_message(s) > 0)
			return 1;
	}

	return 0;
}


/*
 * _pq_getbyte_if_available.
 *
 * Same as pq_getbyte_if_available(), but works with our recv buffer.
 *
 * 1 on success, 0 if no data available, EOF on connection error.
 */
static int
_pq_getbyte_if_available(unsigned char *c)
{
	int rc;

	/*
	 * That is why we re-implementing this function: byte can be already in
	 * our recv buffer, so pqcomm version will miss it.
	 */
	if (recv_bytes > read_bytes)
	{
		*c = recv_buffer[read_bytes++];
		return 1;
	}

	/* defuse secure_read() from blocking */
	MyProcPort->noblock = true;

	/* read directly into *c skipping recv_buffer */
	rc = secure_read(MyProcPort, c, 1);

	if (rc < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			return EOF;
	}
	else if (rc == 0)
	{
		return EOF;
	}
	else
	{
		Assert(rc == 1);
		return rc;
	}

	return 0;

}

static void
dmq_receiver_at_exit(int status, Datum receiver)
{
	int		receiver_id = DatumGetInt32(receiver);
	char	sender_name[DMQ_NAME_MAXLEN];

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	strncpy(sender_name, dmq_state->receivers[receiver_id].name,
			DMQ_NAME_MAXLEN);
	dmq_state->receivers[receiver_id].active = false;
	LWLockRelease(dmq_state->lock);

	if (dmq_receiver_stop_hook)
		dmq_receiver_stop_hook(sender_name);
}


Datum
dmq_receiver_loop(PG_FUNCTION_ARGS)
{
	enum
	{
		NeedByte,
		NeedMessage
	} reader_state = NeedByte;

	dsm_segment		   *seg;
	shm_toc			   *toc;
	StringInfoData		s;
	shm_mq_handle	  **mq_handles;
	char			   *sender_name;
	char			   *proc_name;
	int					i;
	int					receiver_id = -1;
	double				last_message_at = dmq_now();

	sender_name = text_to_cstring(PG_GETARG_TEXT_PP(0));

	proc_name = psprintf("mtm-dmq-receiver %s", sender_name);
	set_ps_display(proc_name, true);

	/* setup queues with backends */
	seg = dsm_create(dmq_toc_size(), 0);
	dsm_pin_mapping(seg);
	toc = shm_toc_create(DMQ_MQ_MAGIC, dsm_segment_address(seg),
						 dmq_toc_size());
	mq_handles = palloc(MaxBackends*sizeof(shm_mq_handle *));
	for (i = 0; i < MaxBackends; i++)
	{
		shm_mq	   *mq;
		mq = shm_mq_create(shm_toc_allocate(toc, DMQ_MQ_SIZE), DMQ_MQ_SIZE);
		shm_toc_insert(toc, i, mq);
		shm_mq_set_sender(mq, MyProc);
		mq_handles[i] = shm_mq_attach(mq, seg, NULL);
	}

	/* register ourself in dmq_state */
	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);

	/* is this sender already connected? */
	for (i = 0; i < DMQ_MAX_RECEIVERS; i++)
	{
		if (strcmp(dmq_state->receivers[i].name, sender_name) == 0)
		{
			if (!dmq_state->receivers[i].active)
			{
				receiver_id = i;
			}
			else
				mtm_log(ERROR, "[DMQ] sender '%s' already connected", sender_name);
		}
	}

	if (receiver_id < 0)
	{
		if (dmq_state->n_receivers >= DMQ_MAX_RECEIVERS)
			mtm_log(ERROR, "[DMQ] maximum number of dmq-receivers reached");

		receiver_id = dmq_state->n_receivers;
		dmq_state->n_receivers++;
		strncpy(dmq_state->receivers[receiver_id].name, sender_name, DMQ_NAME_MAXLEN);
	}

	dmq_state->receivers[receiver_id].dsm_h = dsm_segment_handle(seg);
	dmq_state->receivers[receiver_id].procno = MyProc->pgprocno;
	dmq_state->receivers[receiver_id].active = true;
	dmq_state->receivers[receiver_id].pid = MyProcPid;
	LWLockRelease(dmq_state->lock);

	on_shmem_exit(dmq_receiver_at_exit, Int32GetDatum(receiver_id));

	/* okay, switch client to copyout state */
	pq_beginmessage(&s, 'W');
	pq_sendbyte(&s, 0); /* copy_is_binary */
	pq_sendint16(&s, 0); /* numAttributes */
	pq_endmessage(&s);
	pq_flush();

	pq_startmsgread();

	ModifyWaitEvent(FeBeWaitSet, 0, WL_SOCKET_READABLE, NULL);

	if (dmq_receiver_start_hook)
		dmq_receiver_start_hook(sender_name);

	/* do not hold globalxmin. XXX: try to carefully release snaps */
	MyPgXact->xmin = InvalidTransactionId;

	for (;;)
	{
		unsigned char	qtype;
		WaitEvent		event;
		int				rc;
		int				nevents;

		if (reader_state == NeedByte)
		{
			rc = _pq_getbyte_if_available(&qtype);

			if (rc > 0)
			{
				if (qtype == 'd')
				{
					reader_state = NeedMessage;
				}
				else if (qtype == 'c')
				{
					break;
				}
				else
				{
					mtm_log(ERROR, "[DMQ] invalid message type %c, %s",
								qtype, s.data);
				}
			}
		}

		if (reader_state == NeedMessage)
		{
			rc = _pq_getmessage_if_avalable(&s);

			if (rc > 0)
			{
				dmq_handle_message(&s, mq_handles, seg);
				last_message_at = dmq_now();
				reader_state = NeedByte;
			}
		}

		if (rc == EOF)
		{
			ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("[DMQ] EOF on connection")));
			break;
		}

		nevents = WaitEventSetWait(FeBeWaitSet, 250, &event, 1,
								   WAIT_EVENT_CLIENT_READ);

		if (nevents > 0 && event.events & WL_LATCH_SET)
		{
			ResetLatch(MyLatch);
		}

		if (nevents > 0 && event.events & WL_POSTMASTER_DEATH)
		{
			ereport(FATAL,
				(errcode(ERRCODE_ADMIN_SHUTDOWN),
				 errmsg("[DMQ] exit receiver due to unexpected postmaster exit")));
		}

		// XXX: is it enough?
		CHECK_FOR_INTERRUPTS();

		if (dmq_now() - last_message_at > dmq_heartbeat_timeout)
		{
			mtm_log(ERROR, "[DMQ] exit receiver due to heartbeat timeout");
		}

	}
	pq_endmsgread();

	PG_RETURN_VOID();
}

void
dmq_terminate_receiver(char *name)
{
	int			i;
	pid_t		pid = 0;

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	for (i = 0; i < DMQ_MAX_RECEIVERS; i++)
	{
		if (dmq_state->receivers[i].active &&
			strncmp(dmq_state->receivers[i].name, name, DMQ_NAME_MAXLEN) == 0)
		{
			pid = dmq_state->receivers[i].pid;
			Assert(pid > 0);
			break;
		}
	}
	LWLockRelease(dmq_state->lock);

	if (pid != 0)
		kill(pid, SIGTERM);
}


/*****************************************************************************
 *
 * API stuff
 *
 *****************************************************************************/

static void
ensure_outq_handle()
{
	dsm_segment *seg;
	shm_toc    *toc;
	shm_mq	   *outq;
	MemoryContext oldctx;


	if (dmq_local.mq_outh != NULL)
		return;

	// Assert(dmq_state->handle != DSM_HANDLE_INVALID);
	seg = dsm_attach(dmq_state->out_dsm);
	if (seg == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("unable to map dynamic shared memory segment")));

	dsm_pin_mapping(seg);

	toc = shm_toc_attach(DMQ_MQ_MAGIC, dsm_segment_address(seg));
	if (toc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("bad magic number in dynamic shared memory segment")));

	outq = shm_toc_lookup(toc, MyProc->pgprocno, false);
	shm_mq_set_sender(outq, MyProc);

	oldctx = MemoryContextSwitchTo(TopMemoryContext);
	dmq_local.mq_outh = shm_mq_attach(outq, seg, NULL);
	MemoryContextSwitchTo(oldctx);
}

void
dmq_push(DmqDestinationId dest_id, char *stream_name, char *msg)
{
	shm_mq_result res;
	StringInfoData buf;

	ensure_outq_handle();

	initStringInfo(&buf);
	pq_sendbyte(&buf, dest_id);
	pq_send_ascii_string(&buf, stream_name);
	pq_send_ascii_string(&buf, msg);

	mtm_log(DmqTraceOutgoing, "[DMQ] pushing l=%d '%.*s'",
			buf.len, buf.len, buf.data);

	// XXX: use sendv instead
	res = shm_mq_send(dmq_local.mq_outh, buf.len, buf.data, false);
	if (res != SHM_MQ_SUCCESS)
		mtm_log(ERROR, "[DMQ] dmq_push: can't send to queue");

	resetStringInfo(&buf);
}

static bool push_state = false;
static StringInfoData buf = {NULL, 0, 0, 0};

/*
 * _initStringInfo
 *
 * Replace call of initStringInfo() routine from stringinfo.c.
 * We need larger strings and need to reduce memory allocations to optimize
 * message passing.
 */
static void
_initStringInfo(StringInfo str, size_t size)
{
	if (str->maxlen <= size)
	{
		size_t newsize = (size * 2 < 1024) ? 1024 : (size * 2);

		if (str->data)
			pfree(str->data);

		/*
		 * We try to minimize str->data allocations. It can live all of the
		 * backend life.
		 */
		str->data = (char *) MemoryContextAlloc(TopMemoryContext, newsize);
		str->maxlen = newsize;
	}
	resetStringInfo(str);
}

bool
dmq_push_buffer(DmqDestinationId dest_id, const char *stream_name,
				const void *payload, size_t len, bool nowait)
{
	shm_mq_result res;

	if (!push_state)
	{
		ensure_outq_handle();

		_initStringInfo(&buf, len);
		pq_sendbyte(&buf, dest_id);
		pq_send_ascii_string(&buf, stream_name);
		pq_sendbytes(&buf, payload, len);

		mtm_log(DmqTraceOutgoing, "[DMQ] pushing l=%d '%.*s'",
				buf.len, buf.len, buf.data);
	}

	// XXX: use sendv instead
	res = shm_mq_send(dmq_local.mq_outh, buf.len, buf.data, nowait);

	if (res == SHM_MQ_WOULD_BLOCK)
	{
		Assert(nowait == true);
		push_state = true;
		/* Report on full queue. */
		return false;
	}

	if (res != SHM_MQ_SUCCESS)
		mtm_log(WARNING, "[DMQ] dmq_push: can't send to queue");

	push_state = false;
	return true;
}

static bool
dmq_reattach_shm_mq(int handle_id)
{
	shm_toc		   *toc;
	shm_mq		   *inq;
	MemoryContext oldctx;

	int receiver_id = -1;
	int receiver_procno;
	dsm_handle receiver_dsm;
	int i;

	LWLockAcquire(dmq_state->lock, LW_SHARED);
	for (i = 0; i < DMQ_MAX_RECEIVERS; i++)
	{
		// XXX: change to hash maybe
		if (strcmp(dmq_state->receivers[i].name, dmq_local.inhandles[handle_id].name) == 0
			&& dmq_state->receivers[i].active)
		{
			receiver_id = i;
			receiver_procno = dmq_state->receivers[i].procno;
			receiver_dsm = dmq_state->receivers[i].dsm_h;
			break;
		}
	}
	LWLockRelease(dmq_state->lock);

	if (receiver_id < 0)
	{
		mtm_log(DmqTraceShmMq, "[DMQ] can't find receiver named '%s'",
			dmq_local.inhandles[handle_id].name);
		return false;
	}

	Assert(receiver_dsm != DSM_HANDLE_INVALID);

	if (dsm_find_mapping(receiver_dsm))
	{
		mtm_log(DmqTraceShmMq,
			"[DMQ] we already attached '%s', probably receiver is exiting",
			dmq_local.inhandles[handle_id].name);
		return false;
	}

	mtm_log(DmqTraceShmMq, "[DMQ] attaching '%s', dsm handle %d",
			dmq_local.inhandles[handle_id].name,
			receiver_dsm);

	if (dmq_local.inhandles[handle_id].dsm_seg)
	{
		if (dmq_local.inhandles[handle_id].mqh)
		{
			mtm_log(DmqTraceShmMq, "[DMQ] detach shm_mq_handle %p",
					dmq_local.inhandles[handle_id].mqh);
			shm_mq_detach(dmq_local.inhandles[handle_id].mqh);
		}
		mtm_log(DmqTraceShmMq, "[DMQ] detach dsm_seg %p",
				dmq_local.inhandles[handle_id].dsm_seg);
		dsm_detach(dmq_local.inhandles[handle_id].dsm_seg);
	}

	dmq_local.inhandles[handle_id].dsm_seg = dsm_attach(receiver_dsm);
	if (dmq_local.inhandles[handle_id].dsm_seg == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("unable to map dynamic shared memory segment")));

	dsm_pin_mapping(dmq_local.inhandles[handle_id].dsm_seg);

	toc = shm_toc_attach(DMQ_MQ_MAGIC,
				dsm_segment_address(dmq_local.inhandles[handle_id].dsm_seg));
	if (toc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("bad magic number in dynamic shared memory segment")));

	inq = shm_toc_lookup(toc, MyProc->pgprocno, false);

	/* re-create */
	mtm_log(DmqTraceShmMq, "[DMQ] creating shm_mq handle %p", inq);
	inq = shm_mq_create(inq, DMQ_MQ_SIZE); // XXX
	shm_mq_set_receiver(inq, MyProc);
	shm_mq_set_sender(inq, &ProcGlobal->allProcs[receiver_procno]); // XXX

	oldctx = MemoryContextSwitchTo(TopMemoryContext);
	dmq_local.inhandles[handle_id].mqh = shm_mq_attach(inq,
							dmq_local.inhandles[handle_id].dsm_seg, NULL);
	MemoryContextSwitchTo(oldctx);

	return true;
}

DmqSenderId
dmq_attach_receiver(const char *sender_name, int mask_pos)
{
	int			i;
	int			handle_id = -1;

	/* Search for existed receiver. */
	for (i = 0; i < dmq_local.n_inhandles; i++)
	{
		if (strcmp(sender_name, dmq_local.inhandles[i].name) == 0)
			return i;
	}

	for (i = 0; i < DMQ_MAX_RECEIVERS; i++)
	{
		if (dmq_local.inhandles[i].name[0] == '\0')
		{
			handle_id = i;
			break;
		}
	}

	if (i == DMQ_MAX_RECEIVERS)
		mtm_log(ERROR, "[DMQ] dmq_attach_receiver: max receivers already attached");

	if (handle_id == dmq_local.n_inhandles)
		dmq_local.n_inhandles++;

	dmq_local.inhandles[handle_id].mqh = NULL;
	dmq_local.inhandles[handle_id].mask_pos = mask_pos;
	strncpy(dmq_local.inhandles[handle_id].name, sender_name, DMQ_NAME_MAXLEN);

	dmq_reattach_shm_mq(handle_id);

	return handle_id;
}

void
dmq_detach_receiver(const char *sender_name)
{
	int			i;
	int			handle_id = -1;

	for (i = 0; i < DMQ_MAX_RECEIVERS; i++)
	{
		if (strncmp(dmq_local.inhandles[i].name, sender_name, DMQ_NAME_MAXLEN) == 0)
		{
			handle_id = i;
			break;
		}
	}

	if (handle_id < 0)
		mtm_log(ERROR, "[DMQ] dmq_detach_receiver: receiver from %s not found",
				sender_name);

	if (dmq_local.inhandles[handle_id].mqh)
	{
		shm_mq_detach(dmq_local.inhandles[handle_id].mqh);
		dmq_local.inhandles[handle_id].mqh = NULL;
	}

	if (dmq_local.inhandles[handle_id].dsm_seg)
	{
		// dsm_unpin_mapping(dmq_local.inhandles[handle_id].dsm_seg);
		dsm_detach(dmq_local.inhandles[handle_id].dsm_seg);
		dmq_local.inhandles[handle_id].dsm_seg =  NULL;
	}

	dmq_local.inhandles[handle_id].name[0] = '\0';
}

void
dmq_stream_subscribe(const char *stream_name)
{
	bool found;
	DmqStreamSubscription *sub;

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	sub = (DmqStreamSubscription *) hash_search(dmq_state->subscriptions, stream_name,
												HASH_ENTER, &found);
	if (found && sub->procno != MyProc->pgprocno)
	{
		mtm_log(ERROR,
				"[DMQ] procno%d: %s: subscription is already active for procno %d / %s",
				MyProc->pgprocno, stream_name, sub->procno, sub->stream_name);
	}
	sub->procno = MyProc->pgprocno;
	LWLockRelease(dmq_state->lock);
}

void
dmq_stream_unsubscribe(const char *stream_name)
{
	bool found;

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	hash_search(dmq_state->subscriptions, stream_name, HASH_REMOVE, &found);
	LWLockRelease(dmq_state->lock);

	Assert(found);
}

const char *
dmq_sender_name(DmqSenderId id)
{
	Assert((id >= 0) && (id < dmq_local.n_inhandles));

	if (dmq_local.inhandles[id].name[0] == '\0')
		return NULL;
	return dmq_local.inhandles[id].name;
}

char *
dmq_receiver_name(DmqDestinationId dest_id)
{
	char *recvName;

	LWLockAcquire(dmq_state->lock, LW_SHARED);
	recvName = pstrdup(dmq_state->destinations[dest_id].receiver_name);
	LWLockRelease(dmq_state->lock);
	return recvName;
}

DmqDestinationId
dmq_remote_id(const char *name)
{
	DmqDestinationId i;

	LWLockAcquire(dmq_state->lock, LW_SHARED);
	for (i = 0; i < DMQ_MAX_DESTINATIONS; i++)
	{
		DmqDestination *dest = &(dmq_state->destinations[i]);
		if (strcmp(name, dest->receiver_name) == 0)
			break;
	}
	LWLockRelease(dmq_state->lock);

	if (i == DMQ_MAX_DESTINATIONS)
		return -1;

	return i;
}

/*
 * Get a message from input queue. If waitMsg = true, execution blocking until
 * message will not received. Returns false, if an error is occured.
 *
 * sender_id - identifier of the received message sender.
 * msg - pointer to local buffer that contains received message.
 * len - size of received message.
 */
const char *
dmq_pop(DmqSenderId *sender_id, const void **msg, Size *len, uint64 mask,
		bool waitMsg)
{
	shm_mq_result res;
	char *stream;

	Assert(msg && len);

	for (;;)
	{
		int			i;
		int			rc;
		bool		nowait = false;

		CHECK_FOR_INTERRUPTS();

		for (i = 0; i < dmq_local.n_inhandles; i++)
		{
			void   *data;

			if (!BIT_CHECK(mask, dmq_local.inhandles[i].mask_pos))
				continue;

			if (dmq_local.inhandles[i].mqh)
				res = shm_mq_receive(dmq_local.inhandles[i].mqh, len, &data, true);
			else
				res = SHM_MQ_DETACHED;

			if (res == SHM_MQ_SUCCESS)
			{
				/*
				 * Set message pointer and length.
				 * Stream name is first null-terminated string in
				 * the message buffer.
				 */
				stream = (char *) data;
				*len -= (strlen(stream) + 1);
				Assert(*len > 0);
				*msg = ((char *) data + strlen(stream) + 1);
				*sender_id = i;

				mtm_log(DmqTraceIncoming,
						"[DMQ] dmq_pop: got message %s from %s",
						(char *) data, dmq_local.inhandles[i].name);
				return stream;
			}
			else if (res == SHM_MQ_DETACHED)
			{
				mtm_log(DmqTraceShmMq,
						"[DMQ] dmq_pop: queue '%s' detached, trying to reattach",
						dmq_local.inhandles[i].name);

				if (dmq_reattach_shm_mq(i))
				{
					nowait = true;
				}
				else
				{
					*sender_id = i;
					return NULL;
				}
			}
		}

		if (nowait && waitMsg)
			continue;
		if (!waitMsg)
			return NULL;

		// XXX cache that
		rc = WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT, 10.0,
					   WAIT_EVENT_MQ_RECEIVE);

		if (rc & WL_POSTMASTER_DEATH)
			proc_exit(1);

		if (rc & WL_LATCH_SET)
			ResetLatch(MyLatch);
	}
	return NULL;
}

bool
dmq_pop_nb(DmqSenderId *sender_id, StringInfo msg, uint64 mask)
{
	shm_mq_result res;
	int i;

	for (i = 0; i < dmq_local.n_inhandles; i++)
	{
		Size	len;
		void   *data;

		if (!BIT_CHECK(mask, dmq_local.inhandles[i].mask_pos))
			continue;

		if (dmq_local.inhandles[i].mqh)
			res = shm_mq_receive(dmq_local.inhandles[i].mqh, &len, &data, true);
		else
			res = SHM_MQ_DETACHED;

		if (res == SHM_MQ_SUCCESS)
		{
			msg->data = data;
			msg->len = len;
			msg->maxlen = len;
			msg->cursor = 0;
			*sender_id = i;

			mtm_log(DmqTraceIncoming,
					"[DMQ] dmq_pop_nb: got message %s from %s",
					(char *) data, dmq_local.inhandles[i].name);
			return true;
		}
		else if (res == SHM_MQ_DETACHED)
		{
			if (!dmq_reattach_shm_mq(i))
				mtm_log(WARNING, "[DMQ] dmq_pop_nb: failed to reattach");
			return false;
		}
	}

	return false;
}

/*
 * Create write channel with an instance, defined by connection string.
 * sender_name - a symbolic name of the sender. Remote backend will attach
 * to this channel by sender name.
 * See dmq_attach_receiver() routine for details.
 * Call this function after shared memory initialization.
 */
DmqDestinationId
dmq_destination_add(char *connstr, char *sender_name, char *receiver_name,
					int ping_period)
{
	DmqDestinationId dest_id;
	pid_t sender_pid;

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	for (dest_id = 0; dest_id < DMQ_MAX_DESTINATIONS; dest_id++)
	{
		DmqDestination *dest = &(dmq_state->destinations[dest_id]);
		if (!dest->active)
		{
			strncpy(dest->sender_name, sender_name, DMQ_NAME_MAXLEN);
			strncpy(dest->receiver_name, receiver_name, DMQ_NAME_MAXLEN);
			strncpy(dest->connstr, connstr, DMQ_CONNSTR_MAX_LEN);
			dest->ping_period = ping_period;
			dest->active = true;
			break;
		}
	}
	sender_pid = dmq_state->sender_pid;
	LWLockRelease(dmq_state->lock);

	if (sender_pid)
		kill(sender_pid, SIGHUP);

	if (dest_id == DMQ_MAX_DESTINATIONS)
		mtm_log(ERROR, "Can't add new destination. DMQ_MAX_DESTINATIONS reached.");
	else
		return dest_id;
}

/*
 * Check availability of destination node.
 * It is needed before sending process to prevent data loss.
 */
DmqConnState
dmq_get_destination_status(DmqDestinationId dest_id)
{
	DmqConnState state;
	DmqDestination *dest;

	if ((dest_id < 0) || (dest_id >= DMQ_MAX_DESTINATIONS))
		return -2;

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	dest = &(dmq_state->destinations[dest_id]);
	if (!dest->active)
	{
		LWLockRelease(dmq_state->lock);
		return -1;
	}

	state = dest->state;
	LWLockRelease(dmq_state->lock);
	return state;
}

void
dmq_destination_drop(const char *receiver_name)
{
	DmqDestinationId dest_id;
	pid_t sender_pid;

	LWLockAcquire(dmq_state->lock, LW_EXCLUSIVE);
	for (dest_id = 0; dest_id < DMQ_MAX_DESTINATIONS; dest_id++)
	{
		DmqDestination *dest = &(dmq_state->destinations[dest_id]);
		if (dest->active &&
			strncmp(dest->receiver_name, receiver_name, DMQ_NAME_MAXLEN) == 0)
		{
			dest->active = false;
			break;
		}
	}
	sender_pid = dmq_state->sender_pid;
	LWLockRelease(dmq_state->lock);

	if (sender_pid)
		kill(sender_pid, SIGHUP);
}

