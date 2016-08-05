#ifndef PROTO_H
#define PROTO_H

typedef struct Key {
	char data[64];
} Key;

typedef struct Value {
	char data[64];
} Value;

#define MEAN_FAIL '!'
#define MEAN_OK   '.'
#define MEAN_GET  '?'
#define MEAN_SET  '='

typedef struct Message {
	char meaning;
	Key key;
	Value value;
} Message;

#endif
