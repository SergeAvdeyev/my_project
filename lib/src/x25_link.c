
#include "x25_int.h"

void x25_transmit_link(struct x25_link *nb, char * data, int data_size) {
	switch (nb->state) {
		case X25_LINK_STATE_0:
			cb_queue_tail(&nb->queue, data, data_size);
			nb->state = X25_LINK_STATE_1;
			/* Call lapb_connect_request method */
			//lapb_establish_link;
			break;
		case X25_LINK_STATE_1:
		case X25_LINK_STATE_2:
			cb_queue_tail(&nb->queue, data, data_size);
			break;
		case X25_LINK_STATE_3:
			/* Send Iframe via lapb link */
			//send_frame(data, data_size);
			break;
	};
}
