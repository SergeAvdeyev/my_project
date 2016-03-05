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
 *	Create an empty LAPB control block.
 */
struct lapb_cs *lapb_create_cs(void) {
	struct lapb_cs *lapb;

	lapb = malloc(sizeof(struct lapb_cs));

	if (!lapb)
		goto out;

	/* Zero variables */
	lapb->N2count = 0;
	lapb->va = 0;
	lapb->vr = 0;
	lapb->vs = 0;
	lapb->condition = 0;

	lapb->T201_state = FALSE;
	lapb->T202_state = FALSE;
	lapb->state	= LAPB_STATE_0;
out:
	return lapb;
}

void lapb_default_debug(struct lapb_cs *lapb, int level, const char * format, ...) {
	(void)lapb;
	(void)level;
	(void)format;
}

int lapb_register(struct lapb_callbacks *callbacks, struct lapb_params * params, struct lapb_cs ** lapb) {
	int rc = LAPB_BADTOKEN;

	*lapb = lapb_create_cs();
	rc = LAPB_NOMEM;
	if (!*lapb)
		goto out;

	if (!callbacks->debug)
		callbacks->debug = lapb_default_debug;
	(*lapb)->callbacks = callbacks;

#if INTERNAL_SYNC
	/* Init mutex for sinchronization */
	pthread_mutex_init(&((*lapb)->_mutex), NULL);
#endif

	if (params)
		lapb_set_params(*lapb, params);
	else {
		(*lapb)->mode	= LAPB_DEFAULT_SMODE;
		(*lapb)->window	= LAPB_DEFAULT_SWINDOW;
		(*lapb)->N1		= LAPB_DEFAULT_N1;
		(*lapb)->N2		= LAPB_DEFAULT_N2;
		(*lapb)->T201_interval = LAPB_DEFAULT_T201;
		(*lapb)->T202_interval = LAPB_DEFAULT_T202;
		(*lapb)->low_order_bits = FALSE;
		(*lapb)->auto_connecting = FALSE;
	};

	/* Create write and ack queues */
	if (is_extended(*lapb)) {
		cb_init(&(*lapb)->write_queue, LAPB_EMODULUS, (*lapb)->N1);
		cb_init(&(*lapb)->ack_queue, LAPB_EMODULUS, (*lapb)->N1);
	} else {
		cb_init(&(*lapb)->write_queue, LAPB_SMODULUS, (*lapb)->N1);
		cb_init(&(*lapb)->ack_queue, LAPB_SMODULUS, (*lapb)->N1);
	};

	/* Create timers T201, T202 */
	if ((*lapb)->callbacks->add_timer) {
		(*lapb)->T201_timer_ptr = (*lapb)->callbacks->add_timer((*lapb)->T201_interval, *lapb, lapb_t201timer_expiry);
		(*lapb)->T202_timer_ptr = (*lapb)->callbacks->add_timer((*lapb)->T202_interval, *lapb, lapb_t202timer_expiry);
	};

	/* Fill invert table */
	fill_inv_table();
	if (is_dce(*lapb))
		lapb_start_t201timer((*lapb));

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_unregister(struct lapb_cs * lapb) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	lapb_stop_t201timer(lapb);
	lapb_stop_t202timer(lapb);

	cb_free(&lapb->write_queue);
	cb_free(&lapb->ack_queue);

	free(lapb);
	rc = LAPB_OK;
out:
	return rc;
}

//int lapb_getparms(struct net_device *dev, struct lapb_parms_struct *parms) {
//	int rc = LAPB_BADTOKEN;
//	struct lapb_cb *lapb = lapb_devtostruct(dev);

//	if (!lapb)
//		goto out;

//	parms->t1      = lapb->t1 / HZ;
//	parms->t2      = lapb->t2 / HZ;
//	parms->n2      = lapb->n2;
//	parms->n2count = lapb->n2count;
//	parms->state   = lapb->state;
//	parms->window  = lapb->window;
//	parms->mode    = lapb->mode;

