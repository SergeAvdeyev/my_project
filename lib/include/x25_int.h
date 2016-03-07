/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */

#ifndef X25_INT_H
#define X25_INT_H

#include <stdio.h>

#include "x25_iface.h"
#include "queue.h"

/* Private lapb properties */
struct x25_cs_internal {

#if INTERNAL_SYNC
	pthread_mutex_t		_mutex;
#endif
};

/* x25_iface.c */



/* x25_in.c */



/* x25_out.c */



/* x25_subr.c */
void lock(struct x25_cs *x25);
void unlock(struct x25_cs *x25);
struct x25_cs_internal * x25_get_internal(struct x25_cs *x25);
/* Redefine 'malloc' and 'free' */
void * x25_mem_get(_ulong size);
void x25_mem_free(void *ptr);
void * x25_mem_copy(void *dest, const void *src, _ulong n);
void x25_mem_zero(void *src, _ulong n);
int x25_pacsize_to_bytes(unsigned int pacsize);
void x25_write_internal(struct x25_cs *x25, int frametype);


/* x25_timer.c */

void x25_start_heartbeat(struct x25_cs *x25);
void x25_stop_heartbeat(struct x25_cs *x25);

void x25_start_timer(struct x25_cs *x25, struct x25_timer * _timer);
void x25_stop_timer(struct x25_cs *x25, struct x25_timer * _timer);
int x25_timer_running(struct x25_timer * _timer);
void x25_t21timer_expiry(void * x25_ptr);
void x25_t22timer_expiry(void * x25_ptr);
void x25_t23timer_expiry(void * x25_ptr);
void x25_t2timer_expiry(void * x25_ptr);

#endif // X25_INT_H

