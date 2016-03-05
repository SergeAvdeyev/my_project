#ifndef CLIENT_TIMER_H
#define CLIENT_TIMER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "common.h"

#ifndef UNSIGNED_TYPES
	typedef unsigned char	_uchar;
#endif

struct timer_descr {
	int interval;
	int interval_tmp;
	int active;
	void * lapb_ptr;
	void (*timer_expiry)(void * lapb_ptr);
};


struct timer_thread_struct {
	int						interval;
//	struct timer_descr **	timers_list;
//	int						timers_count;
};

void *timer_thread_function(void * ptr);

void terminate_timer_thread();
int is_timer_thread_started();

void * timer_add(int interval, void * lapb_ptr, void (*timer_expiry));
void timer_del(void * timer);
void timer_start(void * timer);
void timer_stop(void * timer);

#endif // CLIENT_TIMER_H
