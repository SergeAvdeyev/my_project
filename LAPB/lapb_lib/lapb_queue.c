
#include "lapb_queue.h"

void cb_init(struct circular_buffer *cb, size_t capacity, size_t sz) {
	cb->buffer = malloc(capacity * sz);
//	if (cb->buffer == NULL)
//		// handle error
	cb->buffer_end = cb->buffer + capacity * sz;
	cb->capacity = capacity;
	cb->count = 0;
	cb->sz = sz;
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
}

void cb_free(struct circular_buffer *cb) {
	free(cb->buffer);
	// clear out other fields too, just to be safe
}

int cb_queue_tail(struct circular_buffer *cb, const unsigned char *data, unsigned short data_size) {
	if (cb->count == cb->capacity)
		return 0;
	*(unsigned short *)(cb->head) = data_size;
	memcpy(cb->head + 2, data, data_size);
	cb->head = cb->head + cb->sz;
	if (cb->head == cb->buffer_end)
		cb->head = cb->buffer;
	cb->count++;
	return 1;
}

void cb_pop_front(struct circular_buffer *cb, void *item) {
	if (cb->count == 0)
		// handle error
	memcpy(item, cb->tail, cb->sz);
	cb->tail = cb->tail + cb->sz;
	if (cb->tail == cb->buffer_end)
		cb->tail = cb->buffer;
	cb->count--;
}
