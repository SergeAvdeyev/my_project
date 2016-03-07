/*
 *	LAPB release 001
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
	(void)lapb;
#else
	if (!x25) return;

	//pthread_mutex_lock(&(lapb_get_internal(x25)->_mutex));
#endif
}

void unlock(struct x25_cs *x25) {
#if !INTERNAL_SYNC
	(void)lapb;
#else
	if (!x25) return;
	//pthread_mutex_unlock(&(lapb_get_internal(lapb)->_mutex));
#endif
}


char * lapb_buf_to_str(char * data, int data_size) {
	str_buf[0] = '\0';
	if (data_size < 1024) {/* 1 byte for null-terminating */
		x25_mem_copy(str_buf, data, data_size);
		str_buf[data_size] = '\0';
	};
	return str_buf;
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

int x25_pacsize_to_bytes(unsigned int pacsize) {
	int bytes = 1;

	if (!pacsize)
		return 128;

	while (pacsize-- > 0)
		bytes *= 2;

	return bytes;
}




/*
 *  This routine is called when the packet layer internally generates a
 *  control frame.
 */
void x25_write_internal(struct x25_cs *x25, int frametype) {
	//struct x25_sock *x25 = x25_sk(sk);
	struct sk_buff *skb;
	unsigned char  *dptr = NULL;
	//unsigned char  facilities[X25_MAX_FAC_LEN];
	unsigned char  addresses[1 + X25_ADDR_LEN];
	unsigned char  lci1, lci2;
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
	}

	if ((skb = malloc(len)) == NULL)
		return;

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

	if (x25->neighbour->extended) {
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
			len     = x25_addr_aton(addresses, &x25->dest_addr, &x25->source_addr);
			//dptr    = skb_put(skb, len);
			memcpy(dptr, addresses, len);
			len     = x25_create_facilities(facilities, &x25->facilities, &x25->dte_facilities, x25->neighbour->global_facil_mask);
			//dptr    = skb_put(skb, len);
			memcpy(dptr, facilities, len);
			//dptr = skb_put(skb, x25->calluserdata.cudlength);
			memcpy(dptr, x25->calluserdata.cuddata, x25->calluserdata.cudlength);
			x25->calluserdata.cudlength = 0;
			break;

		case X25_CALL_ACCEPTED:
			//dptr    = skb_put(skb, 2);
			*dptr++ = X25_CALL_ACCEPTED;
			*dptr++ = 0x00;		/* Address lengths */
			len     = x25_create_facilities(facilities,
							&x25->facilities,
							&x25->dte_facilities,
							x25->vc_facil_mask);
			//dptr    = skb_put(skb, len);
			memcpy(dptr, facilities, len);

			/* fast select with no restriction on response
				allows call user data. Userland must
				ensure it is ours and not theirs */
			if(x25->facilities.reverse & 0x80) {
				//dptr = skb_put(skb, x25->calluserdata.cudlength);
				memcpy(dptr, x25->calluserdata.cuddata, x25->calluserdata.cudlength);
			}
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
			if (x25->neighbour->extended) {
				//dptr     = skb_put(skb, 2);
				*dptr++  = frametype;
				*dptr++  = (x25->vr << 1) & 0xFE;
			} else {
				//dptr     = skb_put(skb, 1);
				*dptr    = frametype;
				*dptr++ |= (x25->vr << 5) & 0xE0;
			}
			break;

		case X25_CLEAR_CONFIRMATION:
		case X25_INTERRUPT_CONFIRMATION:
		case X25_RESET_CONFIRMATION:
			//dptr  = skb_put(skb, 1);
			*dptr = frametype;
			break;
	}

	x25_transmit_link(skb, x25->neighbour);
}
