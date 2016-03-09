
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
			break;
	};
}
