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


char * x25_error_str(int error) {
	if (error < 0)
		error *= -1;
	switch (error) {
		case X25_OK:
			return X25_OK_STR;
		case X25_BADTOKEN:
			return X25_BADTOKEN_STR;
		case X25_INVALUE:
			return X25_INVALUE_STR;
		case X25_CONNECTED:
			return X25_CONNECTED_STR;
		case X25_NOTCONNECTED:
			return X25_NOTCONNECTED_STR;
		case X25_REFUSED:
			return X25_REFUSED_STR;
		case X25_TIMEDOUT:
			return X25_TIMEDOUT_STR;
		case X25_NOMEM:
			return X25_NOMEM_STR;
		case X25_BUSY:
			return X25_BUSY_STR;
		case X25_ROOT_UNREACH:
			return X25_ROOT_UNREACH_STR;
		case X25_CALLBACK_ERR:
			return X25_CALLBACK_ERR_STR;
		default:
			return "Unknown error";
			break;
	};
}


char * x25_buf_to_str(char * data, int data_size) {
	str_buf[0] = '\0';
	if (data_size < 1024) {/* 1 byte for null-terminating */
		x25_mem_copy(str_buf, data, data_size);
		str_buf[data_size] = '\0';
	};
	return str_buf;
}

void x25_hex_debug(char * data, int data_size, char * out_buffer, int out_buffer_size) {
#ifdef __GNUC__
	if (out_buffer_size < (data_size*3 + 1)) return;

	int i = 0;
	char * out_ptr = out_buffer;

	bzero(out_buffer, out_buffer_size);

	while (i < data_size) {
		//fprintf(stderr, "%02X ", data[i]);
		sprintf(out_ptr, "%02X ", (_uchar)data[i]);
		i++;
		out_ptr += 3;
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

void set_bit(long nr, _ulong * addr) {
	*addr |= nr;
}

void clear_bit(long nr, _ulong * addr) {
	*addr &= ~nr;
}

int test_bit(long nr, _ulong * addr) {
	if (*addr & nr)
		return 1;
	else
		return 0;
}

int test_and_set_bit(long nr, _ulong * addr) {
	int old = 0;

	if (*addr & nr)
		old = 1;
	*addr |= nr;
	return old;
}


/* Check DTE or DCE bit is set */
int x25_is_dte(struct x25_cs * x25) {
	if (x25->mode & X25_DCE)
		return FALSE;
	else
		return TRUE;
}

/* Check STANDARD or EXTENDED bit is set */
int x25_is_extended(struct x25_cs * x25) {
	if (x25->mode & X25_EXTENDED)
		return TRUE;
	else
		return FALSE;
}


struct x25_cs_internal * x25_get_internal(struct x25_cs *x25) {
	return (struct x25_cs_internal *)x25->internal_struct;
}


int x25_parse_address_block(char * data, int data_size, struct x25_address *called_addr, struct x25_address *calling_addr) {
	_uchar len;
	int needed;
	int rc;

	if (data_size == 0) {
		/* packet has no address block */
		rc = 0;
		goto empty;
	};

	len = *data;
	needed = 1 + (len >> 4) + (len & 0x0f);

	if (data_size < needed) {
		/* packet is too short to hold the addresses it claims to hold */
		rc = -1;
		goto empty;
	};

	return x25_addr_ntoa((_uchar *)data, called_addr, calling_addr);

empty:
	*called_addr->x25_addr = 0;
	*calling_addr->x25_addr = 0;

	return rc;
}

int x25_addr_ntoa(_uchar *p, struct x25_address *called_addr, struct x25_address *calling_addr) {
	_uint called_len;
	_uint calling_len;
	char * called;
	char * calling;
	_uint i;

	called_len  = (*p >> 0) & 0x0F;
	calling_len = (*p >> 4) & 0x0F;

	called  = called_addr->x25_addr;
	calling = calling_addr->x25_addr;
	p++;

	for (i = 0; i < (called_len + calling_len); i++) {
		if (i < called_len) {
			if (i % 2 != 0) {
				*called++ = ((*p >> 0) & 0x0F) + '0';
				p++;
			} else
				*called++ = ((*p >> 4) & 0x0F) + '0';
		} else {
			if (i % 2 != 0) {
				*calling++ = ((*p >> 0) & 0x0F) + '0';
				p++;
			} else
				*calling++ = ((*p >> 4) & 0x0F) + '0';
		};
	};

	*called = *calling = '\0';

	return 1 + (called_len + calling_len + 1) / 2;
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
 *	This routine purges all of the queues of frames.
 */
void x25_clear_queues(struct x25_cs * x25) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	cb_clear(&x25_int->ack_queue);
	cb_clear(&x25_int->write_queue);
	cb_clear(&x25_int->receive_queue);
	cb_clear(&x25_int->interrupt_in_queue);
	cb_clear(&x25_int->interrupt_out_queue);
	x25_int->fragment_len = 0;
}


/*
 * This routine purges the input queue of those frames that have been
 * acknowledged. This replaces the boxes labelled "V(a) <- N(r)" on the
 * SDL diagram.
*/
int x25_frames_acked(struct x25_cs * x25, _ushort nr) {
	int modulus = x25_is_extended(x25) ? X25_EMODULUS : X25_SMODULUS;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	/*
	 * Remove all the ack-ed frames from the ack queue.
	 */
	while (cb_peek(&x25_int->ack_queue) && x25_int->va != nr) {
		cb_dequeue(&x25_int->ack_queue, NULL);
		x25_int->va = (x25_int->va + 1) % modulus;
	};
	return x25_int->va == x25_int->vs;
}

void x25_requeue_frames(struct x25_cs * x25) {
	char *buffer;
	int buffer_size;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	/*
	 * Requeue all the un-ack-ed frames on the output queue to be picked
	 * up by x25_kick. This arrangement handles the possibility of an empty
	 * output queue.
	 */
	while ((buffer = cb_dequeue_tail(&x25_int->ack_queue, &buffer_size)) != NULL)
		cb_queue_head(&x25_int->write_queue, buffer, buffer_size);
}


/*
 *	Validate that the value of nr is between va and vs. Return true or
 *	false for testing.
 */
int x25_validate_nr(struct x25_cs * x25, _ushort nr) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);
	_ushort vc = x25_int->va;
	int modulus = x25_is_extended(x25) ? X25_EMODULUS : X25_SMODULUS;

	while (vc != x25_int->vs) {
		if (nr == vc)
			return 1;
		vc = (vc + 1) % modulus;
	};

	return nr == x25_int->vs ? 1 : 0;
}


