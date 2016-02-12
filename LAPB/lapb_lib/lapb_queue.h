#ifndef LAPB_QUEUE_H
#define LAPB_QUEUE_H

#include <stdlib.h>
#include <string.h>

struct circular_buffer {
	unsigned char *buffer;		// data buffer
	unsigned char *buffer_end;	// end of data buffer
	size_t capacity;			// maximum number of items in the buffer
	size_t count;				// number of items in the buffer
	size_t sz;					// size of each item in the buffer
	unsigned char *head;		// pointer to head
	unsigned char *tail;		// pointer to tail
};

void cb_init(struct circular_buffer *cb, size_t capacity, size_t sz);
void cb_free(struct circular_buffer *cb);
int cb_queue_tail(struct circular_buffer *cb, const unsigned char *data, unsigned short data_size);
void cb_pop_front(struct circular_buffer *cb, void *item);

#endif // LAPB_QUEUE_H

