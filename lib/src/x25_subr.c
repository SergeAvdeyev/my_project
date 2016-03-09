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


char str_buf[1024];

void lock(struct x25_cs *x25) {
#if !INTERNAL_SYNC
	(void)x25;
#else
	if (!x25) return;
	pthread_mutex_lock(&(x25_get_internal(x25)->_mutex));
#endif
}

void unlock(struct x25_cs *x25) {
#if !INTERNAL_SYNC
	(void)x25;
#else
	if (!x25) return;
	pthread_mutex_unlock(&(x25_get_internal(x25)->_mutex));
#endif
}


char * x25_buf_to_str(char * data, int data_size) {
	str_buf[0] = '\0';
	if (data_size < 1024) {/* 1 byte for null-terminating */
		x25_mem_copy(str_buf, data, data_size);
		str_buf[data_size] = '\0';
	};
	return str_buf;
}

void x25_hex_debug(char * data, int data_size) {
#ifdef __GNUC__
	int i = 0;
	while (i < data_size) {
		fprintf(stderr, "%02X ", data[i]);
		i++;
	};
#endif
}

void * x25_mem_get(_ulong size) {
#ifdef __GNUC__
	return malloc(size);
#else
	size = 0;
	return (void *)size;
#endif
}

void x25_mem_free(void * ptr) {
#ifdef __GNUC__
	free(ptr);
#else
	*(int *)ptr = 0;
#endif
}

void * x25_mem_copy(void *dest, const void *src, _ulong n) {
#ifdef __GNUC__
	return memcpy(dest, src, n);
#else
	_ulong i = 0;
	while (i < n) {
		*(char *)dest = *(char *)src;
		i++;
	};
	return dest;
#endif
}

void x25_mem_zero(void *src, _ulong n) {
#ifdef __GNUC__
	bzero(src, n);
#else
	_ulong i = 0;
	while (i < n) {
		*(char *)src = 0;
		i++;
	};
#endif
}


struct x25_cs_internal * x25_get_internal(struct x25_cs *x25) {
	return (struct x25_cs_internal *)x25->internal_struct;
}

/*
 *	This routine purges all of the queues of frames.
 */
void x25_clear_queues(struct x25_cs * x25) {
	cb_clear(&x25->ack_queue);
	cb_clear(&x25->interrupt_in_queue);
	cb_clear(&x25->interrupt_out_queue);
	cb_clear(&x25->fragment_queue);
}

int x25_pacsize_to_bytes(unsigned int pacsize) {
	int bytes = 1;

	if (!pacsize)
		return 128;

	while (pacsize-- > 0)
		bytes *= 2;

	return bytes;
}

int x25_addr_aton(_uchar *p, struct x25_address *called_addr, struct x25_address *calling_addr) {
	_uint called_len, calling_len;
	char *called, *calling;
	_uint i;

	called  = called_addr->x25_addr;
	calling = calling_addr->x25_addr;

	called_len  = strlen(called);
	calling_len = strlen(calling);

	*p++ = (calling_len << 4) | (called_len << 0);

	for (i = 0; i < (called_len + calling_len); i++) {
		if (i < called_len) {
			if (i % 2 != 0) {
				*p |= (*called++ - '0') << 0;
				p++;
			} else {
				*p = 0x00;
				*p |= (*called++ - '0') << 4;
			}
		} else {
			if (i % 2 != 0) {
				*p |= (*calling++ - '0') << 0;
				p++;
			} else {
				*p = 0x00;
				*p |= (*calling++ - '0') << 4;
			};
		};
	};

	return 1 + (called_len + calling_len + 1) / 2;
}






/*
 *  This routine is called when the packet layer internally generates a
 *  control frame.
 */
