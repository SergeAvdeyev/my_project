#ifndef CLIENT_TIMER_H
#define CLIENT_TIMER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

#define TRUE             1
#define FALSE            0

struct timer_struct {
	int interval;
	unsigned long int lapb_addr;
	void (*t1timer_expiry)(unsigned long int lapb_addr);
	void (*t2timer_expiry)(unsigned long int lapb_addr);
};


void timer_init();
void *timer_function(void * ptr);

void terminate_timer();
int is_timer_started();

void set_t1_state(int state);
void set_t1_value(int value);
void set_t2_state(int state);
void set_t2_value(int value);

#endif // CLIENT_TIMER_H
