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
 *  This procedure is passed a buffer descriptor for an iframe. It builds
 *  the rest of the control part of the frame and then writes it out.
 */
void lapb_send_iframe(struct lapb_cs *lapb, char *data, int data_size, int poll_bit) {
	if (!data)
		return;

	char *	frame;
	int		frame_size = data_size;
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);


	if (lapb_is_extended(lapb)) {
		frame = cb_push(data, 3);
		frame_size += 3;

		frame[1] = LAPB_I;
		frame[1] |= lapb_int->vs << 1;
		frame[2] = poll_bit ? LAPB_EPF : 0;
		frame[2] |= lapb_int->vr << 1;

		if (lapb->low_order_bits) {
			frame[1] = invert_uchar(frame[1]);
			frame[2] = invert_uchar(frame[2]);
		};
	} else {
		frame = cb_push(data, 2);
		frame_size += 2;

		frame[1] = LAPB_I;
		frame[1] |= poll_bit ? LAPB_SPF : 0;
		frame[1] |= lapb_int->vr << 5;
		frame[1] |= lapb_int->vs << 1;

		if (lapb->low_order_bits)
			frame[1] = invert_uchar(frame[1]);
	};

	lapb->callbacks->debug(1, "[LAPB] S%d TX I(%d) S%d R%d", lapb_int->state, poll_bit, lapb_int->vs, lapb_int->vr);

	lapb_transmit_buffer(lapb, frame, frame_size, LAPB_COMMAND);
	lapb_int->last_vr = lapb_int->vr;
}

void lapb_kick(struct lapb_cs *lapb) {

	unsigned short modulus, start, end;
	char * buffer;
	int buffer_size;
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	modulus = lapb_is_extended(lapb) ? LAPB_EMODULUS : LAPB_SMODULUS;
	start = !cb_peek(&lapb_int->ack_queue) ? lapb_int->va : lapb_int->vs;
	end   = (lapb_int->va + lapb->window) % modulus;

	if (!(lapb_int->condition & LAPB_PEER_RX_BUSY_CONDITION) &&
		start != end && cb_peek(&lapb_int->write_queue)) {
		lapb_int->vs = start;

		/*
		 * Dequeue the frame and copy it.
		 */
		buffer = cb_dequeue(&lapb_int->write_queue, &buffer_size);

		do {
			/*
			 * Transmit the frame copy.
			 */
			lapb_send_iframe(lapb, buffer, buffer_size, LAPB_POLLOFF);

			lapb_int->vs = (lapb_int->vs + 1) % modulus;

			/*
			 * Requeue the original data frame.
			 */
			cb_queue_tail(&lapb_int->ack_queue, buffer, buffer_size, 0);

		} while (lapb_int->vs != end && (buffer = cb_dequeue(&lapb_int->write_queue, &buffer_size)) != NULL);

		lapb_int->condition &= ~LAPB_ACK_PENDING_CONDITION;

		lapb_start_t201timer(lapb);
		lapb_stop_t202timer(lapb);
	};
}

void lapb_transmit_buffer(struct lapb_cs *lapb, char * data, int data_size, int type) {
	char *ptr;
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	ptr = data;

	if (!lapb_is_slp(lapb)) {
		if (lapb_is_dce(lapb)) {
			if (type == LAPB_COMMAND)
				*ptr = LAPB_ADDR_C;
			if (type == LAPB_RESPONSE)
				*ptr = LAPB_ADDR_D;
		} else {
			if (type == LAPB_COMMAND)
				*ptr = LAPB_ADDR_D;
			if (type == LAPB_RESPONSE)
				*ptr = LAPB_ADDR_C;
		};
	} else {
		if (lapb_is_dce(lapb)) {
			if (type == LAPB_COMMAND)
				*ptr = LAPB_ADDR_A;
			if (type == LAPB_RESPONSE)
				*ptr = LAPB_ADDR_B;
		} else {
			if (type == LAPB_COMMAND)
				*ptr = LAPB_ADDR_B;
			if (type == LAPB_RESPONSE)
				*ptr = LAPB_ADDR_A;
		};
	};

#if LAPB_DEBUG >= 2
	if (lapb_is_extended(lapb))
		lapb->callbacks->debug(2, "[LAPB] S%d TX %02X %02X %02X",
							   lapb_int->state, (_uchar)data[0], (_uchar)data[1], (_uchar)data[2]);
	else {
		if (((_uchar)data[1] & 0x01) == 0)
			lapb->callbacks->debug(2, "[LAPB] S%d TX %02X %02X %s",
								   lapb_int->state, (_uchar)data[0], (_uchar)data[1], buf_to_str(&data[2], data_size - 2));
		else {
			if (data_size == 2)
				lapb->callbacks->debug(2, "[LAPB] S%d TX %02X %02X",
									   lapb_int->state, (_uchar)data[0], (_uchar)data[1]);
			else if (data_size == 3)
				lapb->callbacks->debug(2, "[LAPB] S%d TX %02X %02X %02X",
									   lapb_int->state, (_uchar)data[0], (_uchar)data[1], (_uchar)data[2]);
		};
	};
#endif
	if (lapb->low_order_bits)
		data[0] = invert_uchar(data[0]);

	lapb_data_transmit(lapb, data, data_size);
}

void lapb_establish_data_link(struct lapb_cs *lapb) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	lapb_int->condition = 0x00;

	if (lapb_is_extended(lapb)) {
		lapb->callbacks->debug(1, "[LAPB] S%d TX SABME(1)", lapb_int->state);
		lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
	} else {
		lapb->callbacks->debug(1, "[LAPB] S%d TX SABM(1)", lapb_int->state);
		lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
	};

	//if (is_dce(lapb))
		lapb_start_t201timer(lapb);
}

void lapb_enquiry_response(struct lapb_cs *lapb) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	lapb->callbacks->debug(1, "[LAPB] S%d TX RR(1) R%d", lapb_int->state, lapb_int->vr);
	lapb_send_control(lapb, LAPB_RR, LAPB_POLLON, LAPB_RESPONSE);
	lapb_int->condition &= ~LAPB_ACK_PENDING_CONDITION;
}

void lapb_timeout_response(struct lapb_cs *lapb) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	lapb->callbacks->debug(1, "[LAPB] S%d TX RR(0) R%d", lapb_int->state, lapb_int->vr);
	lapb_send_control(lapb, LAPB_RR, LAPB_POLLOFF, LAPB_RESPONSE);
	lapb_int->condition &= ~LAPB_ACK_PENDING_CONDITION;
}

void lapb_check_iframes_acked(struct lapb_cs *lapb, unsigned short nr) {
	if (lapb_frames_acked(lapb, nr))
		lapb_stop_t201timer(lapb);
	else
		lapb_restart_t201timer(lapb);
}

void lapb_check_need_response(struct lapb_cs *lapb, int type, int pf) {
	if ((type == LAPB_COMMAND) && pf)
		lapb_enquiry_response(lapb);
}
