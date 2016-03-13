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

	cb_queue_tail(&x25_int->receive_queue, x25_int->fragment_buffer, x25_int->fragment_len);
	x25_int->fragment_len = 0;
	/* Inform Application about new received data */
	//if (!sock_flag(sk, SOCK_DEAD))
	//	sk->sk_data_ready(sk);

	return 0;
}

/*
 * State machine for state 1, Awaiting Call Accepted State.
 * The handling of the timer(s) is in file x25_timer.c.
 * Handling of state 0 and connection release is in af_x25.c.
 */
int x25_state1_machine(struct x25_cs * x25, char * data, int data_size, int frametype) {
	struct x25_address source_addr, dest_addr;
	int len;
	char * ptr = data;
	int data_size_tmp = data_size;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	switch (frametype) {
		case X25_CALL_ACCEPTED: {
			x25->callbacks->debug(1, "[X25] S1 RX CALL_ACCEPTED");
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
			if(data_size < X25_STD_MIN_LEN)
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

				//skb_copy_bits(skb, 0, x25->calluserdata.cuddata, skb->len);
				x25_mem_copy(x25_int->calluserdata.cuddata, ptr, data_size_tmp);
				x25_int->calluserdata.cudlength = data_size_tmp;
			};
			x25->callbacks->debug(1, "[X25] S1 -> S3");
			x25->callbacks->call_accepted(x25);
			break;
		}
		case X25_CLEAR_REQUEST:
			x25->callbacks->debug(1, "[X25] S1 RX CLEAR_REQUEST");
			if (data_size_tmp < X25_STD_MIN_LEN + 2)
				goto out_clear;

			x25->callbacks->debug(1, "[X25] S1 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, X25_REFUSED, data[3], data[4]);
			break;

		default:
			break;
	};

	return 0;

out_clear:
	x25->callbacks->debug(1, "[X25] S1 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_int->state = X25_STATE_2;
	x25_start_timer(x25, x25_int->T23.timer_ptr);
	x25->callbacks->debug(1, "[X25] S1 -> S2");
	return 0;
}

/*
 * State machine for state 2, Awaiting Clear Confirmation State.
 * The handling of the timer(s) is in file x25_timer.c
 * Handling of state 0 and connection release is in af_x25.c.
 */
int x25_state2_machine(struct x25_cs * x25, char * data, int data_size, int frametype) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);
	switch (frametype) {
		case X25_CLEAR_REQUEST:
			x25->callbacks->debug(1, "[X25] S2 RX CLEAR_REQUEST");
			if (data_size < (X25_STD_MIN_LEN + 2))
				goto out_clear;

			x25->callbacks->debug(1, "[X25] S2 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, 0, data[3], data[4]);
			break;

		case X25_CLEAR_CONFIRMATION:
			x25->callbacks->debug(1, "[X25] S2 RX CLEAR_CONFIRMATION");
			x25_disconnect(x25, 0, 0, 0);
			break;

		default:
			break;
	}

	return 0;

out_clear:
	x25->callbacks->debug(1, "[X25] S2 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_start_timer(x25, &x25_int->T23);
	return 0;
}

/*
 * State machine for state 3, Connected State.
 * The handling of the timer(s) is in file x25_timer.c
 * Handling of state 0 and connection release is in af_x25.c.
 */
int x25_state3_machine(struct x25_cs * x25, char * data, int data_size, int frametype, int ns, int nr, int q, int d, int m) {
	(void)q;
	(void)d;
	int queued = 0;
	int modulus;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	modulus = (x25->link.extended) ? X25_EMODULUS : X25_SMODULUS;

	switch (frametype) {
		case X25_RESET_REQUEST:
			x25->callbacks->debug(1, "[X25] S3 RX RESET_REQUEST");
			x25->callbacks->debug(1, "[X25] S3 TX RESET_CONFIRMATION");
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
			x25->callbacks->debug(1, "[X25] S3 RX CLEAR_REQUEST");
			if (data_size < (X25_STD_MIN_LEN + 2))
				goto out_clear;

			x25->callbacks->debug(1, "[X25] S3 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, 0, data[3], data[4]);
			break;

		case X25_RR:
		case X25_RNR:
			if (frametype == X25_RR)
				x25->callbacks->debug(1, "[X25] S3 RX RR");
			else
				x25->callbacks->debug(1, "[X25] S3 RX RNR");
			if (!x25_validate_nr(x25, nr)) {
				x25_clear_queues(x25);
				x25->callbacks->debug(1, "[X25] S3 TX RESET_REQUEST");
				x25_write_internal(x25, X25_RESET_REQUEST);
				x25_start_timer(x25, &x25_int->T22);
				x25_int->N22_RC = 0;
				x25_int->condition = 0x00;
				x25_int->vs        = 0;
				x25_int->vr        = 0;
				x25_int->va        = 0;
				x25_int->vl        = 0;
				x25_int->state     = X25_STATE_4;
				x25->callbacks->debug(1, "[X25] S3 -> S4");
			} else {
				x25_frames_acked(x25, nr);
				if (frametype == X25_RNR)
					x25_int->condition |= X25_COND_PEER_RX_BUSY;
				else
					x25_int->condition &= ~X25_COND_PEER_RX_BUSY;
			};
			break;

		case X25_DATA:	/* XXX */
			x25->callbacks->debug(1, "[X25] S3 RX DATA");
			x25_int->condition &= ~X25_COND_PEER_RX_BUSY;
			if ((ns != x25_int->vr) || !x25_validate_nr(x25, nr)) {
				x25_clear_queues(x25);
				x25->callbacks->debug(1, "[X25] S3 TX RESET_REQUEST");
				x25_write_internal(x25, X25_RESET_REQUEST);
				x25_int->N22_RC = 0;
				x25_start_timer(x25, &x25_int->T22);
				x25_int->condition = 0x00;
				x25_int->vs        = 0;
				x25_int->vr        = 0;
				x25_int->va        = 0;
				x25_int->vl        = 0;
				x25_int->state     = X25_STATE_4;
				x25->callbacks->debug(1, "[X25] S3 -> S4");
				break;
			}
			x25_frames_acked(x25, nr);
			if (ns == x25_int->vr) {
				if (x25_queue_rx_frame(x25, data, data_size, m) == 0) {
					x25_int->vr = (x25_int->vr + 1) % modulus;
					queued = 1;
				} else {
					/* Should never happen */
					x25_clear_queues(x25);
					x25->callbacks->debug(1, "[X25] S3 TX RESET_REQUEST");
					x25_write_internal(x25, X25_RESET_REQUEST);
					x25_int->N22_RC = 0;
					x25_start_timer(x25, &x25_int->T22);
					x25_int->condition = 0x00;
					x25_int->vs        = 0;
					x25_int->vr        = 0;
					x25_int->va        = 0;
					x25_int->vl        = 0;
					x25_int->state     = X25_STATE_4;
					x25->callbacks->debug(1, "[X25] S3 -> S4");
					break;
				};
//				if (atomic_read(&sk->sk_rmem_alloc) > (sk->sk_rcvbuf >> 1))
//					x25->condition |= X25_COND_OWN_RX_BUSY;
			};
			/*
			 *	If the window is full Ack it immediately, else
			 *	start the holdback timer.
			 */
			if (((x25_int->vl + x25_int->facilities.winsize_in) % modulus) == x25_int->vr) {
				clear_bit(X25_COND_ACK_PENDING, (_ulong *)&x25_int->condition);
				x25_stop_timers(x25);
				x25_enquiry_response(x25);
			} else {
				set_bit(X25_COND_ACK_PENDING, (_ulong *)&x25_int->condition);
				//x25_start_timer(x25, &x25->T2);
			};
			break;

		case X25_INTERRUPT_CONFIRMATION:
			x25->callbacks->debug(1, "[X25] S3 RX INTERRUPT_CONFIRMATION");
			clear_bit(X25_INTERRUPT_FLAG, &x25_int->flags);
			break;

		case X25_INTERRUPT:
			x25->callbacks->debug(1, "[X25] S3 RX INTERRUPT");
			cb_queue_tail(&x25_int->interrupt_in_queue, data, data_size);
			queued = 1;
			x25->callbacks->debug(1, "[X25] S3 TX INTERRUPT_CONFIRMATION");
			x25_write_internal(x25, X25_INTERRUPT_CONFIRMATION);
			break;

		default:
			x25->callbacks->debug(2, "[X25] unknown %02X in state 3", frametype);
			break;
	}

	return queued;

out_clear:
	x25->callbacks->debug(1, "[X25] S3 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_int->state = X25_STATE_2;
	x25_start_timer(x25, &x25_int->T23);
	x25->callbacks->debug(1, "[X25] S3 -> S2");
	return 0;
}

/*
 * State machine for state 4, Awaiting Reset Confirmation State.
 * The handling of the timer(s) is in file x25_timer.c
 * Handling of state 0 and connection release is in af_x25.c.
 */
static int x25_state4_machine(struct x25_cs * x25, char * data, int data_size, int frametype) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	switch (frametype) {
		case X25_RESET_REQUEST:
			x25->callbacks->debug(1, "[X25] S4 RX RESET_REQUEST");
			x25->callbacks->debug(1, "[X25] S4 TX RESET_CONFIRMATION");
			x25_write_internal(x25, X25_RESET_CONFIRMATION);
		case X25_RESET_CONFIRMATION:
			x25->callbacks->debug(1, "[X25] S4 RX RESET_CONFIRMATION");
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
			x25->callbacks->debug(1, "[X25] S4 RX CLEAR_REQUEST");
			if (data_size < (X25_STD_MIN_LEN + 2))
				goto out_clear;

			x25->callbacks->debug(1, "[X25] S4 TX CLEAR_CONFIRMATION");
			x25_write_internal(x25, X25_CLEAR_CONFIRMATION);
			x25_disconnect(x25, 0, data[3], data[4]);
			break;

		default:
			break;
	};

	return 0;

out_clear:
	x25->callbacks->debug(1, "[X25] S4 TX CLEAR_REQUEST");
	x25_write_internal(x25, X25_CLEAR_REQUEST);
	x25_int->state = X25_STATE_2;
	x25->callbacks->debug(1, "[X25] S4 -> S2");
	x25_start_timer(x25, &x25_int->T23);
	return 0;
}




/* Higher level upcall for a LAPB frame */
int x25_process_rx_frame(struct x25_cs *x25, char * data, int data_size) {
	int queued = 0, frametype, ns, nr, q, d, m;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (x25_int->state == X25_STATE_0)
		return 0;

	frametype = x25_decode(x25, data, data_size, &ns, &nr, &q, &d, &m);

	switch (x25_int->state) {
		case X25_STATE_1:
			queued = x25_state1_machine(x25, data, data_size, frametype);
			break;
		case X25_STATE_2:
			queued = x25_state2_machine(x25, data, data_size, frametype);
			break;
		case X25_STATE_3:
			queued = x25_state3_machine(x25, data, data_size, frametype, ns, nr, q, d, m);
			break;
		case X25_STATE_4:
			queued = x25_state4_machine(x25, data, data_size, frametype);
			break;
	};

	x25_kick(x25);

	return queued;
}
