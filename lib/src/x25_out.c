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


int x25_pacsize_to_bytes(_uint pacsize) {
	int bytes = 1;

	if (!pacsize)
		return 128;

	while (pacsize-- > 0)
		bytes *= 2;

	return bytes;
}


/*
 *	This is where all X.25 information frames pass.
 *
 *      Returns the amount of user data bytes sent on success
 *      or a negative error code on failure.
 */
int x25_output(struct x25_cs *x25, char *data, int data_size) {
//	struct sk_buff *skbn;
//	_uchar header[X25_EXT_MIN_LEN];
//	int err;
//	int frontlen;
	int data_size_tmp = data_size;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);
	int len;
	int sent = 0;
//	int noblock = X25_SKB_CB(skb)->flags & MSG_DONTWAIT;
	int header_len = x25->link.extended ? X25_EXT_MIN_LEN : X25_STD_MIN_LEN;
	int max_len = x25_pacsize_to_bytes(x25_int->facilities.pacsize_out);

	if (data_size_tmp - header_len > max_len) {
//		/* Save a copy of the Header */
//		skb_copy_from_linear_data(skb, header, header_len);
//		skb_pull(skb, header_len);

//		frontlen = skb_headroom(skb);

		while (data_size_tmp > 0) {
//			release_sock(sk);
//			skbn = sock_alloc_send_skb(sk, frontlen + max_len, noblock, &err);
//			lock_sock(sk);
//			if (!skbn) {
//				if (err == -EWOULDBLOCK && noblock){
//					kfree_skb(skb);
//					return sent;
//				}
//				pr_debug(sk, "x25_output: fragment alloc failed, err=%d, %d bytes sent\n", err, sent);
//				return err;
//			}

//			skb_reserve(skbn, frontlen);

			len = max_len > data_size_tmp ? data_size_tmp : max_len;

//			/* Copy the user data */
//			skb_copy_from_linear_data(skb, skb_put(skbn, len), len);
//			skb_pull(skb, len);

//			/* Duplicate the Header */
//			skb_push(skbn, header_len);
//			skb_copy_to_linear_data(skbn, header, header_len);

//			if (skb->len > 0) {
//				if (x25->neighbour->extended)
//					skbn->data[3] |= X25_EXT_M_BIT;
//				else
//					skbn->data[2] |= X25_STD_M_BIT;
//			}

			//skb_queue_tail(&x25->write_queue, skbn);
			sent += len;
		};
	} else {
		cb_queue_tail(&x25_int->write_queue, data, data_size);
		sent = data_size - header_len;
	};
	return sent;
}

/*
 *	This procedure is passed a buffer descriptor for an iframe. It builds
 *	the rest of the control part of the frame and then writes it out.
 */
void x25_send_iframe(struct x25_cs *x25, char *data, int data_size) {
	if (!data)
		return;

	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (x25->link.extended) {
		data[2]  = (x25_int->vs << 1) & 0xFE;
		data[3] &= X25_EXT_M_BIT;
		data[3] |= (x25_int->vr << 1) & 0xFE;
	} else {
		data[2] &= X25_STD_M_BIT;
		data[2] |= (x25_int->vs << 1) & 0x0E;
		data[2] |= (x25_int->vr << 5) & 0xE0;
	}

	x25_transmit_link(x25, data, data_size);
}

void x25_kick(struct x25_cs * x25) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);
	if (x25_int->state != X25_STATE_3)
		return;

	_ushort start, end;
	int modulus;
	char * buffer;
	int buffer_size;

	/*
	 *	Transmit interrupt data.
	 */
	if (cb_peek(&x25_int->interrupt_out_queue) != NULL && !test_and_set_bit(X25_INTERRUPT_FLAG, &x25_int->flags)) {
		buffer = cb_dequeue(&x25_int->interrupt_out_queue, &buffer_size);
		x25_transmit_link(x25, buffer, buffer_size);
	};

	if (test_bit(X25_COND_PEER_RX_BUSY, (_ulong *)&x25_int->condition))
		return;

	if (!cb_peek(&x25_int->write_queue))
		return;

	modulus = x25->link.extended ? X25_EMODULUS : X25_SMODULUS;

	start   = cb_peek(&x25_int->ack_queue) ? x25_int->vs : x25_int->va;
	end     = (x25_int->va + x25_int->facilities.winsize_out) % modulus;

	if (start == end)
		return;

	x25_int->vs = start;

	/*
	 * Transmit data until either we're out of data to send or
	 * the window is full.
	 */
	buffer = cb_dequeue(&x25_int->write_queue, &buffer_size);

	do {
		/*
		 * Transmit the frame copy.
		 */
		x25_send_iframe(x25, buffer, buffer_size);

		x25_int->vs = (x25_int->vs + 1) % modulus;

		/*
		 * Requeue the original data frame.
		 */
		cb_queue_tail(&x25_int->ack_queue, buffer, buffer_size);

	} while (x25_int->vs != end && (buffer = cb_dequeue(&x25_int->write_queue, &buffer_size)) != NULL);

	x25_int->vl = x25_int->vr;
	x25_int->condition &= ~X25_COND_ACK_PENDING;

	x25_stop_timers(x25);
}

/*
 * The following routines are taken from page 170 of the 7th ARRL Computer
 * Networking Conference paper, as is the whole state machine.
 */

void x25_enquiry_response(struct x25_cs *x25) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (x25_int->condition & X25_COND_OWN_RX_BUSY)
		x25_write_internal(x25, X25_RNR);
	else
		x25_write_internal(x25, X25_RR);

	x25_int->vl         = x25_int->vr;
	x25_int->condition &= ~X25_COND_ACK_PENDING;

	x25_stop_timers(x25);
}
