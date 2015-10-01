#ifndef __SOCKHUB_H__
#define __SOCKHUB_H__

#include <sys/select.h>

typedef struct {
    unsigned int size : 24; /* size of message without header */
    unsigned int code : 8;  /* any user defined code */
    unsigned int chan;      /* local socket: set by SockHUB */
} ShubMessageHdr;

typedef enum 
{
    SHUB_FATAL_ERROR,
    SHUB_RECOVERABLE_ERROR,
    SHUB_MINOR_ERROR,
} ShubErrorSeverity;

typedef void(*ShubErrorHandler)(char const* msg, ShubErrorSeverity severity);

typedef struct 
{
    int buffer_size;
    int delay;
    int port;
    int queue_size;
    int max_attempts;
    char const* file;
    char const* host;
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

void ShubInitParams(ShubParams* params);
void ShubInitialize(Shub* shub, ShubParams* params);
void ShubLoop(Shub* shub);

#endif
