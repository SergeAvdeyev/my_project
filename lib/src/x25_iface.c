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

static struct x25_address null_x25_address = {"               "};


void x25_default_debug(int level, const char * format, ...) {
	(void)level;
	(void)format;
}


int x25_register(struct x25_callbacks *callbacks, struct x25_params * params, struct x25_cs ** x25) {
	(void)params;
	int rc = X25_CALLBACK_ERR;

	/* Check callbacks */
	if (!callbacks->add_timer ||
		!callbacks->del_timer ||
		!callbacks->start_timer ||
		!callbacks->stop_timer ||
		!callbacks->link_connect_request ||
		!callbacks->link_disconnect_request ||
		!callbacks->link_send_frame ||
		!callbacks->call_indication ||
		!callbacks->call_accepted)
		goto out;

	*x25 = x25_mem_get(sizeof(struct x25_cs));
	rc = X25_NOMEM;
	if (!*x25)
		goto out;


	x25_mem_zero(*x25, sizeof(struct x25_cs));

	struct x25_cs_internal * x25_int = x25_mem_get(sizeof(struct x25_cs_internal));
	x25_mem_zero(x25_int, sizeof(struct x25_cs_internal));
	(*x25)->internal_struct = x25_int;

	if (!callbacks->debug)
		callbacks->debug = x25_default_debug;
	(*x25)->callbacks = callbacks;

#if INTERNAL_SYNC
	/* Init mutex for sinchronization */
	pthread_mutex_init(&(x25_int->_mutex), NULL);
#endif

	if (params)
		x25_set_params(*x25, params);
	else {
		/* Set default values */
		x25_int->T20.interval = X25_DEFAULT_T20;
		x25_int->T21.interval = X25_DEFAULT_T21;
		x25_int->T22.interval = X25_DEFAULT_T22;
		x25_int->T23.interval = X25_DEFAULT_T23;
		(*x25)->N20 = 2;
		(*x25)->N22 = 2;
	};

	/* Create timers T201, T202 */
	x25_int->T20.timer_ptr  = (*x25)->callbacks->add_timer(x25_int->T20.interval,  *x25, x25_t20timer_expiry);
	x25_int->T21.timer_ptr = (*x25)->callbacks->add_timer(x25_int->T21.interval, *x25, x25_t21timer_expiry);
	x25_int->T22.timer_ptr = (*x25)->callbacks->add_timer(x25_int->T22.interval, *x25, x25_t22timer_expiry);
	x25_int->T23.timer_ptr = (*x25)->callbacks->add_timer(x25_int->T23.interval, *x25, x25_t23timer_expiry);

	x25_int->state = X25_STATE_0;
	(*x25)->cudmatchlength = 0;		/* normally no cud on call accept */
	x25_int->flags |= X25_ACCPT_APPRV_FLAG;

	x25_int->facilities.winsize_in  = X25_DEFAULT_WINDOW_SIZE;
	x25_int->facilities.winsize_out = X25_DEFAULT_WINDOW_SIZE;
	x25_int->facilities.pacsize_in  = X25_DEFAULT_PACKET_SIZE;
	x25_int->facilities.pacsize_out = X25_DEFAULT_PACKET_SIZE;
	x25_int->facilities.throughput  = 0;	/* by default don't negotiate
										throughput */
	x25_int->facilities.reverse     = X25_DEFAULT_REVERSE;
	x25_int->dte_facilities.calling_len = 0;
	x25_int->dte_facilities.called_len = 0;
	x25_mem_zero(x25_int->dte_facilities.called_ae, sizeof(x25_int->dte_facilities.called_ae));
	x25_mem_zero(x25_int->dte_facilities.calling_ae, sizeof(x25_int->dte_facilities.calling_ae));

	/* Create queues */
	cb_init(&x25_int->ack_queue, X25_SMODULUS, x25_pacsize_to_bytes(X25_DEFAULT_PACKET_SIZE)*X25_DEFAULT_WINDOW_SIZE);
	cb_init(&x25_int->write_queue, X25_SMODULUS, x25_pacsize_to_bytes(X25_DEFAULT_PACKET_SIZE)*X25_DEFAULT_WINDOW_SIZE);
	cb_init(&x25_int->receive_queue, X25_SMODULUS, x25_pacsize_to_bytes(X25_DEFAULT_PACKET_SIZE)*X25_DEFAULT_WINDOW_SIZE);
	//cb_init(&(*x25)->fragment_queue, X25_SMODULUS, x25_pacsize_to_bytes((*x25)->facilities.pacsize_out));
	cb_init(&x25_int->interrupt_in_queue, X25_SMODULUS, x25_pacsize_to_bytes(x25_int->facilities.pacsize_out));
	cb_init(&x25_int->interrupt_out_queue, X25_SMODULUS, x25_pacsize_to_bytes(x25_int->facilities.pacsize_out));

	(*x25)->link.T2.timer_ptr = NULL;

	rc = X25_OK;
out:
	return rc;
}

