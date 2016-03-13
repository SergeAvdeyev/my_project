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

void x25_start_heartbeat(struct x25_cs *x25) {
	(void)x25;
//	mod_timer(&sk->sk_timer, jiffies + 5 * HZ);
}

void x25_stop_heartbeat(struct x25_cs *x25) {
	(void)x25;
//	del_timer(&sk->sk_timer);
}

void x25_start_timer(struct x25_cs *x25, struct x25_timer * _timer) {
	if (!x25->callbacks->start_timer) return;
	if (_timer->state) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25->callbacks->start_timer(_timer->timer_ptr);
	_timer->state = TRUE;
	if (_timer == &x25_int->T20)
		x25->callbacks->debug(0, "[X25] start t20_timer");
	else if (_timer == &x25_int->T21)
		x25->callbacks->debug(0, "[X25] start t21_timer");
	else if (_timer == &x25_int->T22)
		x25->callbacks->debug(0, "[X25] start t22_timer");
	else if (_timer == &x25_int->T23)
		x25->callbacks->debug(0, "[X25] start t23_timer");
	else if (_timer == &x25->link.T2)
		x25->callbacks->debug(0, "[X25_LINK] start link_timer");
}

void x25_stop_timer(struct x25_cs *x25, struct x25_timer * _timer) {
	if (!x25->callbacks->stop_timer) return;
	if (!_timer->state) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25->callbacks->stop_timer(_timer->timer_ptr);
	_timer->state = FALSE;
	if (_timer == &x25_int->T20)
		x25->callbacks->debug(0, "[X25] stop t20_timer");
	else if (_timer == &x25_int->T21)
		x25->callbacks->debug(0, "[X25] stop t21_timer");
	else if (_timer == &x25_int->T22)
		x25->callbacks->debug(0, "[X25] stop t22_timer");
	else if (_timer == &x25_int->T23)
		x25->callbacks->debug(0, "[X25] stop t23_timer");
	else if (_timer == &x25->link.T2)
		x25->callbacks->debug(0, "[X25_LINK] stop link_timer");
}

int x25_timer_running(struct x25_timer * _timer) {
	return _timer->state;
}

void x25_stop_timers(struct x25_cs *x25) {
	if (!x25->callbacks->stop_timer) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_stop_timer(x25, &x25_int->T20);
	x25_stop_timer(x25, &x25_int->T21);
	x25_stop_timer(x25, &x25_int->T22);
	x25_stop_timer(x25, &x25_int->T23);
}






void x25_t20timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_int->N20_RC++;
	if (x25_int->N20_RC >= x25->N20)
		x25_stop_timer(x25, &x25_int->T20);
	else
		x25_transmit_restart_request(x25);
}


void x25_t21timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_stop_timer(x25, &x25_int->T21);
	/* Transmit clear_request */
	if (!x25) return;
}

void x25_t22timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	if (x25_int->state == X25_STATE_4) {
		x25_int->N22_RC++;
		if (x25_int->N22_RC >= x25->N22) {
			x25_stop_timer(x25, &x25_int->T22);
		} else {
			x25->callbacks->debug(1, "[X25] S4 TX RESET_REQUEST");
			x25_write_internal(x25, X25_RESET_REQUEST);
		};
	}
//	if (x25->N22_RC >= x25->N22)
//		x25_stop_timer(x25, &x25->T22);
//	else
//		x25_transmit_clear_request();
}

void x25_t23timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}

void x25_t2timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}
