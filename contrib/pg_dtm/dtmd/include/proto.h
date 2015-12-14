#ifndef PROTO_H
#define PROTO_H

#define CMD_RESERVE  'r'
#define CMD_BEGIN    'b'
#define CMD_FOR      'y'
#define CMD_AGAINST  'n'
#define CMD_SNAPSHOT 'h'
#define CMD_STATUS   's'

#define RES_FAILED 0xDEADBEEF
#define RES_OK 0xC0FFEE
#define RES_TRANSACTION_COMMITTED 1
#define RES_TRANSACTION_ABORTED 2
#define RES_TRANSACTION_INPROGRESS 3
#define RES_TRANSACTION_UNKNOWN 4

#endif
