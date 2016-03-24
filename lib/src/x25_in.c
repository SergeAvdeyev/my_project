/*
 *	X25 release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */


#include "x25_int.h"



int x25_queue_rx_frame(struct x25_cs * x25, char * data, int data_size, int more) {
	char * ptr;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (data_size == 0)
		return 0;
	ptr = x25_int->fragment_buffer;
	ptr += x25_int->fragment_len;
	/* Copy data to fragment_buffer */
	x25_mem_copy(ptr, data, data_size);
	x25_int->fragment_len += data_size;

	if (more)
		return 0;

	/* Inform Application about new received data */
	x25->callbacks->data_indication(x25, x25_int->fragment_buffer, x25_int->fragment_len);
	x25_int->fragment_len = 0;

	return 0;
}

/*
 * State machine for state 1, Awaiting Call Accepted State.
 * The handling of the timer(s) is in file x25_timer.c.
 */
int x25_state1_machine(struct x25_cs * x25, char * data, int data_size, struct x25_frame * frame) {
	struct x25_address source_addr, dest_addr;
	int len;
	char * ptr = data;
	int data_size_tmp = data_size;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	switch (frame->type) {
		case X25_CALL_ACCEPTED: {
			x25->callbacks->debug(2, "[X25] S1 RX CALL_ACCEPTED");
			x25_stop_timers(x25);
			x25_int->condition = 0x00;
			x25_int->vs        = 0;
			x25_int->va        = 0;
			x25_int->vr        = 0;
			x25_int->vl        = 0;
			x25_int->state     = X25_STATE_3;
			/*
			 *	Parse the data in the frame.
			 */
			if (data_size < X25_STD_MIN_LEN)
				goto out_clear;
			ptr += X25_STD_MIN_LEN;
			data_size_tmp -= X25_STD_MIN_LEN;

			len = x25_parse_address_block(ptr, data_size_tmp, &source_addr, &dest_addr);
			if (len > 0) {
				ptr += len;
				data_size_tmp -= len;
			} else if (len < 0)
				goto out_clear;

			len = x25_parse_facilities(x25, ptr, data_size_tmp,
									   &x25_int->facilities,
									   &x25_int->dte_facilities,
									   &x25_int->vc_facil_mask);
			if (len > 0) {
				ptr += len;
				data_size_tmp -= len;
			} else if (len < 0)
				goto out_clear;
			/*
			 *	Copy any Call User Data.
			 */
			if (data_size_tmp > 0) {
				if (data_size_tmp > X25_MAX_CUD_LEN)
					goto out_clear;

				x25_mem_copy(x25_int->calluserdata.cuddata, ptr, data_size_tmp);
				x25_int->calluserdata.cudlength = data_size_tmp;
			};
			x25->callbacks->debug(1, "[X25] S1 -> S3");
			if (x25->callbacks->call_accepted)
				x25->callbacks->call_accepted(x25);
			break;
		}
		case X25_CLEAR_REQUEST:
			x25->callbacks->debug(2, "[X25] S1 RX CLEAR_REQUEST");
			if (data_size_tmp < X25_STD_MIN_LEN + 2)
				goto out_clear;

			x25->callbacks->debug(2, "[X25] S1 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, X25_REFUSED, data[3], data[4]);
			if (x25->callbacks->clear_indication)
				x25->callbacks->clear_indication(x25);
			break;

		default:
			break;
	};

	return 0;

out_clear:
	x25->callbacks->debug(2, "[X25] S1 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_int->state = X25_STATE_2;
	x25_start_timer(x25, &x25_int->ClearTimer);
	x25->callbacks->debug(1, "[X25] S1 -> S2");
	return 0;
}

/*
 * State machine for state 2, Awaiting Clear Confirmation State.
 * The handling of the timer(s) is in file x25_timer.c
 */
int x25_state2_machine(struct x25_cs * x25, char * data, int data_size, struct x25_frame * frame) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	switch (frame->type) {
		case X25_CLEAR_REQUEST:
			x25->callbacks->debug(2, "[X25] S2 RX CLEAR_REQUEST");
			if (data_size < (X25_STD_MIN_LEN + 2))
				goto out_clear;

			x25->callbacks->debug(2, "[X25] S2 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, 0, data[3], data[4]);
			if (x25->callbacks->clear_indication)
				x25->callbacks->clear_indication(x25);
			break;

		case X25_CLEAR_CONFIRMATION:
			x25->callbacks->debug(2, "[X25] S2 RX CLEAR_CONFIRMATION");
			x25_disconnect(x25, 0, 0, 0);
			if (x25->callbacks->clear_accepted)
				x25->callbacks->clear_accepted(x25);
			break;

		default:
			break;
	}

	return 0;

out_clear:
	x25->callbacks->debug(2, "[X25] S2 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_start_timer(x25, &x25_int->ClearTimer);
	return 0;
}

/*
 * State machine for state 3, Connected State.
 * The handling of the timer(s) is in file x25_timer.c
 */
int x25_state3_machine(struct x25_cs * x25, char * data, int data_size, struct x25_frame * frame) {
	int queued = 0;
	int modulus;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	modulus = (x25_is_extended(x25)) ? X25_EMODULUS : X25_SMODULUS;

	switch (frame->type) {
		case X25_RESET_REQUEST:
			x25->callbacks->debug(2, "[X25] S3 RX RESET_REQUEST");
			x25->callbacks->debug(2, "[X25] S3 TX RESET_CONFIRMATION");
			x25_write_internal(x25, X25_RESET_CONFIRMATION);
			x25_stop_timers(x25);
			x25_int->condition = 0x00;
			x25_int->vs        = 0;
			x25_int->vr        = 0;
			x25_int->va        = 0;
			x25_int->vl        = 0;
			x25_requeue_frames(x25);
			break;

		case X25_CLEAR_REQUEST:
			x25->callbacks->debug(2, "[X25] S3 RX CLEAR_REQUEST");
			if (data_size < (X25_STD_MIN_LEN + 2))
				goto out_clear;

			x25->callbacks->debug(2, "[X25] S3 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, 0, data[3], data[4]);
			if (x25->callbacks->clear_indication)
				x25->callbacks->clear_indication(x25);
			break;

		case X25_RR:
		case X25_RNR:
			if (frame->type == X25_RR)
				x25->callbacks->debug(2, "[X25] S3 RX RR R%d", frame->nr);
			else
				x25->callbacks->debug(2, "[X25] S3 RX RNR R%d", frame->nr);
			if (!x25_validate_nr(x25, frame->nr)) {
				x25_clear_queues(x25);
				x25->callbacks->debug(2, "[X25] S3 TX RESET_REQUEST");
				x25_write_internal(x25, X25_RESET_REQUEST);
				x25_start_timer(x25, &x25_int->ResetTimer);
				x25_int->condition = 0x00;
				x25_int->vs        = 0;
				x25_int->vr        = 0;
				x25_int->va        = 0;
				x25_int->vl        = 0;
				x25_int->state     = X25_STATE_4;
				x25->callbacks->debug(1, "[X25] S3 -> S4");
			} else {
				x25_check_iframes_acked(x25, frame->nr);
				if (frame->type == X25_RNR)
					set_bit(X25_COND_PEER_RX_BUSY, &x25_int->condition);
				else
					clear_bit(X25_COND_PEER_RX_BUSY, &x25_int->condition);
			};
			break;

		case X25_DATA:	/* XXX */
			x25->callbacks->debug(2, "[X25] S3 RX DATA S%d R%d", frame->ns, frame->nr);
			if (!x25_validate_nr(x25, frame->nr)) {
				x25_clear_queues(x25);
				x25->callbacks->debug(2, "[X25] S3 TX RESET_REQUEST(Wrong NR)");
				x25_write_internal(x25, X25_RESET_REQUEST);
				x25_start_timer(x25, &x25_int->ResetTimer);
				x25_int->condition = 0x00;
				x25_int->vs        = 0;
				x25_int->vr        = 0;
				x25_int->va        = 0;
				x25_int->vl        = 0;
				x25_int->state     = X25_STATE_4;
				x25->callbacks->debug(1, "[X25] S3 -> S4");
				break;
			};
			if (test_bit(X25_COND_PEER_RX_BUSY, &x25_int->condition))
				x25_frames_acked(x25, frame->nr);
			else
				x25_check_iframes_acked(x25, frame->nr);
			if (frame->ns == x25_int->vr) {
				int offset = x25_is_extended(x25) ? 4 : 3;
				if (x25_queue_rx_frame(x25, data + offset, data_size - offset, frame->m_flag) == 0) {
					x25_int->vr = (x25_int->vr + 1) % modulus;
					queued = 1;
				} else {
					/* Should never happen */
					x25_clear_queues(x25);
					x25->callbacks->debug(2, "[X25] S3 TX RESET_REQUEST(Not queued)");
					x25_write_internal(x25, X25_RESET_REQUEST);
					x25_start_timer(x25, &x25_int->ResetTimer);
					x25_int->condition = 0x00;
					x25_int->vs        = 0;
					x25_int->vr        = 0;
					x25_int->va        = 0;
					x25_int->vl        = 0;
					x25_int->state     = X25_STATE_4;
					x25->callbacks->debug(1, "[X25] S3 -> S4");
					break;
				};
				/*
				 *	If the window is full Ack it immediately, else
				 *	start the holdback timer.
				 */
				if (((x25_int->vl + x25_int->facilities.winsize_in) % modulus) == x25_int->vr) {
					clear_bit(X25_COND_ACK_PENDING, &x25_int->condition);
					x25_stop_timer(x25, &x25_int->AckTimer);
					x25_enquiry_response(x25);
				} else {
					set_bit(X25_COND_ACK_PENDING, &x25_int->condition);
					x25_start_timer(x25, &x25_int->AckTimer);
				};
			} else {
				if (!test_bit(X25_COND_RESET, &x25_int->condition))
					x25_enquiry_response(x25);
			};
			break;

		case X25_INTERRUPT_CONFIRMATION:
			x25->callbacks->debug(2, "[X25] S3 RX INTERRUPT_CONFIRMATION");
			clear_bit(X25_INTERRUPT_FLAG, &x25_int->flags);
			break;

		case X25_INTERRUPT:
			x25->callbacks->debug(2, "[X25] S3 RX INTERRUPT");
			cb_queue_tail(&x25_int->interrupt_in_queue, data, data_size, 0);
			queued = 1;
			x25->callbacks->debug(2, "[X25] S3 TX INTERRUPT_CONFIRMATION");
			x25_write_internal(x25, X25_INTERRUPT_CONFIRMATION);
			break;

		default:
			x25->callbacks->debug(1, "[X25] unknown %02X in state 3", frame->type);
			break;
	}

	return queued;

out_clear:
	x25->callbacks->debug(2, "[X25] S3 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_int->state = X25_STATE_2;
	x25_start_timer(x25, &x25_int->ClearTimer);
	x25->callbacks->debug(1, "[X25] S3 -> S2");
	return 0;
}

/*
 * State machine for state 4, Awaiting Reset Confirmation State.
 * The handling of the timer(s) is in file x25_timer.c
 */
static int x25_state4_machine(struct x25_cs * x25, char * data, int data_size, struct x25_frame * frame) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	switch (frame->type) {
		case X25_RESET_REQUEST:
			x25->callbacks->debug(2, "[X25] S4 RX RESET_REQUEST");
			x25->callbacks->debug(2, "[X25] S4 TX RESET_CONFIRMATION");
			x25_write_internal(x25, X25_RESET_CONFIRMATION);
		case X25_RESET_CONFIRMATION:
			x25->callbacks->debug(2, "[X25] S4 RX RESET_CONFIRMATION");
			x25_stop_timers(x25);
			x25_int->condition = 0x00;
			x25_int->va        = 0;
			x25_int->vr        = 0;
			x25_int->vs        = 0;
			x25_int->vl        = 0;
			x25_int->state     = X25_STATE_3;
			x25->callbacks->debug(1, "[X25] S4 -> S3");
			x25_requeue_frames(x25);
			break;
		case X25_CLEAR_REQUEST:
			x25->callbacks->debug(2, "[X25] S4 RX CLEAR_REQUEST");
			if (data_size < (X25_STD_MIN_LEN + 2))
				goto out_clear;

			x25->callbacks->debug(2, "[X25] S4 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, 0, data[3], data[4]);
			if (x25->callbacks->clear_indication)
				x25->callbacks->clear_indication(x25);
			break;

		default:
			break;
	};

	return 0;

out_clear:
	x25->callbacks->debug(2, "[X25] S4 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_int->state = X25_STATE_2;
	x25->callbacks->debug(1, "[X25] S4 -> S2");
	x25_start_timer(x25, &x25_int->ClearTimer);
	return 0;
}




/* Higher level upcall for a LAPB frame */
int x25_process_rx_frame(struct x25_cs *x25, char * data, int data_size) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (x25_int->state == X25_STATE_0)
		return 0;

	int queued = 0;
	struct x25_frame frame;

	if (x25_decode(x25, data, data_size, &frame) == X25_ILLEGAL) return 0;

	switch (x25_int->state) {
		case X25_STATE_1:
			queued = x25_state1_machine(x25, data, data_size, &frame);
			break;
		case X25_STATE_2:
			queued = x25_state2_machine(x25, data, data_size, &frame);
			break;
		case X25_STATE_3:
			queued = x25_state3_machine(x25, data, data_size, &frame);
			break;
		case X25_STATE_4:
			queued = x25_state4_machine(x25, data, data_size, &frame);
			break;
	};

	x25_kick(x25);

	return queued;
}
