/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */


#include "queue.h"


void cb_init(struct circular_buffer *cb, size_t capacity, size_t sz) {
	cb->buffer = mem_get(capacity * (sz + 8)); /* 4 bytes for data_size, 4 bytes reserved (for push operations), 2 bytes for FCS */

	cb->buffer_end = cb->buffer + capacity * (sz + 8);
	cb->capacity = capacity;
	cb->count = 0;
	cb->sz = sz;
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
}

void cb_free(struct circular_buffer *cb) {
	if (!cb->buffer) return;
	mem_free(cb->buffer);
	cb->buffer = NULL;
	// clear out other fields too, just to be safe
}

void cb_clear(struct circular_buffer *cb) {
	if (cb->buffer == NULL) return;
	cb->count = 0;
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
}

int cb_queue_head(struct circular_buffer *cb, const char *data, int data_size) {
	if (cb->buffer == NULL) return 0;
	if (cb->count == cb->capacity)
		return 0;

	if (cb->head == cb->buffer)
		cb->head = cb->buffer_end - cb->sz - 8;
	else
		cb->head = cb->head - cb->sz - 8;
	*(int *)(cb->head) = data_size;
	mem_copy(cb->head + 8, data, data_size);

	cb->count++;
	return 1;
}

int cb_queue_tail(struct circular_buffer *cb, const char *data, int data_size) {
	if (cb->buffer == NULL) return 0;
	if (cb->count == cb->capacity)
		return 0;

	*(int *)(cb->tail) = data_size;
	mem_copy(cb->tail + 8, data, data_size);

	cb->tail = cb->tail + cb->sz + 8;
	if (cb->tail == cb->buffer_end)
		cb->tail = cb->buffer;
	cb->count++;
	return 1;
}

char * cb_peek(struct circular_buffer *cb) {
	if (cb->buffer == NULL) return NULL;
	if (cb->count == 0)
		return NULL;
	else
		return cb->head;
}

char * cb_dequeue(struct circular_buffer *cb, int * buffer_size) {
	if (cb->buffer == NULL) return NULL;
	if (cb->count == 0)
		return NULL;
	char * result = NULL;
	if (buffer_size) {
		result = cb->head + 8;
		*buffer_size = *(int *)cb->head;
	};
	cb->head = cb->head + cb->sz + 8;
	if (cb->head == cb->buffer_end)
		cb->head = cb->buffer;
	cb->count--;
	return result;
}

char * cb_dequeue_tail(struct circular_buffer *cb, int * buffer_size) {
	if (cb->buffer == NULL) return NULL;
	if (cb->count == 0)
		return NULL;
	char * result = NULL;

	if (cb->tail == cb->buffer)
		cb->tail = cb->buffer_end - cb->sz - 8;
	else
		cb->tail = cb->tail - cb->sz - 8;

	if (buffer_size) {
		result = cb->tail + 8;
		*buffer_size = *(int *)cb->tail;
	};
	cb->count--;
	return result;
}

char * cb_push(char * data, int len) {
	if ((!data) || (len > 4))
		return NULL;
	return data - len;
}
