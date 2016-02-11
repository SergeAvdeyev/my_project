/*
 *	LAPB release 002
 *
 *	This code REQUIRES 2.1.15 or higher/ NET3.038
 *
 *	This module:
 *		This module is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *	History
 *	LAPB 001	Jonathan Naylor	Started Coding
 *	LAPB 002	Jonathan Naylor	New timer architecture.
 *	2000-10-29	Henner Eisen	lapb_data_indication() return status.
 */



#include <stdlib.h>

#include "net_lapb.h"



//static LIST_HEAD(lapb_list);
//static DEFINE_RWLOCK(lapb_list_lock);

///*
// *	Free an allocated lapb control block.
// */
//static void lapb_free_cb(struct lapb_cb *lapb) {
//	kfree(lapb);
//}

//static __inline__ void lapb_hold(struct lapb_cb *lapb) {
//	atomic_inc(&lapb->refcnt);
//}

//static __inline__ void lapb_put(struct lapb_cb *lapb) {
//	if (atomic_dec_and_test(&lapb->refcnt))
//		lapb_free_cb(lapb);
//}

///*
// *	Socket removal during an interrupt is now safe.
// */
//static void __lapb_remove_cb(struct lapb_cb *lapb) {
//	if (lapb->node.next) {
//		list_del(&lapb->node);
//		lapb_put(lapb);
//	};
//}

///*
// *	Add a socket to the bound sockets list.
// */
//static void __lapb_insert_cb(struct lapb_cb *lapb) {
//	list_add(&lapb->node, &lapb_list);
//	lapb_hold(lapb);
//}

//static struct lapb_cb *__lapb_devtostruct(struct net_device *dev) {
//	struct list_head *entry;
//	struct lapb_cb *lapb, *use = NULL;

//	list_for_each(entry, &lapb_list) {
//		lapb = list_entry(entry, struct lapb_cb, node);
//		if (lapb->dev == dev) {
//			use = lapb;
//			break;
//		};
//	};

//	if (use)
//		lapb_hold(use);

//	return use;
//}

//static struct lapb_cb *lapb_devtostruct(struct net_device *dev) {
//	struct lapb_cb *rc;

//	read_lock_bh(&lapb_list_lock);
//	rc = __lapb_devtostruct(dev);
//	read_unlock_bh(&lapb_list_lock);

//	return rc;
//}






/*
 *	Create an empty LAPB control block.
 */
static struct lapb_cb *lapb_create_cb(void) {
	struct lapb_cb *lapb;

	lapb = malloc(sizeof(struct lapb_cb));

	if (!lapb)
		goto out;

//	skb_queue_head_init(&lapb->write_queue);
//	skb_queue_head_init(&lapb->ack_queue);

//	init_timer(&lapb->t1timer);
//	init_timer(&lapb->t2timer);

	//lapb->write_queue = malloc(LAPB_DEFAULT_N1*LAPB_SMODULUS);
	lapb->write_queue = NULL;
	//lapb->ack_queue = malloc(LAPB_DEFAULT_N1*LAPB_SMODULUS);
	lapb->ack_queue = NULL;

