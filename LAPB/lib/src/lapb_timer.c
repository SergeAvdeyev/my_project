/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  Started Coding
 *
 *	This module:
 *		This module is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */

#include "lapb_int.h"



void lapb_start_t1timer(struct lapb_cb *lapb) {
	if (lapb->T1_state) return;

	if (!lapb->callbacks->start_t1timer) return;

	lapb->callbacks->start_t1timer(lapb);
	lapb->T1_state = TRUE;
}

void lapb_start_t2timer(struct lapb_cb *lapb) {
	if (lapb->T2_state) return;

	if (!lapb->callbacks->start_t2timer) return;
	lapb->callbacks->start_t2timer(lapb);
	lapb->T2_state = TRUE;
}

void lapb_stop_t1timer(struct lapb_cb *lapb) {
	if (!lapb->T1_state) return;

	if (!lapb->callbacks->stop_t1timer) return;
	lapb->callbacks->stop_t1timer(lapb);
	lapb->T1_state = FALSE;
	lapb->N2count = 0;
}

void lapb_stop_t2timer(struct lapb_cb *lapb) {
	if (!lapb->T2_state) return;

	if (!lapb->callbacks->stop_t2timer) return;
	lapb->callbacks->stop_t2timer(lapb);
	lapb->T2_state = FALSE;
}

int lapb_t1timer_running(struct lapb_cb *lapb) {
	return lapb->T1_state;
}

int lapb_t2timer_running(struct lapb_cb *lapb) {
	return lapb->T2_state;
}


void lapb_t2timer_expiry(struct lapb_cb *lapb) {
	if (!lapb) return;

	lapb->callbacks->debug(lapb, 1, "[LAPB] S%d Timer_2 expired", lapb->state);
	if (lapb->condition & LAPB_ACK_PENDING_CONDITION) {
		lapb->condition &= ~LAPB_ACK_PENDING_CONDITION;
		lapb_timeout_response(lapb);
	};
	lapb_stop_t2timer(lapb);
}

void lapb_t1timer_expiry(struct lapb_cb *lapb) {
	if (!lapb) return;

	lapb->N2count++;
	lapb->callbacks->debug(lapb, 1, "[LAPB] S%d Timer_1 expired(%d of %d)", lapb->state, lapb->N2count, lapb->N2);
	switch (lapb->state) {
		/*
		 *	If we are a DCE, keep going DM .. DM .. DM
		 */
		case LAPB_STATE_0:
			if (lapb->mode & LAPB_DCE) {
				lapb->N2count = 0;
				lapb->callbacks->debug(lapb, 1, "[LAPB] S0 TX DM(0)");
				lapb_send_control(lapb, LAPB_DM, LAPB_POLLOFF, LAPB_RESPONSE);
			};
			break;

		/*
		 *	Awaiting connection state, send SABM(E), up to N2 times.
		 */
		case LAPB_STATE_1:
			if (lapb->N2count >= lapb->N2) {
				lapb_stop_t1timer(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
				return;
			} else {
				if (lapb->mode & LAPB_EXTENDED) {
					lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX SABME(1)");
					lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
				} else {
					lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX SABM(1)");
					lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
				};
			};
			break;

		/*
		 *	Awaiting disconnection state, send DISC, up to N2 times.
		 */
		case LAPB_STATE_2:
			if (lapb->N2count >= lapb->N2) {
				lapb_stop_t1timer(lapb);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
				return;
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S2 TX DISC(1)");
				lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
			};
			break;

		/*
		 *	Data transfer state, restransmit I frames, up to N2 times.
		 */
		case LAPB_STATE_3:
			lapb_requeue_frames(lapb);
			if (lapb->N2count >= lapb->N2) {
				lapb_stop_t1timer(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
				return;
			} else
				lapb_kick(lapb);
			break;

		/*
		 *	Frame reject state, restransmit FRMR frames, up to N2 times.
		 */
		case LAPB_STATE_4:
			if (lapb->N2count >= lapb->N2) {
				lapb_stop_t1timer(lapb);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
				return;
			} else {
				lapb_transmit_frmr(lapb);
			};
			break;
	};
}
