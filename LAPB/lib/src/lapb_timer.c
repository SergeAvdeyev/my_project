/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */


#include "lapb_int.h"


void lapb_start_t201timer(struct lapb_cs *lapb) {
	if (!lapb->callbacks->start_timer) return;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (lapb_int->T201_state) return;

	lapb->callbacks->start_timer(lapb_int->T201_timer_ptr);
	lapb_int->T201_state = TRUE;
	lapb->callbacks->debug(0, "[LAPB] start_t201timer is called");
}

void lapb_stop_t201timer(struct lapb_cs *lapb) {
	if (!lapb->callbacks->stop_timer) return;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (!lapb_int->T201_state) return;

	lapb->callbacks->stop_timer(lapb_int->T201_timer_ptr);
	lapb_int->T201_state = FALSE;
	lapb_int->RC = 0;
	lapb->callbacks->debug(0, "[LAPB] stop_t201timer is called");
}

void lapb_restart_t201timer(struct lapb_cs *lapb) {
	if (!lapb->callbacks->start_timer) return;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	//if ((lapb->T1_state) && (lapb->callbacks->stop_t1timer))
	//	lapb->callbacks->stop_t1timer(lapb);
	//lapb->T1_state = FALSE;
	lapb_int->RC = 0;

	if (!lapb_int->T201_state) {
		lapb->callbacks->start_timer(lapb_int->T201_timer_ptr);
		lapb_int->T201_state = TRUE;
	};
}

int lapb_t201timer_running(struct lapb_cs *lapb) {
	return lapb_get_internal(lapb)->T201_state;
}

void lapb_start_t202timer(struct lapb_cs *lapb) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (lapb_int->T202_state) return;

	if (!lapb->callbacks->start_timer) return;
	lapb->callbacks->start_timer(lapb_get_internal(lapb)->T202_timer_ptr);
	lapb_int->T202_state = TRUE;
	lapb->callbacks->debug(0, "[LAPB] start_t202timer is called");
}

void lapb_stop_t202timer(struct lapb_cs *lapb) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (!lapb_int->T202_state) return;

	if (!lapb->callbacks->stop_timer) return;
	lapb->callbacks->stop_timer(lapb_get_internal(lapb)->T202_timer_ptr);
	lapb_int->T202_state = FALSE;
	lapb->callbacks->debug(0, "[LAPB] stop_t202timer is called");
}

int lapb_t202timer_running(struct lapb_cs *lapb) {
	return lapb_get_internal(lapb)->T202_state;
}


void lapb_t202timer_expiry(void * lapb_ptr) {
	struct lapb_cs *lapb = lapb_ptr;
	if (!lapb) return;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	lapb->callbacks->debug(1, "[LAPB] S%d Timer_202 expired", lapb_int->state);
	if (lapb_int->condition & LAPB_ACK_PENDING_CONDITION) {
		lapb_int->condition &= ~LAPB_ACK_PENDING_CONDITION;
		lapb_timeout_response(lapb);
	};
	lapb_stop_t202timer(lapb);
}

void lapb_t201timer_expiry(void * lapb_ptr) {
	struct lapb_cs *lapb = lapb_ptr;
	if (!lapb) return;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	lapb_int->RC++;
	lapb->callbacks->debug(1, "[LAPB] S%d Timer_201 expired(%d of %d)",
						   lapb_int->state, lapb_int->RC, lapb->N201);
	switch (lapb_int->state) {
		/*
		 *	If we are a DCE, keep going DM .. DM .. DM
		 */
		case LAPB_STATE_0:
			if (lapb_is_dce(lapb)) {
				lapb_int->RC = 0;
				lapb->callbacks->debug(1, "[LAPB] S0 TX DM(0)");
				lapb_send_control(lapb, LAPB_DM, LAPB_POLLOFF, LAPB_RESPONSE);
			};
			break;

		/*
		 *	Awaiting connection state, send SABM(E), up to N2 times.
		 */
		case LAPB_STATE_1:
			if (lapb_int->RC >= lapb->N201) {
				lapb_stop_t201timer(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
			} else {
				if (lapb_is_extended(lapb)) {
					lapb->callbacks->debug(1, "[LAPB] S1 TX SABME(1)");
					lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
				} else {
					lapb->callbacks->debug(1, "[LAPB] S1 TX SABM(1)");
					lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
				};
			};
			break;

		/*
		 *	Awaiting disconnection state, send DISC, up to N2 times.
		 */
		case LAPB_STATE_2:
			if (lapb_int->RC >= lapb->N201) {
				lapb_stop_t201timer(lapb);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
			} else {
				lapb->callbacks->debug(1, "[LAPB] S2 TX DISC(1)");
				lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
			};
			break;

		/*
		 *	Data transfer state, restransmit I frames, up to N2 times.
		 */
		case LAPB_STATE_3:
			lapb_requeue_frames(lapb);
			if (lapb_int->RC >= lapb->N201) {
				lapb_stop_t201timer(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
			} else {
				if (lapb_int->condition & LAPB_FRMR_CONDITION)
					lapb_transmit_frmr(lapb);
				else
					lapb_kick(lapb);
			};
			break;

		/*
		 *	Frame reject state, restransmit FRMR frames, up to N2 times.
		 */
		case LAPB_STATE_4:
			if (lapb_int->RC >= lapb->N201) {
				lapb_stop_t201timer(lapb);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_TIMEDOUT);
				lapb_reset(lapb, LAPB_STATE_0);
			} else {
				lapb_transmit_frmr(lapb);
			};
			break;
	};
}
