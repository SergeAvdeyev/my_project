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

#include "lapb_iface.h"
#include "queue.h"
#include "subr.h"

/* Private lapb properties */
struct lapb_cs_internal {
	_uchar		state;
	_ushort		vs, vr, va;
	_uchar		condition;

	void *		T201_timer_ptr;		/* Pointer to timer T201 object */
	void *		T202_timer_ptr;		/* Pointer to timer T202 object */
	_ushort		RC;					/* Retry counter */
	_uchar		T201_state, T202_state;

	/* FRMR control information */
	struct lapb_frame	frmr_data;
	_uchar				frmr_type;

	struct circular_buffer	write_queue;
	struct circular_buffer	ack_queue;

	int extra_before;	/* Flag that need to send sequence of '1' before frame */
	int extra_after;	/* Flag that need to send sequence of '1' after frame */
};

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
void lapb_check_iframes_acked(struct lapb_cs *lapb, _ushort);
void lapb_check_need_response(struct lapb_cs *lapb, int, int);

/* lapb_subr.c */
//char * lapb_buf_to_str(char * data, int data_size);
void lapb_clear_queues(struct lapb_cs *lapb);
int lapb_frames_acked(struct lapb_cs *lapb, _ushort nr);
void lapb_requeue_frames(struct lapb_cs *lapb);
int lapb_validate_nr(struct lapb_cs *lapb, _ushort);
int lapb_decode(struct lapb_cs *lapb, char * data, int data_size, struct lapb_frame * frame);
void lapb_send_control(struct lapb_cs *lapb, int, int, int);
void lapb_transmit_frmr(struct lapb_cs *lapb);
void fill_inv_table();
_uchar invert_uchar(_uchar value);
struct lapb_cs_internal * lapb_get_internal(struct lapb_cs *lapb);


/* lapb_timer.c */
void lapb_start_t201timer(struct lapb_cs *lapb);
void lapb_stop_t201timer(struct lapb_cs *lapb);
void lapb_restart_t201timer(struct lapb_cs *lapb);
int lapb_t201timer_running(struct lapb_cs *lapb);
void lapb_start_t202timer(struct lapb_cs *lapb);
void lapb_stop_t202timer(struct lapb_cs *lapb);
int lapb_t202timer_running(struct lapb_cs *lapb);

#endif // LAPB_INT_H

