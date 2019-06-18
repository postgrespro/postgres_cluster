#ifndef DMQ_H
#define DMQ_H

#include "libpq-fe.h"
#include "lib/stringinfo.h"

typedef enum
{
	Idle,			/* upon init or falure */
	Connecting,		/* upon PQconnectStart */
	Negotiating,	/* upon PQconnectPoll == OK */
	Active,			/* upon dmq_receiver_loop() response */
} DmqConnState;

typedef int8 DmqDestinationId;
typedef int8 DmqSenderId;

#define DMQ_NAME_MAXLEN 64
#define DMQ_MAX_DESTINATIONS 127
#define DMQ_MAX_RECEIVERS 100

extern int dmq_heartbeat_timeout;

extern void dmq_init(const char *library_name);

extern DmqDestinationId dmq_destination_add(char *connstr,
											char *sender_name,
											char *receiver_name,
											int ping_period);
extern DmqConnState dmq_get_destination_status(DmqDestinationId dest_id);
extern void dmq_destination_drop(const char *receiver_name);

extern DmqSenderId dmq_attach_receiver(const char *sender_name, int mask_pos);
extern void dmq_detach_receiver(const char *sender_name);

extern void dmq_terminate_receiver(char *name);

extern void dmq_stream_subscribe(const char *stream_name);
extern void dmq_stream_unsubscribe(const char *stream_name);
extern const char *dmq_sender_name(DmqSenderId id);
extern char *dmq_receiver_name(DmqDestinationId dest_id);
extern DmqDestinationId dmq_remote_id(const char *name);

extern const char *
dmq_pop(DmqSenderId *sender_id, const void **msg, Size *len, uint64 mask,
		bool waitMsg);
extern bool dmq_pop_nb(DmqSenderId *sender_id, StringInfo msg, uint64 mask);

extern void dmq_push(DmqDestinationId dest_id, char *stream_name, char *msg);
extern bool dmq_push_buffer(DmqDestinationId dest_id, const char *stream_name,
							const void *buffer, size_t len, bool nowait);

typedef void (*dmq_receiver_hook_type) (const char *);
extern dmq_receiver_hook_type dmq_receiver_start_hook;
extern dmq_receiver_hook_type dmq_receiver_stop_hook;

#endif
