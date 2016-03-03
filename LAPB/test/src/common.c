
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



char * buf_to_str(char * data, int data_size) {

	bzero(str_buf, 1024);
	if (data_size < 1023) /* 1 byte for null-terminating */
		memcpy(str_buf, data, data_size);
	return str_buf;
}






/*
 * LAPB callback functions for X.25
 *
*/
/* Called by LAPB to inform X25 that Connect Request is confirmed */
void connect_confirmation(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	lapb_debug(NULL, 0, "[X25_CB] connect_confirmation event is called(%s)", lapb_error_str(reason));
	out_counter = 0;
}

/* Called by LAPB to inform X25 that connection was initiated by the remote system */
void connect_indication(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	lapb_debug(NULL, 0, "[X25_CB] connect_indication event is called(%s)", lapb_error_str(reason));
	out_counter = 0;
}

/* Called by LAPB to inform X25 that Disconnect Request is confirmed */
void disconnect_confirmation(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	lapb_debug(NULL, 0, "[X25_CB] disconnect_confirmation event is called(%s)", lapb_error_str(reason));
}

/* Called by LAPB to inform X25 that connection was terminated by the remote system */
void disconnect_indication(struct lapb_cs * lapb, int reason) {
	char * buffer;
	int buffer_size;

	lapb_debug(NULL, 0, "[X25_CB] disconnect_indication event is called(%s)", lapb_error_str(reason));
	if (lapb->write_queue.count) {
		printf("\nUnacked data:\n");
		while ((buffer = lapb_dequeue(lapb, &buffer_size)) != NULL) {
			printf("%s\n", buf_to_str(buffer, buffer_size));
		};
	};
}

/* Called by LAPB to inform X25 about new data */
int data_indication(struct lapb_cs * lapb, char * data, int data_size) {
	(void)lapb;
	//lapb_debug(NULL, 0, "[X25_CB] received new data");
	printf("%s\n", buf_to_str(data, data_size));
	return 0;
}


/* Called by LAPB to write debug info */
void lapb_debug(struct lapb_cs *lapb, int level, const char * format, ...) {
	(void)lapb;
	if (level < LAPB_DEBUG) {
		char buf[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buf, 256, format, argList);
		va_end(argList);
		logger_enqueue(buf, strlen(buf) + 1);
	};
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

int wait_stdin(struct lapb_cs * lapb, unsigned char break_condition, int run_once) {
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
		if ((lapb) && (lapb->state != break_condition))
			break;
	};
	return sr;
}



void print_commands_0(struct lapb_cs * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED) {
		printf("Disconnected state(modulo 128):\n");
		printf("1 Send SABME\n");
	} else {
		printf("Disconnected state(modulo 8):\n");
		printf("1 Send SABM\n");
	};
	printf("--\n");
	printf("0 Exit application\n");
	fprintf(stderr, ">");
}

void print_commands_1(struct lapb_cs * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED)
		printf("Awaiting connection state(modulo 128):\n");
	else
		printf("Awaiting connection state(modulo 8):\n");
	printf("1 Cancel\n");
	printf("--\n");
	printf("0 Exit application\n");
	fprintf(stderr, ">");
}

void print_commands_2(struct lapb_cs * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED)
		printf("Awaiting disconnection state(modulo 128):\n");
	else
		printf("Awaiting disconnection state(modulo 8):\n");
	printf("1 Cancel\n");
	printf("--\n");
	printf("0 Exit application\n");
	fprintf(stderr, ">");
}

void print_commands_3(struct lapb_cs * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED)
		printf("Connected state(modulo 128):\n");
	else
		printf("Connected state(modulo 8):\n");
	printf("Enter text or select command\n");
	printf("1 Send test buffer(10 bytes)\n");
	printf("2 Send sequence of data(500 frames)\n");
	printf("3 Send sequence of data(500 frames - Bad FCS)\n");
	printf("4 Send sequence of data(500 frames - Bad N(R))\n");
	printf("9 Send DISC\n");
	printf("--\n");
	printf("0 Exit application\n");
}

void print_commands_5() {
	printf("Awaiting physical connection:\n");
	printf("--\n");
	printf("0 Exit application\n");
	fprintf(stderr, ">");
}


