#ifndef DMQ_H
#define DMQ_H

#include "libpq-fe.h"
#include "lib/stringinfo.h"

typedef int8 DmqDestinationId;
typedef int8 DmqSenderId;

#define DMQ_NAME_MAXLEN 32

extern void dmq_init(const char *library_name);

extern DmqDestinationId dmq_destination_add(char *connstr, char *sender_name,
											char *receiver_name, int ping_period);
extern void dmq_destination_drop(char *receiver_name);

extern DmqSenderId dmq_attach_receiver(const char *sender_name, int mask_pos);
extern void dmq_detach_receiver(const char *sender_name);

extern void dmq_terminate_receiver(char *name);

extern void dmq_stream_subscribe(const char *stream_name);
extern void dmq_stream_unsubscribe(const char *stream_name);
extern const char *dmq_sender_name(DmqSenderId id);
extern DmqDestinationId dmq_remote_id(const char *name);

extern const char *
dmq_pop(DmqSenderId *sender_id, void **msg, Size *len, uint64 mask, bool waitMsg);
extern bool dmq_pop_nb(DmqSenderId *sender_id, StringInfo msg, uint64 mask);

extern void dmq_push(DmqDestinationId dest_id, char *stream_name, char *msg);
extern void dmq_push_buffer(DmqDestinationId dest_id, char *stream_name, const void *buffer, size_t len);

typedef void (*dmq_receiver_hook_type) (char *);
extern dmq_receiver_hook_type dmq_receiver_start_hook;
extern dmq_receiver_hook_type dmq_receiver_stop_hook;

#endif
