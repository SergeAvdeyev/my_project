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

/* Define Link State constants. */
enum {
	X25_LINK_STATE_0,
	X25_LINK_STATE_1,
	X25_LINK_STATE_2,
	X25_LINK_STATE_3
};


/* Private lapb properties */
struct x25_cs_internal {

#if INTERNAL_SYNC
	pthread_mutex_t		_mutex;
#endif
};

/* x25_iface.c */



/* x25_in.c */



/* x25_out.c */


/* x25_link.c */
void x25_transmit_link(struct x25_link *nb, char * data, int data_size);


/* x25_facilities.c */
int x25_parse_facilities(struct x25_cs *x25,
						 struct sk_buff *skb,
						 struct x25_facilities *facilities,
						 struct x25_dte_facilities *dte_facs,
						 _ulong *vc_fac_mask);
int x25_create_facilities(//struct x25_cs * x25,
						  _uchar *buffer,
						  struct x25_facilities *facilities,
						  struct x25_dte_facilities *dte_facs,
						  _ulong facil_mask);
int x25_negotiate_facilities(struct x25_cs *x25,
							 struct sk_buff *skb,
							 struct x25_facilities *_new,
							 struct x25_dte_facilities *dte);
void x25_limit_facilities(struct x25_cs *x25);

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
int x25_addr_aton(_uchar *p, struct x25_address *called_addr, struct x25_address *calling_addr);
void x25_write_internal(struct x25_cs *x25, int frametype);


/* x25_timer.c */

void x25_start_heartbeat(struct x25_cs *x25);
void x25_stop_heartbeat(struct x25_cs *x25);

void x25_start_timer(struct x25_cs *x25, struct x25_timer * _timer);
void x25_stop_timer(struct x25_cs *x25, struct x25_timer * _timer);
int x25_timer_running(struct x25_timer * _timer);
void x25_t20timer_expiry(void * x25_ptr);
void x25_t21timer_expiry(void * x25_ptr);
void x25_t22timer_expiry(void * x25_ptr);
void x25_t23timer_expiry(void * x25_ptr);
void x25_t2timer_expiry(void * x25_ptr);

#endif // X25_INT_H

