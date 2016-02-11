
#include "my_timer.h"
#include "common.h"

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







/*
 * LAPB callback functions for X.25
 *
*/
/* Called by LAPB to inform X25 that SABM(SABME) is confirmed */
void connect_confirmation(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "[X25_CB] connect_confirmation is called(%s)", lapb_error_str(reason));
}

/* Called by LAPB to inform X25 that SABM(SABME) is received and UA sended */
void connect_indication(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "[X25_CB] connect_indication is called(%s)", lapb_error_str(reason));
}

/* Called by LAPB to inform X25 that DISC is confirmed */
void disconnect_confirmation(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "[X25_CB] disconnect_confirmation is called(%s)", lapb_error_str(reason));
}

/* Called by LAPB to inform X25 that DISC is received and UA sended */
void disconnect_indication(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "[X25_CB] disconnect_indication is called(%s)", lapb_error_str(reason));
}

/* Called by LAPB to inform X25 about new data */
int data_indication(struct lapb_cb * lapb, unsigned char * data, int data_size) {
	(void)lapb;
	(void)data;
	(void)data_size;
	syslog(LOG_NOTICE, "[X25_CB] data_indication is called");
	printf((char *)data);
	return 0;
}

///* Called by LAPB to transmit data via physical connection */
//void data_transmit(struct lapb_cb * lapb, struct lapb_buff *skb) {
//	(void)lapb;
//	syslog(LOG_NOTICE, "[PHYS_CB] data_transmit is called");

//	int n = write(tcp_client_socket(), skb->data, skb->data_size);
//	if (n < 0)
//		syslog(LOG_ERR, "[PHYS_CB] ERROR writing to socket, %s", strerror(errno));
//}

/* Called by LAPB to start timer T1 */
void start_t1timer(struct lapb_cb * lapb) {
	set_t1_value(lapb->T1 * 1000);
	set_t1_state(TRUE);
	syslog(LOG_NOTICE, "[LAPB] start_t1timer is called");
}

/* Called by LAPB to stop timer T1 */
void stop_t1timer() {
	set_t1_state(FALSE);
	set_t1_value(0);
	syslog(LOG_NOTICE, "[LAPB] stop_t1timer is called");
}

/* Called by LAPB to start timer T2 */
void start_t2timer(struct lapb_cb * lapb) {
	set_t2_value(lapb->T2 * 1000);
	set_t2_state(TRUE);
	syslog(LOG_NOTICE, "[LAPB] start_t2timer is called");
}

/* Called by LAPB to stop timer T1 */
void stop_t2timer() {
	set_t2_state(FALSE);
	set_t2_value(0);
	syslog(LOG_NOTICE, "[LAPB] stop_t2timer is called");
}


/* Called by LAPB to write debug info */
void lapb_debug(struct lapb_cb *lapb, int level, const char * format, ...) {
	(void)lapb;
	if (level < LAPB_DEBUG) {
		char buf[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buf, 256, format, argList);
		//sprintf(buf, format, argList);
		va_end(argList);
		syslog(LOG_NOTICE, "[LAPB] %s", buf);
	};
}




/*
 * Timer callback functions
 *
*/
void t1timer_expiry(unsigned long int lapb_addr) {
	struct lapb_cb * lapb = (struct lapb_cb *)lapb_addr;
	syslog(LOG_NOTICE, "[X25_CB] Timer_1 expired");
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
	//if (result == 0)
	//	return 0;
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
	//printf("2 Send DISC\n");
	//printf("------------\n");
	//printf("3 Send RR\n");
	//printf("4 Send RNR\n");
	//printf("5 Send REJ\n");
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
	printf("1 Send test buffer(128 byte)\n");
	printf("2 Send DISC\n");
	//printf("-----------\n");
	//printf("4 Send RR\n");
	//printf("5 Send RNR\n");
	//printf("6 Send REJ\n");
	printf("--\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}

void print_commands_5() {
	printf("Awaiting physical connection:\n");
	//printf("1 Cancel\n");
	printf("--\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}


void main_loop(struct lapb_cb *lapb, const struct main_callbacks * callbacks, unsigned char lapb_equipment_type, unsigned char lapb_modulo) {
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
						lapb->state = LAPB_STATE_0;
						lapb->mode = lapb_modulo | LAPB_SLP | lapb_equipment_type;
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
						if (lapb->state == LAPB_NOT_READY)
							printf("TCP client disconnected");
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
					if (buffer == pEnd)
						action = 99;
					switch (action) {
						case 1: default:  // Send test buffer (128 byte)
							if (action == 1) {
								n = 0;
								while (n < 127) {
									buffer[n] = '1';
									n++;
								};
								buffer[127] = '\n';
							};
							main_lock();
							lapb_res = lapb_data_request(lapb, (unsigned char *)buffer, 128);
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
//						default:
//							printf("Command is not supported\n\n");
//							break;
					};
				};
				break;

		};
	};

}
