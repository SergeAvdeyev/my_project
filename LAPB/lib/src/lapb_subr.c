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


char str_buf[1024];

char * lapb_buf_to_str(char * data, int data_size) {

	bzero(str_buf, 1024);
	if (data_size < 1023) /* 1 byte for null-terminating */
		memcpy(str_buf, data, data_size);
	return str_buf;
}

/*
 *	This routine delete all the queues of frames.
 */
void lapb_clear_queues(struct lapb_cb *lapb) {
	cb_clear(&lapb->write_queue);
	cb_clear(&lapb->ack_queue);
	//skb_queue_purge(&lapb->write_queue);
	//skb_queue_purge(&lapb->ack_queue);

}

/*
 * This routine purges the input queue of those frames that have been
 * acknowledged. This replaces the boxes labelled "V(a) <- N(r)" on the
 * SDL diagram.
 */
void lapb_frames_acked(struct lapb_cb *lapb, unsigned short nr) {
	//struct sk_buff *skb;
	int modulus;

	modulus = (lapb->mode & LAPB_EXTENDED) ? LAPB_EMODULUS : LAPB_SMODULUS;

	/*
	 * Remove all the ack-ed frames from the ack queue.
	 */
	if (lapb->va != nr)
		while (cb_peek(&lapb->ack_queue) && lapb->va != nr) {
			cb_dequeue(&lapb->ack_queue, NULL);
			lapb->va = (lapb->va + 1) % modulus;
		};
}

void lapb_requeue_frames(struct lapb_cb *lapb) {
	char *buffer;
	int buffer_size;
	/*
	 * Requeue all the un-ack-ed frames on the output queue to be picked
	 * up by lapb_kick called from the timer. This arrangement handles the
	 * possibility of an empty output queue.
	 */
	while ((buffer = cb_dequeue_tail(&lapb->ack_queue, &buffer_size)) != NULL) {
		cb_queue_head(&lapb->write_queue, buffer, buffer_size);
	};
}

/*
 *	Validate that the value of nr is between va and vs. Return true or
 *	false for testing.
 */
int lapb_validate_nr(struct lapb_cb *lapb, unsigned short nr) {
	unsigned short vc = lapb->va;
	int modulus;

	modulus = (lapb->mode & LAPB_EXTENDED) ? LAPB_EMODULUS : LAPB_SMODULUS;

	while (vc != lapb->vs) {
		if (nr == vc)
			return 1;
		vc = (vc + 1) % modulus;
	};

	return nr == lapb->vs;
}

/*
 *	This routine is the centralised routine for parsing the control
 *	information for the different frame formats.
 */
int lapb_decode(struct lapb_cb * lapb, char * data, int data_size, 	struct lapb_frame * frame) {

	frame->type = LAPB_ILLEGAL;


	/* We always need to look at 2 bytes, sometimes we need
	 * to look at 3 and those cases are handled below.
	 */
	if (data_size < 2)
		return -1;

	if (lapb->mode & LAPB_MLP) {
		if (lapb->mode & LAPB_DCE) {
			if (data[0] == LAPB_ADDR_D)
				frame->cr = LAPB_COMMAND;
			else if (data[0] == LAPB_ADDR_C)
				frame->cr = LAPB_RESPONSE;
		} else {
			if (data[0] == LAPB_ADDR_C)
				frame->cr = LAPB_COMMAND;
			else if (data[0] == LAPB_ADDR_D)
				frame->cr = LAPB_RESPONSE;
		};
	} else {
		if (lapb->mode & LAPB_DCE) {
			if (data[0] == LAPB_ADDR_B)
				frame->cr = LAPB_COMMAND;
			else if (data[0] == LAPB_ADDR_A)
				frame->cr = LAPB_RESPONSE;
		} else {
			if (data[0] == LAPB_ADDR_A)
				frame->cr = LAPB_COMMAND;
			else if (data[0] == LAPB_ADDR_B)
				frame->cr = LAPB_RESPONSE;
		};
	};

	if (lapb->mode & LAPB_EXTENDED) {
		if (!(data[1] & LAPB_S)) {
			/*
			 * I frame - carries NR/NS/PF
			 */
			frame->type       = LAPB_I;
			frame->ns         = (data[1] >> 1) & 0x7F;
			frame->nr         = (data[2] >> 1) & 0x7F;
			frame->pf         = data[2] & LAPB_EPF;
			frame->control[0] = data[1];
			frame->control[1] = data[2];
		} else if ((data[1] & LAPB_U) == 1) {
			/*
			 * S frame - take out PF/NR
			 */
			frame->type       = data[1] & 0x0F;
			frame->nr         = (data[2] >> 1) & 0x7F;
			frame->pf         = data[2] & LAPB_EPF;
			frame->control[0] = data[1];
			frame->control[1] = data[2];
		} else if ((data[1] & LAPB_U) == 3) {
			/*
			 * U frame - take out PF
			 */
			frame->type       = data[1] & ~LAPB_SPF;
			frame->pf         = data[1] & LAPB_SPF;
			frame->control[0] = data[1];
			frame->control[1] = 0x00;
		};
	} else {
		if (!(data[1] & LAPB_S)) {
			/*
			 * I frame - carries NR/NS/PF
			 */
			lapb->callbacks->debug(lapb, 2, "S%d RX %02X %02X %s", (int)lapb->state, (_uchar)data[0], (_uchar)data[1], lapb_buf_to_str(&data[2], data_size - 2));
			frame->type = LAPB_I;
			frame->ns   = (data[1] >> 1) & 0x07;
			frame->nr   = (data[1] >> 5) & 0x07;
			frame->pf   = data[1] & LAPB_SPF;
		} else if ((data[1] & LAPB_U) == 1) {
			/*
			 * S frame - take out PF/NR
			 */
			frame->type = data[1] & 0x0F;
			frame->nr   = (data[1] >> 5) & 0x07;
			frame->pf   = data[1] & LAPB_SPF;
		} else if ((data[1] & LAPB_U) == 3) {
			/*
			 * U frame - take out PF
			 */
			frame->type = data[1] & ~LAPB_SPF;
			frame->pf   = data[1] & LAPB_SPF;
		};

		frame->control[0] = data[1];
	};

	frame->pf = frame->pf >> 4;

	return 0;
}

