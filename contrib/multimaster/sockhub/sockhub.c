#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include "sockhub.h"

static void default_error_handler(char const* msg, ShubErrorSeverity severity)
{
    perror(msg);
    if (severity == SHUB_FATAL_ERROR) { 
        exit(1);
    }
}    
    

void ShubInitParams(ShubParams* params)
{
    memset(params, 0, sizeof(*params));
    params->buffer_size = 64*1024;
    params->port = 54321;
    params->queue_size = 100;
    params->max_attempts = 10;
    params->error_handler = default_error_handler;
}


static int resolve_host_by_name(const char *hostname, unsigned* addrs, unsigned* n_addrs)
{
    struct sockaddr_in sin;
    struct hostent* hp;
    unsigned i;
    
    sin.sin_addr.s_addr = inet_addr(hostname);
    if (sin.sin_addr.s_addr != INADDR_NONE) {
        memcpy(&addrs[0], &sin.sin_addr.s_addr, sizeof(sin.sin_addr.s_addr));
        *n_addrs = 1;
        return 1;
    }

    hp = gethostbyname(hostname);
    if (hp == NULL || hp->h_addrtype != AF_INET) { 
        return 0;
    }
    for (i = 0; hp->h_addr_list[i] != NULL && i < *n_addrs; i++) { 
        memcpy(&addrs[i], hp->h_addr_list[i], sizeof(addrs[i]));
    }
    *n_addrs = i;
    return 1;
}

            

static void close_socket(Shub* shub, int fd)
{
    close(fd);
    FD_CLR(fd, &shub->inset);
}

int ShubReadSocketEx(int sd, void* buf, int min_size, int max_size)
{
    int received = 0;
    assert(min_size <= max_size);
    while (received < min_size) { 
        int n = recv(sd, (char*)buf + received, max_size - received, 0);
        if (n <= 0) { 
            break;
        } 
        received += n;
    }
    return received;
}

int ShubReadSocket(int sd, void* buf, int size)
{
    return ShubReadSocketEx(sd, buf, size, size) == size;
}
    
int ShubWriteSocket(int sd, void const* buf, int size)
{
    char* src = (char*)buf;
    while (size != 0) { 
        int n = send(sd, src, size, 0);
        if (n <= 0) { 
            return 0;
        } 
        size -= n;
        src += n;
    }
    return 1;
}