int lapb_not_ready(struct lapb_cs *lapb, const struct main_callbacks * callbacks) {
	int while_flag;
	int wait_stdin_result;;
	char buffer[256];
	int result = 0;

	if (callbacks->is_connected()) {
		if (!old_state) {
			lapb_reset(lapb, LAPB_STATE_0);
			printf("\nPhysical connection established\n\n");
			old_state = TRUE;
		};
		return 1;
	};
	old_state = FALSE;
	while_flag = TRUE;
	print_commands_5();
	while (while_flag) {
		if (!callbacks->is_connected()) {
			wait_stdin_result = wait_stdin(lapb, LAPB_STATE_0, TRUE);
			if (wait_stdin_result <= 0) {
				sleep_ms(100);
				continue;
			};
		} else {
			lapb_reset(lapb, LAPB_STATE_0);
			printf("\nPhysical connection established\n\n");
			result = 1;
			break;
		};
		bzero(buffer, sizeof(buffer));
		while (read(0, buffer, sizeof(buffer)) <= 1)
			fprintf(stderr, ">");
		printf("\n");
		int action = atoi(buffer);
		switch (action) {
			case 0:
				exit_flag = TRUE;
				while_flag = FALSE;
				break;
			default:
				printf("Command is not supported\n\n");
				break;
		};
	};
	return result;
}

void lapb_state_0(struct lapb_cs *lapb, const struct main_callbacks * callbacks) {
	int wait_stdin_result;;
	char buffer[256];
	int lapb_res;

	int while_flag;
	while_flag = TRUE;
	print_commands_0(lapb);
	while (while_flag) {
		wait_stdin_result = wait_stdin(lapb, LAPB_STATE_0, TRUE);
		if (wait_stdin_result <= 0) {
			if (!callbacks->is_connected()) {
				printf("\nPhysical connection lost\n");
				printf("Reconnecting\n\n");
				break;
			} else {
				if (lapb->state != LAPB_STATE_0) {
					printf("\n\n");
					break;
				};
				sleep_ms(100);
				continue;
			};
			//printf("\n\n");
			//break;
		};
		bzero(buffer, sizeof(buffer));
		while (read(0, buffer, sizeof(buffer)) <= 1)
			fprintf(stderr, ">");
		printf("\n");
		int action = atoi(buffer);
		switch (action) {
			case 1:  /* SABM or SABME */
				lapb_res = lapb_connect_request(lapb);
				if (lapb_res != LAPB_OK) {
					printf("ERROR: %s\n\n", lapb_error_str(lapb_res));
					lapb_reset(lapb, LAPB_STATE_0);
				} else
					while_flag = FALSE;
				break;
			case 0:
				exit_flag = TRUE;
				while_flag = FALSE;
				break;
			default:
				printf("Command is not supported\n\n");
				break;
		};
	};
}

void lapb_state_1(struct lapb_cs *lapb, const struct main_callbacks * callbacks) {
	int while_flag;
	int wait_stdin_result;
	char buffer[256];

	while_flag = TRUE;
	while (while_flag) {
		print_commands_1(lapb);
		wait_stdin_result = wait_stdin(lapb, LAPB_STATE_1, FALSE);
		if (wait_stdin_result <= 0) {
			if (!callbacks->is_connected()) {
				printf("\nPhysical connection lost\n");
				printf("Reconnecting");
			};
			printf("\n\n");
			break;
		};
		bzero(buffer, sizeof(buffer));
		while (read(0, buffer, sizeof(buffer)) <= 1)
			fprintf(stderr, ">");
		printf("\n");
		int action = atoi(buffer);
		switch (action) {
			case 1:  /* Cancel */
				lapb_reset(lapb, LAPB_STATE_0);
				while_flag = FALSE;
				break;
			case 0:
				exit_flag = TRUE;
				while_flag = FALSE;
				break;
			default:
				printf("Command is not supported\n\n");
				break;
		};
	};
}

