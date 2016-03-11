
#include "x25_int.h"

void x25_transmit_link(struct x25_cs *x25, char * data, int data_size) {
	switch (x25->link.state) {
		case X25_LINK_STATE_0:
			cb_queue_tail(&x25->link.queue, data, data_size);
			x25->link.state = X25_LINK_STATE_1;
			/* Exec link callback function link_connect_request method */
			x25->callbacks->link_connect_request(x25->link.link_ptr);
			break;
		case X25_LINK_STATE_1:
		case X25_LINK_STATE_2:
			cb_queue_tail(&x25->link.queue, data, data_size);
			break;
		case X25_LINK_STATE_3:
			/* Send Iframe via lapb link */
			//send_frame(data, data_size);
			x25->callbacks->link_send_frame(x25->link.link_ptr, data, data_size);
			break;
	};
}

/*
 *	Called when the link layer has become established.
 */
void x25_link_established(void *x25_ptr) {
	struct x25_cs * x25 = (struct x25_cs *)x25_ptr;

	if (x25 == NULL) return;

	switch (x25->link.state) {
		case X25_LINK_STATE_0:
			x25->link.state = X25_LINK_STATE_2;
			break;
		case X25_LINK_STATE_1:
			x25_transmit_restart_request(x25);
			x25->link.state = X25_LINK_STATE_2;
			x25_start_timer(x25, x25->link.T20.timer_ptr);
			break;
	};
}

/*
 *	Called when the link layer has terminated, or an establishment
 *	request has failed.
 */
void x25_link_terminated(void *x25_ptr) {
	struct x25_cs * x25 = (struct x25_cs *)x25_ptr;

	if (x25 == NULL) return;

	x25->link.state = X25_LINK_STATE_0;
	/* Out of order: clear existing virtual calls (X.25 03/93 4.6.3) */
	x25_disconnect(x25, X25_REFUSED, 0, 0);
}

int x25_link_receive_data(void *x25_ptr, char * data, int data_size) {
	struct x25_cs * x25 = x25_ptr;
	_uchar frametype;
	_uint lci;

//	x25->callbacks->debug(2,
//						  "[X25] x25_link_receive_data(): %02x %02x %02x %02x %02x",
//						  (_uchar)data[0],
//						  (_uchar)data[1],
//						  (_uchar)data[2],
//						  (_uchar)data[3],
//						  (_uchar)data[4]);
	//if (!pskb_may_pull(skb, X25_STD_MIN_LEN))
	if (data_size < X25_STD_MIN_LEN)
		return 0;


	frametype = (_uchar)data[2];
	lci = ((data[0] << 8) & 0xF00) + ((data[1] << 0) & 0x0FF);

	/*
	 *	LCI of zero is always for us, and its always a link control frame.
	 */
	if (lci == 0) {
		x25_link_control(x25, data, data_size, frametype);
		return 0;
	};

	/* Find an existing socket. */
	if (x25->lci == lci) {
		int queued = 1;

		queued = x25_process_rx_frame(x25, data, data_size);
		return queued;
	};

	/* Is it a Call Request ? if so process it. */
	if (frametype == X25_CALL_REQUEST)
		return x25_rx_call_request(x25, data, data_size, lci);

//	/* Its not a Call Request, nor is it a control frame. Can we forward it? */
//	if (x25_forward_data(lci, nb, skb)) {
//		if (frametype == X25_CLEAR_CONFIRMATION)
//			x25_clear_forward_by_lci(lci);
//		//kfree_skb(skb);
//		return 1;
//	};


	x25_transmit_clear_request(x25, lci, 0x0D);
	if (frametype != X25_CLEAR_CONFIRMATION)
		x25->callbacks->debug(2, "[X25] x25_link_receive_data(): unknown frame type %2x", frametype);

	return 0;
}

/*
 *	This handles all restart and diagnostic frames.
 */
void x25_link_control(struct x25_cs *x25, char * data, int data_size, _uchar frametype) {
	//struct sk_buff *skbn;
	int confirm;

	switch (frametype) {
		case X25_RESTART_REQUEST:
			confirm = !x25->link.T20.state;
			x25->callbacks->stop_timer(x25->link.T20.timer_ptr);
			x25->link.state = X25_LINK_STATE_3;
			if (confirm)
				x25_transmit_restart_confirmation(x25);
			break;

		case X25_RESTART_CONFIRMATION:
			x25->callbacks->stop_timer(x25->link.T20.timer_ptr);
			x25->link.state = X25_LINK_STATE_3;
			break;

		case X25_DIAGNOSTIC:
			if (data_size < 7)
				break;

			x25->callbacks->debug(2, "[X25] diagnostic #%d - %02X %02X %02X", data[3], data[4], data[5], data[6]);
			break;

		default:
			x25->callbacks->debug(2, "[X25] received unknown %02X with LCI 000", frametype);
			break;
	};

	char * buffer;
	int buffer_size;
	if (x25->link.state == X25_LINK_STATE_3)
		while ((buffer = cb_dequeue(&(x25->link.queue), &buffer_size)) != NULL)
			x25->callbacks->link_send_frame(x25->link.link_ptr, buffer, buffer_size);
}

/*
 *	This routine is called when a Restart Request is needed
 */
void x25_transmit_restart_request(struct x25_cs *x25) {
	int data_size = 5;
	char data[data_size];

	data[0] = x25->link.extended ? X25_GFI_EXTSEQ : X25_GFI_STDSEQ;
	data[1] = 0x00;
	data[2] = X25_RESTART_REQUEST;
	data[3] = 0x00;
	data[4] = 0;

	//x25_send_frame(skb, nb);
	x25->callbacks->link_send_frame(x25->link.link_ptr, data, data_size);
	x25->callbacks->debug(2, "[X25] Send restart request");
}


/*
 * This routine is called when a Restart Confirmation is needed
 */
void x25_transmit_restart_confirmation(struct x25_cs *x25) {
	int data_size = 5;
	char data[data_size];

	data[0] = x25->link.extended ? X25_GFI_EXTSEQ : X25_GFI_STDSEQ;
	data[1] = 0x00;
	data[2] = X25_RESTART_CONFIRMATION;

	//x25_send_frame(skb, nb);
	x25->callbacks->link_send_frame(x25->link.link_ptr, data, data_size);
	x25->callbacks->debug(2, "[X25] Send restart confirmation");
}

/*
 *	This routine is called when a Clear Request is needed outside of the context
 *	of a connected socket.
 */
void x25_transmit_clear_request(struct x25_cs *x25, unsigned int lci, unsigned char cause) {
	int data_size = 5;
	char data[data_size];

	data[0] = ((lci >> 8) & 0x0F) | (x25->link.extended ? X25_GFI_EXTSEQ : X25_GFI_STDSEQ);
	data[1] = (lci >> 0) & 0xFF;
	data[2] = X25_CLEAR_REQUEST;
	data[3] = cause;
	data[4] = 0x00;

	//x25_send_frame(skb, nb);
	x25->callbacks->link_send_frame(x25->link.link_ptr, data, data_size);
	x25->callbacks->debug(2, "[X25] Send clear request");
}