	/* Zero variables */
	lapb->N2count = 0;
	lapb->va = 0;
	lapb->vr = 0;
	lapb->vs = 0;
	lapb->T1	= LAPB_DEFAULT_T1;
	lapb->T2	= LAPB_DEFAULT_T2;
	lapb->N1	= LAPB_DEFAULT_N1;
	lapb->N2	= LAPB_DEFAULT_N2;
	lapb->mode	= LAPB_DEFAULT_MODE;
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

int lapb_register(struct lapb_register_struct *callbacks, struct lapb_cb ** out_lapb) {
	int rc = LAPB_BADTOKEN;

	*out_lapb = lapb_create_cb();
	rc = LAPB_NOMEM;
	if (!*out_lapb)
		goto out;

	if (!callbacks->debug)
		callbacks->debug = default_debug;
	(*out_lapb)->callbacks = callbacks;

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

	lapb_clear_queues(lapb);

	free(lapb);
	rc = LAPB_OK;
out:
	return rc;
}

int lapb_reset(struct lapb_cb * lapb, unsigned char init_state) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	lapb_stop_t1timer(lapb);
	lapb_stop_t2timer(lapb);
	lapb_clear_queues(lapb);
	lapb->T1      = LAPB_DEFAULT_T1;
	lapb->T2      = LAPB_DEFAULT_T2;
	lapb->N2      = LAPB_DEFAULT_N2;
	lapb->mode    = LAPB_DEFAULT_MODE;
	lapb->window  = LAPB_DEFAULT_WINDOW;
	unsigned char old_state = lapb->state;
	lapb->state   = init_state;
	lapb->callbacks->debug(lapb, 0, "S%d -> S%d\n", old_state, init_state);

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_getparms(struct lapb_cb *lapb, struct lapb_parms_struct *parms) {
	int rc = LAPB_BADTOKEN;

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
	return rc;
}

int lapb_setparms(struct lapb_cb *lapb, struct lapb_parms_struct *parms) {
	int rc = LAPB_BADTOKEN;

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
	return rc;
}

int lapb_connect_request(struct lapb_cb *lapb) {
	int rc = LAPB_BADTOKEN;

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

	lapb->callbacks->debug(lapb, 0, "S0 -> S1\n");

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_disconnect_request(struct lapb_cb *lapb) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	switch (lapb->state) {
		case LAPB_STATE_0:
			rc = LAPB_NOTCONNECTED;
			goto out;

		case LAPB_STATE_1:
			lapb->callbacks->debug(lapb, 1, "S1 TX DISC(1)\n");
			lapb->callbacks->debug(lapb, 0, "S1 -> S0\n");
			lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
			lapb->state = LAPB_STATE_0;
			lapb_start_t1timer(lapb);
			rc = LAPB_NOTCONNECTED;
			goto out;

		case LAPB_STATE_2:
			rc = LAPB_OK;
			goto out;
	};

	lapb->callbacks->debug(lapb, 1, "S3 DISC(1)\n");
	lapb_clear_queues(lapb);
	lapb->N2count = 0;
	lapb_send_control(lapb, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
	lapb->state = LAPB_STATE_2;
	if ((lapb->mode & LAPB_DCE) == LAPB_DCE)
		lapb_start_t1timer(lapb);
	//lapb_stop_t2timer(lapb);

	lapb->callbacks->debug(lapb, 0, "S3 -> S2\n");

	rc = LAPB_OK;
out:
	return rc;
}

int lapb_data_request(struct lapb_cb *lapb, unsigned char *data, int data_size) {
	int rc = LAPB_BADTOKEN;

	if (!lapb)
		goto out;

	rc = LAPB_NOTCONNECTED;
	if (lapb->state != LAPB_STATE_3 && lapb->state != LAPB_STATE_4)
		goto out;

	//skb_queue_tail(&lapb->write_queue, skb);
	lapb_kick(lapb, data, data_size);
	rc = LAPB_OK;

out:
	return rc;
}

int lapb_data_received(struct lapb_cb *lapb, unsigned char *data, int data_size) {
	int rc = LAPB_BADTOKEN;

	if (lapb) {
		lapb_data_input(lapb, data, data_size);
		rc = LAPB_OK;
	};

	return rc;
}

void lapb_connect_confirmation(struct lapb_cb *lapb, int reason) {
	if (lapb->callbacks->connect_confirmation)
		lapb->callbacks->connect_confirmation(lapb, reason);
}

void lapb_connect_indication(struct lapb_cb *lapb, int reason) {
	if (lapb->callbacks->connect_indication)
		lapb->callbacks->connect_indication(lapb, reason);
}

void lapb_disconnect_confirmation(struct lapb_cb *lapb, int reason) {
	if (lapb->callbacks->disconnect_confirmation)
		lapb->callbacks->disconnect_confirmation(lapb, reason);
}

void lapb_disconnect_indication(struct lapb_cb *lapb, int reason) {
	if (lapb->callbacks->disconnect_indication)
		lapb->callbacks->disconnect_indication(lapb, reason);
}

int lapb_data_indication(struct lapb_cb *lapb, unsigned char * data, int data_size) {
	if (lapb->callbacks->data_indication)
		return lapb->callbacks->data_indication(lapb, data, data_size);

	//free(skb);
	//return NET_RX_SUCCESS; /* For now; must be != NET_RX_DROP */
	return 0;
}

int lapb_data_transmit(struct lapb_cb *lapb, unsigned char *data, int data_size) {
	int used = 0;

	if (lapb->callbacks->data_transmit) {
		lapb->callbacks->data_transmit(lapb, data, data_size);
		used = 1;
	};

	return used;
}

//static int __init lapb_init(void) {
//	return 0;
//}

//static void __exit lapb_exit(void) {
//	WARN_ON(!list_empty(&lapb_list));
//}

