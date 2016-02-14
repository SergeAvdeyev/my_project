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

//#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


#include <stdlib.h>
#include "net_lapb.h"

/*
 *  This procedure is passed a buffer descriptor for an iframe. It builds
 *  the rest of the control part of the frame and then writes it out.
 */
//static void lapb_send_iframe(struct lapb_cb *lapb, struct sk_buff *skb, int poll_bit)
void lapb_send_iframe(struct lapb_cb *lapb, char *data, int data_size, int poll_bit) {
	if (!data)
		return;

	char * frame;
	int				frame_size = data_size;


	if (lapb->mode & LAPB_EXTENDED) {
		frame = cb_push(data, 3);
		frame_size += 3;

		frame[1] = LAPB_I;
		frame[1] |= lapb->vs << 1;
		frame[2] = poll_bit ? LAPB_EPF : 0;
		frame[2] |= lapb->vr << 1;
	} else {
		frame = cb_push(data, 2);
		frame_size += 2;

		frame[1] = LAPB_I;
		frame[1] |= poll_bit ? LAPB_SPF : 0;
		frame[1] |= lapb->vr << 5;
		frame[1] |= lapb->vs << 1;
	};

	lapb->callbacks->debug(lapb, 1, "S%d TX I(%d) S%d R%d\n", lapb->state, poll_bit, lapb->vs, lapb->vr);

	lapb_transmit_buffer(lapb, frame, frame_size, LAPB_COMMAND);


//	if (!data)
//		return;

//	unsigned char *	frame;
//	int				frame_size = data_size + 2;
//	unsigned char *	ptr;

//	frame = malloc(frame_size);
//	memcpy(frame + 2, data, data_size);
//	ptr = frame + 1;

//	if (lapb->mode & LAPB_EXTENDED) {
////		frame = skb_push(skb, 2);

//		ptr[0] = LAPB_I;
//		ptr[0] |= lapb->vs << 1;
//		ptr[1] = poll_bit ? LAPB_EPF : 0;
//		ptr[1] |= lapb->vr << 1;
//	} else {
////		frame = skb_push(skb, 1);

//		ptr[0] = LAPB_I;
//		ptr[0] |= poll_bit ? LAPB_SPF : 0;
//		ptr[0] |= lapb->vr << 5;
//		ptr[0] |= lapb->vs << 1;
//	};

//	lapb->callbacks->debug(lapb, 1, "S%d TX I(%d) S%d R%d\n", lapb->state, poll_bit, lapb->vs, lapb->vr);

//	lapb_transmit_buffer(lapb, frame, frame_size, LAPB_COMMAND);
}

//void lapb_kick(struct lapb_cb *lapb, unsigned char *data, int data_size) {
void lapb_kick(struct lapb_cb *lapb) {

	//struct sk_buff *skb, *skbn;
	unsigned short modulus, start, end;
	char * buffer;
	int buffer_size;
	//struct lapb_buff * lpb;

	modulus = (lapb->mode & LAPB_EXTENDED) ? LAPB_EMODULUS : LAPB_SMODULUS;
	start = !cb_peek(&lapb->ack_queue) ? lapb->va : lapb->vs;
	end   = (lapb->va + lapb->window) % modulus;

	if (!(lapb->condition & LAPB_PEER_RX_BUSY_CONDITION) &&
		start != end && cb_peek(&lapb->write_queue)) {
		lapb->vs = start;

		/*
		 * Dequeue the frame and copy it.
		 */
		buffer = cb_dequeue(&lapb->write_queue, &buffer_size);

		do {
			/*
			 * Transmit the frame copy.
			 */
			lapb_send_iframe(lapb, buffer, buffer_size, LAPB_POLLOFF);

			lapb->vs = (lapb->vs + 1) % modulus;

			/*
			 * Requeue the original data frame.
			 */
			cb_queue_tail(&lapb->ack_queue, buffer, buffer_size);

		} while (lapb->vs != end && (buffer = cb_dequeue(&lapb->write_queue, &buffer_size)) != NULL);

		lapb->condition &= ~LAPB_ACK_PENDING_CONDITION;

		if (!lapb_t1timer_running(lapb))
			lapb_start_t1timer(lapb);
	};
}

void lapb_transmit_buffer(struct lapb_cb *lapb, char * data, int data_size, int type) {
	char *ptr;

	ptr = data;

	if (lapb->mode & LAPB_MLP) {
		if (lapb->mode & LAPB_DCE) {
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
		if (lapb->mode & LAPB_DCE) {
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

	lapb_data_transmit(lapb, data, data_size);

	if (lapb->mode & LAPB_EXTENDED)
		lapb->callbacks->debug(lapb, 2, "S%d TX %02X %02X %02X %02X %02X\n", lapb->state, data[0], data[1], data[2], data[3], data[4]);
	else {
		if (data_size == 4)
			lapb->callbacks->debug(lapb, 2, "S%d TX %02X %02X %02X %02X\n", lapb->state, data[0], data[1], (_uchar)data[2], (_uchar)data[3]);
		else
			lapb->callbacks->debug(lapb, 2, "S%d TX %02X %02X %s\n", lapb->state, (_uchar)data[0], (_uchar)data[1], (char *)&data[2]);
	};

	//free(data);
}

void lapb_establish_data_link(struct lapb_cb *lapb) {
	lapb->condition = 0x00;
	//lapb->n2count   = 0;

	if (lapb->mode & LAPB_EXTENDED) {
		lapb->callbacks->debug(lapb, 1, "S%d TX SABME(1)\n", lapb->state);
		lapb_send_control(lapb, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
	} else {
		lapb->callbacks->debug(lapb, 1, "S%d TX SABM(1)\n", lapb->state);
		lapb_send_control(lapb, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
	};

	if ((lapb->mode & LAPB_DCE) == LAPB_DCE)
		lapb_start_t1timer(lapb);
	//lapb_stop_t2timer(lapb);
}

void lapb_enquiry_response(struct lapb_cb *lapb) {
	lapb->callbacks->debug(lapb, 1, "S%d TX RR(1) R%d\n", lapb->state, lapb->vr);

	lapb_send_control(lapb, LAPB_RR, LAPB_POLLON, LAPB_RESPONSE);

	lapb->condition &= ~LAPB_ACK_PENDING_CONDITION;
}

void lapb_timeout_response(struct lapb_cb *lapb) {
	lapb->callbacks->debug(lapb, 1, "S%d TX RR(0) R%d\n", lapb->state, lapb->vr);

	lapb_send_control(lapb, LAPB_RR, LAPB_POLLOFF, LAPB_RESPONSE);

	lapb->condition &= ~LAPB_ACK_PENDING_CONDITION;
}

void lapb_check_iframes_acked(struct lapb_cb *lapb, unsigned short nr) {
	if (lapb->vs == nr) {
		lapb_frames_acked(lapb, nr);
		lapb_stop_t1timer(lapb);
		//lapb->n2count = 0;
	} else if (lapb->va != nr) {
		lapb_frames_acked(lapb, nr);
		lapb_start_t1timer(lapb);
	};
}

void lapb_check_need_response(struct lapb_cb *lapb, int type, int pf) {
	if (type == LAPB_COMMAND && pf)
		lapb_enquiry_response(lapb);
}
