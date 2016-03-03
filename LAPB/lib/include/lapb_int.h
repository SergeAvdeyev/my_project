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


/* lapb_iface.c */
void lapb_connect_confirmation(struct lapb_cs *lapb, int);
void lapb_connect_indication(struct lapb_cs *lapb, int);
void lapb_disconnect_confirmation(struct lapb_cs *lapb, int);
void lapb_disconnect_indication(struct lapb_cs *lapb, int);
int lapb_data_indication(struct lapb_cs *lapb, char * data, int data_size);
int lapb_data_transmit(struct lapb_cs *lapb, char *data, int data_size);

/* lapb_in.c */
void lapb_data_input(struct lapb_cs *lapb, char * data, int data_size);

/* lapb_out.c */
void lapb_kick(struct lapb_cs *lapb);
void lapb_transmit_buffer(struct lapb_cs *lapb, char *data, int data_size, int type);
void lapb_establish_data_link(struct lapb_cs *lapb);
void lapb_enquiry_response(struct lapb_cs *lapb);
void lapb_timeout_response(struct lapb_cs *lapb);
void lapb_check_iframes_acked(struct lapb_cs *lapb, unsigned short);
void lapb_check_need_response(struct lapb_cs *lapb, int, int);

/* lapb_subr.c */
void lock(struct lapb_cs *lapb);
void unlock(struct lapb_cs *lapb);
char * lapb_buf_to_str(char * data, int data_size);
void lapb_clear_queues(struct lapb_cs *lapb);
int lapb_frames_acked(struct lapb_cs *lapb, unsigned short nr);
void lapb_requeue_frames(struct lapb_cs *lapb);
int lapb_validate_nr(struct lapb_cs *lapb, unsigned short);
int lapb_decode(struct lapb_cs *lapb, char * data, int data_size, struct lapb_frame * frame);
void lapb_send_control(struct lapb_cs *lapb, int, int, int);
void lapb_transmit_frmr(struct lapb_cs *lapb);
void fill_inv_table();
_uchar invert_uchar(_uchar value);


/* lapb_timer.c */
void lapb_start_t1timer(struct lapb_cs *lapb);
void lapb_stop_t1timer(struct lapb_cs *lapb);
void lapb_restart_t1timer(struct lapb_cs *lapb);
int lapb_t1timer_running(struct lapb_cs *lapb);
void lapb_start_t2timer(struct lapb_cs *lapb);
void lapb_stop_t2timer(struct lapb_cs *lapb);
int lapb_t2timer_running(struct lapb_cs *lapb);

/* lapb_queue.c */
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

#endif // LAPB_INT_H

