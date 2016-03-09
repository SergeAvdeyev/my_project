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


//char str_buf[1024];
_uchar uchar_inv_table[256];

void lapb_lock(struct lapb_cs *lapb) {
#if !INTERNAL_SYNC
	(void)lapb;
#else
	if (!lapb) return;

	pthread_mutex_trylock(&(lapb_get_internal(lapb)->_mutex));
#endif
}

void lapb_unlock(struct lapb_cs *lapb) {
#if !INTERNAL_SYNC
	(void)lapb;
#else
	if (!lapb) return;
	pthread_mutex_unlock(&(lapb_get_internal(lapb)->_mutex));
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

//char * lapb_buf_to_str(char * data, int data_size) {
//	str_buf[0] = '\0';
//	if (data_size < 1024) {/* 1 byte for null-terminating */
//		lapb_mem_copy(str_buf, data, data_size);
//		str_buf[data_size] = '\0';
//	};
//	return str_buf;
//}

//void * lapb_mem_get(_ulong size) {
//#ifdef __GNUC__
//	return malloc(size);
//#else
//	size = 0;
//	return (void *)size;
//#endif
//}

//void lapb_mem_free(void * ptr) {
//#ifdef __GNUC__
//	free(ptr);
//#else
//	*(int *)ptr = 0;
//#endif
//}

//void * lapb_mem_copy(void *dest, const void *src, _ulong n) {
//#ifdef __GNUC__
//	return memcpy(dest, src, n);
//#else
//	_ulong i = 0;
//	while (i < n) {
//		*(char *)dest = *(char *)src;
//		i++;
//	};
//	return dest;
//#endif
//}

//void lapb_mem_zero(void *src, _ulong n) {
//#ifdef __GNUC__
//	bzero(src, n);
//#else
//	_ulong i = 0;
//	while (i < n) {
//		*(char *)src = 0;
//		i++;
//	};
//#endif
//}

/*  */
int lapb_is_dce(struct lapb_cs *lapb) {
	return (lapb->mode & LAPB_DCE);
}

/*   */
int lapb_is_extended(struct lapb_cs *lapb) {
	return (lapb->mode & LAPB_EXTENDED);
}

int lapb_is_slp(struct lapb_cs *lapb) {
	return !(lapb->mode & LAPB_MLP);
}

struct lapb_cs_internal * lapb_get_internal(struct lapb_cs *lapb) {
	return (struct lapb_cs_internal *)lapb->internal_struct;
}

/*
 *	This routine delete all the queues of frames.
 */
void lapb_clear_queues(struct lapb_cs *lapb) {
	cb_clear(&lapb_get_internal(lapb)->write_queue);
	cb_clear(&lapb_get_internal(lapb)->ack_queue);
}

/*
 * This routine purges the input queue of those frames that have been
 * acknowledged. This replaces the boxes labelled "V(a) <- N(r)" on the
 * SDL diagram.
 */
int lapb_frames_acked(struct lapb_cs *lapb, unsigned short nr) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);
	int modulus = lapb_is_extended(lapb) ? LAPB_EMODULUS : LAPB_SMODULUS;

	/*
	 * Remove all the ack-ed frames from the ack queue.
	 */
	while (cb_peek(&lapb_int->ack_queue) && lapb_int->va != nr) {
		cb_dequeue(&lapb_int->ack_queue, NULL);
		lapb_int->va = (lapb_int->va + 1) % modulus;
	};
	return lapb_int->va == lapb_int->vs;
}

/*
 * Requeue all the un-ack-ed frames on the output queue to be picked
 * up by lapb_kick called from the timer. This arrangement handles the
 * possibility of an empty output queue.
 */
void lapb_requeue_frames(struct lapb_cs *lapb) {
	char *buffer;
	int buffer_size;
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	while ((buffer = cb_dequeue_tail(&lapb_int->ack_queue, &buffer_size)) != NULL)
		cb_queue_head(&lapb_int->write_queue, buffer, buffer_size);
}

/*
 *	Validate that the value of nr is between va and vs. Return true or
 *	false for testing.
 */
int lapb_validate_nr(struct lapb_cs *lapb, unsigned short nr) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);
	unsigned short vc = lapb_int->va;
	int modulus;

	modulus = lapb_is_extended(lapb) ? LAPB_EMODULUS : LAPB_SMODULUS;

	while (vc != lapb_int->vs) {
		if (nr == vc)
			return 1;
		vc = (vc + 1) % modulus;
	};

	return nr == lapb_int->vs;
}

/*
 *	This routine is the centralised routine for parsing the control
 *	information for the different frame formats.
 */
int lapb_decode(struct lapb_cs * lapb, char * data, int data_size, 	struct lapb_frame * frame) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	frame->type = LAPB_ILLEGAL;

	/* We always need to look at 2 bytes, sometimes we need
	 * to look at 3 and those cases are handled below.
	 */
	if (data_size < 2)
		return -1;

	/* Address firs (1 byte) */
	if (lapb->low_order_bits)
		data[0] = invert_uchar(data[0]);

	if (!lapb_is_slp(lapb)) {
		if (lapb_is_dce(lapb)) {
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
		if (lapb_is_dce(lapb)) {
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


	if (lapb_is_extended(lapb)) {
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
			lapb->callbacks->debug(2, "[LAPB] S%d RX %02X %02X %s",
								   lapb_int->state, (_uchar)data[0], (_uchar)data[1], buf_to_str(&data[2], data_size - 2));
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
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);
	char frame[3]; /* Address[1]+Control[1/2]] */
	int frame_size = 2;

	//zero_mem(frame, 3);

	if (lapb_is_extended(lapb)) {
		if ((frametype & LAPB_U) == LAPB_U) {
			frame[1]  = frametype;
			frame[1] |= poll_bit ? LAPB_SPF : 0;

			if (lapb->low_order_bits)
				frame[1] = invert_uchar(frame[1]);
		} else {
			frame_size = 3;
			frame[1]  = frametype;
			frame[2]  = (lapb_int->vr << 1);
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
			frame[1] |= (lapb_int->vr << 5);

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
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (lapb_is_extended(lapb)) {
		frame_size = 7;
		frame[1] = LAPB_FRMR;
		frame[2] = lapb_int->frmr_data.control[0];
		frame[3] = lapb_int->frmr_data.control[1];
		frame[4] = (lapb_int->vs << 1) & 0xFE;
		frame[5] = (lapb_int->vr << 1) & 0xFE;
		if (lapb_int->frmr_data.cr == LAPB_RESPONSE)
			frame[5] |= 0x01;
		frame[6] = lapb_int->frmr_type;

		lapb->callbacks->debug(1, "[LAPB] S%d TX FRMR %02X %02X %02X %02X %02X",
							   lapb_int->state, (_uchar)frame[2], (_uchar)frame[3], (_uchar)frame[4], (_uchar)frame[5], (_uchar)frame[6]);

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
		frame[2] = lapb_int->frmr_data.control[0];
		frame[3] =  (lapb_int->vs << 1) & 0x0E;
		frame[3] |= (lapb_int->vr << 5) & 0xE0;
		if (lapb_int->frmr_data.cr == LAPB_RESPONSE)
			frame[3] |= 0x10;
		frame[4] = lapb_int->frmr_type;

		lapb->callbacks->debug(1, "[LAPB] S%d TX FRMR %02X %02X %02X %02X",
							   lapb_int->state, (_uchar)frame[1], (_uchar)frame[2], (_uchar)frame[3], (_uchar)frame[4]);

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
