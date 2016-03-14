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

#include "subr.h"

struct circular_buffer {
		char * buffer;		/* data buffer */
		char * buffer_end;	/* end of data buffer */
		unsigned int capacity;	/* maximum number of items in the buffer */
		unsigned int count;		/* number of items in the buffer */
		unsigned int sz;			/* size of each item in the buffer */
		char * head;		/* pointer to head */
		char * tail;		/* pointer to tail */
};

/* queue.c */
void cb_init(struct circular_buffer * cb, unsigned int capacity, unsigned int sz);
void cb_free(struct circular_buffer *cb);
void cb_clear(struct circular_buffer *cb);
int cb_queue_head(struct circular_buffer *cb, const char *data, int data_size);
char * cb_queue_tail(struct circular_buffer * cb, const char * data, unsigned int data_size, unsigned int offset);
char * cb_peek(struct circular_buffer * cb);
/* Remove buffer from head of queue */
char * cb_dequeue(struct circular_buffer * cb, int * buffer_size);
/* Remove buffer from tail of queue */
char * cb_dequeue_tail(struct circular_buffer *cb, int * buffer_size);
char * cb_push(char * data, int len);

#endif

