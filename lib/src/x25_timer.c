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
	x25->callbacks->start_timer(_timer->timer_ptr);
	_timer->state = TRUE;
	if (_timer == &x25->T2)
		x25->callbacks->debug(0, "[X25] start_t2timer is called");
	else if (_timer == &x25->T21)
		x25->callbacks->debug(0, "[X25] start_t21timer is called");
	else if (_timer == &x25->T22)
		x25->callbacks->debug(0, "[X25] start_t22timer is called");
	else if (_timer == &x25->T23)
		x25->callbacks->debug(0, "[X25] start_t23timer is called");
}

void x25_stop_timer(struct x25_cs *x25, struct x25_timer * _timer) {
	if (!x25->callbacks->stop_timer) return;
	if (!_timer->state) return;
	x25->callbacks->stop_timer(_timer->timer_ptr);
	_timer->state = FALSE;
	if (_timer == &x25->T2)
		x25->callbacks->debug(0, "[X25] stop_t2timer is called");
	else if (_timer == &x25->T21)
		x25->callbacks->debug(0, "[X25] stop_t21timer is called");
	else if (_timer == &x25->T22)
		x25->callbacks->debug(0, "[X25] stop_t22timer is called");
	else if (_timer == &x25->T23)
		x25->callbacks->debug(0, "[X25] stop_t23timer is called");
}

int x25_timer_running(struct x25_timer * _timer) {
	return _timer->state;
}







void x25_t20timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}


void x25_t21timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}

void x25_t22timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}

void x25_t23timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}

void x25_t2timer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}