//	if (!timer_pending(&lapb->t1timer))
//		parms->t1timer = 0;
//	else
//		parms->t1timer = (lapb->t1timer.expires - jiffies) / HZ;

//	if (!timer_pending(&lapb->t2timer))
//		parms->t2timer = 0;
//	else
//		parms->t2timer = (lapb->t2timer.expires - jiffies) / HZ;

//	lapb_put(lapb);
//	rc = LAPB_OK;
//out:
//	return rc;
//}

int lapb_set_params(struct lapb_cs * lapb, struct lapb_params *params) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	rc = LAPB_INVALUE;
	if (params->T201_interval < 10 ||
		params->T201_interval > 20000) /* Value must be between 10ms and 20s */
		goto out;
	if (params->T202_interval < 10 ||
		params->T202_interval > 20000) /* Value must be between 10ms and 20s */
		goto out;
	if (params->N2 < 1 || params->N2 > 20) /* Value must be between 1 and 20 retries */
		goto out;
	if (params->N1 < 10 || params->N1 > 2048) /* Value must be between 10 and 2048 bytes */
		goto out;
	if (params->mode & LAPB_EXTENDED) {
		if (params->window < 1 || params->window > 127) /* Window must be between 1 and 127 frames */
			goto out;
	} else {
		if (params->window < 1 || params->window > 7) /* Window must be between 1 and 7 frames */
			goto out;
	};

	if (lapb->state == LAPB_STATE_0) {
		lapb->mode    = params->mode;
		lapb->window  = params->window;
	};

	lapb->T201_interval = params->T201_interval;
	lapb->T202_interval = params->T202_interval;

	lapb->N1 = params->N1;
	lapb->N2 = params->N2;
	lapb->low_order_bits = params->low_order_bits;
	lapb->auto_connecting = params->auto_connecting;

	rc = LAPB_OK;
out:
	return rc;
}


char * lapb_dequeue(struct lapb_cs * lapb, int * buffer_size) {
	char *result = NULL;

	if (!lapb) return NULL;

	//lock(lapb);
	if (cb_peek(&lapb->ack_queue))
		result = cb_dequeue(&lapb->ack_queue, buffer_size);
	else if (cb_peek(&lapb->write_queue))
		result = cb_dequeue(&lapb->write_queue, buffer_size);

	//unlock(lapb);
	return result;
}

int lapb_reset(struct lapb_cs * lapb, unsigned char init_state) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	lapb_stop_t201timer(lapb);
	lapb_stop_t202timer(lapb);
	lapb_clear_queues(lapb);
	/* Zero variables */
	lapb->N2count = 0;
	lapb->va = 0;
	lapb->vr = 0;
	lapb->vs = 0;
	lapb->condition = 0x00;
	unsigned char old_state = lapb->state;
	lapb->state   = init_state;
	if ((is_dce(lapb)) && (init_state == LAPB_STATE_0))
		lapb_start_t201timer(lapb);

	lapb->callbacks->debug(lapb, 0, "[LAPB] S%d -> S%d", old_state, init_state);

	rc = LAPB_OK;
out:
	return rc;
}



int lapb_connect_request(struct lapb_cs *lapb) {
	int rc = LAPB_BADTOKEN;

	lock(lapb);
	if (!lapb)
		goto out;

	rc = LAPB_OK;
	if (lapb->state == LAPB_STATE_1)
		goto out;

	rc = LAPB_CONNECTED;
	//if (lapb->state == LAPB_STATE_3 || lapb->state == LAPB_STATE_4)
	if (lapb->state == LAPB_STATE_3)
		goto out;

	lapb_establish_data_link(lapb);
	lapb->state = LAPB_STATE_1;

	lapb->callbacks->debug(lapb, 0, "[LAPB] S0 -> S1");

	rc = LAPB_OK;
out:
	unlock(lapb);
	return rc;
}

