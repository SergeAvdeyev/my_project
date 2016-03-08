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
	int rc = X25_BADTOKEN;

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
		(*x25)->T21.interval = X25_DEFAULT_T21;
		(*x25)->T22.interval = X25_DEFAULT_T22;
		(*x25)->T23.interval = X25_DEFAULT_T23;
		(*x25)->T2.interval  = X25_DEFAULT_T2;
	};

	/* Create timers T201, T202 */
	if ((*x25)->callbacks->add_timer) {
		(*x25)->T21.timer_ptr = (*x25)->callbacks->add_timer((*x25)->T21.interval, *x25, x25_t21timer_expiry);
		(*x25)->T22.timer_ptr = (*x25)->callbacks->add_timer((*x25)->T22.interval, *x25, x25_t22timer_expiry);
		(*x25)->T23.timer_ptr = (*x25)->callbacks->add_timer((*x25)->T23.interval, *x25, x25_t23timer_expiry);
		(*x25)->T2.timer_ptr  = (*x25)->callbacks->add_timer((*x25)->T2.interval,  *x25, x25_t2timer_expiry);
	};
	(*x25)->state = X25_STATE_0;
	(*x25)->cudmatchlength = 0;

	(*x25)->facilities.winsize_in  = X25_DEFAULT_WINDOW_SIZE;
	(*x25)->facilities.winsize_out = X25_DEFAULT_WINDOW_SIZE;
	(*x25)->facilities.pacsize_in  = X25_DEFAULT_PACKET_SIZE;
	(*x25)->facilities.pacsize_out = X25_DEFAULT_PACKET_SIZE;
	(*x25)->facilities.throughput  = 0;	/* by default don't negotiate
										throughput */
	(*x25)->facilities.reverse     = X25_DEFAULT_REVERSE;
	(*x25)->dte_facilities.calling_len = 0;
	(*x25)->dte_facilities.called_len = 0;
	x25_mem_zero((*x25)->dte_facilities.called_ae, sizeof((*x25)->dte_facilities.called_ae));
	x25_mem_zero((*x25)->dte_facilities.calling_ae, sizeof((*x25)->dte_facilities.calling_ae));

	/* Create queues */
	cb_init(&(*x25)->ack_queue, X25_SMODULUS, x25_pacsize_to_bytes((*x25)->facilities.pacsize_out));
	cb_init(&(*x25)->fragment_queue, X25_SMODULUS, x25_pacsize_to_bytes((*x25)->facilities.pacsize_out));
	cb_init(&(*x25)->interrupt_in_queue, X25_SMODULUS, x25_pacsize_to_bytes((*x25)->facilities.pacsize_out));
	cb_init(&(*x25)->interrupt_out_queue, X25_SMODULUS, x25_pacsize_to_bytes((*x25)->facilities.pacsize_out));

	rc = X25_OK;
out:
	return rc;
}

int x25_unregister(struct x25_cs * x25) {
	int rc = X25_BADTOKEN;

	if (!x25)
		goto out;

	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_stop_timer(x25, &x25->T2);
	x25_stop_timer(x25, &x25->T21);
	x25_stop_timer(x25, &x25->T22);
	x25_stop_timer(x25, &x25->T23);

	cb_free(&x25->ack_queue);
	cb_free(&x25->fragment_queue);
	cb_free(&x25->interrupt_in_queue);
	cb_free(&x25->interrupt_out_queue);

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
	(void)x25;
	return 0;
}


void x25_add_link(struct x25_cs *x25, void * link, int extended) {
	x25->neighbour.link_ptr = link;
	x25->neighbour.state    = X25_LINK_STATE_0;
	x25->neighbour.extended = extended == 0 ? 0 : 1;
	cb_init(&x25->neighbour.queue, 10, 1024);
	x25->neighbour.T20.interval = X25_DEFAULT_T20;
	if (x25->callbacks->add_timer)
		x25->neighbour.T20.timer_ptr = x25->callbacks->add_timer(x25->T21.interval, x25, x25_t20timer_expiry);
	x25->neighbour.global_facil_mask =	X25_MASK_REVERSE |
										X25_MASK_THROUGHPUT |
										X25_MASK_PACKET_SIZE |
										X25_MASK_WINDOW_SIZE;
}




//int x25_connect_request(struct x25_cs * x25, struct x25_address *uaddr, int addr_len) {
int x25_connect_request(struct x25_cs * x25, struct x25_address *dest_addr) {
	int rc = X25_BADTOKEN;

	if (!x25)
		goto out;
	lock(x25);

//	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	rc = X25_NOTCONNECTED;
	if (x25->state == X25_STATE_1)
		goto out_unlock;

	rc = X25_CONNECTED;
	if (x25->state == X25_STATE_3)
		goto out_unlock;

//	rc = X25_ROOT_UNREACH;
//	rt = x25_get_route(&addr);
//	if (!rt)
//		goto out_unlock;

//	x25->neighbour = x25_get_neigh(rt->dev);
//	if (!x25->neighbour)
//		goto out_put_route;

	x25_limit_facilities(x25);

//	x25->lci = x25_new_lci(x25->neighbour);
//	if (!x25->lci)
//		goto out_put_neigh;


	if (!strcmp(x25->source_addr.x25_addr, null_x25_address.x25_addr))
		memset(&x25->source_addr, '\0', X25_ADDR_LEN);

	x25->dest_addr = *dest_addr;

	x25->state = X25_STATE_1;

	x25_write_internal(x25, X25_CALL_REQUEST);

	x25_start_heartbeat(x25);
	x25_start_timer(x25, x25->T21.timer_ptr);

//	/* Now the loop */
//	rc = -EINPROGRESS;
//	if (sk->sk_state != TCP_ESTABLISHED && (flags & O_NONBLOCK))
//		goto out_put_neigh;

//	rc = x25_wait_for_connection_establishment(sk);
//	if (rc)
//		goto out_put_neigh;

	rc = 0;
//out_put_neigh:
//	if (rc)
//		x25_neigh_put(x25->neighbour);
//out_put_route:
//	x25_route_put(rt);
out_unlock:
	unlock(x25);
out:
//	release_sock(sk);
	return rc;
}

