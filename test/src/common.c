
#include <execinfo.h>

#include "my_timer.h"
#include "common.h"
#include "logger.h"

char str_buf[1024];
int break_flag = FALSE;
int out_counter;
//int old_state = FALSE;


static pthread_mutex_t main_mutex;


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

static void signal_error(int sig) {
	(void)sig;
	//(void)ptr;
	//void* ErrorAddr;
	void* Trace[100];
	int    x;
	int    TraceSize;
	char** Messages;

	// запишем в лог что за сигнал пришел
	syslog(LOG_ERR, "[DAEMON] Signal: %s", strsignal(sig));

	// произведем backtrace чтобы получить весь стек вызовов
	TraceSize = backtrace(Trace, sizeof(Trace) / sizeof(void *));
	//Trace[1] = ErrorAddr;

	// получим расшифровку трасировки
	Messages = backtrace_symbols(Trace, TraceSize);

	if (Messages) {
		syslog(LOG_ERR, "== Backtrace ==");

		// запишем в лог
		for (x = 0; x < TraceSize; x++) {
			syslog(LOG_ERR, "%s", Messages[x]);
		};

		syslog(LOG_ERR, "== End Backtrace ==");
		free(Messages);
	};

	syslog(LOG_ERR, "[DAEMON] Stopped");
	closelog();

	// завершим процесс с кодом требующим перезапуска
	exit(0);
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


	sigemptyset(&sa.sa_mask);
	// сигналы об ошибках в программе будут обрататывать более тщательно
	// указываем что хотим получать расширенную информацию об ошибках
	sa.sa_flags = 0;
	// задаем функцию обработчик сигналов
	sa.sa_handler = signal_error;

	// установим наш обработчик на сигналы
	signal(SIGFPE,	&signal_error); // ошибка FPU
	signal(SIGILL,	&signal_error); // ошибочная инструкция
	signal(SIGSEGV,	&signal_error); // ошибка доступа к памяти

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

/* Mutex functions for synchronization */
void main_mutex_init() {
	pthread_mutex_init(&main_mutex, NULL);
}

void main_lock() {
	pthread_mutex_lock(&main_mutex);
}

void main_unlock() {
	pthread_mutex_unlock(&main_mutex);
}




/*
 * LAPB callback functions for X.25
 *
*/
/* Called by LAPB to inform X25 that Connect Request is confirmed */
void lapb_connect_confirmation_cb(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	custom_debug(0, "[LAPB_CB] connect_confirmation event is called(%s)", lapb_error_str(reason));
	x25_link_established(lapb->L3_ptr);
	out_counter = 0;
}

/* Called by LAPB to inform X25 that connection was initiated by the remote system */
void lapb_connect_indication_cb(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	custom_debug(0, "[LAPB_CB] connect_indication event is called(%s)", lapb_error_str(reason));
	x25_link_established(lapb->L3_ptr);
	out_counter = 0;
}

/* Called by LAPB to inform X25 that Disconnect Request is confirmed */
void lapb_disconnect_confirmation_cb(struct lapb_cs * lapb, int reason) {
	(void)lapb;
	custom_debug(0, "[LAPB_CB] disconnect_confirmation event is called(%s)", lapb_error_str(reason));
	x25_link_terminated(lapb->L3_ptr);
}

/* Called by LAPB to inform X25 that connection was terminated by the remote system */
void lapb_disconnect_indication_cb(struct lapb_cs * lapb, int reason) {
	//char * buffer;
	//int buffer_size;

	custom_debug(0, "[LAPB_CB] disconnect_indication event is called(%s)", lapb_error_str(reason));
	x25_link_terminated(lapb->L3_ptr);
//	buffer = lapb_dequeue(lapb, &buffer_size);
//	if (buffer) {
//		printf("\nUnacked data:\n");
//		printf("%s\n", buf_to_str(buffer, buffer_size));
//		while ((buffer = lapb_dequeue(lapb, &buffer_size)) != NULL)
//			printf("%s\n", buf_to_str(buffer, buffer_size));
//	};
}

/* Called by LAPB to inform X25 about new data */
int lapb_data_indication_cb(struct lapb_cs * lapb, char * data, int data_size) {
	custom_debug(0, "[LAPB_CB] received new data");
	x25_link_receive_data(lapb->L3_ptr, data, data_size);
	//printf("%s\n", buf_to_str(data, data_size));
	return 0;
}





/* Called by X25 to inform App that Call request was initiated by the remote system */
void x25_call_indication_cb(struct x25_cs * x25) {
	(void)x25;
	custom_debug(0, "[X25_CB] call_indication event is called");
}

/* Called by X25 to inform App that Call request is confirmed */
void x25_call_accepted_cb(struct x25_cs * x25) {
	(void)x25;
	custom_debug(0, "[X25_CB] call_accepted event is called");
}

/* Called by X25 to inform App that Call request is confirmed */
int x25_data_indication_cb(struct x25_cs * x25, char * data, int data_size) {
	(void)x25;
	custom_debug(0, "[X25_CB] data_indication event is called");
	printf("%s\n", buf_to_str(data, data_size));
	return 0;
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

int wait_stdin(struct x25_cs * x25, _uchar break_condition, int run_once) {
	fd_set			read_set;
	struct timeval	timeout;
	int sr = 0;

	while (1) {
		FD_ZERO(&read_set);
		FD_SET(fileno(stdin), &read_set);
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		sr = select(fileno(stdin) + 1, &read_set, NULL, NULL, &timeout);
		if ((sr > 0) || (run_once))
			break;
		if ((x25) && (x25_get_state(x25) != break_condition))
			break;
	};
	return sr;
}



void print_commands_0(struct x25_cs * x25) {
	if (x25->mode & X25_EXTENDED)
		printf("Disconnected state(modulo 128):\n");
	else
		printf("Disconnected state(modulo 8):\n");
	printf("1 Make Call to...\n");
	printf("--\n");
	printf("0 Exit application\n");
	fprintf(stderr, ">");
}

void print_commands_1(struct x25_cs * x25) {
	if (x25->mode & X25_EXTENDED)
		printf("Awaiting connection state(modulo 128):\n");
	else
		printf("Awaiting connection state(modulo 8):\n");
	printf("1 Cancel\n");
	printf("--\n");
	printf("0 Exit application\n");
	fprintf(stderr, ">");
}

void print_commands_2(struct x25_cs * x25) {
	if (x25->mode & X25_EXTENDED)
		printf("Awaiting disconnection state(modulo 128):\n");
	else
		printf("Awaiting disconnection state(modulo 8):\n");
	printf("1 Cancel\n");
	printf("--\n");
	printf("0 Exit application\n");
	fprintf(stderr, ">");
}

void print_commands_3(struct x25_cs * x25) {
	if (x25->mode & X25_EXTENDED)
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







void x25_state_0(struct x25_cs *x25, struct x25_address * dest_addr) {
	int wait_stdin_result;;
	char buffer[256];
	int res;

	int while_flag;
	while_flag = TRUE;
	print_commands_0(x25);
	while (while_flag) {
		wait_stdin_result = wait_stdin(x25, X25_STATE_0, FALSE);
		if (wait_stdin_result <= 0) {
			if (x25_get_state(x25) != X25_STATE_0) {
				printf("\n\n");
				break;
			};
			sleep_ms(100);
			continue;
		};
		bzero(buffer, sizeof(buffer));
		while (read(0, buffer, sizeof(buffer)) <= 1)
			fprintf(stderr, ">");
		printf("\n");
		int action = atoi(buffer);
		switch (action) {
			case 1:  /* Call request */
				printf("Enter local X25 address[%s]: ", dest_addr->x25_addr);
				struct x25_address addr;
				fgets(addr.x25_addr, 16, stdin);
				int len = strlen(addr.x25_addr);
				if (len == 1)
					sprintf(addr.x25_addr, dest_addr->x25_addr);
				else
					addr.x25_addr[len - 1] = 0;
				/* Lock resources */
				main_lock();
				res = x25_call_request(x25, &addr);
				/* Unlock resources */
				main_unlock();
				if (res != X25_OK) {
					printf("ERROR: %s\n\n", x25_error_str(res));
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


void x25_state_1(struct x25_cs *x25) {
	int while_flag;
	int wait_stdin_result;
	char buffer[256];

	while_flag = TRUE;
	while (while_flag) {
		print_commands_1(x25);
		wait_stdin_result = wait_stdin(x25, X25_STATE_1, FALSE);
		if (wait_stdin_result <= 0) {
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

void x25_state_2(struct x25_cs *x25) {
	int while_flag;
	int wait_stdin_result;
	char buffer[256];

	while_flag = TRUE;
	while (while_flag) {
		print_commands_2(x25);
		wait_stdin_result = wait_stdin(x25, X25_STATE_2, FALSE);
		if (wait_stdin_result <= 0) {
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


void x25_state_3(struct x25_cs *x25) {
	int while_flag;
	int wait_stdin_result;
	char buffer[256];
	int n;
	int res = X25_OK;

	while_flag = TRUE;
	print_commands_3(x25);
	while (while_flag) {
		fprintf(stderr, ">");
		wait_stdin_result = wait_stdin(x25, X25_STATE_3, FALSE);
		if (wait_stdin_result <= 0) {
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
					data_size = 128;
					n = 0;
					while (n < data_size) {
						buffer[n] = 'a'; // + n;
						n++;
					};
				} else
					data_size = strlen(buffer) - 1;
				buffer[data_size] = 0;
				main_lock();
				res = x25_sendmsg(x25, buffer, data_size, FALSE, FALSE);
				main_unlock();
				if (res < 0) {
					printf("ERROR: %s\n\n", x25_error_str(res));
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
					res = x25_sendmsg(x25, buffer, data_size, FALSE, FALSE);
					if (res < 0) {
						//if (res != X25_BUSY) {
							printf("ERROR: %s\n", x25_error_str(res));
							break;
						//};
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
				main_lock();
				res = x25_clear_request(x25);
				main_unlock();
				if (res != X25_OK) {
					printf("ERROR: %s\n\n", x25_error_str(res));
					//lapb_reset(lapb, LAPB_STATE_0);
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


void main_loop(struct x25_cs * x25, struct x25_address *dest_addr) {
	error_type = 0; /* No errors */
	error_counter = 1;
	while (!exit_flag) {
		switch (x25_get_state(x25)) {
			case X25_STATE_0:
				x25_state_0(x25, dest_addr);
				break;
			case X25_STATE_1:
				x25_state_1(x25);
				break;
			case X25_STATE_2:
				x25_state_2(x25);
				break;
			case X25_STATE_3:
				x25_state_3(x25);
				break;
			default:
				sleep_ms(1000);
		};
	};

}
