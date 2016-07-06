#ifndef BUFFER_H
#define BUFFER_H

typedef struct Buffer {
	size_t bytes_written;
	size_t bytes_read;
	char data[BUFLEN];
} Buffer;

void buffer_init(Buffer *b);
size_t buffer_make_room(Buffer *b);
size_t buffer_can_write(Buffer *b);
size_t buffer_can_read(Buffer *b);

size_t buffer_extract_nomore(Buffer *b, void *dst, size_t bytes);
bool buffer_extract_exactly(Buffer *b, void *dst, size_t bytes);

#endif
