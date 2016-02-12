
#include "lapb_queue.h"

void cb_init(struct circular_buffer *cb, size_t capacity, size_t sz) {
	cb->buffer = malloc(capacity * (sz + 8)); /* 4 bytes for data_size, 4 bytes reserved (for push operations) */

	cb->buffer_end = cb->buffer + capacity * (sz + 8);
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
//int cb_queue_tail(struct circular_buffer *cb, struct lapb_buff * lpb) {
	if (cb->count == cb->capacity)
		return 0;

	struct lapb_buff * lpb = (struct lapb_buff *)(cb->tail);
	lpb->data_size = data_size;
	//*(unsigned short *)(cb->tail) = data_size;
	memcpy(lpb->data, data, data_size);

	cb->tail = cb->tail + cb->sz + 8;
	if (cb->tail == cb->buffer_end)
		cb->tail = cb->buffer;
	cb->count++;
	return 1;
}

unsigned char * cb_peek(struct circular_buffer *cb) {
	if (cb->count == 0)
		return NULL;
	else
		return cb->head;
}

unsigned char * cb_dequeue(struct circular_buffer *cb) {
	if (cb->count == 0)
		return NULL;
	unsigned char * result = cb->head;
	//memcpy(item, cb->tail, cb->sz);
	cb->head = cb->head + cb->sz + 8;
	if (cb->head == cb->buffer_end)
		cb->head = cb->buffer;
	cb->count--;
	return result;
}

unsigned char * cb_push(unsigned char * data, unsigned int len) {
	if ((!data) || (len > sizeof(unsigned int)))
		return NULL;
	return data - len;
}