static void reconnect(Shub* shub)
{
    struct sockaddr_in sock_inet;
    unsigned addrs[128];
    unsigned i, n_addrs = sizeof(addrs) / sizeof(addrs[0]);
    int max_attempts = shub->params->max_attempts;
    char* host = (char*)shub->params->host;
    if (shub->output >= 0) { 
        close_socket(shub, shub->output);
    }

    sock_inet.sin_family = AF_INET;  
    sock_inet.sin_port = htons(shub->params->port);

    while (1) { 
        char* sep = strchr(host, ',');
        ShubErrorSeverity severity = SHUB_FATAL_ERROR;
        if (sep != NULL) {
            *sep = '\0';
            severity = SHUB_RECOVERABLE_ERROR;
        }        
        if (!resolve_host_by_name(host, addrs, &n_addrs)) { 
            shub->params->error_handler("Failed to resolve host by name", severity);
            goto TryNextHost;
        }            
        shub->output = socket(AF_INET, SOCK_STREAM, 0);
        if (shub->output < 0) { 
            shub->params->error_handler("Failed to create inet socket", severity);
            goto TryNextHost;
        }
        while (1) { 
            int rc = -1;
            for (i = 0; i < n_addrs; ++i) {
                memcpy(&sock_inet.sin_addr, &addrs[i], sizeof sock_inet.sin_addr);
                do { 
                    rc = connect(shub->output, (struct sockaddr*)&sock_inet, sizeof(sock_inet));
                } while (rc < 0 && errno == EINTR);
                
                if (rc >= 0 || errno == EINPROGRESS) { 
                    break;
                }
            }
            if (rc < 0) {             
                if (errno != ENOENT && errno != ECONNREFUSED && errno != EINPROGRESS) {
                    shub->params->error_handler("Connection can not be establish", severity);
                    goto TryNextHost;
                }
                if (max_attempts-- != 0) {
                    sleep(1);
                } else {
                    shub->params->error_handler("Failed to connect to host", severity);
                    goto TryNextHost;
                }                
            } else { 
                int optval = 1;
                setsockopt(shub->output, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
                FD_SET(shub->output, &shub->inset);
                if (sep != NULL) { 
                    *sep = ',';
                }
                return;
            }
        }
      TryNextHost:
        *sep = ',';
        host = sep + 1;
    }
}

static void notify_disconnect(Shub* shub, int chan)
{
    ShubMessageHdr* hdr;
    assert(shub->in_buffer_used + sizeof(ShubMessageHdr) <= shub->params->buffer_size);
    hdr = (ShubMessageHdr*)&shub->in_buffer[shub->in_buffer_used];
    hdr->size = 0;
    hdr->chan = chan;
    hdr->code = MSG_DISCONNECT;
    shub->in_buffer_used += sizeof(ShubMessageHdr);
    if (shub->in_buffer_used + sizeof(ShubMessageHdr) > shub->params->buffer_size) { 
        while (!ShubWriteSocket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
            shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
            reconnect(shub);
        }
        shub->in_buffer_used = 0;
    }
}

static void recovery(Shub* shub)
{
    int i, max_fd;

    for (i = 0, max_fd = shub->max_fd; i <= max_fd; i++) {
        if (FD_ISSET(i, &shub->inset)) { 
            struct timeval tm = {0,0};
            fd_set tryset;
            FD_ZERO(&tryset);
            FD_SET(i, &tryset);
            if (select(i+1, &tryset, NULL, NULL, &tm) < 0) { 
                if (i != shub->input && i != shub->output) { 
                    notify_disconnect(shub, i);
                }
                close_socket(shub, i);
            }
        }
    }
}

void ShubInitialize(Shub* shub, ShubParams* params)
{
    struct sockaddr sock;

    shub->params = params;

    sock.sa_family = AF_UNIX;
    strcpy(sock.sa_data, params->file);
    unlink(params->file);
    shub->input = socket(AF_UNIX, SOCK_STREAM, 0);
    if (shub->input < 0) { 
        shub->params->error_handler("Failed to create local socket", SHUB_FATAL_ERROR);
    }
    if (bind(shub->input, &sock, ((char*)sock.sa_data - (char*)&sock) + strlen(params->file)) < 0) {
        shub->params->error_handler("Failed to bind local socket", SHUB_FATAL_ERROR);
    }    
    if (listen(shub->input, params->queue_size) < 0) {
        shub->params->error_handler("Failed to listen local socket", SHUB_FATAL_ERROR);
    }            
    FD_ZERO(&shub->inset);
    FD_SET(shub->input, &shub->inset);
    
    shub->output = -1;
    shub->max_fd = shub->input;
    reconnect(shub);

    shub->in_buffer = malloc(params->buffer_size);
    shub->out_buffer = malloc(params->buffer_size);
    if (shub->in_buffer == NULL || shub->out_buffer == NULL) { 
        shub->params->error_handler("Failed to allocate buffer", SHUB_FATAL_ERROR);
    }
    shub->in_buffer_used = 0;
    shub->out_buffer_used = 0;
}

static int stop = 0;
static void die(int sig) {
    stop = 1;
}

void ShubLoop(Shub* shub)
{    
    int buffer_size = shub->params->buffer_size;
    sigset_t sset;
    signal(SIGINT, die);
    signal(SIGQUIT, die);
    signal(SIGTERM, die);
    /* signal(SIGHUP, die); */
    sigfillset(&sset);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);

    while (!stop) { 
        fd_set events;
        struct timeval tm;
        int i, rc;
        int max_fd = shub->max_fd;

        tm.tv_sec = shub->params->delay/1000;
        tm.tv_usec = shub->params->delay % 1000 * 1000;

        events = shub->inset;
        rc = select(max_fd+1, &events, NULL, NULL, shub->in_buffer_used == 0 ? NULL : &tm);
        if (rc < 0) { 
            if (errno != EINTR) {                
                shub->params->error_handler("Select failed", SHUB_RECOVERABLE_ERROR);
                recovery(shub);
            }
        } else {
            if (rc > 0) {
                for (i = 0; i <= max_fd; i++) { 
                    if (FD_ISSET(i, &events)) { 
                        if (i == shub->input) { /* accept incomming connection */ 
                            int s = accept(i, NULL, NULL);
                            if (s < 0) { 
                                shub->params->error_handler("Failed to accept socket", SHUB_RECOVERABLE_ERROR);
                            } else {
                                if (s > shub->max_fd) {
                                    shub->max_fd = s;
                                }
                                FD_SET(s, &shub->inset);
                            }
                        } else if (i == shub->output) { /* receive response from server */
                            /* try to read as much as possible */
                            int available = shub->out_buffer_used;
                            int pos = 0;
                            ShubMessageHdr* firstHdr = NULL;
                            rc = ShubReadSocketEx(shub->output, shub->out_buffer + available, 1, buffer_size - available);
                            if (rc <= 0) { 
                                shub->params->error_handler("Failed to read inet socket", SHUB_RECOVERABLE_ERROR);
                                reconnect(shub);
                                continue;
                            }
                            available += rc;
                            
                            /* loop through all received responses */
                            while (pos + sizeof(ShubMessageHdr) <= available) { 
                                ShubMessageHdr* hdr = (ShubMessageHdr*)&shub->out_buffer[pos];
                                int chan = hdr->chan;
                                int processed = pos;
                                pos += sizeof(ShubMessageHdr) + hdr->size;
                                if (firstHdr != NULL && (firstHdr->chan != chan || pos > available)) { 
                                    assert(hdr > firstHdr);
                                    if (!ShubWriteSocket(firstHdr->chan, firstHdr, (char*)hdr - (char*)firstHdr)) { 
                                        shub->params->error_handler("Failed to write to local socket", SHUB_RECOVERABLE_ERROR);
                                        close_socket(shub, firstHdr->chan);
                                        notify_disconnect(shub, firstHdr->chan);
                                    }
                                    firstHdr = NULL;
                                }
                                if (pos <= available) { 
                                    if (!firstHdr) { 
                                        firstHdr = hdr;
                                    }
                                } else { 
                                    assert(firstHdr == NULL);
                                    if (processed != 0) {
                                        pos = processed;
                                    } else { 
                                        /* read rest of message if it doesn't fit in the buffer */
                                        int tail = pos - available;
                                        if (!ShubWriteSocket(chan, hdr, available)) { 
                                            shub->params->error_handler("Failed to write to local socket", SHUB_RECOVERABLE_ERROR);
                                            close_socket(shub, chan);
                                            chan = -1;
                                        }
                                        do {
                                            int n = tail < buffer_size ? tail : buffer_size;
                                            if (!ShubReadSocket(shub->output, shub->out_buffer, n)) { 
                                                shub->params->error_handler("Failed to read inet socket", SHUB_RECOVERABLE_ERROR);
                                                reconnect(shub);
                                                continue;
                                            }
                                            if (chan >= 0 && !ShubWriteSocket(chan, shub->out_buffer, n)) { 
                                                shub->params->error_handler("Failed to write to local socket", SHUB_RECOVERABLE_ERROR);
                                                close_socket(shub, chan);
                                                notify_disconnect(shub, chan);
                                                chan = -1;
                                            }                                       
                                            tail -= n;
                                        } while (tail != 0);
                                        
                                        available = pos;
                                    }
                                    break;
                                }
                            }
                            if (firstHdr != NULL) { 
                               assert(&shub->out_buffer[pos] > (char*)firstHdr);
                                if (!ShubWriteSocket(firstHdr->chan, firstHdr, &shub->out_buffer[pos] - (char*)firstHdr)) { 
                                    shub->params->error_handler("Failed to write to local socket", SHUB_RECOVERABLE_ERROR);
                                    close_socket(shub, firstHdr->chan);
                                    notify_disconnect(shub, firstHdr->chan);
                                }
                            }
                            /* Move partly fetched message header (if any) to the beginning of buffer */
                            memmove(shub->out_buffer, shub->out_buffer + pos, available - pos);
                            shub->out_buffer_used = available - pos;
                        } else { /* receive request from client */
                            int chan = i;
                            int available = 0;
                            int pos = shub->in_buffer_used;

                            do { 
                                assert(sizeof(ShubMessageHdr) > available);
                                /* read as much as possible */
                                rc = ShubReadSocketEx(chan, &shub->in_buffer[pos + available], sizeof(ShubMessageHdr) - available, buffer_size - pos - available);
                                if (rc < sizeof(ShubMessageHdr) - available) { 
                                    shub->params->error_handler("Failed to read local socket", SHUB_RECOVERABLE_ERROR);
                                    close_socket(shub, i);
                                    shub->in_buffer_used = pos;
                                    notify_disconnect(shub, i);
                                    pos = shub->in_buffer_used;
                                    break;
                                }
                                available += pos + rc;
                                /* loop through all fetched messages */
                                while (pos + sizeof(ShubMessageHdr) <= available) {
                                    ShubMessageHdr* hdr = (ShubMessageHdr*)&shub->in_buffer[pos];
                                    unsigned int size = hdr->size;
                                    int processed = pos;
                                    pos += sizeof(ShubMessageHdr) + size;
                                    hdr->chan = chan; /* remember socket descriptor from which this message was read */
                                    if (pos <= available) {
                                        /* message cmopletely fetched */
                                        continue;
                                    }
                                    if (pos + sizeof(ShubMessageHdr) > buffer_size) { 
                                        /* message doesn't completely fit in buffer */
                                        while (!ShubWriteSocket(shub->output, shub->in_buffer, available)) {
                                            shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                            reconnect(shub);
                                        }
                                        processed = 0;
                                        hdr = NULL;
                                    } else { 
                                        processed = available;
                                    }
                                    size = pos - available;  /* rest of message */                                    
                                    
                                    /* fetch rest of message body */
                                    do { 
                                        unsigned int n = processed + size > buffer_size ? buffer_size - processed : size;
                                        if (chan >= 0 && !ShubReadSocket(chan, shub->in_buffer + processed, n)) { 
                                            shub->params->error_handler("Failed to read local socket", SHUB_RECOVERABLE_ERROR);
                                            close_socket(shub, chan);
                                            if (hdr != NULL) { /* if message header is not yet sent to the server... */
                                                /* ... then skip this message */
                                                shub->in_buffer_used = (char*)hdr - shub->in_buffer;
                                                notify_disconnect(shub, chan);
                                                processed = shub->in_buffer_used;
                                                break;
                                            } else { /* if message was partly sent to the server, we can not skip it, so we have to send garbage to the server */
                                                chan = -1; /* do not try to read rest of body of this message */
                                            }
                                        } 
                                        processed += n;
                                        size -= n;
                                        /* if there is no more free space in the buffer to receive new message header... */
                                        if (processed + sizeof(ShubMessageHdr) > buffer_size) {
                                            /* ... then send buffer to the server */
                                            while (!ShubWriteSocket(shub->output, shub->in_buffer, processed)) {
                                                shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                                reconnect(shub);
                                            }
                                            hdr = NULL; /* message is partly sent to the server: can not skip it any more */
                                            processed = 0;
                                        }
                                    } while (size != 0); /* repeat until all message body is received */
                                    
                                    if (chan < 0) { 
                                        shub->in_buffer_used = processed;
                                        notify_disconnect(shub, i);
                                        processed = shub->in_buffer_used;
                                    }                                    
                                    pos = available = processed;
                                    break;
                                }
                                if (pos + sizeof(ShubMessageHdr) > buffer_size) {
                                    while (!ShubWriteSocket(shub->output, shub->in_buffer, pos)) {
                                        shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                        reconnect(shub);
                                    }
                                    memmove(shub->in_buffer, shub->in_buffer + pos, available -= pos);
                                    pos = 0;
                                } else {
                                    available -= pos;
                                }
                            } while (available != 0);                   

                            assert(pos + sizeof(ShubMessageHdr) <= buffer_size);
                            shub->in_buffer_used = pos;
                        }
                    }
                }
                if (shub->params->delay != 0) { 
                    continue;
                }
            }
            if (shub->in_buffer_used != 0) { /* if buffer is not empty... */
                /* ...then send it */
#if SHOW_SENT_STATISTIC
                static size_t total_sent;
                static size_t total_count;
                total_sent += shub->in_buffer_used;
                if (++total_count % 1024 == 0) { 
                    printf("Average sent buffer size: %ld\n", total_sent/total_count);
                }
#endif
                while (!ShubWriteSocket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
                    shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                    reconnect(shub);
                }
                shub->in_buffer_used = 0;
            }    
        } 
    }
}
       
