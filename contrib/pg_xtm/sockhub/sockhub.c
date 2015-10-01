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
    memset(params, 0, sizeof params);
    params->buffer_size = 64*1025;
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
    int i, max_fd;
    fd_set copy;
    FD_ZERO(&copy);
    close(fd);
    for (i = 0, max_fd = shub->max_fd; i <= max_fd; i++) {
        if (i != fd && FD_ISSET(i, &shub->inset)) { 
            FD_SET(i, &copy);
        }
    }
    FD_COPY(&copy, &shub->inset);
}

static int read_socket(int sd, char* buf, int size)
{
    while (size != 0) { 
        int n = recv(sd, buf, size , 0);
        if (n <= 0) { 
            return 0;
        } 
        size -= n;
        buf += n;
    }
    return 1;
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
                if (rc >= 0) {
                }
                break;
            }
        }
        if (rc < 0) {             
            if (errno != ENOENT && errno != ECONNREFUSED) {
                shub->params->error_handler("Connection can not be establish", SHUB_FATAL_ERROR);
            }
            if (max_attempts-- != 0) {
                sleep(1);
            } else {
                shub->params->error_handler("Failed to connect to host", SHUB_FATAL_ERROR);
            }                
        } else { 
            int optval = 1;
            setsockopt(shub->output, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(int));
            FD_SET(shub->output, &shub->inset);
            break;
        }
    }
}

static void recovery(Shub* shub)
{
    int i, max_fd;
    fd_set okset;
    fd_set tryset;

    for (i = 0, max_fd = shub->max_fd; i <= max_fd; i++) {
        if (FD_ISSET(i, &shub->inset)) { 
            struct timeval tm = {0,0};
            FD_ZERO(&tryset);
            FD_SET(i, &tryset);
            if (select(i+1, &tryset, NULL, NULL, &tm) >= 0) { 
                FD_SET(i, &okset);
            }
        }
    }
    FD_COPY(&okset, &shub->inset);
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
    
    reconnect(shub);

    shub->in_buffer = malloc(params->buffer_size);
    shub->out_buffer = malloc(params->buffer_size);
    if (shub->in_buffer == NULL || shub->out_buffer == NULL) { 
        shub->params->error_handler("Failed to allocate buffer", SHUB_FATAL_ERROR);
    }
}


void ShubLoop(Shub* shub)
{    
    int buffer_size = shub->params->buffer_size;

    while (1) { 
        fd_set events;
        struct timeval tm;
        int i, max_fd, rc;
        unsigned int n, size;

        tm.tv_sec = shub->params->delay/1000;
        tm.tv_usec = shub->params->delay % 1000 * 1000;


        FD_COPY(&shub->inset, &events);
        rc = select(shub->max_fd+1, &events, NULL, NULL, shub->in_buffer_used == 0 ? NULL : &tm);
        if (rc < 0) { 
            if (errno != EINTR) {                
                shub->params->error_handler("Select failed", SHUB_RECOVERABLE_ERROR);
                recovery(shub);
            }
        } else {
            if (rc > 0) {
                for (i = 0, max_fd = shub->max_fd; i <= max_fd; i++) { 
                    if (FD_ISSET(i, &events)) { 
                        if (i == shub->input) { 
                            int s = accept(i, NULL, NULL);
                            if (s < 0) { 
                                shub->params->error_handler("Failed to accept socket", SHUB_RECOVERABLE_ERROR);
                            } else {
                                if (s > max_fd) {
                                    shub->max_fd = s;
                                }
                                FD_SET(s, &shub->inset);
                            }
                        } else if (i == shub->output) {
                            int available = recv(shub->output, shub->out_buffer + shub->out_buffer_used, buffer_size - shub->out_buffer_used, 0);
                            int pos = 0;
                            if (available <= 0) { 
                                shub->params->error_handler("Failed to read inet socket", SHUB_RECOVERABLE_ERROR);
                                reconnect(shub);
                            }
                            shub->out_buffer_used += available;
                            while (pos + sizeof(ShubMessageHdr) <= shub->out_buffer_used) { 
                                ShubMessageHdr* hdr = (ShubMessageHdr*)shub->out_buffer;
                                int chan = hdr->chan;
                                pos += sizeof(ShubMessageHdr);
                                n = pos + hdr->size <= shub->out_buffer_used ? hdr->size + sizeof(ShubMessageHdr) : shub->out_buffer_used - pos;
                                if (!write_socket(chan, (char*)hdr, n)) { 
                                    shub->params->error_handler("Failed to write to local socket", SHUB_RECOVERABLE_ERROR);
                                    close_socket(shub, chan);
                                    chan = -1;
                                }
                                if (n != hdr->size + sizeof(ShubMessageHdr)) { 
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
                                }
                            }
                            memcpy(shub->out_buffer, shub->out_buffer + pos, shub->out_buffer_used - pos);
                            shub->out_buffer_used -= pos;
                        } else {
                            ShubMessageHdr* hdr = (ShubMessageHdr*)&shub->in_buffer[shub->in_buffer_used];
                            if (!read_socket(i, (char*)hdr, sizeof(ShubMessageHdr))) { 
                                shub->params->error_handler("Failed to read local socket", SHUB_RECOVERABLE_ERROR);
                                close_socket(shub, i);
                            } else { 
                                size = hdr->size;
                                hdr->chan = i;
                                if (size + shub->in_buffer_used + sizeof(ShubMessageHdr) > buffer_size) { 
                                    if (shub->in_buffer_used != 0) { 
                                        while (!write_socket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
                                            shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                            reconnect(shub);
                                        }
                                        memcpy(shub->in_buffer, shub->in_buffer + shub->in_buffer_used, sizeof(ShubMessageHdr));
                                        shub->in_buffer_used = 0;
                                    }
                                } 
                                shub->in_buffer_used += sizeof(ShubMessageHdr);

                                while (1) { 
                                    unsigned int n = size + shub->in_buffer_used > buffer_size ? buffer_size - shub->in_buffer_used : size;
                                    if (!read_socket(i, shub->in_buffer + shub->in_buffer_used, n)) { 
                                        shub->params->error_handler("Failed to read local socket", SHUB_RECOVERABLE_ERROR);
                                        close_socket(shub, i);
                                        break;
                                    } else { 
                                        if (n != size) { 
                                            while (!write_socket(shub->output, shub->in_buffer, n)) {
                                                shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                                                reconnect(shub);
                                            }
                                            size -= n;
                                            shub->in_buffer_used = 0;
                                        } else { 
                                            shub->in_buffer_used += n;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (shub->in_buffer_used != 0) { 
                while (!write_socket(shub->output, shub->in_buffer, shub->in_buffer_used)) {
                    shub->params->error_handler("Failed to write to inet socket", SHUB_RECOVERABLE_ERROR);
                    reconnect(shub);
                }                
            }    
        } 
    }
}
       
