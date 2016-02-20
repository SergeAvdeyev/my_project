/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */

#ifndef LAPB_INT_H
#define LAPB_INT_H

#include <stdio.h>

#include "lapb_iface.h"
#include "lapb_queue.h"

#if INTERNAL_SYNC
#include <pthread.h>
#endif

/* lapb_iface.c */
void lapb_connect_indication(struct lapb_cb *lapb, int);
void lapb_disconnect_indication(struct lapb_cb *lapb, int);
int lapb_data_indication(struct lapb_cb *lapb, char * data, int data_size);
int lapb_data_transmit(struct lapb_cb *lapb, char *data, int data_size);

/* lapb_in.c */
void lapb_data_input(struct lapb_cb *lapb, char * data, int data_size);

/* lapb_out.c */
void lapb_kick(struct lapb_cb *lapb);
void lapb_transmit_buffer(struct lapb_cb *lapb, char *data, int data_size, int type);
void lapb_establish_data_link(struct lapb_cb *lapb);
void lapb_enquiry_response(struct lapb_cb *lapb);
void lapb_timeout_response(struct lapb_cb *lapb);
void lapb_check_iframes_acked(struct lapb_cb *lapb, unsigned short);
void lapb_check_need_response(struct lapb_cb *lapb, int, int);

/* lapb_subr.c */
void lock(struct lapb_cb *lapb);
void unlock(struct lapb_cb *lapb);
char * lapb_buf_to_str(char * data, int data_size);
void lapb_clear_queues(struct lapb_cb *lapb);
int lapb_frames_acked(struct lapb_cb *lapb, unsigned short nr);
void lapb_requeue_frames(struct lapb_cb *lapb);
int lapb_validate_nr(struct lapb_cb *lapb, unsigned short);
int lapb_decode(struct lapb_cb *lapb, char * data, int data_size, struct lapb_frame * frame);
void lapb_send_control(struct lapb_cb *lapb, int, int, int);
void lapb_transmit_frmr(struct lapb_cb *lapb);
void fill_inv_table();
_uchar invert_uchar(_uchar value);


/* lapb_timer.c */
void lapb_start_t1timer(struct lapb_cb *lapb);
void lapb_start_t2timer(struct lapb_cb *lapb);
void lapb_stop_t1timer(struct lapb_cb *lapb);
void lapb_stop_t2timer(struct lapb_cb *lapb);
int lapb_t1timer_running(struct lapb_cb *lapb);
int lapb_t2timer_running(struct lapb_cb *lapb);

#endif // LAPB_INT_H

