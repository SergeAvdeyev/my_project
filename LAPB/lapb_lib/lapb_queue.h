#ifndef LAPB_QUEUE_H
#define LAPB_QUEUE_H

#include <stdlib.h>
#include <string.h>

struct lapb_buff {
	unsigned int data_size;
	unsigned int reserved;
	unsigned char * data;
};


struct circular_buffer {
	unsigned char * buffer;		// data buffer
	unsigned char * buffer_end;	// end of data buffer
	size_t capacity;			// maximum number of items in the buffer
	size_t count;				// number of items in the buffer
	size_t sz;					// size of each item in the buffer
	//size_t sz_extra;			// extra bytes befor start of cb element, i.e. pointer to data of head element is head + sz_extra
	unsigned char * head;		// pointer to head
	unsigned char * tail;		// pointer to tail
};

void cb_init(struct circular_buffer * cb, size_t capacity, size_t sz);
void cb_free(struct circular_buffer * cb);
int cb_queue_tail(struct circular_buffer * cb, const unsigned char * data, unsigned short data_size);
//int cb_queue_tail(struct circular_buffer * cb, struct lapb_buff * lpb);
unsigned char * cb_peek(struct circular_buffer * cb);
unsigned char * cb_dequeue(struct circular_buffer * cb);

unsigned char * cb_push(unsigned char * data, unsigned int len);

#endif // LAPB_QUEUE_H

