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

	lapb = mem_get(sizeof(struct lapb_cs));

	if (!lapb)
		goto out;

	struct lapb_cs_internal * lapb_int = mem_get(sizeof(struct lapb_cs_internal));
	lapb->internal_struct = lapb_int;

	/* Zero variables */
	lapb_int->RC = 0;
	lapb_int->va = 0;
	lapb_int->vr = 0;
	lapb_int->vs = 0;
	lapb_int->condition = 0;

	lapb_int->T201_state = FALSE;
	lapb_int->T202_state = FALSE;
	lapb_int->state	= LAPB_STATE_0;
out:
	return lapb;
}

void lapb_default_debug(int level, const char * format, ...) {
	(void)level;
	(void)format;
}


int lapb_register(struct lapb_callbacks *callbacks, struct lapb_params * params, struct lapb_cs ** lapb) {
	int rc = LAPB_BADTOKEN;

	*lapb = lapb_create_cs();
	rc = LAPB_NOMEM;
	if (!*lapb)
		goto out;

	(*lapb)->L3_ptr = NULL;
	struct lapb_cs_internal * lapb_int = lapb_get_internal(*lapb);

	if (!callbacks->debug)
		callbacks->debug = lapb_default_debug;
	(*lapb)->callbacks = callbacks;

	if (params)
		lapb_set_params(*lapb, params);
	else {
		(*lapb)->mode	= LAPB_DEFAULT_SMODE;
		(*lapb)->window	= LAPB_DEFAULT_SWINDOW;
		(*lapb)->N1		= LAPB_DEFAULT_N1;
		(*lapb)->N201	= LAPB_DEFAULT_N202;
		(*lapb)->T201_interval = LAPB_DEFAULT_T201;
		(*lapb)->T202_interval = LAPB_DEFAULT_T202;
		(*lapb)->low_order_bits = FALSE;
		(*lapb)->auto_connecting = FALSE;
	};

	/* Create write and ack queues */
	if (lapb_is_extended(*lapb)) {
		cb_init(&lapb_int->write_queue, LAPB_EMODULUS, (*lapb)->N1);
		cb_init(&lapb_int->ack_queue, LAPB_EMODULUS, (*lapb)->N1);
	} else {
		cb_init(&lapb_int->write_queue, LAPB_SMODULUS, (*lapb)->N1);
		cb_init(&lapb_int->ack_queue, LAPB_SMODULUS, (*lapb)->N1);
	};

	/* Create timers T201, T202 */
	if ((*lapb)->callbacks->add_timer) {
		lapb_int->T201_timer_ptr = (*lapb)->callbacks->add_timer((*lapb)->T201_interval, *lapb, lapb_t201timer_expiry);
		lapb_int->T202_timer_ptr = (*lapb)->callbacks->add_timer((*lapb)->T202_interval, *lapb, lapb_t202timer_expiry);
	};

	/* Fill invert table */
	fill_inv_table();
	if (lapb_is_dce(*lapb) && (*lapb)->auto_connecting)
		lapb_start_t201timer((*lapb));

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_unregister(struct lapb_cs * lapb) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	lapb_stop_t201timer(lapb);
	lapb_stop_t202timer(lapb);

	lapb->callbacks->del_timer(lapb_int->T201_timer_ptr);
	lapb->callbacks->del_timer(lapb_int->T202_timer_ptr);

	cb_free(&lapb_int->write_queue);
	cb_free(&lapb_int->ack_queue);

	mem_free(lapb->internal_struct);
	mem_free(lapb);
	rc = LAPB_OK;
out:
	return rc;
}

int lapb_get_params(struct lapb_cs * lapb, struct lapb_params * params) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	params->N201			= lapb->N201;
	params->T201_interval	= lapb->T201_interval;
	params->T202_interval	= lapb->T202_interval;
	params->window			= lapb->window;
	params->N1				= lapb->N1;
	params->mode			= lapb->mode;
	params->low_order_bits	= lapb->low_order_bits;
	params->auto_connecting	= lapb->auto_connecting;

	rc = LAPB_OK;
out:
	return rc;
}

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
	if (params->N201 < 1 || params->N201 > 20) /* Value must be between 1 and 20 retries */
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

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (lapb_int->state == LAPB_STATE_0) {
		lapb->mode    = params->mode;
		lapb->window  = params->window;
	};

	lapb->T201_interval = params->T201_interval;
	lapb->T202_interval = params->T202_interval;

	lapb->N1 = params->N1;
	lapb->N201 = params->N201;
	lapb->low_order_bits = params->low_order_bits;
	lapb->auto_connecting = params->auto_connecting;

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_get_state(struct lapb_cs *lapb) {
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);
	return lapb_int->state;
}

char * lapb_dequeue(struct lapb_cs * lapb, int * buffer_size) {
	if (!lapb) return NULL;

	char *result = NULL;
	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	if (cb_peek(&lapb_int->ack_queue))
		result = cb_dequeue(&lapb_int->ack_queue, buffer_size);
	else if (cb_peek(&lapb_int->write_queue))
		result = cb_dequeue(&lapb_int->write_queue, buffer_size);

	return result;
}

