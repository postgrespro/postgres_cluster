#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
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
#include <stddef.h>
#include <assert.h>

#include "sockhub.h"

#define MAX_CONNECT_ATTEMPTS 10

typedef struct 
{ 
    ShubMessageHdr hdr;
    int data;
} Message;

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

int connect_to_server(char const* host, int port, int max_attempts)
{
    struct sockaddr_in sock_inet;
    unsigned addrs[128];
    unsigned i, n_addrs = sizeof(addrs) / sizeof(addrs[0]);   
    int rc;
    int sd;

    if (strcmp(host, "localhost") == 0) {
        struct sockaddr sock;
        int len = offsetof(struct sockaddr, sa_data) + sprintf(sock.sa_data, "/tmp/p%u", port);
        sock.sa_family = AF_UNIX; 
        sd = socket(sock.sa_family,  SOCK_STREAM, 0);
        if (sd < 0) { 
            return -1;
        }

        while (1) { 
            do { 
                rc = connect(sd, &sock, len);
            } while (rc < 0 && EINTR);
            
             if (rc < 0) {             
                if (errno != ENOENT && errno != ECONNREFUSED && errno != EINPROGRESS) {
                    return -1;
                }
                if (max_attempts-- != 0) {
                    sleep(1);
                } else {
                    return -1;
                }                
             } else { 
                 break;
             }
        }                
    } else { 
        sock_inet.sin_family = AF_INET;  
        sock_inet.sin_port = htons(port);
        if (!resolve_host_by_name(host, addrs, &n_addrs)) { 
            return -1;
        }            
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if (sd < 0) { 
            return -1;
        }
        while (1) { 
            int rc = -1;
            for (i = 0; i < n_addrs; ++i) {
                memcpy(&sock_inet.sin_addr, &addrs[i], sizeof sock_inet.sin_addr);
                do { 
                    rc = connect(sd, (struct sockaddr*)&sock_inet, sizeof(sock_inet));
                } while (rc < 0 && errno == EINTR);
                
                if (rc >= 0 || errno == EINPROGRESS) { 
                    break;
                }
            }
            if (rc < 0) {             
                if (errno != ENOENT && errno != ECONNREFUSED && errno != EINPROGRESS) {
                    return -1;
                }
                if (max_attempts-- != 0) {
                    sleep(1);
                } else {
                    return -1;
                }                
            } else { 
                int optval = 1;
                setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(int));
                break;
            }
        }
    }
    return sd;
}



int main(int argc, char* argv[])
{
    int sd;
    int rc;
    int i;
    int n_iter = 10000;
    time_t start;

    if (argc < 3) {
        fprintf(stderr, "Usage: ./test-client HOST PORT [N_ITERATIONS]\n");
        return 1;
    }

    sd = connect_to_server(argv[1], atoi(argv[2]), MAX_CONNECT_ATTEMPTS);
    if (sd < 0) { 
        perror("Failed to connect to socket");
        return 1;
    }
    
    if (argc >= 3) {
        n_iter = atoi(argv[3]);
    }

    start = time(NULL);

    for (i = 0; i < n_iter; i++) {
        Message msg;
        msg.data = i;
        msg.hdr.size = sizeof(Message) - sizeof(ShubMessageHdr);
        rc = send(sd, &msg, sizeof(msg), 0);
        assert(rc == sizeof(msg));
        rc = recv(sd, &msg, sizeof(msg), 0);
        assert(rc == sizeof(msg) && msg.data == i+1);
    }
    
    printf("Elapsed time for %d iterations: %d\n", n_iter, (int)(time(NULL) - start));
    return 0;
}
    
    
    
