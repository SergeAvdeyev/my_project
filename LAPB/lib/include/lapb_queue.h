/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */

#ifndef LAPB_QUEUE_H
#define LAPB_QUEUE_H

#include "lapb_iface.h"


void cb_init(struct circular_buffer * cb, size_t capacity, size_t sz);
void cb_free(struct circular_buffer *cb);
void cb_clear(struct circular_buffer *cb);
int cb_queue_head(struct circular_buffer *cb, const char *data, int data_size);
int cb_queue_tail(struct circular_buffer * cb, const char * data, int data_size);
char * cb_peek(struct circular_buffer * cb);
/* Remove buffer from head of queue */
char * cb_dequeue(struct circular_buffer * cb, int * buffer_size);
/* Remove buffer from tail of queue */
char * cb_dequeue_tail(struct circular_buffer *cb, int * buffer_size);
char * cb_push(char * data, int len);

#endif // LAPB_QUEUE_H

