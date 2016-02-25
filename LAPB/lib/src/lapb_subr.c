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


char str_buf[1024];
_uchar uchar_inv_table[256];

void lock(struct lapb_cs *lapb) {
#if !INTERNAL_SYNC
	(void)lapb;
#else
	if (!lapb) return;
	pthread_mutex_lock(&(lapb->_mutex));
#endif
}

void unlock(struct lapb_cs *lapb) {
#if !INTERNAL_SYNC
	(void)lapb;
#else
	if (!lapb) return;
	pthread_mutex_unlock(&(lapb->_mutex));
#endif
}

void fill_inv_table() {
	_uchar i = 0;
	_uchar tmp;
	while (1) {
		tmp =  (i & 0x01) << 7;
		tmp += (i & 0x80) >> 7;

		tmp += (i & 0x02) << 5;
		tmp += (i & 0x40) >> 5;

		tmp += (i & 0x04) << 3;
		tmp += (i & 0x20) >> 3;

		tmp += (i & 0x08) << 1;
		tmp += (i & 0x10) >> 1;

		uchar_inv_table[i] = tmp;
		if (i == 255) break;
		i++;
	};
}

_uchar invert_uchar(_uchar value) {
	return uchar_inv_table[value];
}

char * lapb_buf_to_str(char * data, int data_size) {
	str_buf[0] = '\0';
	if (data_size < 1024) {/* 1 byte for null-terminating */
		memcpy(str_buf, data, data_size);
		str_buf[data_size] = '\0';
	};
	return str_buf;
}

/*
 *	This routine delete all the queues of frames.
 */
void lapb_clear_queues(struct lapb_cs *lapb) {
	cb_clear(&lapb->write_queue);
	cb_clear(&lapb->ack_queue);
}

/*
 * This routine purges the input queue of those frames that have been
 * acknowledged. This replaces the boxes labelled "V(a) <- N(r)" on the
 * SDL diagram.
 */
int lapb_frames_acked(struct lapb_cs *lapb, unsigned short nr) {
	int modulus = (lapb->mode & LAPB_EXTENDED) ? LAPB_EMODULUS : LAPB_SMODULUS;

	/*
	 * Remove all the ack-ed frames from the ack queue.
	 */
	while (cb_peek(&lapb->ack_queue) && lapb->va != nr) {
		cb_dequeue(&lapb->ack_queue, NULL);
		lapb->va = (lapb->va + 1) % modulus;
	};
	return lapb->va == lapb->vs;
}

/*
 * Requeue all the un-ack-ed frames on the output queue to be picked
 * up by lapb_kick called from the timer. This arrangement handles the
 * possibility of an empty output queue.
 */
void lapb_requeue_frames(struct lapb_cs *lapb) {
	char *buffer;
	int buffer_size;

	while ((buffer = cb_dequeue_tail(&lapb->ack_queue, &buffer_size)) != NULL)
		cb_queue_head(&lapb->write_queue, buffer, buffer_size);
}

/*
 *	Validate that the value of nr is between va and vs. Return true or
 *	false for testing.
 */
