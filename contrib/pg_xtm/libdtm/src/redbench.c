#include <hiredis.h>
#include <execinfo.h>
#include <stdio.h>

static redisContext *c = NULL;
static redisReply *reply;
const char *hostname = "127.0.0.1";
static int port = 6379;
static struct timeval timeout = { 1, 500000 }; // 1.5 seconds


int main(){
    char buf[100];
    int xid;
    int i;


    if (c == NULL) {
          c = redisConnectWithTimeout(hostname, port, timeout);
      if (c->err) {
          printf("Connection error: %s\n", c->errstr);
          redisFree(c);
      } else {
          printf("Connected to redis \n");
      }
    }

    for (i=0; i<100000; i++){
        reply = redisCommand(c,"INCR pgXid");
        xid = reply->integer;
        freeReplyObject(reply);

        // sprintf(buf, "HSET pgXids tx%u 0", xid);
        // reply = redisCommand(c, buf);
        // freeReplyObject(reply);

        // sprintf(buf, "HSET pgXids tx%u 1", xid);
        // reply = redisCommand(c, buf);
        // freeReplyObject(reply);
    }



}

