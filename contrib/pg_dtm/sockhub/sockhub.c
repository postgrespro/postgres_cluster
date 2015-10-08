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
#include <errno.h>

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

static int read_socket_ex(int sd, char* buf, int min_size, int max_size)
{
    int received = 0;
    while (received < min_size) { 
        int n = recv(sd, buf + received, max_size - received, 0);
        if (n <= 0) { 
            break;
        } 
        received += n;
    }
    return received;
}

static int read_socket(int sd, char* buf, int size)
{
    return read_socket_ex(sd, buf, size, size) == size;
}
    
static int write_socket(int sd, char const* buf, int size)
{
    while (size != 0) { 
        int n = send(sd, buf, size, 0);
        if (n <= 0) { 
            return 0;
        } 
        size -= n;
        buf += n;
    }
    return 1;
}


static void reconnect(Shub* shub)
{
    struct sockaddr_in sock_inet;
    unsigned addrs[128];
    unsigned i, n_addrs = sizeof(addrs) / sizeof(addrs[0]);
    int max_attempts = shub->params->max_attempts;

    if (shub->output >= 0) { 
        close_socket(shub, shub->output);
    }

    sock_inet.sin_family = AF_INET;  
    sock_inet.sin_port = htons(shub->params->port);
    if (!resolve_host_by_name(shub->params->host, addrs, &n_addrs)) { 
        shub->params->error_handler("Failed to resolve host by name", SHUB_FATAL_ERROR);
    }            
    shub->output = socket(AF_INET, SOCK_STREAM, 0);
    if (shub->output < 0) { 
        shub->params->error_handler("Failed to create inet socket", SHUB_FATAL_ERROR);
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
                shub->params->error_handler("Connection can not be establish", SHUB_FATAL_ERROR);
            }
            if (max_attempts-- != 0) {
                sleep(1);
            } else {
                shub->params->error_handler("Failed to connect to host", SHUB_FATAL_ERROR);
            }                
        } else { 
            int optval = 1;
            setsockopt(shub->output, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
            FD_SET(shub->output, &shub->inset);
            break;
        }
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


void ShubLoop(Shub* shub)
{    
    int buffer_size = shub->params->buffer_size;

    while (1) { 
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
                            int available = read_socket_ex(shub->output, shub->out_buffer + shub->out_buffer_used, sizeof(ShubMessageHdr), buffer_size - shub->out_buffer_used);
                            int pos = 0;
                            if (available < sizeof(ShubMessageHdr)) { 
                                shub->params->error_handler("Failed to read inet socket", SHUB_RECOVERABLE_ERROR);
                                reconnect(shub);
                                continue;
                            }
                            shub->out_buffer_used += available;

                            /* loop through all received responses */
                            while (pos + sizeof(ShubMessageHdr) <= shub->out_buffer_used) { 
                                ShubMessageHdr* hdr = (ShubMessageHdr*)&shub->out_buffer[pos];
                                int chan = hdr->chan;
                                unsigned int n = pos + sizeof(ShubMessageHdr) + hdr->size <= shub->out_buffer_used 
                                    ? hdr->size + sizeof(ShubMessageHdr) 
                                    : shub->out_buffer_used - pos;
                                if (!write_socket(chan, (char*)hdr, n)) { 
                                    shub->params->error_handler("Failed to write to local socket", SHUB_RECOVERABLE_ERROR);
                                    close_socket(shub, chan);
                                    chan = -1;
                                }
                                if (n != hdr->size + sizeof(ShubMessageHdr)) { 
                                    /* read rest of message if it doesn't fit in the buffer */
                                    int tail = hdr->size + sizeof(ShubMessageHdr) - n;
                                    do {
                                        n = tail < buffer_size ? tail : buffer_size;
                                        if (!read_socket(shub->output, shub->out_buffer, n)) { 
                                            shub->params->error_handler("Failed to read inet socket", SHUB_RECOVERABLE_ERROR);
                                            reconnect(shub);
                                            continue;
                                        }
                                        if (chan >= 0 && !write_socket(chan, shub->out_buffer, n)) { 
                                            shub->params->error_handler("Failed to write to local socket", SHUB_RECOVERABLE_ERROR);
                                            close_socket(shub, chan);
                                            chan = -1;
                                        }                                       
                                        tail -= n;
                                    } while (tail != 0);

                                    pos = shub->out_buffer_used;
                                    break;
                                }
                                pos += n;
                            }
                            /* Move partly fetched message header (if any) to the beginning of buffer */
                            memcpy(shub->out_buffer, shub->out_buffer + pos, shub->out_buffer_used - pos);
                            shub->out_buffer_used -= pos;
                        } else { /* receive request from client */
                            int chan = i;
                            int available = 0;
                            while (1) { 
                                available += read_socket_ex(chan, &shub->in_buffer[shub->in_buffer_used + available], sizeof(ShubMessageHdr) - available, buffer_size - shub->in_buffer_used - available);
                                if (available < sizeof(ShubMessageHdr)) { 
                                    shub->params->error_handler("Failed to read local socket", SHUB_RECOVERABLE_ERROR);
                                    close_socket(shub, i);
                                } else { 
                                    int pos = 0;
                                    /* loop through all fetched messages */
                                    while (pos + sizeof(ShubMessageHdr) <= available) {
                                        ShubMessageHdr* hdr = (ShubMessageHdr*)&shub->in_buffer[shub->in_buffer_used];
                                        unsigned int size = hdr->size;
                                        pos += sizeof(ShubMessageHdr) + size;
                                        hdr->chan = chan; /* remember socket descriptor from which this message was read */
                                        if (pos <= available) {
                                            shub->in_buffer_used += sizeof(ShubMessageHdr) + size;
                                            continue;
                                        }
                                        
                                        if (shub->in_buffer_used + sizeof(ShubMessageHdr) + size > buffer_size) { 
                                            /* message doesn't completely fit in buffer */
                                            if (shub->in_buffer_used != 0) { /* if buffer is not empty...*/
                                                /* ... then send it */
                                                while (!write_socket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
                                                    shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                                    reconnect(shub);
                                                }
                                                /* move received message header to the beginning of the buffer */
                                                memcpy(shub->in_buffer, shub->in_buffer + shub->in_buffer_used, buffer_size - shub->in_buffer_used);
                                                shub->in_buffer_used = 0;
                                            }
                                        } 
                                        shub->in_buffer_used += sizeof(ShubMessageHdr) + size - (pos - available);
                                        size = pos - available; 
                                        
                                        do { 
                                            unsigned int n = size + shub->in_buffer_used > buffer_size ? buffer_size - shub->in_buffer_used : size;
                                            /* fetch rest of message body */
                                            if (chan >= 0 && !read_socket(chan, shub->in_buffer + shub->in_buffer_used, n)) { 
                                                shub->params->error_handler("Failed to read local socket", SHUB_RECOVERABLE_ERROR);
                                                close_socket(shub, chan);
                                                if (hdr != NULL) { /* if message header is not yet sent to the server... */
                                                    /* ... then skip this message */
                                                    shub->in_buffer_used = (char*)hdr - shub->in_buffer;
                                                    break;
                                                } else { /* if message was partly sent to the server, we can not skip it, so we have to send garbage to the server */
                                                    chan = -1; /* do not try to read rest of body of this message */
                                                }
                                            } 
                                            shub->in_buffer_used += n;
                                            size -= n;
                                            /* if there is no more free space in the buffer to receive new message header... */
                                            if (shub->in_buffer_used + sizeof(ShubMessageHdr) > buffer_size) {
                                                /* ... then send buffer to the server */
                                                while (!write_socket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
                                                    shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                                    reconnect(shub);
                                                }
                                                hdr = NULL; /* message is partly sent to the server: can not skip it any more */
                                                shub->in_buffer_used = 0;
                                            }
                                        } while (size != 0); /* repeat until all message body is received */

                                        pos = available;
                                        break;
                                    }
                                    if (chan >= 0 && pos != available) { /* partly fetched message header */
                                        if (shub->in_buffer_used + sizeof(ShubMessageHdr) > buffer_size) { 
                                            /* message doesn't completely fit in buffer */
                                            while (!write_socket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
                                                shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                                reconnect(shub);
                                            }
                                            /* move received message header to the beginning of the buffer */
                                            memcpy(shub->in_buffer, shub->in_buffer + shub->in_buffer_used, available - pos);
                                            shub->in_buffer_used = 0;
                                        }
                                        available -= pos;
                                        continue;
                                    }                                     
                                }
                                break;
                            }                        
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
                while (!write_socket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
                    shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                    reconnect(shub);
                }
                shub->in_buffer_used = 0;
            }    
        } 
    }
}
       