int lapb_reset(struct lapb_cs * lapb, _uchar init_state) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	lapb_stop_t201timer(lapb);
	lapb_stop_t202timer(lapb);
	lapb_clear_queues(lapb);
	/* Zero variables */
	lapb_int->RC = 0;
	lapb_int->va = 0;
	lapb_int->vr = 0;
	lapb_int->vs = 0;
	lapb_int->condition = 0x00;
	_uchar old_state = lapb_int->state;
	lapb_int->state   = init_state;
	if ((lapb_is_dce(lapb)) && (init_state == LAPB_STATE_0))
		lapb_start_t201timer(lapb);

	lapb->callbacks->debug(1, "[LAPB] S%d -> S%d", old_state, init_state);
	if (((old_state == LAPB_STATE_3) || (old_state == LAPB_STATE_4)) && (init_state == LAPB_STATE_0))
		lapb->callbacks->disconnect_indication(lapb, LAPB_REFUSED);

	rc = LAPB_OK;
out:
	return rc;
}



//int lapb_connect_request(struct lapb_cs *lapb) {
int lapb_connect_request(void *lapb_ptr) {
	struct lapb_cs *lapb = lapb_ptr;
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	rc = LAPB_OK;
	if (lapb_int->state == LAPB_STATE_1)
		goto out;

	rc = LAPB_CONNECTED;
	//if (lapb->state == LAPB_STATE_3 || lapb->state == LAPB_STATE_4)
	if (lapb_int->state == LAPB_STATE_3)
		goto out;

	lapb_int->extra_before = TRUE;
	lapb_establish_data_link(lapb);
	lapb_int->state = LAPB_STATE_1;
	lapb->auto_connecting = TRUE;

	lapb->callbacks->debug(1, "[LAPB] S0 -> S1");

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_disconnect_request(void *lapb_ptr) {
	struct lapb_cs *lapb = lapb_ptr;
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	switch (lapb_int->state) {
		case LAPB_STATE_0:
			rc = LAPB_NOTCONNECTED;
			goto out;

		case LAPB_STATE_1:
			lapb->callbacks->debug(2, "[LAPB] S1 TX DISC(1)");
			lapb->callbacks->debug(1, "[LAPB] S1 -> S2");
			lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
			lapb_int->state = LAPB_STATE_2;
			lapb_start_t201timer(lapb);
			rc = LAPB_NOTCONNECTED;
			goto out;

		case LAPB_STATE_2:
			rc = LAPB_OK;
			goto out;
	};

	lapb->callbacks->debug(1, "[LAPB] S3 DISC(1)");
	lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
	lapb_int->state = LAPB_STATE_2;
	lapb_int->RC = 0;
	lapb_start_t201timer(lapb);
	lapb_stop_t202timer(lapb);
	lapb->auto_connecting = FALSE;

	lapb->callbacks->debug(1, "[LAPB] S3 -> S2");

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_data_request(void *lapb_ptr, char *data, int data_size) {
	struct lapb_cs *lapb = lapb_ptr;
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);

	rc = LAPB_NOTCONNECTED;
	if (lapb_int->state != LAPB_STATE_3 && lapb_int->state != LAPB_STATE_4)
	//if (lapb_int->state != LAPB_STATE_3)
		goto out;


	rc = LAPB_BUSY;
	if (lapb_int->condition == LAPB_FRMR_CONDITION)
		goto out;
	/* check the filling of the window */
	int actual_window_size;
	if (lapb_int->vs >= lapb_int->va)
		actual_window_size = lapb_int->vs - lapb_int->va;
	else {
		if (lapb_is_extended(lapb))
			actual_window_size = lapb_int->vs + LAPB_EMODULUS - lapb_int->va;
		else
			actual_window_size = lapb_int->vs + LAPB_SMODULUS - lapb_int->va;
	};
	if (actual_window_size >= lapb->window)
		goto out;

	cb_queue_tail(&lapb_int->write_queue, data, data_size, 0);
	lapb_kick(lapb);
	rc = LAPB_OK;

out:
	return rc;
}

int lapb_data_received(struct lapb_cs *lapb, char *data, int data_size, _ushort fcs) {
	int rc = LAPB_BADFCS;

	if (fcs != 0) /* Drop frames with bad summ */
		goto out;

	rc = LAPB_BADTOKEN;
	if (lapb != NULL) {
		lapb_data_input(lapb, data, data_size);
		rc = LAPB_OK;
	};
out:
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
		struct lapb_cs_internal * lapb_int = lapb_get_internal(lapb);
		lapb->callbacks->transmit_data(lapb, data, data_size, lapb_int->extra_before, lapb_int->extra_after);
		lapb_int->extra_before = 0;
		lapb_int->extra_after = 0;
		used = 1;
	};

	return used;
}