int lapb_disconnect_request(struct lapb_cs *lapb) {
	int rc = LAPB_BADTOKEN;

	lock(lapb);
	if (!lapb)
		goto out;

	switch (lapb->state) {
		case LAPB_STATE_0:
			rc = LAPB_NOTCONNECTED;
			goto out;

		case LAPB_STATE_1:
			lapb->callbacks->debug(lapb, 1, "[LAPB] S1 TX DISC(1)");
			lapb->callbacks->debug(lapb, 0, "[LAPB] S1 -> S2");
			lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
			lapb->state = LAPB_STATE_2;
			lapb_start_t201timer(lapb);
			rc = LAPB_NOTCONNECTED;
			goto out;

		case LAPB_STATE_2:
			rc = LAPB_OK;
			goto out;
	};

	lapb->callbacks->debug(lapb, 1, "[LAPB] S3 DISC(1)");
	lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
	lapb->state = LAPB_STATE_2;
	lapb->N2count = 0;
	lapb_start_t201timer(lapb);
	lapb_stop_t202timer(lapb);

	lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S2");

	rc = LAPB_OK;
out:
	unlock(lapb);
	return rc;
}

int lapb_data_request(struct lapb_cs *lapb, char *data, int data_size) {
	int rc = LAPB_BADTOKEN;

	lock(lapb);
	if (!lapb)
		goto out;

	rc = LAPB_NOTCONNECTED;
	//if (lapb->state != LAPB_STATE_3 && lapb->state != LAPB_STATE_4)
	if (lapb->state != LAPB_STATE_3)
		goto out;


	rc = LAPB_BUSY;
	if (lapb->condition == LAPB_FRMR_CONDITION)
		goto out;
	/* check the filling of the window */
	int actual_window_size;
	if (lapb->vs >= lapb->va)
		actual_window_size = lapb->vs - lapb->va;
	else {
		if (is_extended(lapb))
			actual_window_size = lapb->vs + LAPB_EMODULUS - lapb->va;
		else
			actual_window_size = lapb->vs + LAPB_SMODULUS - lapb->va;
	};
	if (actual_window_size >= lapb->window)
		goto out;

	cb_queue_tail(&lapb->write_queue, data, data_size);
//	if (!cb_queue_tail(&lapb->write_queue, data, data_size))
//		goto out;
	lapb_kick(lapb);
	rc = LAPB_OK;

out:
	unlock(lapb);
	return rc;
}

int lapb_data_received(struct lapb_cs *lapb, char *data, int data_size, _ushort fcs) {
	int rc = LAPB_BADFCS;

	lock(lapb);
	if (fcs != 0) /* Drop frames with bad summ */
		goto out;

	rc = LAPB_BADTOKEN;
	if (lapb != NULL) {
		lapb_data_input(lapb, data, data_size);
		rc = LAPB_OK;
	};
out:
	unlock(lapb);
	return rc;
}


/* Callback functions */
void lapb_connect_confirmation(struct lapb_cs *lapb, int reason) {
	if (lapb->callbacks->connect_confirmation)
		lapb->callbacks->connect_confirmation(lapb, reason);
}

void lapb_connect_indication(struct lapb_cs *lapb, int reason) {
	if (lapb->callbacks->connect_indication)
		lapb->callbacks->connect_indication(lapb, reason);
}

void lapb_disconnect_confirmation(struct lapb_cs *lapb, int reason) {
	if (lapb->callbacks->disconnect_confirmation)
		lapb->callbacks->disconnect_confirmation(lapb, reason);
}

void lapb_disconnect_indication(struct lapb_cs *lapb, int reason) {
	if (lapb->callbacks->disconnect_indication)
		lapb->callbacks->disconnect_indication(lapb, reason);
}

int lapb_data_indication(struct lapb_cs *lapb, char * data, int data_size) {
	if (lapb->callbacks->data_indication)
		return lapb->callbacks->data_indication(lapb, data, data_size);

	return 0;
}

int lapb_data_transmit(struct lapb_cs *lapb, char *data, int data_size) {
	int used = 0;

	if (lapb->callbacks->transmit_data) {
		lapb->callbacks->transmit_data(lapb, data, data_size);
		used = 1;
	};

	return used;
}


