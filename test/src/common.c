
#include "my_timer.h"
#include "common.h"
#include "logger.h"

char str_buf[1024];
int break_flag = FALSE;
int out_counter;
int old_state = FALSE;

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

	// Will always fail, SIGKILL is intended to force kill your process
	//if (sigaction(SIGKILL, &sa, NULL) == -1) {
	//	perror("Cannot handle SIGKILL"); // Will always happen
	//	printf("You can never handle SIGKILL anyway...\n");
	//};

	if (sigaction(SIGINT, &sa, NULL) == -1)
		perror("Error: cannot handle SIGINT"); // Should not happen
}






/*
 * LAPB callback functions for X.25
 *
*/
/* Called by LAPB to inform X25 that Connect Request is confirmed */
void connect_confirmation(struct x25_cs * lapb, int reason) {
	(void)lapb;
	x25_debug(0, "[X25_CB] connect_confirmation event is called(%s)", x25_error_str(reason));
	out_counter = 0;
}

/* Called by LAPB to inform X25 that connection was initiated by the remote system */
void connect_indication(struct x25_cs * lapb, int reason) {
	(void)lapb;
	x25_debug(0, "[X25_CB] connect_indication event is called(%s)", x25_error_str(reason));
	out_counter = 0;
}

/* Called by LAPB to inform X25 that Disconnect Request is confirmed */
void disconnect_confirmation(struct x25_cs * lapb, int reason) {
	(void)lapb;
	x25_debug(0, "[X25_CB] disconnect_confirmation event is called(%s)", x25_error_str(reason));
}

/* Called by LAPB to inform X25 that connection was terminated by the remote system */
void disconnect_indication(struct x25_cs * lapb, int reason) {
	char * buffer;
	int buffer_size;

	x25_debug(0, "[X25_CB] disconnect_indication event is called(%s)", x25_error_str(reason));
	buffer = x25_dequeue(lapb, &buffer_size);
	if (buffer) {
		printf("\nUnacked data:\n");
		printf("%s\n", buf_to_str(buffer, buffer_size));
		while ((buffer = x25_dequeue(lapb, &buffer_size)) != NULL)
			printf("%s\n", buf_to_str(buffer, buffer_size));
	};
}

/* Called by LAPB to inform X25 about new data */
int data_indication(struct x25_cs * lapb, char * data, int data_size) {
	(void)lapb;
	//lapb_debug(NULL, 0, "[X25_CB] received new data");
	printf("%s\n", buf_to_str(data, data_size));
	return 0;
}


/* Called by LAPB to write debug info */
void x25_debug(int level, const char * format, ...) {
	if (level < LAPB_DEBUG) {
		char buf[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buf, 256, format, argList);
		va_end(argList);
		logger_enqueue(buf, strlen(buf) + 1);
	};
}





char * x25_error_str(int error) {
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

int wait_stdin(struct x25_cs * lapb, unsigned char break_condition, int run_once) {
	fd_set			read_set;
	struct timeval	timeout;
	int sr = 0;

	//while (lapb->state == break_condition) {
	while (1) {
		FD_ZERO(&read_set);
		FD_SET(fileno(stdin), &read_set);
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		sr = select(fileno(stdin) + 1, &read_set, NULL, NULL, &timeout);
		if ((sr > 0) || (run_once))
			break;
		if ((lapb) && (x25_get_state(lapb) != break_condition))
			break;
	};
	return sr;
}




void main_loop(struct x25_cs *lapb) {
	(void)lapb;
	error_type = 0; /* No errors */
	error_counter = 1;
	while (!exit_flag) {
//		switch (lapb_get_state(lapb)) {
//			case LAPB_STATE_0:
//				lapb_state_0(lapb);
//				break;
//			case LAPB_STATE_1:
//				lapb_state_1(lapb);
//				break;
//			case LAPB_STATE_2:
//				lapb_state_2(lapb);
//				break;
//			case LAPB_STATE_3:
//				lapb_state_3(lapb);
//				break;
//		};
	};

}
