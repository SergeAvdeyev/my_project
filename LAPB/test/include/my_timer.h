#ifndef CLIENT_TIMER_H
#define CLIENT_TIMER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "common.h"

struct timer_struct {
	int interval;
	unsigned long int lapb_addr;
	void (*t1timer_expiry)(unsigned long int lapb_addr);
	void (*t2timer_expiry)(unsigned long int lapb_addr);
};


void *timer_function(void * ptr);

void terminate_timer();
int is_timer_started();

void timer_t1_start();
void timer_t1_stop();
void timer_t1_set_interval(int value);
int  timer_t1_get_state();

void timer_t2_start();
void timer_t2_stop();
void timer_t2_set_interval(int value);
int  timer_t2_get_state();

#endif // CLIENT_TIMER_H
