/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  Started Coding
 *
 *	This module:
 *		This module is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */


#include "lapb_int.h"


/*
 *	Create an empty LAPB control block.
 */
static struct lapb_cb *lapb_create_cb(void) {
	struct lapb_cb *lapb;

	lapb = malloc(sizeof(struct lapb_cb));

	if (!lapb)
		goto out;

	//cb_init(&lapb->write_queue, LAPB_DEFAULT_WINDOW, LAPB_DEFAULT_N1);
	//cb_init(&lapb->ack_queue, LAPB_DEFAULT_WINDOW, LAPB_DEFAULT_N1);
	//lapb->write_queue.buffer = NULL;
	//lapb->ack_queue.buffer = NULL;

	/* Zero variables */
	lapb->N2count = 0;
	lapb->va = 0;
	lapb->vr = 0;
	lapb->vs = 0;
	lapb->T1	= LAPB_DEFAULT_T1;
	lapb->T2	= LAPB_DEFAULT_T2;
	lapb->T1_state = FALSE;
	lapb->T2_state = FALSE;
	lapb->N1	= LAPB_DEFAULT_N1;
	lapb->N2	= LAPB_DEFAULT_N2;
	//lapb->mode	= LAPB_DEFAULT_MODE;
	lapb->window = LAPB_DEFAULT_WINDOW;
	lapb->state	= LAPB_NOT_READY;
out:
	return lapb;
}

void default_debug(struct lapb_cb *lapb, int level, const char * format, ...) {
	(void)lapb;
	(void)level;
	(void)format;
}

extern int lapb_register(struct lapb_register_struct *callbacks,
						 _uchar modulo,
						 _uchar protocol,
						 _uchar equipment,
						 struct lapb_cb ** lapb) {
	int rc = LAPB_BADTOKEN;

	*lapb = lapb_create_cb();
	rc = LAPB_NOMEM;
	if (!*lapb)
		goto out;

	if (!callbacks->debug)
		callbacks->debug = default_debug;
	(*lapb)->callbacks = callbacks;

	/* Init mutex for sinchronization */
	pthread_mutex_init(&((*lapb)->_mutex), NULL);

	(*lapb)->mode = modulo | protocol | equipment;
	/* Create write and ack queues */
	if (modulo == LAPB_STANDARD) {
		cb_init(&(*lapb)->write_queue, LAPB_SMODULUS, LAPB_DEFAULT_N1);
		cb_init(&(*lapb)->ack_queue, LAPB_SMODULUS, LAPB_DEFAULT_N1);
	} else {
		cb_init(&(*lapb)->write_queue, LAPB_EMODULUS, LAPB_DEFAULT_N1);
		cb_init(&(*lapb)->ack_queue, LAPB_EMODULUS, LAPB_DEFAULT_N1);
	};

	lapb_start_t2timer(*lapb);
	rc = LAPB_OK;
out:
	return rc;
}

