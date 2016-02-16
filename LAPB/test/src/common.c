
#include "my_timer.h"
#include "common.h"
#include "logger.h"

char str_buf[1024];

/* Signals handler */
void signal_callback_handler(int signum) {
	printf("Caught signal %d\n", signum);
	// Cleanup and close up stuff here
	switch (signum) {
		case SIGINT: case SIGKILL: case SIGTERM:
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
/* Called by LAPB to inform X25 that SABM(SABME) is confirmed or UA sended on SABM(SABME) command */
void on_connected(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "[X25_CB] connected event is called(%s)", lapb_error_str(reason));
}

/* Called by LAPB to inform X25 that DISC is received or UA sended on DISC command */
void on_disconnected(struct lapb_cb * lapb, int reason) {
	char * buffer;
	int buffer_size;

	syslog(LOG_NOTICE, "[X25_CB] disconnected event is called(%s)", lapb_error_str(reason));
	if (lapb->write_queue.count) {
		printf("\n\nUnacked data:\n");
		while ((buffer = lapb_dequeue(lapb, &buffer_size)) != NULL) {
			printf("%s\n", buf_to_str(buffer, buffer_size));
		};
	};
}

/* Called by LAPB to inform X25 about new data */
int on_new_incoming_data(struct lapb_cb * lapb, char * data, int data_size) {
	(void)lapb;
	syslog(LOG_NOTICE, "[X25_CB] received new data");
	printf("%s\n", buf_to_str(data, data_size));
	return 0;
}

/* Called by LAPB to start timer T1 */
void start_t1timer(struct lapb_cb * lapb) {
	set_t1_value(lapb->T1);
	set_t1_state(TRUE);
	syslog(LOG_NOTICE, "[LAPB] start_t1timer is called");
}

/* Called by LAPB to stop timer T1 */
void stop_t1timer() {
	set_t1_state(FALSE);
	set_t1_value(0);
	syslog(LOG_NOTICE, "[LAPB] stop_t1timer is called");
}

/* Called by LAPB to check timer T1 state */
int t1timer_running() {
	return get_t1_state();
}

/* Called by LAPB to start timer T2 */
void start_t2timer(struct lapb_cb * lapb) {
	set_t2_value(lapb->T2);
	set_t2_state(TRUE);
	syslog(LOG_NOTICE, "[LAPB] start_t2timer is called");
}

/* Called by LAPB to stop timer T1 */
void stop_t2timer() {
	set_t2_state(FALSE);
	set_t2_value(0);
	syslog(LOG_NOTICE, "[LAPB] stop_t2timer is called");
}

/* Called by LAPB to check timer T2 state */
int t2timer_running() {
	return get_t2_state();
}


/* Called by LAPB to write debug info */
void lapb_debug(struct lapb_cb *lapb, int level, const char * format, ...) {
	(void)lapb;
	if (level < LAPB_DEBUG) {
		char buf[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buf, 256, format, argList);
		va_end(argList);
		//syslog(LOG_NOTICE, "[LAPB] %s", buf);
		logger_enqueue(buf, strlen(buf));
	};
}




/*
 * Timer callback functions
 *
*/
void t1timer_expiry(unsigned long int lapb_addr) {
	struct lapb_cb * lapb = (struct lapb_cb *)lapb_addr;
	lapb_t1timer_expiry(lapb);
}

void t2timer_expiry(unsigned long int lapb_addr) {
	struct lapb_cb * lapb = (struct lapb_cb *)lapb_addr;
	syslog(LOG_NOTICE, "[X25_CB] Timer_2 expired");
	lapb_t2timer_expiry(lapb);
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

int wait_stdin(struct lapb_cb * lapb, unsigned char break_condition, int run_once) {
	fd_set			read_set;
	struct timeval	timeout;
	int sr = 0;

	while (lapb->state == break_condition) {
		FD_ZERO(&read_set);
		FD_SET(fileno(stdin), &read_set);
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		sr = select(fileno(stdin) + 1, &read_set, NULL, NULL, &timeout);
		if ((sr > 0) || (run_once))
			break;
	};
	return sr;
}

void main_lock() {
	pthread_mutex_lock(&main_mutex);
}

void main_unlock() {
	pthread_mutex_unlock(&main_mutex);
}



void print_commands_0(struct lapb_cb * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED) {
		printf("Disconnected state(modulo 128):\n");
		printf("1 Send SABME\n");
	} else {
		printf("Disconnected state(modulo 8):\n");
		printf("1 Send SABM\n");
	};
	printf("--\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}

void print_commands_1(struct lapb_cb * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED)
		printf("Awaiting connection state(modulo 128):\n");
	else
		printf("Awaiting connection state(modulo 8):\n");
	printf("1 Cancel\n");
	printf("--\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}

void print_commands_2(struct lapb_cb * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED)
		printf("Awaiting disconnection state(modulo 128):\n");
	else
		printf("Awaiting disconnection state(modulo 8):\n");
	printf("1 Cancel\n");
	printf("--\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}

void print_commands_3(struct lapb_cb * lapb) {
	if ((lapb->mode & LAPB_EXTENDED) == LAPB_EXTENDED)
		printf("Connected state(modulo 128):\n");
	else
		printf("Connected state(modulo 8):\n");
	printf("Enter text or select command\n");
	printf("1 Send test buffer(10 bytes)\n");
	printf("2 Send DISC\n");
	printf("--\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}

void print_commands_5() {
	printf("Awaiting physical connection:\n");
	printf("--\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}


void main_loop(struct lapb_cb *lapb, const struct main_callbacks * callbacks) {
	int wait_stdin_result;;
	char buffer[2048];
	int lapb_res;
	int n;

	int while_flag = TRUE;
	while (!exit_flag) {
		switch (lapb->state) {
			case LAPB_NOT_READY:

				while_flag = TRUE;
				print_commands_5();
				while (while_flag) {
					if (!callbacks->is_connected()) {
						wait_stdin_result = wait_stdin(lapb, LAPB_NOT_READY, TRUE);
						if (wait_stdin_result <= 0) {
							sleep_ms(100);
							continue;
						};
						//printf("\n\n");
						//break;
					} else {
						//lapb->state = LAPB_STATE_0;
						lapb_reset(lapb, LAPB_STATE_0);
						//lapb->mode = lapb_modulo | LAPB_SLP | lapb_equipment_type;
						//if (lapb_equipment_type == LAPB_DCE)
						//	lapb_start_t1timer(lapb);
						printf("\nPhysical connection established\n\n");
						break;
					};
					bzero(buffer, sizeof(buffer));
					while (read(0, buffer, sizeof(buffer)) <= 1)
						write(0, ">", 1);
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

				break;
			case LAPB_STATE_0:
				while_flag = TRUE;
				while (while_flag) {
					print_commands_0(lapb);
					wait_stdin_result = wait_stdin(lapb, LAPB_STATE_0, FALSE);
					if (wait_stdin_result <= 0) {
						if (lapb->state == LAPB_NOT_READY) {
							printf("\nPhysical connection lost\n");
							printf("Reconnecting");
						};
						printf("\n\n");
						break;
					};
					bzero(buffer, sizeof(buffer));
					while (read(0, buffer, sizeof(buffer)) <= 1)
						write(0, ">", 1);
					printf("\n");
					int action = atoi(buffer);
					switch (action) {
						case 1:  // SABM or SABME
							main_lock();
							lapb_res = lapb_connect_request(lapb);
							main_unlock();
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

				break;
			case LAPB_STATE_1:
				while_flag = TRUE;
				while (while_flag) {
					print_commands_1(lapb);
					wait_stdin_result = wait_stdin(lapb, LAPB_STATE_1, FALSE);
					if (wait_stdin_result <= 0) {
						if (lapb->state == LAPB_NOT_READY) {
							printf("\nPhysical connection lost\n");
							printf("Reconnecting");
						};
						printf("\n\n");
						break;
					};
					bzero(buffer, sizeof(buffer));
					while (read(0, buffer, sizeof(buffer)) <= 1)
						write(0, ">", 1);
					printf("\n");
					int action = atoi(buffer);
					switch (action) {
						case 1:  // Cancel
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
				break;
			case LAPB_STATE_2:
				while_flag = TRUE;
				while (while_flag) {
					print_commands_2(lapb);
					wait_stdin_result = wait_stdin(lapb, LAPB_STATE_2, FALSE);
					if (wait_stdin_result <= 0) {
						if (lapb->state == LAPB_NOT_READY) {
							printf("\nPhysical connection lost\n");
							printf("Reconnecting");
						};
						printf("\n\n");
						break;
					};
					bzero(buffer, sizeof(buffer));
					while (read(0, buffer, sizeof(buffer)) <= 1)
						write(0, ">", 1);
					printf("\n");
					int action = atoi(buffer);
					switch (action) {
						case 1:  // Cancel
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
				break;
			case LAPB_STATE_3:
				while_flag = TRUE;
				while (while_flag) {
					print_commands_3(lapb);
					wait_stdin_result = wait_stdin(lapb, LAPB_STATE_3, FALSE);
					if (wait_stdin_result <= 0) {
						if (lapb->state == LAPB_NOT_READY) {
							printf("\nPhysical connection lost\n");
							printf("Reconnecting");
						};
						printf("\n\n");
						break;
					};
					bzero(buffer, sizeof(buffer));
					while (read(0, buffer, sizeof(buffer)) <= 1)
						write(0, ">", 1);
					printf("\n");
					//int action = atoi(buffer);
					char * pEnd;
					int action = strtol(buffer, &pEnd, 10);
					int data_size;
					if (buffer == pEnd)
						action = 99;
					switch (action) {
						case 1: default:  // Send test buffer (128 byte) or text from console
							if (action == 1) {
								data_size = 10;
								n = 0;
								while (n < data_size) {
									buffer[n] = 'a' + n;
									n++;
								};
								//buffer[n] = '2';
								//buffer[n + 1] = '\n';
							} else
								data_size = strlen(buffer) - 1;
							buffer[data_size] = 0;
							main_lock();
							lapb_res = lapb_data_request(lapb, buffer, data_size);
							main_unlock();
							if (lapb_res != LAPB_OK) {
								printf("ERROR: %s\n\n", lapb_error_str(lapb_res));
								lapb_reset(lapb, LAPB_STATE_0);
							} else
								while_flag = FALSE;
							break;
						case 2: // DISC
							main_lock();
							lapb_res = lapb_disconnect_request(lapb);
							main_unlock();
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
				break;



			case LAPB_STATE_4:
				while_flag = TRUE;
				while (while_flag) {
					//print_commands_2(lapb);
					wait_stdin_result = wait_stdin(lapb, LAPB_STATE_4, FALSE);
					if (wait_stdin_result <= 0) {
						if (lapb->state == LAPB_NOT_READY) {
							printf("\nPhysical connection lost\n");
							printf("Reconnecting");
						};
						printf("\n\n");
						break;
					};
					bzero(buffer, sizeof(buffer));
					while (read(0, buffer, sizeof(buffer)) <= 1)
						write(0, ">", 1);
					printf("\n");
					int action = atoi(buffer);
					switch (action) {
//						case 1:  // Cancel
//							lapb_reset(lapb, LAPB_STATE_0);
//							while_flag = FALSE;
//							break;
						case 0:
							exit_flag = TRUE;
							while_flag = FALSE;
							break;
						default:
							printf("Command is not supported\n\n");
							break;
					};
				};
				break;





		};
	};

}