int x25_unregister(struct x25_cs * x25) {
	int rc = X25_BADTOKEN;

	if (!x25)
		goto out;

	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_stop_timer(x25, &x25_int->T20);
	x25_stop_timer(x25, &x25_int->T21);
	x25_stop_timer(x25, &x25_int->T22);
	x25_stop_timer(x25, &x25_int->T23);
	x25->callbacks->del_timer(x25_int->T20.timer_ptr);
	x25->callbacks->del_timer(x25_int->T21.timer_ptr);
	x25->callbacks->del_timer(x25_int->T22.timer_ptr);
	x25->callbacks->del_timer(x25_int->T23.timer_ptr);

	cb_free(&x25_int->ack_queue);
	cb_free(&x25_int->write_queue);
	cb_free(&x25_int->receive_queue);
	//cb_free(&x25->fragment_queue);
	cb_free(&x25_int->interrupt_in_queue);
	cb_free(&x25_int->interrupt_out_queue);

	if (x25->link.T2.timer_ptr) {
		x25_stop_timer(x25, &x25->link.T2);
		x25->callbacks->del_timer(x25->link.T2.timer_ptr);
	};
	cb_free(&x25->link.queue);

	x25_mem_free(x25_int);
	x25_mem_free(x25);
	rc = X25_OK;
out:
	return rc;
}

int x25_get_params(struct x25_cs * x25, struct x25_params * params) {
	(void)x25;
	(void)params;
	return 0;
}

int x25_set_params(struct x25_cs * x25, struct x25_params *params) {
	(void)x25;
	(void)params;
	return 0;
}

int x25_get_state(struct x25_cs *x25) {
	return x25_get_internal(x25)->state;
}


void x25_add_link(struct x25_cs *x25, void * link, int extended) {
	x25->link.link_ptr = link;
	x25->link.state    = X25_LINK_STATE_0;
	x25->link.extended = extended == 0 ? 0 : 1;
	cb_init(&x25->link.queue, 10, 1024);
	x25->link.T2.interval = X25_DEFAULT_T2;
	x25->link.T2.timer_ptr = x25->callbacks->add_timer(x25->link.T2.interval, x25, x25_t2timer_expiry);
	x25->link.global_facil_mask =	X25_MASK_REVERSE |
									X25_MASK_THROUGHPUT |
									X25_MASK_PACKET_SIZE |
									X25_MASK_WINDOW_SIZE;
}




//int x25_connect_request(struct x25_cs * x25, struct x25_address *uaddr, int addr_len) {
int x25_call_request(struct x25_cs * x25, struct x25_address *dest_addr) {
	int rc = X25_BADTOKEN;

	if (!x25)
		goto out;
//	lock(x25);

	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	rc = X25_NOTCONNECTED;
	if (x25_int->state == X25_STATE_1)
		//goto out_unlock;
		goto out;

	rc = X25_CONNECTED;
	if (x25_int->state == X25_STATE_3)
		//goto out_unlock;
		goto out;

//	rc = X25_ROOT_UNREACH;
//	rt = x25_get_route(&addr);
//	if (!rt)
//		goto out_unlock;

	x25_limit_facilities(x25);

//	x25->lci = x25_new_lci(x25->neighbour);
//	if (!x25->lci)
//		goto out_put_neigh;


	if (!strcmp(x25->source_addr.x25_addr, null_x25_address.x25_addr))
		memset(&x25->source_addr, '\0', X25_ADDR_LEN);

	x25->dest_addr = *dest_addr;

	x25->callbacks->debug(1, "[X25] S%d Make Call to %s", x25_int->state, dest_addr->x25_addr);

	x25->callbacks->debug(1, "[X25] S%d TX CALL_REQUEST", x25_int->state);
	x25->callbacks->debug(1, "[X25] S%d -> S1", x25_int->state);
	x25_int->state = X25_STATE_1;
	x25_write_internal(x25, X25_CALL_REQUEST);

	x25_start_heartbeat(x25);
	x25_start_timer(x25, x25_int->T21.timer_ptr);

//	/* Now the loop */
//	rc = -EINPROGRESS;
//	if (sk->sk_state != TCP_ESTABLISHED && (flags & O_NONBLOCK))
//		goto out_put_neigh;

//	rc = x25_wait_for_connection_establishment(sk);
//	if (rc)
//		goto out_put_neigh;

	rc = 0;
//out_unlock:
//	unlock(x25);
out:
	return rc;
}



int x25_clear_request(struct x25_cs * x25) {
	if (!x25)
		return 0;
//	lock(x25);
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	switch (x25_int->state) {
		case X25_STATE_0:
		case X25_STATE_2:
			x25_disconnect(x25, 0, 0, 0);
			break;

		case X25_STATE_1:
		case X25_STATE_3:
		case X25_STATE_4:
			x25_clear_queues(x25);
			x25->callbacks->debug(1, "[X25] S%d TX CLEAR_REQUEST", x25_int->state);
			_uchar old_state = x25_int->state;
			x25_int->state = X25_STATE_2;
			x25_write_internal(x25, X25_CLEAR_REQUEST);
			x25_start_timer(x25, &x25_int->T23);
			x25->callbacks->debug(1, "[X25] S%d -> S2", old_state);
			break;
	};
//	unlock(x25);

	return 0;
}

