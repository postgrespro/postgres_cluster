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
#include <assert.h>

#include "../sockhub.h"

#define BUFFER_SIZE 64*1024
#define LISTEN_QUEUE_SIZE 100

typedef struct 
{ 
    ShubMessageHdr hdr;
    int data;
} Message;


int main(int argc, char* argv[])
{
    int sd;
    int i;
    int max_fd;
    struct sockaddr_in sock;
    fd_set inset;
    int port;
    int optval = 1;
    char buf[BUFFER_SIZE/sizeof(Message)*sizeof(Message)];

    if (argc < 2) {
        fprintf(stderr, "Usage: ./test-server PORT\n");
        return 1;
    }
    port = atoi(argv[1]);

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) { 
        perror("Failed to connect to socket");
        return 1;
    }
    setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char const*)&optval, sizeof(optval));
    
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = htonl(INADDR_ANY);
    sock.sin_port = htons(port);
    if (bind(sd, (struct sockaddr*)&sock, sizeof(sock))) { 
        perror("Failed to bind socket");
        return 1;
    }    
    if (listen(sd, LISTEN_QUEUE_SIZE) < 0) {
        perror("Failed to listen socket");
        return 1;
    }    
    FD_ZERO(&inset);
    FD_SET(sd, &inset);
    max_fd = sd;

    while (1) {
        fd_set events = inset;
        int rc = select(max_fd+1, &events, NULL, NULL, NULL);
        if (rc > 0) { 
            for (i = 0; i <= max_fd; i++) { 
                if (FD_ISSET(i, &events)) { 
                    if (i == sd) {
                        int s = accept(sd, NULL, NULL);
                        if (s < 0) { 
                            perror("Failed to accept socket");
                        } else {
                            FD_SET(s, &inset);
                            if (s > max_fd) { 
                                max_fd = s;
                            }
                        }
                    } else { 
                        int available = ShubReadSocketEx(i, buf, sizeof(ShubMessageHdr), sizeof(buf));
                        if (available >= sizeof(ShubMessageHdr)) {
                            int pos = 0;
                            while (pos + sizeof(Message) <= available) {
                                Message* msg = (Message*)&buf[pos];
                                if (msg->hdr.code == MSG_DISCONNECT) { 
                                    assert(msg->hdr.size == 0);
                                    printf("Disconnect client [%d:%d]\n", i, msg->hdr.chan);
                                    memmove(buf + pos, buf + pos + sizeof(ShubMessageHdr), available - pos - sizeof(ShubMessageHdr));
                                    available -= sizeof(ShubMessageHdr);
                                } else {                                     
                                    assert(sizeof(ShubMessageHdr) + msg->hdr.size == sizeof(Message));
                                    msg->data += 1;
                                    pos += sizeof(Message); 
                                }
                            }              
                            if (pos < available) {
                                ShubMessageHdr* hdr;
                                if (pos + sizeof(ShubMessageHdr) > available) {
                                    rc = ShubReadSocket(i, buf + available, sizeof(ShubMessageHdr) - (available - pos));
                                    assert(rc);
                                    available = pos + sizeof(ShubMessageHdr);
                                }
                                hdr = (ShubMessageHdr*)&buf[pos];
                                if (hdr->code == MSG_DISCONNECT) { 
                                    assert(pos + sizeof(ShubMessageHdr) == available);
                                    printf("Disconnect client [%d:%d]\n", i, hdr->chan);
                                } else {
                                    Message* msg = (Message*)hdr;
                                    if (pos + sizeof(Message) > available) {
                                        rc = ShubReadSocket(i, buf + available, sizeof(Message) - (available - pos));
                                        assert(rc);
                                    }                                    
                                    assert(sizeof(ShubMessageHdr) + msg->hdr.size == sizeof(Message));
                                    msg->data += 1;
                                    pos += sizeof(Message);
                                }
                            }              
                            rc = ShubWriteSocket(i, buf, pos);
                            assert(rc);
                        } else { 
                            perror("Failed to read socket");
                            FD_CLR(i, &inset);
                        }
                    }
                }
            }
        }
    }
}
