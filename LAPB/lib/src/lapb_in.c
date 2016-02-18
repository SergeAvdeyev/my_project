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

/*
 *	State machine for state 0, Disconnected State.
 *	The handling of the timer(s) is in file lapb_timer.c.
 */
static void lapb_state0_machine(struct lapb_cb *lapb, struct lapb_frame *frame) {
	switch (frame->type) {
		case LAPB_SABM:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S0 RX SABM(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S0 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S0 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb->state     = LAPB_STATE_3;
				lapb_stop_t1timer(lapb);
				lapb->condition = 0x00;
				lapb->vs        = 0;
				lapb->vr        = 0;
				lapb->va        = 0;
				lapb->callbacks->debug(lapb, 0, "[LAPB] S0 -> S3");
				lapb_connect_indication(lapb, LAPB_OK);
			};
			break;

		case LAPB_SABME:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S0 RX SABME(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S0 TX UA(%d)", frame->pf);
				lapb->callbacks->debug(lapb, 0, "[LAPB] S0 -> S3");
				lapb->state     = LAPB_STATE_3;
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t1timer(lapb);
				lapb->condition = 0x00;
				lapb->vs        = 0;
				lapb->vr        = 0;
				lapb->va        = 0;
				lapb_connect_indication(lapb, LAPB_OK);
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S0 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			};
			break;

		case LAPB_DISC:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S0 RX DISC(%d)", frame->pf);
			lapb->callbacks->debug(lapb, 1, "[LAPB] S0 TX UA(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			break;

		default:
			break;
	};
}

/*
 *	State machine for state 1, Awaiting Connection State.
 *	The handling of the timer(s) is in file lapb_timer.c.
 */
static void lapb_state1_machine(struct lapb_cb *lapb, struct lapb_frame *frame) {
	switch (frame->type) {
		case LAPB_SABM:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S1 RX SABM(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX DM(%d)", frame->pf);
				/* We send SABME, receive SABM. Turn to State0 and send DM response */
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
				lapb_reset(lapb, LAPB_STATE_0);
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX UA(%d)", frame->pf);
				/* Sended and received commands are the same. Send UA */
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			};
			break;

		case LAPB_SABME:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S1 RX SABME(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX DM(%d)", frame->pf);
				/* We send SABM, receive SABME. Turn to State0 and send DM response */
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
				lapb_reset(lapb, LAPB_STATE_0);
			};
			break;

		case LAPB_DISC:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S1 RX DISC(%d)", frame->pf);
			lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX DM(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			break;

		case LAPB_UA:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S1 RX UA(%d)", frame->pf);
			if (frame->pf) {
				//if ((lapb->mode & LAPB_DCE) == LAPB_DCE)
					lapb_stop_t1timer(lapb);
				lapb->state     = LAPB_STATE_3;
				lapb->condition = 0x00;
				lapb->vs        = 0;
				lapb->vr        = 0;
				lapb->va        = 0;

				lapb->callbacks->debug(lapb, 0, "[LAPB] S1 -> S3");
				lapb_connect_indication(lapb, LAPB_OK);
			};
			break;

		case LAPB_DM:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S1 RX DM(%d)", frame->pf);
			if (frame->pf) {
				lapb->callbacks->debug(lapb, 0, "[LAPB] S1 -> S0");
				lapb->state = LAPB_STATE_0;
				lapb_stop_t1timer(lapb);
				lapb_disconnect_indication(lapb, LAPB_REFUSED);
			};
			break;
	};

}

/*
 *	State machine for state 2, Awaiting Release State.
 *	The handling of the timer(s) is in file lapb_timer.c
 */
static void lapb_state2_machine(struct lapb_cb *lapb, struct lapb_frame *frame) {
	switch (frame->type) {
		case LAPB_SABM:
		case LAPB_SABME:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 RX {SABM,SABME}(%d)", frame->pf);
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 TX DM(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			break;

		case LAPB_DISC:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 RX DISC(%d)", frame->pf);
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 TX UA(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			break;

		case LAPB_UA:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 RX UA(%d)", frame->pf);
			if (frame->pf) {
				lapb->state = LAPB_STATE_0;
				lapb_stop_t1timer(lapb);
				lapb->callbacks->debug(lapb, 0, "[LAPB] S2 -> S0");
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_OK);
			};
			break;

		case LAPB_DM:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 RX DM(%d)", frame->pf);
			if (frame->pf) {
				lapb->state = LAPB_STATE_0;
				lapb_start_t1timer(lapb);
				lapb->callbacks->debug(lapb, 0, "[LAPB] S2 -> S0");
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_NOTCONNECTED);
			};
			break;

		case LAPB_I:
		case LAPB_REJ:
		case LAPB_RNR:
		case LAPB_RR:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 RX {I,REJ,RNR,RR}(%d)", frame->pf);
			lapb->callbacks->debug(lapb, 1, "[LAPB] S2 RX DM(%d)", frame->pf);
			if (frame->pf)
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			break;
	};
}

/*
 *	State machine for state 3, Connected State.
 *	The handling of the timer(s) is in file lapb_timer.c
 */
static void lapb_state3_machine(struct lapb_cb *lapb, char * data, int data_size, struct lapb_frame *frame) {
	//int queued = 0;
	int modulus = (lapb->mode & LAPB_EXTENDED) ? LAPB_EMODULUS : LAPB_SMODULUS;

	switch (frame->type) {
		case LAPB_SABM:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX SABM(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S3 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S3 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t1timer(lapb);
				lapb->condition = 0x00;
				lapb->vs        = 0;
				lapb->vr        = 0;
				lapb->va        = 0;
				lapb_requeue_frames(lapb);
			};
			break;

		case LAPB_SABME:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX SABME(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S3 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t1timer(lapb);
				lapb->condition = 0x00;
				lapb->vs        = 0;
				lapb->vr        = 0;
				lapb->va        = 0;
				lapb_requeue_frames(lapb);
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S3 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			};
			break;

		case LAPB_DISC:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX DISC(%d)", frame->pf);
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 TX UA(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			lapb_requeue_frames(lapb);
			lapb_disconnect_indication(lapb, LAPB_OK);
			lapb_reset(lapb, LAPB_STATE_0);
			if (lapb->mode & LAPB_DCE)
				lapb_start_t1timer(lapb);
			break;

		case LAPB_DM:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX DM(%d)", frame->pf);
			lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S0");
			lapb_requeue_frames(lapb);
			lapb_disconnect_indication(lapb, LAPB_NOTCONNECTED);
			lapb_reset(lapb, LAPB_STATE_0);
			if (lapb->mode & LAPB_DCE)
				lapb_start_t1timer(lapb);
			break;

		case LAPB_RNR:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX RNR(%d) R%d", frame->pf, frame->nr);
			lapb->condition |= LAPB_PEER_RX_BUSY_CONDITION;
			lapb_check_need_response(lapb, frame->cr, frame->pf);
			if (lapb_validate_nr(lapb, frame->nr)) {
				lapb_check_iframes_acked(lapb, frame->nr);
			} else {
				lapb->frmr_data = *frame;
				lapb->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_start_t1timer(lapb);
				lapb->state   = LAPB_STATE_4;
				lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S4");
			};
			break;

		case LAPB_RR:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX RR(%d) R%d", frame->pf, frame->nr);
			lapb->condition &= ~LAPB_PEER_RX_BUSY_CONDITION;
			lapb_check_need_response(lapb, frame->cr, frame->pf);
			if (lapb_validate_nr(lapb, frame->nr)) {
				lapb_check_iframes_acked(lapb, frame->nr);
			} else {
				lapb->frmr_data = *frame;
				lapb->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_start_t1timer(lapb);
				lapb->state   = LAPB_STATE_4;
				lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S4");
			};
			break;

		case LAPB_REJ:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX REJ(%d) R%d", frame->pf, frame->nr);
			lapb->condition &= ~LAPB_PEER_RX_BUSY_CONDITION;
			lapb_check_need_response(lapb, frame->cr, frame->pf);
			if (lapb_validate_nr(lapb, frame->nr)) {
				lapb_frames_acked(lapb, frame->nr);
				lapb_stop_t1timer(lapb);
				lapb_requeue_frames(lapb);
			} else {
				lapb->frmr_data = *frame;
				lapb->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_start_t1timer(lapb);
				lapb->state   = LAPB_STATE_4;
				lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S4");
			};
			break;

		case LAPB_I:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX I(%d) S%d R%d", frame->pf, frame->ns, frame->nr);
			if (!lapb_validate_nr(lapb, frame->nr)) {
				lapb->frmr_data = *frame;
				lapb->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_start_t1timer(lapb);
				lapb->state   = LAPB_STATE_4;
				lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S4");
				break;
			};
			if (lapb->condition & LAPB_PEER_RX_BUSY_CONDITION)
				lapb_frames_acked(lapb, frame->nr);
			else
				lapb_check_iframes_acked(lapb, frame->nr);

			if (frame->ns == lapb->vr) {
				int cn;
				cn = lapb_data_indication(lapb, data + 2, data_size - 2);
				/*
				 * If upper layer has dropped the frame, we
				 * basically ignore any further protocol
				 * processing. This will cause the peer
				 * to re-transmit the frame later like
				 * a frame lost on the wire.
				 */
				if (cn != 0) {
					lapb->callbacks->debug(lapb, 1, "[LAPB] S3 Upper layer has dropped the frame");
					break;
				};
				lapb->vr = (lapb->vr + 1) % modulus;
				lapb->condition &= ~LAPB_REJECT_CONDITION;
				if (frame->pf)
					lapb_enquiry_response(lapb);
				else {
					//if (!(lapb->condition & LAPB_ACK_PENDING_CONDITION)) {
						lapb->condition |= LAPB_ACK_PENDING_CONDITION;
						lapb_start_t2timer(lapb);
					//}
					//else
					//	lapb->callbacks->debug(lapb, 1, "[LAPB] S3 lapb->condition=%d", lapb->condition);
				};
			} else {
				if (lapb->condition & LAPB_REJECT_CONDITION) {
					if (frame->pf)
						lapb_enquiry_response(lapb);
				} else {
					lapb->callbacks->debug(lapb, 1, "[LAPB] S3 TX REJ(%d) R%d", frame->pf, lapb->vr);
					lapb->condition |= LAPB_REJECT_CONDITION;
					lapb_send_control(lapb, LAPB_REJ, frame->pf, LAPB_RESPONSE);
					lapb->condition &= ~LAPB_ACK_PENDING_CONDITION;
				};
			};
			break;

		case LAPB_FRMR:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX FRMR(%d) %02X %02X %02X %02X %02X",
								   frame->pf, (_uchar)data[0], (_uchar)data[1], (_uchar)data[2], (_uchar)data[3], (_uchar)data[4]);
			lapb_establish_data_link(lapb);
			lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S1");
			lapb_requeue_frames(lapb);
			lapb->state = LAPB_STATE_1;
			break;

		case LAPB_ILLEGAL:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S3 RX ILLEGAL(%d)", frame->pf);
			lapb->frmr_data = *frame;
			lapb->frmr_type = LAPB_FRMR_W;
			lapb_transmit_frmr(lapb);
			lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S4");
			lapb_start_t1timer(lapb);
			lapb->state   = LAPB_STATE_4;
			break;
	};

}

/*
 *	State machine for state 4, Frame Reject State.
 *	The handling of the timer(s) is in file lapb_timer.c.
 */
static void lapb_state4_machine(struct lapb_cb *lapb, struct lapb_frame *frame) {
	switch (frame->type) {
		case LAPB_SABM:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S4 RX SABM(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S4 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			} else {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S4 TX UA(%d)", frame->pf);
				lapb->callbacks->debug(lapb, 0, "[LAPB] S4 -> S3");
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t1timer(lapb);
				lapb->state     = LAPB_STATE_3;
				lapb->condition = 0x00;
				lapb->vs        = 0;
				lapb->vr        = 0;
				lapb->va        = 0;
				lapb_connect_indication(lapb, LAPB_OK);
			};
			break;

		case LAPB_SABME:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S4 RX SABME(%d)", frame->pf);
			if (lapb->mode & LAPB_EXTENDED) {
				lapb->callbacks->debug(lapb, 1, "[LAPB] S4 TX UA(%d)", frame->pf);
				lapb->callbacks->debug(lapb, 0, "[LAPB] S4 -> S3");
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t1timer(lapb);
				lapb->state     = LAPB_STATE_3;
				lapb->condition = 0x00;
				lapb->vs        = 0;
				lapb->vr        = 0;
				lapb->va        = 0;
				lapb_connect_indication(lapb, LAPB_OK);
			} else {
				lapb->callbacks->debug(lapb, 1,  "[LAPB] S4 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			};
			break;
	};
}

/*
 *	Process an incoming LAPB frame
 */
void lapb_data_input(struct lapb_cb *lapb, char *data, int data_size) {
	struct lapb_frame frame;

	if (lapb->state == LAPB_NOT_READY)
		return;
	if (lapb_decode(lapb, data, data_size, &frame) < 0)
		return;

	switch (lapb->state) {
		case LAPB_STATE_0:
			lapb_state0_machine(lapb, &frame);
			break;
		case LAPB_STATE_1:
			lapb_state1_machine(lapb, &frame);
			break;
		case LAPB_STATE_2:
			lapb_state2_machine(lapb, &frame);
			break;
		case LAPB_STATE_3:
			lapb_state3_machine(lapb, data, data_size, &frame);
			break;
		case LAPB_STATE_4:
			lapb_state4_machine(lapb, &frame);
			break;
	};

	lapb_kick(lapb);
}
