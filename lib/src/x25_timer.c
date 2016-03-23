/*
 *	X25 release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */


#include "x25_int.h"

void x25_start_timer(struct x25_cs *x25, struct x25_timer * timer) {
	if (!x25->callbacks->start_timer) return;
	if (timer->state) return;

	x25->callbacks->start_timer(timer->timer_ptr);
	timer->state = TRUE;
	timer->RC = 0;
#if X25_DEBUG >= 3
	struct x25_cs_internal * x25_int = x25_get_internal(x25);
	if (timer == &x25_int->RestartTimer)
		x25->callbacks->debug(0, "[X25_LINK] start restart_timer");
	else if (timer == &x25_int->CallTimer)
		x25->callbacks->debug(0, "[X25] start call_timer");
	else if (timer == &x25_int->ResetTimer)
		x25->callbacks->debug(0, "[X25] start reset_timer");
	else if (timer == &x25_int->ClearTimer)
		x25->callbacks->debug(0, "[X25] start clear_timer");
	else if (timer == &x25_int->AckTimer)
		x25->callbacks->debug(0, "[X25] start ack_timer");
	else if (timer == &x25_int->DataTimer)
		x25->callbacks->debug(0, "[X25] start data_timer");
#endif
}

void x25_stop_timer(struct x25_cs *x25, struct x25_timer * timer) {
	if (!x25->callbacks->stop_timer) return;
	if (!timer->state) return;

	x25->callbacks->stop_timer(timer->timer_ptr);
	timer->state = FALSE;
#if X25_DEBUG >= 3
	struct x25_cs_internal * x25_int = x25_get_internal(x25);
	if (timer == &x25_int->RestartTimer)
		x25->callbacks->debug(0, "[X25_LINK] stop restart_timer");
	else if (timer == &x25_int->CallTimer)
		x25->callbacks->debug(0, "[X25] stop call_timer");
	else if (timer == &x25_int->ResetTimer)
		x25->callbacks->debug(0, "[X25] stop reset_timer");
	else if (timer == &x25_int->ClearTimer)
		x25->callbacks->debug(0, "[X25] stop clear_timer");
	else if (timer == &x25_int->AckTimer)
		x25->callbacks->debug(0, "[X25] stop ack_timer");
	else if (timer == &x25_int->DataTimer)
		x25->callbacks->debug(0, "[X25] stop data_timer");
#endif
}

int x25_timer_running(struct x25_timer * timer) {
	return timer->state;
}

void x25_stop_timers(struct x25_cs *x25) {
	if (!x25->callbacks->stop_timer) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_stop_timer(x25, &x25_int->RestartTimer);
	x25_stop_timer(x25, &x25_int->CallTimer);
	x25_stop_timer(x25, &x25_int->ResetTimer);
	x25_stop_timer(x25, &x25_int->ClearTimer);
	x25_stop_timer(x25, &x25_int->AckTimer);
	x25_stop_timer(x25, &x25_int->DataTimer);
}





void x25_RestartTimer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_int->RestartTimer.RC++;
	if (x25_int->RestartTimer.RC >= x25->RestartTimer_NR)
		x25_stop_timer(x25, &x25_int->RestartTimer);
	else
		x25_transmit_restart_request(x25);
}


void x25_CallTimer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_stop_timer(x25, &x25_int->CallTimer);
	/* Transmit clear_request */
	x25->callbacks->debug(1, "[X25] S%d TX CLEAR_REQUEST", x25_int->state);
	x25_write_internal(x25, X25_CLEAR_REQUEST);
}

void x25_ResetTimer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_int->ResetTimer.RC++;
	if (x25_int->ResetTimer.RC >= x25->ResetTimer_NR) {
		x25_stop_timer(x25, &x25_int->ResetTimer);
		x25_disconnect(x25, X25_TIMEDOUT, 0, 0);
	} else {
		x25->callbacks->debug(1, "[X25] S%d TX RESET_REQUEST", x25_int->state);
		x25_write_internal(x25, X25_RESET_REQUEST);
	};
}

void x25_ClearTimer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
	x25_disconnect(x25, X25_TIMEDOUT, 0, 0);
}

void x25_AckTimer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
}

void x25_DataTimer_expiry(void * x25_ptr) {
	struct x25_cs *x25 = x25_ptr;
	if (!x25) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25);

	x25_int->DataTimer.RC++;
	x25->callbacks->debug(1, "[X25] S%d data_timer expired(%d of %d)",
						   x25_int->state, x25_int->DataTimer.RC, x25->DataTimer_NR);
	if (x25_int->DataTimer.RC >= x25->DataTimer_NR) {
		x25_stop_timer(x25, &x25_int->DataTimer);
		x25_disconnect(x25, X25_TIMEDOUT, 0, 0);
	} else {
		x25_requeue_frames(x25);
		x25_kick(x25);
	};
}



void x25_timer_expiry(void * x25_ptr, void * timer_ptr) {
	if (!x25_ptr) return;
	struct x25_cs_internal * x25_int = x25_get_internal(x25_ptr);
	if (timer_ptr == x25_int->RestartTimer.timer_ptr)
		x25_RestartTimer_expiry(x25_ptr);
	else if (timer_ptr == x25_int->CallTimer.timer_ptr)
		x25_CallTimer_expiry(x25_ptr);
	else if (timer_ptr == x25_int->ResetTimer.timer_ptr)
		x25_ResetTimer_expiry(x25_ptr);
	else if (timer_ptr == x25_int->ClearTimer.timer_ptr)
		x25_ClearTimer_expiry(x25_ptr);
	else if (timer_ptr == x25_int->AckTimer.timer_ptr)
		x25_AckTimer_expiry(x25_ptr);
	else if (timer_ptr == x25_int->DataTimer.timer_ptr)
		x25_DataTimer_expiry(x25_ptr);
}

