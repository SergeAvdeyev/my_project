#ifndef CLIENT_TIMER_H
#define CLIENT_TIMER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "common.h"

struct timer_descr {
	int interval;
	int interval_tmp;
	int active;
	void (*timer_expiry)(unsigned long int lapb_addr);
};


struct timer_thread_struct {
	int interval;
	struct timer_descr * timers_list[10];
	unsigned long int lapb_addr;
};

void *timer_thread_function(void * ptr);

void terminate_timer_thread();
int is_timer_thread_started();

void timer_start(void * timer);
void timer_stop(void * timer);

#endif // CLIENT_TIMER_H