void lapb_state_2(struct lapb_cs *lapb, const struct main_callbacks * callbacks) {
	int while_flag;
	int wait_stdin_result;
	char buffer[256];

	while_flag = TRUE;
	while (while_flag) {
		print_commands_2(lapb);
		wait_stdin_result = wait_stdin(lapb, LAPB_STATE_2, FALSE);
		if (wait_stdin_result <= 0) {
			if (!callbacks->is_connected()) {
				printf("\nPhysical connection lost\n");
				printf("Reconnecting");
			};
			printf("\n\n");
			break;
		};
		bzero(buffer, sizeof(buffer));
		while (read(0, buffer, sizeof(buffer)) <= 1)
			fprintf(stderr, ">");
		printf("\n");
		int action = atoi(buffer);
		switch (action) {
			case 1:  /* Cancel */
				lapb_reset(lapb, LAPB_STATE_0);
				while_flag = FALSE;
				break;
			case 0:
				exit_flag = TRUE;
				while_flag = FALSE;
				break;
			default:
				printf("Command is not supported\n\n");
				break;
		};
	};
}

void lapb_state_3(struct lapb_cs *lapb, const struct main_callbacks * callbacks) {
	int while_flag;
	int wait_stdin_result;
	char buffer[256];
	int n;
	int lapb_res;

	while_flag = TRUE;
	print_commands_3(lapb);
	while (while_flag) {
		fprintf(stderr, ">");
		wait_stdin_result = wait_stdin(lapb, LAPB_STATE_3, FALSE);
		if (wait_stdin_result <= 0) {
			if (!callbacks->is_connected()) {
				printf("\nPhysical connection lost\n");
				printf("Reconnecting");
			};
			printf("\n\n");
			break;
		};
		bzero(buffer, sizeof(buffer));
		while (read(0, buffer, sizeof(buffer)) <= 1) {
			char move_up[] = { 0x1b, '[', '1', 'A', 0 };
			char move_right[] = { 0x1b, '[', '1', 'C', 0 };
			fprintf(stderr, "%s%s", move_up, move_right);
		};
		char * pEnd;
		int action = strtol(buffer, &pEnd, 10);
		int data_size;
		if (buffer == pEnd)
			action = 99;
		switch (action) {
			case 1: default:  /* Send test buffer (128 byte) or text from console */
				if (action == 1) {
					data_size = 10;
					n = 0;
					while (n < data_size) {
						buffer[n] = 'a' + n;
						n++;
					};
				} else
					data_size = strlen(buffer) - 1;
				buffer[data_size] = 0;
				lapb_res = lapb_data_request(lapb, buffer, data_size);
				if (lapb_res != LAPB_OK) {
					printf("ERROR: %s\n\n", lapb_error_str(lapb_res));
					lapb_reset(lapb, LAPB_STATE_0);
				};
				//else
				//	while_flag = FALSE;
				break;
			case 2: case 3: case 4: /* Send sequence of data */
				error_type = action - 2;
				error_counter = 1;
				while (!break_flag) {
					bzero(buffer, sizeof(buffer));
					sprintf(buffer, "abcdefghij_%d", out_counter);
					lapb_res = lapb_data_request(lapb, buffer, strlen(buffer));
					if (lapb_res != LAPB_OK) {
						if (lapb_res != LAPB_BUSY) {
							printf("ERROR: %s\n", lapb_error_str(lapb_res));
							break;
						};
						sleep_ms(50);
						continue;
					};
					//sleep_ms(100);
					out_counter++;
					if (out_counter % 500 == 0) break;
					if (error_type != 0)
						error_counter = (error_counter + 1) % 13;
				};
				break;
			case 9: /* DISC */
				lapb_res = lapb_disconnect_request(lapb);
				if (lapb_res != LAPB_OK) {
					printf("ERROR: %s\n\n", lapb_error_str(lapb_res));
					lapb_reset(lapb, LAPB_STATE_0);
				} else
					while_flag = FALSE;
				break;
			case 0:
				exit_flag = TRUE;
				while_flag = FALSE;
				break;
		};
	};
}

void main_loop(struct lapb_cs *lapb, const struct main_callbacks * callbacks) {

	error_type = 0; /* No errors */
	error_counter = 1;
	while (!exit_flag) {
		if (!lapb_not_ready(lapb, callbacks))
			continue;
		switch (lapb->state) {
			case LAPB_STATE_0:
				lapb_state_0(lapb, callbacks);
				break;
			case LAPB_STATE_1:
				lapb_state_1(lapb, callbacks);
				break;
			case LAPB_STATE_2:
				lapb_state_2(lapb, callbacks);
				break;
			case LAPB_STATE_3:
				lapb_state_3(lapb, callbacks);
				break;
		};
	};

}
