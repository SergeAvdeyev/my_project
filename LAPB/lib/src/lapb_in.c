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

/*
 *	State machine for state 0, Disconnected State.
 *	The handling of the timer(s) is in file lapb_timer.c.
 */
void lapb_state0_machine(struct lapb_cs * lapb, struct lapb_frame * frame) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	switch (frame->type) {
		case LAPB_SABM:
			lapb->callbacks->debug(1, "[LAPB] S0 RX SABM(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				lapb->callbacks->debug(1, "[LAPB] S0 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			} else {
				lapb_stop_t201timer(lapb);
				lapb->callbacks->debug(1, "[LAPB] S0 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_int->state     = LAPB_STATE_3;
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb->callbacks->debug(0, "[LAPB] S0 -> S3");
				lapb_connect_indication(lapb, LAPB_OK);
			};
			break;

		case LAPB_SABME:
			lapb->callbacks->debug(1, "[LAPB] S0 RX SABME(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				lapb_stop_t201timer(lapb);
				lapb->callbacks->debug(1, "[LAPB] S0 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_int->state     = LAPB_STATE_3;
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb->callbacks->debug(0, "[LAPB] S0 -> S3");
				lapb_connect_indication(lapb, LAPB_OK);
			} else {
				lapb->callbacks->debug(1, "[LAPB] S0 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			};
			break;

		case LAPB_DM:
			if (!lapb->auto_connecting) break;
			lapb->callbacks->debug(1, "[LAPB] S0 RX DM(%d)", frame->pf);
			lapb_int->condition = 0x00;
			if (lapb_is_extended(lapb)) {
				lapb->callbacks->debug(1, "[LAPB] S0 TX SABME(1)");
				lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
			} else {
				lapb->callbacks->debug(1, "[LAPB] S0 TX SABM(1)");
				lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
			};
			lapb_int->state = LAPB_STATE_1;
			lapb->callbacks->debug(0, "[LAPB] S0 -> S1");
			break;

//		case LAPB_DISC:
//			lapb_debug(lapb, 1, "[LAPB] S0 RX DISC(%d)", frame->pf);
//			lapb_debug(lapb, 1, "[LAPB] S0 TX UA(%d)", frame->pf);
//			lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
//			break;

		default:
			break;
	};
}

/*
 *	State machine for state 1, Awaiting Connection State.
 *	The handling of the timer(s) is in file lapb_timer.c.
 */
void lapb_state1_machine(struct lapb_cs * lapb, struct lapb_frame * frame) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	switch (frame->type) {
		case LAPB_SABM:
			lapb->callbacks->debug(1, "[LAPB] S1 RX SABM(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				/* Unrecognized mode */
				lapb->callbacks->debug(1, "[LAPB] S1 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			} else {
				/* Collision state, send UA and switch to state_3 */
				lapb->callbacks->debug(1, "[LAPB] S1 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t201timer(lapb);
				lapb_int->state     = LAPB_STATE_3;
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb->callbacks->debug(0, "[LAPB] S1 -> S3");
				lapb_connect_confirmation(lapb, LAPB_OK);
			};
			break;

		case LAPB_SABME:
			lapb->callbacks->debug(1, "[LAPB] S1 RX SABME(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				/* Collision state, send UA and switch to state_3 */
				lapb->callbacks->debug(1, "[LAPB] S1 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t201timer(lapb);
				lapb_int->state     = LAPB_STATE_3;
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb->callbacks->debug(0, "[LAPB] S1 -> S3");
				lapb_connect_confirmation(lapb, LAPB_OK);
			} else {
				/* Unrecognized mode */
				lapb->callbacks->debug(1, "[LAPB] S1 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
			};
			break;

//		case LAPB_DISC:
//			lapb->callbacks->debug(1, "[LAPB] S1 RX DISC(%d)", frame->pf);
//			lapb->callbacks->debug(1, "[LAPB] S1 TX DM(%d)", frame->pf);
//			lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
//			lapb_requeue_frames(lapb);
//			lapb_disconnect_indication(lapb, LAPB_REFUSED);
//			lapb_reset(lapb, LAPB_STATE_0);
//			break;

		case LAPB_UA:
			lapb->callbacks->debug(1, "[LAPB] S1 RX UA(%d)", frame->pf);
			if (frame->pf) {
				lapb_stop_t201timer(lapb);
				lapb_int->state     = LAPB_STATE_3;
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;

				lapb->callbacks->debug(0, "[LAPB] S1 -> S3");
				lapb_connect_confirmation(lapb, LAPB_OK);
			};
			break;

		case LAPB_DM:
			lapb->callbacks->debug(1, "[LAPB] S1 RX DM(%d)", frame->pf);
			if (frame->pf) {
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_REFUSED);
				lapb_reset(lapb, LAPB_STATE_0);
			};
			break;
	};
}

/*
 *	State machine for state 2, Awaiting Release State.
 *	The handling of the timer(s) is in file lapb_timer.c
 */
void lapb_state2_machine(struct lapb_cs * lapb, struct lapb_frame * frame) {
	//struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	switch (frame->type) {
//		case LAPB_SABM:
//		case LAPB_SABME:
//			lapb->callbacks->debug(1, "[LAPB] S2 RX {SABM,SABME}(%d)", frame->pf);
//			lapb->callbacks->debug(1, "[LAPB] S2 TX DM(%d)", frame->pf);
//			lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
//			lapb_reset(lapb, LAPB_STATE_0);
//			break;

		case LAPB_DISC:
			lapb->callbacks->debug(1, "[LAPB] S2 RX DISC(%d)", frame->pf);
			lapb->callbacks->debug(1, "[LAPB] S2 TX UA(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			break;

		case LAPB_UA:
			lapb->callbacks->debug(1, "[LAPB] S2 RX UA(%d)", frame->pf);
			lapb_requeue_frames(lapb);
			lapb_disconnect_confirmation(lapb, LAPB_OK);
			lapb_reset(lapb, LAPB_STATE_0);
			break;

		case LAPB_DM:
			lapb->callbacks->debug(1, "[LAPB] S2 RX DM(%d)", frame->pf);
			lapb_requeue_frames(lapb);
			lapb_disconnect_confirmation(lapb, LAPB_NOTCONNECTED);
			lapb_reset(lapb, LAPB_STATE_0);
			break;

//		case LAPB_I:
//		case LAPB_REJ:
//		case LAPB_RNR:
//		case LAPB_RR:
//			lapb->callbacks->debug(1, "[LAPB] S2 RX {I,REJ,RNR,RR}(%d)", frame->pf);
//			lapb->callbacks->debug(1, "[LAPB] S2 TX DM(%d)", frame->pf);
//			if (frame->pf)
//				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
//			break;
	};
}

/*
 *	State machine for state 3, Connected State.
 *	The handling of the timer(s) is in file lapb_timer.c
 */
void lapb_state3_machine(struct lapb_cs * lapb, char * data, int data_size, struct lapb_frame * frame) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	//int queued = 0;
	int modulus = lapb_is_extended(lapb) ? LAPB_EMODULUS : LAPB_SMODULUS;

	switch (frame->type) {
		case LAPB_SABM:
			lapb->callbacks->debug(1, "[LAPB] S3 RX SABM(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				lapb->callbacks->debug(1, "[LAPB] S3 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_OK);
				lapb_reset(lapb, LAPB_STATE_0);
			} else {
				lapb->callbacks->debug(1, "[LAPB] S3 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t201timer(lapb);
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb_requeue_frames(lapb);
				lapb_start_t201timer(lapb); /* to kick data */
			};
			break;

		case LAPB_SABME:
			lapb->callbacks->debug(1, "[LAPB] S3 RX SABME(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				lapb->callbacks->debug(1, "[LAPB] S3 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_stop_t201timer(lapb);
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb_requeue_frames(lapb);
				lapb_start_t201timer(lapb); /* to kick data */
			} else {
				lapb->callbacks->debug(1, "[LAPB] S3 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_OK);
				lapb_reset(lapb, LAPB_STATE_0);
			};
			break;

		case LAPB_DISC:
			lapb->callbacks->debug(1, "[LAPB] S3 RX DISC(%d)", frame->pf);
			lapb->callbacks->debug(1, "[LAPB] S3 TX UA(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			lapb_requeue_frames(lapb);
			lapb_disconnect_indication(lapb, LAPB_OK);
			lapb_reset(lapb, LAPB_STATE_0);
			break;

		case LAPB_DM:
			lapb->callbacks->debug(1, "[LAPB] S3 RX DM(%d)", frame->pf);
			lapb_requeue_frames(lapb);
			lapb_disconnect_indication(lapb, LAPB_NOTCONNECTED);
			lapb_reset(lapb, LAPB_STATE_0);
			break;

		case LAPB_RNR:
			lapb->callbacks->debug(1, "[LAPB] S3 RX RNR(%d) R%d", frame->pf, frame->nr);
			lapb_int->condition |= LAPB_PEER_RX_BUSY_CONDITION;
			lapb_check_need_response(lapb, frame->cr, frame->pf);
			if (lapb_validate_nr(lapb, frame->nr)) {
				lapb_check_iframes_acked(lapb, frame->nr);
			} else {
				lapb_int->frmr_data = *frame;
				lapb_int->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_int->condition = LAPB_FRMR_CONDITION;
				lapb_int->state     = LAPB_STATE_4;
				lapb_int->RC = 0;
				lapb_start_t201timer(lapb);
				lapb_stop_t202timer(lapb);
			};
			break;

		case LAPB_RR:
			lapb->callbacks->debug(1, "[LAPB] S3 RX RR(%d) R%d", frame->pf, frame->nr);
			lapb_int->condition &= ~LAPB_PEER_RX_BUSY_CONDITION;
			lapb_check_need_response(lapb, frame->cr, frame->pf);
			if (lapb_validate_nr(lapb, frame->nr)) {
				lapb_check_iframes_acked(lapb, frame->nr);
			} else {
				lapb_int->frmr_data = *frame;
				lapb_int->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_int->condition = LAPB_FRMR_CONDITION;
				lapb_int->state     = LAPB_STATE_4;
				lapb_int->RC = 0;
				lapb_start_t201timer(lapb);
				lapb_stop_t202timer(lapb);
			};
			break;

		case LAPB_REJ:
			lapb->callbacks->debug(1, "[LAPB] S3 RX REJ(%d) R%d", frame->pf, frame->nr);
			lapb_int->condition &= ~LAPB_PEER_RX_BUSY_CONDITION;
			lapb_check_need_response(lapb, frame->cr, frame->pf);
			if (lapb_validate_nr(lapb, frame->nr)) {
				lapb_frames_acked(lapb, frame->nr);
				lapb_stop_t201timer(lapb);
				lapb_requeue_frames(lapb);
			} else {
				lapb_int->frmr_data = *frame;
				lapb_int->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_int->condition = LAPB_FRMR_CONDITION;
				lapb_int->state     = LAPB_STATE_4;
				lapb_int->RC = 0;
				lapb_start_t201timer(lapb);
				lapb_stop_t202timer(lapb);
			};
			break;

		case LAPB_I:
			if (lapb_int->condition & LAPB_FRMR_CONDITION)
				break;
			lapb->callbacks->debug(1, "[LAPB] S3 RX I(%d) S%d R%d", frame->pf, frame->ns, frame->nr);
			if (!lapb_validate_nr(lapb, frame->nr)) {
				lapb_int->frmr_data = *frame;
				lapb_int->frmr_type = LAPB_FRMR_Z;
				lapb_transmit_frmr(lapb);
				lapb_int->condition = LAPB_FRMR_CONDITION;
				lapb_int->state     = LAPB_STATE_4;
				lapb_int->RC = 0;
				lapb_start_t201timer(lapb);
				break;
			};
			if (lapb_int->condition & LAPB_PEER_RX_BUSY_CONDITION)
				lapb_frames_acked(lapb, frame->nr);
			else
				lapb_check_iframes_acked(lapb, frame->nr);

			if (frame->ns == lapb_int->vr) {
				int cn;
				if (lapb_is_extended(lapb))
					cn = lapb_data_indication(lapb, data + 3, data_size - 3);
				else
					cn = lapb_data_indication(lapb, data + 2, data_size - 2);
				/*
				 * If upper layer has dropped the frame, we
				 * basically ignore any further protocol
				 * processing. This will cause the peer
				 * to re-transmit the frame later like
				 * a frame lost on the wire.
				 */
				if (cn != 0) {
					lapb->callbacks->debug(1, "[LAPB] S3 Upper layer has dropped the frame");
					break;
				};
				lapb_int->vr = (lapb_int->vr + 1) % modulus;
				lapb_int->condition &= ~LAPB_REJECT_CONDITION;
				if (frame->pf)
					lapb_enquiry_response(lapb);
				else {
					if (!(lapb_int->condition & LAPB_ACK_PENDING_CONDITION)) {
						lapb_int->condition |= LAPB_ACK_PENDING_CONDITION;
						lapb_start_t202timer(lapb);
					};
					//else
					//	lapb->callbacks->debug(lapb, 1, "[LAPB] S3 lapb->condition=%d", lapb->condition);
				};
			} else {
				if (lapb_int->condition & LAPB_REJECT_CONDITION) {
					if (frame->pf)
						lapb_enquiry_response(lapb);
				} else {
					lapb->callbacks->debug(1, "[LAPB] S3 TX REJ(%d) R%d", frame->pf, lapb_int->vr);
					lapb_int->condition |= LAPB_REJECT_CONDITION;
					lapb_send_control(lapb, LAPB_REJ, frame->pf, LAPB_RESPONSE);
					lapb_int->condition &= ~LAPB_ACK_PENDING_CONDITION;
				};
			};
			break;

		case LAPB_FRMR:
			lapb->callbacks->debug(1, "[LAPB] S3 RX FRMR(%d) %02X %02X %02X %02X %02X",
									frame->pf, (_uchar)data[0], (_uchar)data[1], (_uchar)data[2], (_uchar)data[3], (_uchar)data[4]);
			if (lapb_int->condition & LAPB_FRMR_CONDITION) {
				/* FRMR Collision */
				lapb->callbacks->debug(1, "[LAPB] S3 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				break;
			};
			lapb_int->condition = LAPB_FRMR_CONDITION;
			lapb_stop_t201timer(lapb);
			_ushort nr_tmp;
			if (lapb_is_extended(lapb))
				nr_tmp = frame->control[1] >> 5;
			else
				nr_tmp = (_uchar)(data[3] >> 5) & 0x07;
			lapb_frames_acked(lapb, nr_tmp);
			lapb_requeue_frames(lapb);

			if (lapb_is_extended(lapb)) {
				lapb->callbacks->debug(1, "[LAPB] S3 TX SABME(1)");
				lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
			} else {
				lapb->callbacks->debug(1, "[LAPB] S3 TX SABM(1)");
				lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
			};
			break;

		case LAPB_UA:
			lapb->callbacks->debug(1, "[LAPB] S3 RX UA(%d)", frame->pf);
			if (lapb_int->condition & LAPB_FRMR_CONDITION) {
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb_start_t201timer(lapb); /* to kick data */
			} else {
				/* Reset data link */
				lapb_int->condition = LAPB_FRMR_CONDITION;
				lapb_requeue_frames(lapb);
				if (lapb_is_extended(lapb)) {
					lapb->callbacks->debug(1, "[LAPB] S3 TX SABME(1)");
					lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
				} else {
					lapb->callbacks->debug(1, "[LAPB] S3 TX SABM(1)");
					lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
				};
			};
			break;

		case LAPB_ILLEGAL:
			lapb->callbacks->debug(1, "[LAPB] S3 RX ILLEGAL(%d)", frame->pf);
			lapb_int->frmr_data = *frame;
			lapb_int->frmr_type = LAPB_FRMR_W;
			lapb_transmit_frmr(lapb);
			lapb_int->condition = LAPB_FRMR_CONDITION;
			lapb_int->state     = LAPB_STATE_4;
			lapb_start_t201timer(lapb);
			lapb_stop_t202timer(lapb);
			break;
	};

}

/*
 *	State machine for state 4, Frame Reject State.
 *	The handling of the timer(s) is in file lapb_timer.c.
 */
void lapb_state4_machine(struct lapb_cs * lapb, struct lapb_frame * frame) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	switch (frame->type) {
		/* Command */
		case LAPB_SABM:
			lapb->callbacks->debug(1, "[LAPB] S4 RX SABM(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				lapb->callbacks->debug(1, "[LAPB] S4 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_REFUSED);
				lapb_reset(lapb, LAPB_STATE_0);
			} else {
				lapb->callbacks->debug(1, "[LAPB] S4 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_restart_t201timer(lapb);
				lapb_int->state     = LAPB_STATE_3;
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb->callbacks->debug(0, "[LAPB] S4 -> S3");
				lapb_connect_indication(lapb, LAPB_OK);
			};
			break;

		/* Command */
		case LAPB_SABME:
			lapb->callbacks->debug(1, "[LAPB] S4 RX SABME(%d)", frame->pf);
			if (lapb_is_extended(lapb)) {
				lapb->callbacks->debug(1, "[LAPB] S4 TX UA(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
				lapb_restart_t201timer(lapb);
				lapb_int->state     = LAPB_STATE_3;
				lapb_int->condition = 0x00;
				lapb_int->vs        = 0;
				lapb_int->vr        = 0;
				lapb_int->va        = 0;
				lapb->callbacks->debug(0, "[LAPB] S4 -> S3");
				lapb_connect_indication(lapb, LAPB_OK);
			} else {
				lapb->callbacks->debug(1,  "[LAPB] S4 TX DM(%d)", frame->pf);
				lapb_send_control(lapb, LAPB_DM, frame->pf, LAPB_RESPONSE);
				lapb_requeue_frames(lapb);
				lapb_disconnect_indication(lapb, LAPB_REFUSED);
				lapb_reset(lapb, LAPB_STATE_0);
			};
			break;

		/* Command */
		case LAPB_DISC:
			lapb->callbacks->debug(1, "[LAPB] S4 RX DISC(%d)", frame->pf);
			lapb->callbacks->debug(1, "[LAPB] S4 TX UA(%d)", frame->pf);
			lapb_send_control(lapb, LAPB_UA, frame->pf, LAPB_RESPONSE);
			lapb_requeue_frames(lapb);
			lapb_disconnect_indication(lapb, LAPB_OK);
			lapb_reset(lapb, LAPB_STATE_0);
			break;

		/* Response */
		case LAPB_DM:
			lapb->callbacks->debug(1, "[LAPB] S4 RX DM(%d)", frame->pf);
			lapb_requeue_frames(lapb);
			lapb_disconnect_indication(lapb, LAPB_NOTCONNECTED);
			lapb_reset(lapb, LAPB_STATE_0);
			break;

		/* Response */
		case LAPB_FRMR:
			lapb->callbacks->debug(1, "[LAPB] S4 RX FRMR(%d)", frame->pf);
			break;
	};
}

/*
 *	Process an incoming LAPB frame
 */
void lapb_data_input(struct lapb_cs *lapb, char *data, int data_size) {
	struct lapb_frame frame;
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (lapb_decode(lapb, data, data_size, &frame) < 0)
		return;

	switch (lapb_int->state) {
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
//		case LAPB_STATE_4:
//			lapb_state4_machine(lapb, &frame);
//			break;
	};

	if (!(lapb_int->condition & LAPB_FRMR_CONDITION))
		lapb_kick(lapb);
}