int lapb_unregister(struct lapb_cb * lapb) {
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

char * lapb_dequeue(struct lapb_cb * lapb, int * buffer_size) {
	char *result = NULL;

	lock(lapb);
	if (cb_peek(&lapb->ack_queue))
		result = cb_dequeue(&lapb->write_queue, buffer_size);
	else if (cb_peek(&lapb->write_queue))
		result = cb_dequeue(&lapb->write_queue, buffer_size);

	unlock(lapb);
	return result;
}

int lapb_reset(struct lapb_cb * lapb, unsigned char init_state) {
	int rc = LAPB_BADTOKEN;

	//lock(lapb);
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
	//unlock(lapb);
	return rc;
}

int lapb_getparms(struct lapb_cb *lapb, struct lapb_parms_struct *parms) {
	int rc = LAPB_BADTOKEN;

	lock(lapb);
	if (!lapb)
		goto out;

	parms->T1      = lapb->T1;
	parms->T2      = lapb->T2;
	parms->N1      = lapb->N1;
	parms->N2      = lapb->N2;
	parms->N2count = lapb->N2count;
	parms->state   = lapb->state;
	parms->window  = lapb->window;
	parms->mode    = lapb->mode;

//	if (!timer_pending(&lapb->t1timer))
//		parms->t1timer = 0;
//	else
//		parms->t1timer = (lapb->t1timer.expires - jiffies) / HZ;

//	if (!timer_pending(&lapb->t2timer))
//		parms->t2timer = 0;
//	else
//		parms->t2timer = (lapb->t2timer.expires - jiffies) / HZ;

	rc = LAPB_OK;
out:
	unlock(lapb);
	return rc;
}

int lapb_setparms(struct lapb_cb *lapb, struct lapb_parms_struct *parms) {
	int rc = LAPB_BADTOKEN;

	lock(lapb);
	if (!lapb)
		goto out;

	rc = LAPB_INVALUE;
	if (parms->T1 < 1 || parms->T2 < 1 || parms->N2 < 1)
		goto out;

	if (lapb->state == LAPB_STATE_0) {
		if (parms->mode & LAPB_EXTENDED) {
			if (parms->window < 1 || parms->window > 127)
				goto out;
		} else {
			if (parms->window < 1 || parms->window > 7)
				goto out;
		};
		lapb->mode    = parms->mode;
		lapb->window  = parms->window;
	};

	lapb->T1    = parms->T1;
	lapb->T2    = parms->T2;
	lapb->N2    = parms->N2;
	lapb->N1    = parms->N1;

	rc = LAPB_OK;
out:
	unlock(lapb);
	return rc;
}

int lapb_connect_request(struct lapb_cb *lapb) {
	int rc = LAPB_BADTOKEN;

	lock(lapb);
	if (!lapb)
		goto out;

	rc = LAPB_OK;
	if (lapb->state == LAPB_STATE_1)
		goto out;

	rc = LAPB_CONNECTED;
	if (lapb->state == LAPB_STATE_3 || lapb->state == LAPB_STATE_4)
		goto out;

	lapb_establish_data_link(lapb);
	lapb->state = LAPB_STATE_1;

	lapb->callbacks->debug(lapb, 0, "[LAPB] S0 -> S1");

	rc = LAPB_OK;
out:
	unlock(lapb);
	return rc;
}

int lapb_disconnect_request(struct lapb_cb *lapb) {
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

	lapb->callbacks->debug(lapb, 0, "[LAPB] S3 -> S2");

	rc = LAPB_OK;
out:
	unlock(lapb);
	return rc;
}

int lapb_data_request(struct lapb_cb *lapb, char *data, int data_size) {
	int rc = LAPB_BADTOKEN;

	lock(lapb);
	if (!lapb)
		goto out;

	rc = LAPB_NOTCONNECTED;
	if (lapb->state != LAPB_STATE_3 && lapb->state != LAPB_STATE_4)
		goto out;


	rc = LAPB_NOMEM;
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

int lapb_data_received(struct lapb_cb *lapb, char *data, int data_size, _ushort fcs) {
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

void lapb_connect_indication(struct lapb_cb *lapb, int reason) {
	if (lapb->callbacks->on_connected)
		lapb->callbacks->on_connected(lapb, reason);
}

void lapb_disconnect_indication(struct lapb_cb *lapb, int reason) {
	if (lapb->callbacks->on_disconnected)
		lapb->callbacks->on_disconnected(lapb, reason);
}

int lapb_data_indication(struct lapb_cb *lapb, char * data, int data_size) {
	if (lapb->callbacks->on_new_data)
		return lapb->callbacks->on_new_data(lapb, data, data_size);

	return 0;
}

int lapb_data_transmit(struct lapb_cb *lapb, char *data, int data_size) {
	int used = 0;

	if (lapb->callbacks->transmit_data) {
		lapb->callbacks->transmit_data(lapb, data, data_size);
		used = 1;
	};

	return used;
}