/*
 *	This routine is called when the HDLC layer internally  generates a
 *	command or  response  for  the remote machine ( eg. RR, UA etc. ).
 *	Only supervisory or unnumbered frames are processed, FRMRs are handled
 *	by lapb_transmit_frmr below.
 */
void lapb_send_control(struct lapb_cb *lapb, int frametype, int poll_bit, int type) {
	char frame[3]; /* Address[1]+Control[1/2]] */
	int frame_size = 2;

	bzero(frame, 3);

	if (lapb->mode & LAPB_EXTENDED) {
		if ((frametype & LAPB_U) == LAPB_U) {
			frame[1]  = frametype;
			frame[1] |= poll_bit ? LAPB_SPF : 0;
		} else {
			frame_size = 3;
			frame[1]  = frametype;
			frame[2]  = (lapb->vr << 1);
			frame[2] |= poll_bit ? LAPB_EPF : 0;
		};
	} else {
		frame[1]  = frametype;
		frame[1] |= poll_bit ? LAPB_SPF : 0;
		if ((frametype & LAPB_U) == LAPB_S)	/* S frames carry NR */
			frame[1] |= (lapb->vr << 5);
	};

	lapb_transmit_buffer(lapb, frame, frame_size, type);
}

/*
 *	This routine generates FRMRs based on information previously stored in
 *	the LAPB control block.
 */
void lapb_transmit_frmr(struct lapb_cb *lapb) {
	char  *dptr;

	char data[6];
	int data_size;

	bzero(data, 6);

	dptr = data;

	if (lapb->mode & LAPB_EXTENDED) {
		data_size = 6;
		*dptr++ = LAPB_FRMR;
		*dptr++ = lapb->frmr_data.control[0];
		*dptr++ = lapb->frmr_data.control[1];
		*dptr++ = (lapb->vs << 1) & 0xFE;
		*dptr   = (lapb->vr << 1) & 0xFE;
		if (lapb->frmr_data.cr == LAPB_RESPONSE)
			*dptr |= 0x01;
		dptr++;
		*dptr++ = lapb->frmr_type;

		lapb->callbacks->debug(lapb, 1, "S%d TX FRMR %02X %02X %02X %02X %02X", lapb->state, (_uchar)data[1], (_uchar)data[2], (_uchar)data[3], (_uchar)data[4], (_uchar)data[5]);
	} else {
		data_size = 4;
		*dptr++ = LAPB_FRMR;
		*dptr++ = lapb->frmr_data.control[0];
		*dptr   = (lapb->vs << 1) & 0x0E;
		*dptr  |= (lapb->vr << 5) & 0xE0;
		if (lapb->frmr_data.cr == LAPB_RESPONSE)
			*dptr |= 0x10;
		dptr++;
		*dptr++ = lapb->frmr_type;

		lapb->callbacks->debug(lapb, 1, "S%d TX FRMR %02X %02X %02X", lapb->state, (_uchar)data[1], (_uchar)data[2], (_uchar)data[3]);
	};

	lapb_transmit_buffer(lapb, data, data_size, LAPB_RESPONSE);
}