int lapb_validate_nr(struct lapb_cs *lapb, unsigned short nr) {
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
int lapb_decode(struct lapb_cs * lapb, char * data, int data_size, 	struct lapb_frame * frame) {

	frame->type = LAPB_ILLEGAL;

	/* We always need to look at 2 bytes, sometimes we need
	 * to look at 3 and those cases are handled below.
	 */
	if (data_size < 2)
		return -1;

	/* Address firs (1 byte) */
	if (lapb->low_order_bits)
		data[0] = invert_uchar(data[0]);

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
		if (lapb->low_order_bits)
			data[1] = invert_uchar(data[1]);
		if (!(data[1] & LAPB_S)) {
			/*
			 * I frame - carries NR/NS/PF
			 */
			if (lapb->low_order_bits)
				data[2] = invert_uchar(data[2]);

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
			if (lapb->low_order_bits)
				data[2] = invert_uchar(data[2]);

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
		if (lapb->low_order_bits)
			data[1] = invert_uchar(data[1]);

		if (!(data[1] & LAPB_S)) {
			/*
			 * I frame - carries NR/NS/PF
			 */
			lapb->callbacks->debug(lapb, 2, "[LAPB] S%d RX %02X %02X %s", (int)lapb->state, (_uchar)data[0], (_uchar)data[1], lapb_buf_to_str(&data[2], data_size - 2));
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
			frame->type = (_uchar)data[1] & ~LAPB_SPF;
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
void lapb_send_control(struct lapb_cs *lapb, int frametype, int poll_bit, int type) {
	char frame[3]; /* Address[1]+Control[1/2]] */
	int frame_size = 2;

	bzero(frame, 3);

	if (lapb->mode & LAPB_EXTENDED) {
		if ((frametype & LAPB_U) == LAPB_U) {
			frame[1]  = frametype;
			frame[1] |= poll_bit ? LAPB_SPF : 0;

			if (lapb->low_order_bits)
				frame[1] = invert_uchar(frame[1]);
		} else {
			frame_size = 3;
			frame[1]  = frametype;
			frame[2]  = (lapb->vr << 1);
			frame[2] |= poll_bit ? LAPB_EPF : 0;

			if (lapb->low_order_bits) {
				frame[1] = invert_uchar(frame[1]);
				frame[2] = invert_uchar(frame[2]);
			};
		};
	} else {
		frame[1]  = frametype;
		frame[1] |= poll_bit ? LAPB_SPF : 0;
		if ((frametype & LAPB_U) == LAPB_S)	/* S frames carry NR */
			frame[1] |= (lapb->vr << 5);

		if (lapb->low_order_bits)
			frame[1] = invert_uchar(frame[1]);
	};

	lapb_transmit_buffer(lapb, frame, frame_size, type);
}

/*
 *	This routine generates FRMRs based on information previously stored in
 *	the LAPB control block.
 */
void lapb_transmit_frmr(struct lapb_cs *lapb) {
	char frame[7];
	int frame_size;

	if (lapb->mode & LAPB_EXTENDED) {
		frame_size = 7;
		frame[1] = LAPB_FRMR;
		frame[2] = lapb->frmr_data.control[0];
		frame[3] = lapb->frmr_data.control[1];
		frame[4] = (lapb->vs << 1) & 0xFE;
		frame[5] = (lapb->vr << 1) & 0xFE;
		if (lapb->frmr_data.cr == LAPB_RESPONSE)
			frame[5] |= 0x01;
		frame[6] = lapb->frmr_type;

		lapb->callbacks->debug(lapb,
							   1,
							   "[LAPB] S%d TX FRMR %02X %02X %02X %02X %02X",
							   lapb->state,
							   (_uchar)frame[2], (_uchar)frame[3], (_uchar)frame[4], (_uchar)frame[5], (_uchar)frame[6]);

		if (lapb->low_order_bits) {
			frame[1] = invert_uchar(frame[1]);
			char tmp = invert_uchar(frame[2]);
			frame[2] = invert_uchar(frame[3]);
			frame[3] = tmp;
			frame[4] = invert_uchar(frame[4]);
			frame[5] = invert_uchar(frame[5]);
			frame[6] = invert_uchar(frame[6]);
		};
	} else {
		frame_size = 5;
		frame[1] = LAPB_FRMR;
		frame[2] = lapb->frmr_data.control[0];
		frame[3] =  (lapb->vs << 1) & 0x0E;
		frame[3] |= (lapb->vr << 5) & 0xE0;
		if (lapb->frmr_data.cr == LAPB_RESPONSE)
			frame[3] |= 0x10;
		frame[4] = lapb->frmr_type;

		lapb->callbacks->debug(lapb, 1,
							   "[LAPB] S%d TX FRMR %02X %02X %02X %02X", lapb->state,
							   (_uchar)frame[1], (_uchar)frame[2], (_uchar)frame[3], (_uchar)frame[4]);

		if (lapb->low_order_bits) {
			/* Invert bytes */
			frame[1] = invert_uchar(frame[1]);
			frame[2] = invert_uchar(frame[2]);
			frame[3] = invert_uchar(frame[3]);
			frame[4] = invert_uchar(frame[4]);
		};
	};

	lapb_transmit_buffer(lapb, frame, frame_size, LAPB_RESPONSE);
}
