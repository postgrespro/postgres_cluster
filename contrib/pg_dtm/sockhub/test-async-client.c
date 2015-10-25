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
    int i, j;
    int n_iter = 10000;
    int n_msgs;
    time_t start, elapsed;
    int buffer_size = 64*1024;
    int port = 5001;
    char const* host = NULL;
    Message* msgs;

    for (i = 1; i < argc; i++) { 
        if (argv[i][0] == '-') { 
            switch (argv[i][1]) { 
              case 'h':
                host = argv[++i];
                break;
              case 'p':
                port = atoi(argv[++i]);
                break;
              case 'i':
                n_iter = atoi(argv[++i]);
                break;
              case 'b':
                buffer_size = atoi(argv[++i]);
                break;
              default:
                goto Usage;
            }
        } else {
            goto Usage;
        }
    }
    if (host == NULL) { 
      Usage:
        fprintf(stderr, "Usage: ./test-async-client [options]\n"
               "\t-h HOST server address\n"
               "\t-p PORT server port\n"
               "\t-i N number of iterations\n"
               "\t-b SIZE buffer size\n");
        return 1;
    }
    n_msgs = buffer_size / sizeof(Message);
    buffer_size = n_msgs*sizeof(Message);
    msgs = (Message*)malloc(buffer_size);

    sd = connect_to_server(host, port, MAX_CONNECT_ATTEMPTS);
    if (sd < 0) { 
        perror("Failed to connect to socket");
        return 1;
    }
    start = time(NULL);

    for (i = 0; i < n_iter; i++) {
        for (j = 0; j < n_msgs; j++) {
            msgs[j].data = i;
            msgs[j].hdr.size = sizeof(Message) - sizeof(ShubMessageHdr);
            msgs[j].hdr.code = MSG_FIRST_USER_CODE;
        }
        rc = ShubWriteSocket(sd, msgs, buffer_size);
        assert(rc);
        rc = ShubReadSocket(sd, msgs, buffer_size);
        assert(rc);
        for (j = 0; j < n_msgs; j++) {
            assert(msgs[j].data == i+1);
        }
    }
    
    elapsed = time(NULL) - start;
    printf("Elapsed time for %d iterations=%ld sec, TPS=%ld\n", n_iter, elapsed, (long)n_iter*n_msgs/elapsed);
    return 0;
}
    
    
    
