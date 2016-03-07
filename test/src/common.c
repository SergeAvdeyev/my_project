
#include "my_timer.h"
#include "common.h"
#include "logger.h"

char str_buf[1024];
int break_flag = FALSE;
//int out_counter;
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

	// Will always fail, SIGKILL is intended to force kill your process
	//if (sigaction(SIGKILL, &sa, NULL) == -1) {
	//	perror("Cannot handle SIGKILL"); // Will always happen
	//	printf("You can never handle SIGKILL anyway...\n");
	//};

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