void x25_write_internal(struct x25_cs *x25, int frametype) {
	//struct x25_sock *x25 = x25_sk(sk);
	//struct sk_buff *skb;
	_uchar data[209];  /* 2 bytes for GFI & LCI */
						/*  */
	_uchar  *dptr = &data[0];
	_uchar  facilities[X25_MAX_FAC_LEN];
	_uchar  addresses[1 + X25_ADDR_LEN];
	_uchar  lci1, lci2;
	/*
	 *	Default safe frame size.
	 */
	//int len = X25_MAX_L2_LEN + X25_EXT_MIN_LEN;
	int len = X25_EXT_MIN_LEN;

	/*
	 *	Adjust frame size.
	 */
	switch (frametype) {
		case X25_CALL_REQUEST:
			len += 1 + X25_ADDR_LEN + X25_MAX_FAC_LEN + X25_MAX_CUD_LEN;
			break;
		case X25_CALL_ACCEPTED: /* fast sel with no restr on resp */
			if (x25->facilities.reverse & 0x80) {
				len += 1 + X25_MAX_FAC_LEN + X25_MAX_CUD_LEN;
			} else {
				len += 1 + X25_MAX_FAC_LEN;
			};
			break;
		case X25_CLEAR_REQUEST:
		case X25_RESET_REQUEST:
			len += 2;
			break;
		case X25_RR:
		case X25_RNR:
		case X25_REJ:
		case X25_CLEAR_CONFIRMATION:
		case X25_INTERRUPT_CONFIRMATION:
		case X25_RESET_CONFIRMATION:
			break;
		default:
			x25->callbacks->debug(0, "invalid frame type %02X\n", frametype);
			return;
	};

//	if ((skb = malloc(len)) == NULL)
//		return;

	/*
	 *	Space for Ethernet and 802.2 LLC headers.
	 */
	//skb_reserve(skb, X25_MAX_L2_LEN);

	/*
	 *	Make space for the GFI and LCI, and fill them in.
	 */
	//dptr = skb_put(skb, 2);

	lci1 = (x25->lci >> 8) & 0x0F;
	lci2 = (x25->lci >> 0) & 0xFF;

	if (x25->link.extended) {
		*dptr++ = lci1 | X25_GFI_EXTSEQ;
		*dptr++ = lci2;
	} else {
		*dptr++ = lci1 | X25_GFI_STDSEQ;
		*dptr++ = lci2;
	};

	/*
	 *	Now fill in the frame type specific information.
	 */
	switch (frametype) {

		case X25_CALL_REQUEST:
			//dptr    = skb_put(skb, 1);
			*dptr++ = X25_CALL_REQUEST;
			len = x25_addr_aton(addresses, &x25->dest_addr, &x25->source_addr);
			//dptr    = skb_put(skb, len);
			mem_copy(dptr, addresses, len);
			dptr += len;
			len = x25_create_facilities(facilities, &x25->facilities, &x25->dte_facilities, x25->link.global_facil_mask);
			//dptr    = skb_put(skb, len);
			mem_copy(dptr, facilities, len);
			dptr += len;
			if (x25->calluserdata.cudlength != 0) {
				//dptr = skb_put(skb, x25->calluserdata.cudlength);
				mem_copy(dptr, x25->calluserdata.cuddata, x25->calluserdata.cudlength);
				x25->calluserdata.cudlength = 0;
				//dptr += len;
			};
			break;

		case X25_CALL_ACCEPTED:
			//dptr    = skb_put(skb, 2);
			*dptr++ = X25_CALL_ACCEPTED;
			*dptr++ = 0x00;		/* Address lengths */
			len     = x25_create_facilities(facilities, &x25->facilities, &x25->dte_facilities, x25->vc_facil_mask);
			//dptr    = skb_put(skb, len);
			mem_copy(dptr, facilities, len);

			/* fast select with no restriction on response
				allows call user data. Userland must
				ensure it is ours and not theirs */
			if (x25->facilities.reverse & 0x80) {
				//dptr = skb_put(skb, x25->calluserdata.cudlength);
				mem_copy(dptr, x25->calluserdata.cuddata, x25->calluserdata.cudlength);
			};
			x25->calluserdata.cudlength = 0;
			break;

		case X25_CLEAR_REQUEST:
			//dptr    = skb_put(skb, 3);
			*dptr++ = frametype;
			*dptr++ = x25->causediag.cause;
			*dptr++ = x25->causediag.diagnostic;
			break;

		case X25_RESET_REQUEST:
			//dptr    = skb_put(skb, 3);
			*dptr++ = frametype;
			*dptr++ = 0x00;		/* XXX */
			*dptr++ = 0x00;		/* XXX */
			break;

		case X25_RR:
		case X25_RNR:
		case X25_REJ:
			if (x25->link.extended) {
				//dptr     = skb_put(skb, 2);
				*dptr++  = frametype;
				*dptr++  = (x25->vr << 1) & 0xFE;
			} else {
				//dptr     = skb_put(skb, 1);
				*dptr    = frametype;
				*dptr++ |= (x25->vr << 5) & 0xE0;
			};
			break;

		case X25_CLEAR_CONFIRMATION:
		case X25_INTERRUPT_CONFIRMATION:
		case X25_RESET_CONFIRMATION:
			//dptr  = skb_put(skb, 1);
			*dptr = frametype;
			break;
	};

	int n = (_ulong)(dptr) - (_ulong)data;
	x25_hex_debug((char *)data, n);
	x25_transmit_link(x25, (char *)data, n);
}



