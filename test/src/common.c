
#include "my_timer.h"
#include "common.h"
#include "logger.h"

char str_buf[1024];
int break_flag = FALSE;
int out_counter;
//int old_state = FALSE;

/* Signals handler */
void signal_callback_handler(int signum) {
	printf("Caught signal %d\n", signum);
	// Cleanup and close up stuff here
	switch (signum) {
		case SIGINT:
			break_flag = TRUE;
			break;
		case SIGKILL: case SIGTERM:
			exit_flag = TRUE;
			break;
		default:
			break;
	};
}

void setup_signals_handler() {
	struct sigaction sa;

	bzero(&sa, sizeof(struct sigaction));
	sa.sa_handler = signal_callback_handler;
	sigemptyset(&sa.sa_mask);

	// Intercept SIGHUP and SIGINT
	if (sigaction(SIGHUP, &sa, NULL) == -1)
		perror("Error: cannot handle SIGHUP"); // Should not happen

	if (sigaction(SIGTERM, &sa, NULL) == -1)
		perror("Error: cannot handle SIGUSR1"); // Should not happen

	if (sigaction(SIGINT, &sa, NULL) == -1)
		perror("Error: cannot handle SIGINT"); // Should not happen
}


void custom_debug(int level, const char * format, ...) {
	if (level < LIB_DEBUG) {
		char buf[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buf, 256, format, argList);
		va_end(argList);
		logger_enqueue(buf, strlen(buf) + 1);
	};
}



/*
 * LAPB callback functions for X.25
 *
*/
/* Called by LAPB to inform X25 that Connect Request is confirmed */
void lapb_connect_confirmation_cb(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	custom_debug(0, "[X25_CB] connect_confirmation event is called(%s)", lapb_error_str(reason));
	x25_link_established(lapb->L3_ptr);
	out_counter = 0;
}

/* Called by LAPB to inform X25 that connection was initiated by the remote system */
void lapb_connect_indication_cb(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	custom_debug(0, "[X25_CB] connect_indication event is called(%s)", lapb_error_str(reason));
	x25_link_established(lapb->L3_ptr);
	out_counter = 0;
}

/* Called by LAPB to inform X25 that Disconnect Request is confirmed */
void lapb_disconnect_confirmation_cb(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	custom_debug(0, "[X25_CB] disconnect_confirmation event is called(%s)", lapb_error_str(reason));
}

/* Called by LAPB to inform X25 that connection was terminated by the remote system */
void lapb_disconnect_indication_cb(struct lapb_cs * lapb, int reason) {
	char * buffer;
	int buffer_size;

	custom_debug(0, "[X25_CB] disconnect_indication event is called(%s)", lapb_error_str(reason));
	buffer = lapb_dequeue(lapb, &buffer_size);
	if (buffer) {
		printf("\nUnacked data:\n");
		printf("%s\n", buf_to_str(buffer, buffer_size));
		while ((buffer = lapb_dequeue(lapb, &buffer_size)) != NULL)
			printf("%s\n", buf_to_str(buffer, buffer_size));
	};
}

/* Called by LAPB to inform X25 about new data */
int lapb_data_indication_cb(struct lapb_cs * lapb, char * data, int data_size) {
	custom_debug(0, "[X25_CB] received new data");
	x25_link_receive_data(lapb->L3_ptr, data, data_size);
	//printf("%s\n", buf_to_str(data, data_size));
	return 0;
}

char * lapb_error_str(int error) {
	switch (error) {
		case LAPB_OK:
			return LAPB_OK_STR;
		case LAPB_BADTOKEN:
			return LAPB_BADTOKEN_STR;
		case LAPB_INVALUE:
			return LAPB_INVALUE_STR;
		case LAPB_CONNECTED:
			return LAPB_CONNECTED_STR;
		case LAPB_NOTCONNECTED:
			return LAPB_NOTCONNECTED_STR;
		case LAPB_REFUSED:
			return LAPB_REFUSED_STR;
		case LAPB_TIMEDOUT:
			return LAPB_TIMEDOUT_STR;
		case LAPB_NOMEM:
			return LAPB_NOMEM_STR;
		case LAPB_BUSY:
			return LAPB_BUSY_STR;
		case LAPB_BADFCS:
			return LAPB_BADFCS_STR;
		default:
			return "Unknown error";
			break;
	};
}





int sleep_ms(int milliseconds) {
	int result = 0;

	struct timespec ts, ts2;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	ts2.tv_sec = 0;
	ts2.tv_nsec = 0;
	result = nanosleep(&ts, &ts2);
	if ((ts.tv_sec != ts2.tv_sec) || (ts.tv_nsec > ts2.tv_nsec))
		if (ts2.tv_nsec != -1)
			result = -1;

	return result;
}





void main_loop() {
	error_type = 0; /* No errors */
	error_counter = 1;
	while (!exit_flag) {
		sleep_ms(1000);
	};

}