/*
 *  This routine is called when the packet layer internally generates a
 *  control frame.
 */
void x25_write_internal(struct x25_cs *x25, int frametype) {
	_uchar data[209];
	_uchar  *dptr = &data[0];
	_uchar  facilities[X25_MAX_FAC_LEN];
	_uchar  addresses[1 + X25_ADDR_LEN];
	_uchar  lci1, lci2;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	/*
	 *	Default safe frame size.
	 */
	int len = X25_EXT_MIN_LEN;

	/*
	 *	Adjust frame size.
	 */
	switch (frametype) {
		case X25_CALL_REQUEST:
			len += 1 + X25_ADDR_LEN + X25_MAX_FAC_LEN + X25_MAX_CUD_LEN;
			break;
		case X25_CALL_ACCEPTED: /* fast sel with no restr on resp */
			if (x25_int->facilities.reverse & 0x80) {
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

	/*
	 *	Make space for the GFI and LCI, and fill them in.
	 */
	lci1 = (x25->lci >> 8) & 0x0F;
	lci2 = (x25->lci >> 0) & 0xFF;

	if (x25_is_extended(x25)) {
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
			*dptr++ = X25_CALL_REQUEST;
			len = x25_addr_aton(addresses, &x25->dest_addr, &x25->source_addr);
			mem_copy(dptr, addresses, len);
			dptr += len;
			len = x25_create_facilities(facilities,
										&x25_int->facilities,
										&x25_int->dte_facilities,
										x25->link.global_facil_mask);
			mem_copy(dptr, facilities, len);
			dptr += len;
			if (x25_int->calluserdata.cudlength != 0) {
				mem_copy(dptr, x25_int->calluserdata.cuddata, x25_int->calluserdata.cudlength);
				x25_int->calluserdata.cudlength = 0;
			};
			break;

		case X25_CALL_ACCEPTED:
			*dptr++ = X25_CALL_ACCEPTED;
			*dptr++ = 0x00;		/* Address lengths */
			len     = x25_create_facilities(facilities,
											&x25_int->facilities,
											&x25_int->dte_facilities,
											x25_int->vc_facil_mask);
			mem_copy(dptr, facilities, len);
			dptr += len;

			/* fast select with no restriction on response
				allows call user data. Userland must
				ensure it is ours and not theirs */
			if (x25_int->facilities.reverse & 0x80) {
				mem_copy(dptr, x25_int->calluserdata.cuddata, x25_int->calluserdata.cudlength);
			};
			x25_int->calluserdata.cudlength = 0;
			break;

		case X25_CLEAR_REQUEST:
			*dptr++ = frametype;
			*dptr++ = x25_int->causediag.cause;
			*dptr++ = x25_int->causediag.diagnostic;
			break;

		case X25_RESET_REQUEST:
			*dptr++ = frametype;
			*dptr++ = 0x00;		/* XXX */
			*dptr++ = 0x00;		/* XXX */
			set_bit(X25_COND_RESET, &x25_int->condition);
			break;

		case X25_RR:
		case X25_RNR:
		case X25_REJ:
			if (x25_is_extended(x25)) {
				*dptr++  = frametype;
				*dptr++  = (x25_int->vr << 1) & 0xFE;
			} else {
				*dptr    = frametype;
				*dptr++ |= (x25_int->vr << 5) & 0xE0;
			};
			break;

		case X25_CLEAR_CONFIRMATION:
		case X25_INTERRUPT_CONFIRMATION:
		case X25_RESET_CONFIRMATION:
			*dptr++ = frametype;
			break;
	};

	int n = (_ulong)(dptr) - (_ulong)data;

	char debug_buf[2048];
	x25_hex_debug((char *)data, n, debug_buf, 2048);
	x25->callbacks->debug(1, "[X25] S%d TX '%s'", x25_int->state, debug_buf);
	x25_transmit_link(x25, (char *)data, n);
}



void x25_disconnect(void * x25_ptr, int reason, _uchar cause, _uchar diagnostic) {
	(void)reason;
	struct x25_cs *x25 = x25_ptr;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_clear_queues(x25);
	x25_stop_timers(x25);

	x25->peer_lci = 0;
	x25->callbacks->debug(1, "[X25] S%d -> S0", x25_int->state);
	x25_int->state = X25_STATE_0;

	x25_int->causediag.cause      = cause;
	x25_int->causediag.diagnostic = diagnostic;
}



/*
 *	Unpick the contents of the passed X.25 Packet Layer frame.
 */
int x25_decode(struct x25_cs * x25, char * data, int data_size, struct x25_frame *frame) {

	if (data_size < X25_STD_MIN_LEN)
		return X25_ILLEGAL;

	_uchar * data_ptr = (_uchar *)data;

	x25_mem_zero(frame, sizeof(struct x25_frame));

	switch (data_ptr[2]) {
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
			frame->type = data_ptr[2];
			return 0;
	};

	if (x25_is_extended(x25)) {
		if (data_ptr[2] == X25_RR  || data_ptr[2] == X25_RNR || data_ptr[2] == X25_REJ) {
			if (data_size <  X25_EXT_MIN_LEN)
				return X25_ILLEGAL;

			frame->nr = (data_ptr[3] >> 1) & 0x7F;
			frame->type = data_ptr[2];
			return 0;
		};
		if ((data_ptr[2] & 0x01) == X25_DATA) {
			if (data_size <  X25_EXT_MIN_LEN)
				return X25_ILLEGAL;

			frame->q_flag  = (data_ptr[0] & X25_Q_BIT) == X25_Q_BIT;
			frame->d_flag  = (data_ptr[0] & X25_D_BIT) == X25_D_BIT;
			frame->m_flag  = (data_ptr[3] & X25_EXT_M_BIT) == X25_EXT_M_BIT;
			frame->nr = (data_ptr[3] >> 1) & 0x7F;
			frame->ns = (data_ptr[2] >> 1) & 0x7F;
			frame->type = X25_DATA;
			return 0;
		};
	} else {
		if ((data_ptr[2] & 0x1F) == X25_RR  || (data_ptr[2] & 0x1F) == X25_RNR || (data_ptr[2] & 0x1F) == X25_REJ) {
			frame->nr = (data_ptr[2] >> 5) & 0x07;
			frame->type = data_ptr[2] & 0x1F;
			return 0;
		};
		if ((data_ptr[2] & 0x01) == X25_DATA) {
			frame->q_flag  = (data_ptr[0] & X25_Q_BIT) == X25_Q_BIT;
			frame->d_flag  = (data_ptr[0] & X25_D_BIT) == X25_D_BIT;
			frame->m_flag  = (data_ptr[2] & X25_STD_M_BIT) == X25_STD_M_BIT;
			frame->nr = (data_ptr[2] >> 5) & 0x07;
			frame->ns = (data_ptr[2] >> 1) & 0x07;
			frame->type = X25_DATA;
			return 0;
		};
	};

	x25->callbacks->debug(2, "[X25] Invalid PLP frame %02X %02X %02X", frame[0], frame[1], frame[2]);

	return X25_ILLEGAL;
}



int x25_rx_call_request(struct x25_cs *x25, char *data, int data_size, _uint lci) {
	struct x25_address source_addr, dest_addr;
	//struct x25_facilities facilities;
	struct x25_dte_facilities dte_facilities;
	int len;
	int addr_len, rc;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	char * ptr;
	int data_size_tmp = data_size;

	x25->callbacks->debug(1, "[X25] S%d RX CALL_REQUEST", x25_int->state);
	/*
	 *	Remove the LCI and frame type.
	 */
	ptr = data + X25_STD_MIN_LEN;
	data_size_tmp -= X25_STD_MIN_LEN;

	/*
	 *	Extract the X.25 addresses and convert them to ASCII strings,
	 *	and remove them.
	 *
	 *	Address block is mandatory in call request packets
	 */
	addr_len = x25_parse_address_block(ptr, data_size_tmp, &source_addr, &dest_addr);
	if (addr_len <= 0)
		goto out_clear_request;
	ptr += addr_len;
	data_size_tmp -= addr_len;

	/*
	 *	Get the length of the facilities, skip past them for the moment
	 *	get the call user data because this is needed to determine
	 *	the correct listener
	 *
	 *	Facilities length is mandatory in call request packets
	 */
	if (data_size_tmp == 0)
		goto out_clear_request;
	len = ptr[0] + 1;
	if (data_size_tmp < len)
		goto out_clear_request;
	ptr += len;
	data_size_tmp -= len;

	/*
	 *	Ensure that the amount of call user data is valid.
	 */
	if (data_size_tmp > X25_MAX_CUD_LEN)
		goto out_clear_request;

	ptr -= len;
	data_size_tmp += len;

	/*
	 *	Try to reach a compromise on the requested facilities.
	 */
	len = x25_negotiate_facilities(x25, ptr, data_size_tmp, &dte_facilities);
	if (len == -1)
		goto out_clear_request;

	/*
	 * current link might impose additional limits on certain facilties
	 */
	x25_limit_facilities(x25);

	/*
	 *	Remove the facilities
	 */
	ptr += len;
	data_size_tmp -= len;

	x25->peer_lci  = lci;
	x25->dest_addr = dest_addr;

	/* ensure no reverse facil on accept */
	x25_int->vc_facil_mask &= ~X25_MASK_REVERSE;
	/* ensure no calling address extension on accept */
	x25_int->vc_facil_mask &= ~X25_MASK_CALLING_AE;

	/* Normally all calls are accepted immediately */
	if (x25_int->flags & X25_ACCPT_APPRV_FLAG) {
		x25->callbacks->debug(1, "[X25] S%d TX CALL_ACCEPTED", x25_int->state);
		x25_write_internal(x25, X25_CALL_ACCEPTED);
		x25->callbacks->debug(1, "[X25] S%d -> S3", x25_int->state);
		x25_int->state = X25_STATE_3;
	};

	/*
	 *	Incoming Call User Data.
	 */
	x25_mem_copy(x25_int->calluserdata.cuddata, ptr, data_size_tmp);
	x25_int->calluserdata.cudlength = data_size_tmp;

	rc = 1;
	/* Notify Application about new incoming call */
	if (x25->callbacks->call_indication)
		x25->callbacks->call_indication(x25);

	return rc;

out_clear_request:
	rc = 0;
	x25_transmit_clear_request(x25, lci, 0x01);
	return 0;
}
