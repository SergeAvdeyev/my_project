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

/*
 *	Create an empty LAPB control block.
 */
struct lapb_cs *x25_create_cs(void) {
	return NULL;
}

void x25_default_debug(int level, const char * format, ...) {
	(void)level;
	(void)format;
}


int x25_register(struct x25_callbacks *callbacks, struct x25_params * params, struct x25_cs ** x25) {
	(void)callbacks;
	(void)params;
	(void)x25;
	return 0;
}

int x25_unregister(struct x25_cs * x25) {
	(void)x25;
	return 0;
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

char * x25_dequeue(struct x25_cs * x25, int * buffer_size) {
	(void)x25;
	(void)buffer_size;
	return NULL;
}

int x25_reset(struct x25_cs * x25, unsigned char init_state) {
	(void)x25;
	(void)init_state;
	return 0;
}



int x25_connect_request(struct x25_cs *x25) {
	(void)x25;
	return 0;
}

int x25_disconnect_request(struct x25_cs *x25) {
	(void)x25;
	return 0;
}

//int x25_data_request(struct lapb_cs *x25, char *data, int data_size) {
//	(void)lapb;
//	(void)data;
//	(void)data_size;
//	return 0;
//}

//int x25_data_received(struct lapb_cs *lapb, char *data, int data_size, _ushort fcs) {
//	(void)lapb;
//	(void)data;
//	(void)data_size;
//	(void)fcs;
//	return 0;
//}


/* Callback functions */
//void lapb_connect_confirmation(struct lapb_cs *lapb, int reason) {
//	if (lapb->callbacks->connect_confirmation)
//		lapb->callbacks->connect_confirmation(lapb, reason);
//}

//void lapb_connect_indication(struct lapb_cs *lapb, int reason) {
//	if (lapb->callbacks->connect_indication)
//		lapb->callbacks->connect_indication(lapb, reason);
//}

//void lapb_disconnect_confirmation(struct lapb_cs *lapb, int reason) {
//	if (lapb->callbacks->disconnect_confirmation)
//		lapb->callbacks->disconnect_confirmation(lapb, reason);
//}

//void lapb_disconnect_indication(struct lapb_cs *lapb, int reason) {
//	if (lapb->callbacks->disconnect_indication)
//		lapb->callbacks->disconnect_indication(lapb, reason);
//}

//int lapb_data_indication(struct lapb_cs *lapb, char * data, int data_size) {
//	if (lapb->callbacks->data_indication)
//		return lapb->callbacks->data_indication(lapb, data, data_size);

//	return 0;
//}

//int lapb_data_transmit(struct lapb_cs *lapb, char *data, int data_size) {
//	int used = 0;

//	if (lapb->callbacks->transmit_data) {
//		lapb->callbacks->transmit_data(lapb, data, data_size);
//		used = 1;
//	};

//	return used;
//}


