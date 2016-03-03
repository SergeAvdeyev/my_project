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
	lapb->T1	= LAPB_DEFAULT_T1;
	lapb->T2	= LAPB_DEFAULT_T2;
	lapb->T1_state = FALSE;
	lapb->T2_state = FALSE;
	lapb->N1	= LAPB_DEFAULT_N1;
	lapb->N2	= LAPB_DEFAULT_N2;
	lapb->mode	= LAPB_DEFAULT_SMODE;
	lapb->window = LAPB_DEFAULT_SWINDOW;
	lapb->state	= LAPB_STATE_0;
	lapb->low_order_bits = FALSE;
out:
	return lapb;
}

void lapb_default_debug(struct lapb_cs *lapb, int level, const char * format, ...) {
	(void)lapb;
	(void)level;
	(void)format;
}

int lapb_register(struct lapb_callbacks *callbacks,
						 _uchar modulo,
						 _uchar protocol,
						 _uchar equipment,
						 struct lapb_cs ** lapb) {
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

	(*lapb)->mode = modulo | protocol | equipment;
	/* Create write and ack queues */
	if (modulo == LAPB_STANDARD) {
		cb_init(&(*lapb)->write_queue, LAPB_SMODULUS, LAPB_DEFAULT_N1);
		cb_init(&(*lapb)->ack_queue, LAPB_SMODULUS, LAPB_DEFAULT_N1);
	} else {
		(*lapb)->window = LAPB_DEFAULT_EWINDOW;
		cb_init(&(*lapb)->write_queue, LAPB_EMODULUS, LAPB_DEFAULT_N1);
		cb_init(&(*lapb)->ack_queue, LAPB_EMODULUS, LAPB_DEFAULT_N1);
	};

	/* Fill invert table */
	fill_inv_table();

	//lapb_start_t2timer(*lapb);
	rc = LAPB_OK;
out:
	return rc;
}

int lapb_unregister(struct lapb_cs * lapb) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	lapb_stop_t1timer(lapb);
	lapb_stop_t2timer(lapb);

	cb_free(&lapb->write_queue);
	cb_free(&lapb->ack_queue);

	free(lapb);
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

	lapb_stop_t1timer(lapb);
	lapb_stop_t2timer(lapb);
	lapb_clear_queues(lapb);
	/* Zero variables */
	lapb->N2count = 0;
	lapb->va = 0;
	lapb->vr = 0;
	lapb->vs = 0;
	lapb->condition = 0x00;
	unsigned char old_state = lapb->state;
	lapb->state   = init_state;
	if ((lapb->mode & LAPB_DCE) && (init_state == LAPB_STATE_0))
		lapb_start_t1timer(lapb);

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
			lapb_start_t1timer(lapb);
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
	lapb_start_t1timer(lapb);
	lapb_stop_t2timer(lapb);

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
		if (lapb->mode & LAPB_EXTENDED)
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