int x25_sendmsg(struct x25_cs * x25, char * data, int data_size, _uchar out_of_band, _uchar q_bit_flag) {
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	int data_size_tmp = data_size;
	char * ptr = data;
	int qbit = 0;
	int rc = -X25_INVALUE;

//	lock(x25);

	rc = -X25_NOTCONNECTED;
	if ((x25_int->state != X25_STATE_3) && (x25_int->state != X25_STATE_4))
		goto out;

	/* Sanity check the packet size */
	if (data_size_tmp > 65535) {
		rc = -X25_MSGSIZE;
		goto out;
	};

	if ((out_of_band) && data_size_tmp > 32)
		data_size_tmp = 32;

//	size = len + X25_EXT_MIN_LEN;

	/*
	 *	If the Q BIT Include socket option is in force, the first
	 *	byte of the user data is the logical value of the Q Bit.
	 */
	if (q_bit_flag) {
		qbit = ptr[0];
		ptr++;
		data_size_tmp--;
		//skb_pull(skb, 1);
	};

	rc = -X25_NOMEM;
	if (out_of_band) {
		ptr = cb_queue_tail(&x25_int->interrupt_out_queue, ptr, data_size_tmp, X25_STD_MIN_LEN);
		if (!ptr)
			goto out;

		if (x25->link.extended)
			*ptr++ = ((x25->lci >> 8) & 0x0F) | X25_GFI_EXTSEQ;
		else
			*ptr++ = ((x25->lci >> 8) & 0x0F) | X25_GFI_STDSEQ;
		*ptr++ = (x25->lci >> 0) & 0xFF;
		*ptr++ = X25_INTERRUPT;
		rc = data_size_tmp + X25_STD_MIN_LEN;
	} else
		rc = x25_output(x25, ptr, data_size_tmp, qbit);

	x25_kick(x25);
	//rc = len;
out:
//	unlock(x25);
	return rc;
}


//static int x25_recvmsg(struct socket *sock, struct msghdr *msg, size_t size,
//		       int flags)
//{
//	struct sock *sk = sock->sk;
//	struct x25_sock *x25 = x25_sk(sk);
//	DECLARE_SOCKADDR(struct sockaddr_x25 *, sx25, msg->msg_name);
//	size_t copied;
//	int qbit, header_len;
//	struct sk_buff *skb;
//	unsigned char *asmptr;
//	int rc = -ENOTCONN;

//	lock_sock(sk);

//	if (x25->neighbour == NULL)
//		goto out;

//	header_len = x25->neighbour->extended ?
//		X25_EXT_MIN_LEN : X25_STD_MIN_LEN;

//	/*
//	 * This works for seqpacket too. The receiver has ordered the queue for
//	 * us! We do one quick check first though
//	 */
//	if (sk->sk_state != TCP_ESTABLISHED)
//		goto out;

//	if (flags & MSG_OOB) {
//		rc = -EINVAL;
//		if (sock_flag(sk, SOCK_URGINLINE) ||
//		    !skb_peek(&x25->interrupt_in_queue))
//			goto out;

//		skb = skb_dequeue(&x25->interrupt_in_queue);

//		if (!pskb_may_pull(skb, X25_STD_MIN_LEN))
//			goto out_free_dgram;

//		skb_pull(skb, X25_STD_MIN_LEN);

//		/*
//		 *	No Q bit information on Interrupt data.
//		 */
//		if (test_bit(X25_Q_BIT_FLAG, &x25->flags)) {
//			asmptr  = skb_push(skb, 1);
//			*asmptr = 0x00;
//		}

//		msg->msg_flags |= MSG_OOB;
//	} else {
//		/* Now we can treat all alike */
//		release_sock(sk);
//		skb = skb_recv_datagram(sk, flags & ~MSG_DONTWAIT,
//					flags & MSG_DONTWAIT, &rc);
//		lock_sock(sk);
//		if (!skb)
//			goto out;

//		if (!pskb_may_pull(skb, header_len))
//			goto out_free_dgram;

//		qbit = (skb->data[0] & X25_Q_BIT) == X25_Q_BIT;

//		skb_pull(skb, header_len);

//		if (test_bit(X25_Q_BIT_FLAG, &x25->flags)) {
//			asmptr  = skb_push(skb, 1);
//			*asmptr = qbit;
//		}
//	}

//	skb_reset_transport_header(skb);
//	copied = skb->len;

//	if (copied > size) {
//		copied = size;
//		msg->msg_flags |= MSG_TRUNC;
//	}

//	/* Currently, each datagram always contains a complete record */
//	msg->msg_flags |= MSG_EOR;

//	rc = skb_copy_datagram_msg(skb, 0, msg, copied);
//	if (rc)
//		goto out_free_dgram;

//	if (sx25) {
//		sx25->sx25_family = AF_X25;
//		sx25->sx25_addr   = x25->dest_addr;
//		msg->msg_namelen = sizeof(*sx25);
//	}

//	x25_check_rbuf(sk);
//	rc = copied;
//out_free_dgram:
//	skb_free_datagram(sk, skb);
//out:
//	release_sock(sk);
//	return rc;
//}



