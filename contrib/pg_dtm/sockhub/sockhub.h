#ifndef __SOCKHUB_H__
#define __SOCKHUB_H__

#include <sys/select.h>

typedef struct {
    unsigned int size : 24; /* size of message without header */
    unsigned int code : 8;  /* any user defined code */
    unsigned int chan;      /* local socket: set by SockHUB */
} ShubMessageHdr;

enum ShubMessageCodes
{
    MSG_DISCONNECT,
    MSG_FIRST_USER_CODE /* all codes >= 1 are user defined */
};

typedef enum 
{
    SHUB_FATAL_ERROR,
    SHUB_RECOVERABLE_ERROR,
    SHUB_MINOR_ERROR,
} ShubErrorSeverity;

typedef void(*ShubErrorHandler)(char const* msg, ShubErrorSeverity severity);

typedef struct host_t
{
    char *host;
    int port;
    struct host_t *next;
    struct host_t *prev;
} host_t;

typedef struct 
{
    int buffer_size;
    int delay;
    int queue_size;
    int max_attempts;
    char const* file;
    host_t *leader;
    ShubErrorHandler error_handler;
} ShubParams;
   
typedef struct
{
    int    output;
    int    input;
    int    max_fd;
    fd_set inset;
    char*  in_buffer;
    char*  out_buffer;
    int    in_buffer_used;
    int    out_buffer_used;
    ShubParams* params;
} Shub;

int ShubReadSocketEx(int sd, void* buf, int min_size, int max_size);
int ShubReadSocket(int sd, void* buf, int size);
int ShubWriteSocket(int sd, void const* buf, int size);

void ShubInitParams(ShubParams* params);
void ShubParamsSetHosts(ShubParams* params, char* hoststring);
void ShubInitialize(Shub* shub, ShubParams* params);
void ShubLoop(Shub* shub);

#endif

// vim: sts=4 ts=4 sw=4 expandtab