void x25_disconnect(void * x25_ptr, int reason, unsigned char cause, unsigned char diagnostic) {
	(void)reason;
	struct x25_cs *x25 = x25_ptr;

	x25_clear_queues(x25);
	x25->callbacks->stop_timer(x25->T2.timer_ptr);
	x25->callbacks->stop_timer(x25->T21.timer_ptr);
	x25->callbacks->stop_timer(x25->T22.timer_ptr);
	x25->callbacks->stop_timer(x25->T23.timer_ptr);

	x25->lci   = 0;
	x25->state = X25_STATE_0;

	x25->causediag.cause      = cause;
	x25->causediag.diagnostic = diagnostic;
}



/*
 *	Unpick the contents of the passed X.25 Packet Layer frame.
 */
int x25_decode(struct x25_cs * x25, char * data, int data_size, int *ns, int *nr, int *q, int *d, int *m) {
	_uchar *frame;

	if (data_size < X25_STD_MIN_LEN)
		return X25_ILLEGAL;
	frame = (_uchar *)data;

	*ns = *nr = *q = *d = *m = 0;

	switch (frame[2]) {
		case X25_CALL_REQUEST:
		case X25_CALL_ACCEPTED:
		case X25_CLEAR_REQUEST:
		case X25_CLEAR_CONFIRMATION:
		case X25_INTERRUPT:
		case X25_INTERRUPT_CONFIRMATION:
		case X25_RESET_REQUEST:
		case X25_RESET_CONFIRMATION:
		case X25_RESTART_REQUEST:
		case X25_RESTART_CONFIRMATION:
		case X25_REGISTRATION_REQUEST:
		case X25_REGISTRATION_CONFIRMATION:
		case X25_DIAGNOSTIC:
			return frame[2];
	};

	if (x25->link.extended) {
		if (frame[2] == X25_RR  || frame[2] == X25_RNR || frame[2] == X25_REJ) {
			if (data_size <  X25_EXT_MIN_LEN)
				return X25_ILLEGAL;
			frame = (_uchar *)data;

			*nr = (frame[3] >> 1) & 0x7F;
			return frame[2];
		};
	} else {
		if ((frame[2] & 0x1F) == X25_RR  || (frame[2] & 0x1F) == X25_RNR || (frame[2] & 0x1F) == X25_REJ) {
			*nr = (frame[2] >> 5) & 0x07;
			return frame[2] & 0x1F;
		};
	};

	if (x25->link.extended) {
		if ((frame[2] & 0x01) == X25_DATA) {
			if (data_size <  X25_EXT_MIN_LEN)
				return X25_ILLEGAL;
			frame = (_uchar *)data;

			*q  = (frame[0] & X25_Q_BIT) == X25_Q_BIT;
			*d  = (frame[0] & X25_D_BIT) == X25_D_BIT;
			*m  = (frame[3] & X25_EXT_M_BIT) == X25_EXT_M_BIT;
			*nr = (frame[3] >> 1) & 0x7F;
			*ns = (frame[2] >> 1) & 0x7F;
			return X25_DATA;
		};
	} else {
		if ((frame[2] & 0x01) == X25_DATA) {
			*q  = (frame[0] & X25_Q_BIT) == X25_Q_BIT;
			*d  = (frame[0] & X25_D_BIT) == X25_D_BIT;
			*m  = (frame[2] & X25_STD_M_BIT) == X25_STD_M_BIT;
			*nr = (frame[2] >> 5) & 0x07;
			*ns = (frame[2] >> 1) & 0x07;
			return X25_DATA;
		};
	};

	x25->callbacks->debug(2, "[X25] Invalid PLP frame %02X %02X %02X", frame[0], frame[1], frame[2]);

	return X25_ILLEGAL;
}
