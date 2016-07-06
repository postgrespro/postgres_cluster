#include "buffer.h"

void buffer_init(Buffer *b)
{
	b->bytes_written = 0;
	b->bytes_read = 0;
}

size_t buffer_make_room(Buffer *b)
{
	if (b->bytes_read > 0)
	{
		memmove(
			b->data,
			b->data + b->bytes_read,
			b->bytes_written - b->bytes_read
		);
		b->bytes_written -= b->bytes_read;
		b->bytes_read = 0;
	}
	return buffer_can_write(b);
}

size_t buffer_can_write(Buffer *b)
{
	return sizeof(b->data) - b->bytes_written;
}

size_t buffer_can_read(Buffer *b)
{
	return b->bytes_written - b->bytes_read;
}

bool buffer_from_socket(Buffer *b, int socket)
{
	ssize_t recved;
	size_t avail = buffer_can_write(b);
	if (avail < sizeof(int))
		avail = buffer_make_room(b);
	if (!avail) return true;

	recved = recv(socket, b->data + b->bytes_written, avail, MSG_DONTWAIT);
	if (recved == 0) return false;
	if (recved < 0) return (errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR);

	b->bytes_written += recved;
	return true;
}

bool buffer_to_socket(Buffer *b, int socket)
{
	ssize_t sent;
	size_t avail = buffer_can_read(b);
	if (!avail) return true;

	sent = send(socket, b->data + b->bytes_read, avail, MSG_DONTWAIT);
	if (sent == 0) return false;
	if (sent < 0) return (errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR);

	b->bytes_read += sent;
	return true;
}

size_t buffer_extract_nomore(Buffer *b, void *dst, size_t bytes)
{
	size_t avail = buffer_can_read(b);
	if (avail < bytes) bytes = avail;
	memcpy(dst, b->data + b->bytes_read, bytes);
	b->bytes_read += bytes;
	return bytes;
}

bool buffer_extract_exactly(Buffer *b, void *dst, size_t bytes)
{
	size_t avail = buffer_can_read(b);
	if (avail < bytes) return false;

	memcpy(dst, b->data + b->bytes_read, bytes);
	b->bytes_read += bytes;

	return true;
}
