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
int x25_output(struct x25_cs *x25, char *data, int data_size, int q_bit_flag) {
	_uchar header[X25_EXT_MIN_LEN];
	struct x25_cs_internal * x25_int = x25_get_internal(x25);
	int len;
	int sent = 0;
	int header_len = x25_is_extended(x25) ? X25_EXT_MIN_LEN : X25_STD_MIN_LEN;
	int max_len = x25_pacsize_to_bytes(x25_int->facilities.pacsize_out);
	char * ptr = data;
	char * tmp_buf;

	/* Build X25 Header */
	if (x25_is_extended(x25)) {
		/* Build an Extended X.25 header */
		header[0] = ((x25->lci >> 8) & 0x0F) | X25_GFI_EXTSEQ;
		header[1] = (x25->lci >> 0) & 0xFF;
		header[2] = X25_DATA;
		header[3] = X25_DATA;
	} else {
		/* Build an Standard X.25 header */
		header[0] = ((x25->lci >> 8) & 0x0F) | X25_GFI_STDSEQ;
		header[1] = (x25->lci >> 0) & 0xFF;
		header[2] = X25_DATA;
	};
	if (q_bit_flag)
		header[0] |= X25_Q_BIT;

//	if (data_size > max_len) {
		while (data_size > 0) {
			len = max_len > data_size ? data_size : max_len;
			tmp_buf = cb_queue_tail(&x25_int->write_queue, ptr, len, header_len);
			if (x25_is_extended(x25)) {
				/* Duplicate the Header */
				tmp_buf[0] = header[0];
				tmp_buf[1] = header[1];
				tmp_buf[2] = header[2];
				tmp_buf[3] = header[3];
			} else {
				/* Duplicate the Header */
				tmp_buf[0] = header[0];
				tmp_buf[1] = header[1];
				tmp_buf[2] = header[2];
			};
			data_size -= len;
			ptr += len;
			if (data_size > 0) {
				if (x25_is_extended(x25))
					tmp_buf[3] |= X25_EXT_M_BIT;
				else
					tmp_buf[2] |= X25_STD_M_BIT;
			};
			sent += len;
		};
//	} else {
//		tmp_buf = cb_queue_tail(&x25_int->write_queue, data, data_size, 0);
//		sent = data_size;
//	};
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

	if (x25_is_extended(x25)) {
		data[2]  = (x25_int->vs << 1) & 0xFE;
		data[3] &= X25_EXT_M_BIT;
		data[3] |= (x25_int->vr << 1) & 0xFE;
	} else {
		data[2] &= X25_STD_M_BIT;
		data[2] |= (x25_int->vs << 1) & 0x0E;
		data[2] |= (x25_int->vr << 5) & 0xE0;
	}

	x25->callbacks->debug(1, "[X25] S%d TX DATA S%d R%d", x25_int->state, x25_int->vs, x25_int->vr);
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

	if (test_bit(X25_COND_PEER_RX_BUSY, &x25_int->condition))
		return;

	if (!cb_peek(&x25_int->write_queue))
		return;

	modulus = x25_is_extended(x25) ? X25_EMODULUS : X25_SMODULUS;

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
		cb_queue_tail(&x25_int->ack_queue, buffer, buffer_size, 0);
	} while (x25_int->vs != end && (buffer = cb_dequeue(&x25_int->write_queue, &buffer_size)) != NULL);

	x25_int->vl = x25_int->vr;
	clear_bit(X25_COND_ACK_PENDING, &x25_int->condition);

	x25_stop_timer(x25, &x25_int->AckTimer);
	x25_start_timer(x25, &x25_int->DataTimer);
}

void x25_enquiry_response(struct x25_cs *x25) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (test_bit(X25_COND_OWN_RX_BUSY, &x25_int->condition)) {
		x25->callbacks->debug(1, "[X25] S%d TX RNR S%d R%d", x25_int->state, x25_int->vs, x25_int->vr);
		x25_write_internal(x25, X25_RNR);
	} else {
		x25->callbacks->debug(1, "[X25] S%d TX RR S%d R%d", x25_int->state, x25_int->vs, x25_int->vr);
		x25_write_internal(x25, X25_RR);
	};

	x25_int->vl = x25_int->vr;
	clear_bit(X25_COND_ACK_PENDING, &x25_int->condition);

	x25_stop_timer(x25, &x25_int->AckTimer);
}

void x25_check_iframes_acked(struct x25_cs *x25, _ushort nr) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (x25_frames_acked(x25, nr))
		x25_stop_timer(x25, &x25_int->DataTimer);
	else {
		x25_int->DataTimer.RC = 0;
		x25_start_timer(x25, &x25_int->DataTimer);
	};
}

