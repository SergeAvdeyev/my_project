/*
 *	LAPB release 002
 *
 *	This code REQUIRES 2.1.15 or higher/ NET3.038
 *
 *	This module:
 *		This module is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *	History
 *	LAPB 001	Jonathan Naylor	Started Coding
 *	LAPB 002	Jonathan Naylor	New timer architecture.
 */


#include "net_lapb.h"



void lapb_start_t1timer(struct lapb_cb *lapb) {
	if (lapb->callbacks->start_t1timer)
		lapb->callbacks->start_t1timer(lapb);
}

void lapb_start_t2timer(struct lapb_cb *lapb) {
	if (lapb->callbacks->start_t2timer)
		lapb->callbacks->start_t2timer(lapb);
}

void lapb_stop_t1timer(struct lapb_cb *lapb) {
	if (lapb->callbacks->stop_t1timer)
		lapb->callbacks->stop_t1timer(lapb);
	lapb->N2count = 0;
}

void lapb_stop_t2timer(struct lapb_cb *lapb) {
	if (lapb->callbacks->stop_t2timer)
		lapb->callbacks->stop_t2timer(lapb);
}

int lapb_t1timer_running(struct lapb_cb *lapb) {
	if (lapb->callbacks->t1timer_running)
		return lapb->callbacks->t1timer_running();
	return 0;
}



void lapb_t2timer_expiry(struct lapb_cb *lapb) {
	if (!lapb) return;

	if (lapb->condition & LAPB_ACK_PENDING_CONDITION) {
		lapb->condition &= ~LAPB_ACK_PENDING_CONDITION;
		lapb_timeout_response(lapb);
	};
}

void lapb_t1timer_expiry(struct lapb_cb *lapb) {
	if (!lapb) return;

	switch (lapb->state) {

		/*
		 *	If we are a DCE, keep going DM .. DM .. DM
		 */
		case LAPB_STATE_0:
			if (lapb->mode & LAPB_DCE)
				lapb_send_control(lapb, LAPB_DM, LAPB_POLLOFF, LAPB_RESPONSE);
			break;

		/*
		 *	Awaiting connection state, send SABM(E), up to N2 times.
		 */
		case LAPB_STATE_1:
			if (lapb->N2count == lapb->N2) {
				lapb_clear_queues(lapb);
				lapb->state = LAPB_STATE_0;
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb->callbacks->debug(lapb, 0, "S1 -> S0\n");
				return;
			} else {
				lapb->N2count++;
				if (lapb->mode & LAPB_EXTENDED) {
					lapb->callbacks->debug(lapb, 1, "S1 TX SABME(1)\n");
					lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
				} else {
					lapb->callbacks->debug(lapb, 1, "S1 TX SABM(1)\n");
					lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
				};
			};
			break;

		/*
		 *	Awaiting disconnection state, send DISC, up to N2 times.
		 */
		case LAPB_STATE_2:
			if (lapb->N2count == lapb->N2) {
				lapb_clear_queues(lapb);
				lapb->state = LAPB_STATE_0;
				lapb_disconnect_confirmation(lapb, LAPB_TIMEDOUT);
				lapb->callbacks->debug(lapb, 0, "S2 -> S0\n");
				return;
			} else {
				lapb->N2count++;
				lapb->callbacks->debug(lapb, 1, "S2 TX DISC(1)\n");
				lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
			};
			break;

		/*
		 *	Data transfer state, restransmit I frames, up to N2 times.
		 */
		case LAPB_STATE_3:
			if (lapb->N2count == lapb->N2) {
				lapb_clear_queues(lapb);
				lapb->state = LAPB_STATE_0;
				lapb_stop_t2timer(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb->callbacks->debug(lapb, 0, "S3 -> S0\n");
				return;
			} else {
				lapb->N2count++;
				lapb_requeue_frames(lapb);
				lapb_kick(lapb, NULL, 0);
			};
			break;

		/*
		 *	Frame reject state, restransmit FRMR frames, up to N2 times.
		 */
		case LAPB_STATE_4:
			if (lapb->N2count == lapb->N2) {
				lapb_clear_queues(lapb);
				lapb->state = LAPB_STATE_0;
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb->callbacks->debug(lapb, 0, "S4 -> S0\n");
				return;
			} else {
				lapb->N2count++;
				lapb_transmit_frmr(lapb);
			};
			break;
	};

	lapb_start_t1timer(lapb);
}
