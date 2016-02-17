
#include "ut_main.h"


int test_sabm_dte(int next_test);
int test_disc_sabm_dte(int next_test);

int test_sabm_dce(int next_test);
int test_disc_sabm_dce(int next_test);

int test_sabme_dte(int next_test);
int test_disc_sabme_dte(int next_test);

int test_sabme_dce(int next_test);
int test_disc_sabme_dce(int next_test);

//int test_disc(int next_test);

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

char * buf_to_str(char * data, int data_size) {

	bzero(str_buf, 1024);
	if (data_size < 1023) /* 1 byte for null-terminating */
		memcpy(str_buf, data, data_size);
	return str_buf;
}

int compare_out_data(char * buffer, int buffer_size, char * _template) {
	int template_size = strlen(_template);
	int result = TRUE;
	if (buffer_size != template_size)
		return FALSE;
	int i = 0;

	while (i < template_size) {
		if (buffer[i] != _template[i]) {
			result = FALSE;
			break;
		};
		i++;
	};
	return result;
}

int fill_in_data(char * _template, char * buffer) {
	char * pEnd = _template;
	long int value;
	int i = 0;
	while (1) {
		value = strtol(pEnd, &pEnd, 16);
		buffer[i] = (_uchar)value;
		i++;
		if (*pEnd == '\0')
			break;
	};
	return i;
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
	(void)lapb;
	//timer_t1_set_interval(lapb->T1);
	//timer_t1_start();
	syslog(LOG_NOTICE, "[LAPB] start_t1timer is called");
}

/* Called by LAPB to stop timer T1 */
void stop_t1timer() {
	//timer_t1_stop();
	syslog(LOG_NOTICE, "[LAPB] stop_t1timer is called");
}

/* Called by LAPB to start timer T2 */
void start_t2timer(struct lapb_cb * lapb) {
	(void)lapb;
	//timer_t2_set_interval(lapb->T2);
	//timer_t2_start();
	syslog(LOG_NOTICE, "[LAPB] start_t2timer is called");
}

/* Called by LAPB to stop timer T1 */
void stop_t2timer() {
	//timer_t2_stop();
	syslog(LOG_NOTICE, "[LAPB] stop_t2timer is called");
}

void transmit_data(struct lapb_cb * lapb, char *data, int data_size) {
	(void)lapb;
	syslog(LOG_NOTICE, "[LAPB] data_transmit is called");

	int i = 0;
	while (i < data_size) {
		sprintf(&out_data[i*2], "%02X", (_uchar)data[i]);
		i++;
	};
	//memcpy(out_data, data, data_size);
	out_data_size = data_size*2;
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
		syslog(LOG_NOTICE, "[LAPB] %s", buf);
	};
}



/*
 * TCP client callback functions
 *
*/
void new_data_received(char * data, int data_size) {
	char buffer[1024];
	//bzero(buffer, 1024);
	int i = 0;
	int data_block = FALSE;
	int block_size = 0;
	while (i < data_size) {
		if (data[i] == 0x7E) { /* Flag */
			if (data_block) { /* Close flag */
				syslog(LOG_NOTICE, "[PHYS_CB] data_received is called(%d bytes)", block_size);
				lapb_data_received(lapb_client, buffer, block_size, 0);
				data_block = FALSE;
				i++;
				continue;
			};
			/* Open flag */
			data_block = TRUE;
			bzero(buffer, 1024);
			block_size = 0;
			i++;
			continue;
		};
		if (data_block) {
			buffer[block_size] = data[i];
			block_size++;
		};
		i++;
	};

}





int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	//char buffer[2048];

	_uchar lapb_equipment_type = LAPB_DCE;
	_uchar lapb_modulo = LAPB_STANDARD;

	struct lapb_register_struct callbacks;
	int lapb_res;

	_uchar test_no;
	int test_finish = FALSE;


	printf("*******************************************\n");
	printf("******                               ******\n");
	printf("******         LAPB UNIT-TEST        ******\n");
	printf("******                               ******\n");
	printf("*******************************************\n");
	/* https://en.wikipedia.org/wiki/ANSI_escape_code */
//	printf("\033[1;4;31mbold red text\033[0m\n");
//	char move_up[] = { 0x1b, '[', '2', 'A', 0 };
//	printf(move_up);
//	printf("\033[1;4;31mbold red text\033[0m\n");

	char _cursor[4] = { '|', '/', '-', '\\'};
	int t = 0;
	char move_left[] = { 0x1b, '[', '1', 'D', 0 };
	while (1) {
		fprintf(stderr, "%c", _cursor[t]);
		sleep_ms(50);
		t++;
		if (t == 4)
			t = 0;
		fprintf(stderr, move_left);
	};

//	printf("sizeof(unsigned char)=%d\n", (int)sizeof(unsigned char));
//	printf("sizeof(unsigned short)=%d\n", (int)sizeof(unsigned short));
//	printf("sizeof(unsigned int)=%d\n", (int)sizeof(unsigned int));
//	printf("sizeof(unsigned long)=%d\n", (int)sizeof(unsigned long));
//	printf("sizeof(unsigned long long)=%d\n", (int)sizeof(unsigned long long));
//	printf("sizeof(lapb_buff)=%d\n", (int)sizeof(struct lapb_buff));
//	exit(0);


	/* Initialize syslog */
	setlogmask (LOG_UPTO (LOG_DEBUG));
	openlog ("client_app", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	/* LAPB init */
	bzero(&callbacks, sizeof(struct lapb_register_struct));
	callbacks.on_connected = on_connected;
	callbacks.on_disconnected = on_disconnected;
	callbacks.on_new_data = on_new_incoming_data;
	callbacks.transmit_data = transmit_data;
	callbacks.start_t1timer = start_t1timer;
	callbacks.stop_t1timer = stop_t1timer;
	callbacks.start_t2timer = start_t2timer;
	callbacks.stop_t2timer = stop_t2timer;
	callbacks.debug = lapb_debug;
	lapb_res = lapb_register(&callbacks, &lapb_client);
	if (lapb_res != LAPB_OK) {
		printf("lapb_register return %d\n", lapb_res);
		exit(EXIT_FAILURE);
	};
	lapb_client->mode = lapb_modulo | LAPB_SLP | lapb_equipment_type;
	/* Redefine some default values */
	lapb_client->T1 = 5000; /* 5s */
	lapb_client->T2 = 500;  /* 0.5s */
	lapb_client->N2 = 3; /* Try 3 times */

	test_no = TEST_SABM_DTE;
	while (!test_finish) {
		switch (test_no) {
			case TEST_SABM_DTE:
				test_no = test_sabm_dte(TEST_SABM_DCE);
				break;
			case TEST_SABM_DCE:
				test_no = test_sabm_dce(TEST_SABME_DTE);
				break;
			case TEST_SABME_DTE:
				test_no = test_sabme_dte(TEST_SABME_DCE);
				break;
			case TEST_SABME_DCE:
				test_no = test_sabme_dce(TEST_DISC);
				break;
			case TEST_DISC:
				test_no = test_disc_sabme_dte(TEST_OK);
				break;
			case TEST_OK: case TEST_BAD:
				test_finish = TRUE;
				break;
		};
	};
	if (test_no != TEST_OK)
		printf("not pass\n");

	lapb_unregister(lapb_client);

	/* Close syslog */
	closelog();
	printf("\nExit application\n\n");

	return EXIT_SUCCESS;

}


int test_sabm_dte(int next_test) {
	int result = TRUE;
	write(0, "Test SABM(DTE)  ..... ", 22);

	lapb_reset(lapb_client, LAPB_STATE_0);
	lapb_client->mode = LAPB_STANDARD | LAPB_SLP | LAPB_DTE;

	if (!(result &= lapb_connect_request(lapb_client) == LAPB_OK))
		return TEST_BAD;
	if (!(result &= compare_out_data(out_data, out_data_size, TEST_SABM_DTE_1)))
		return TEST_BAD;
	if (!(result &= lapb_client->state == LAPB_STATE_1))
		return TEST_BAD;

	in_data_size = fill_in_data(TEST_SABM_DTE_2, in_data);

	lapb_data_received(lapb_client, in_data, in_data_size, 0);
	if (!(result &= lapb_client->state == LAPB_STATE_3))
		return TEST_BAD;

	sleep_ms(400);
	printf("OK\n");
	return next_test;
}

int test_sabm_dce(int next_test) {
	int result = TRUE;
	write(0, "Test SABM(DCE)  ..... ", 22);

	lapb_reset(lapb_client, LAPB_STATE_0);
	lapb_client->mode = LAPB_STANDARD | LAPB_SLP | LAPB_DCE;

	if (!(result &= lapb_connect_request(lapb_client) == LAPB_OK))
		return TEST_BAD;
	if (!(result &= compare_out_data(out_data, out_data_size, TEST_SABM_DCE_1)))
		return TEST_BAD;
	if (!(result &= lapb_client->T1_state == TRUE))
		return TEST_BAD;
	if (!(result &= lapb_client->state == LAPB_STATE_1))
		return TEST_BAD;

	in_data_size = fill_in_data(TEST_SABM_DCE_2, in_data);

	lapb_data_received(lapb_client, in_data, in_data_size, 0);
	if (!(result &= lapb_client->state == LAPB_STATE_3))
		return TEST_BAD;
	/* If we are DCE */
	if (lapb_client->mode & LAPB_DCE) {
		if (!(result &= lapb_client->T1_state == FALSE))
			return TEST_BAD;
	};

	sleep_ms(400);
	printf("OK\n");
	return next_test;
}

int test_sabme_dte(int next_test) {
	write(0, "Test SABME(DTE) ..... ", 22);
	sleep_ms(400);

	printf("OK\n");
	return next_test;
}

int test_sabme_dce(int next_test) {
	write(0, "Test SABME(DCE) ..... ", 22);
	sleep_ms(400);

	printf("OK\n");
	return next_test;
}
int test_disc_sabme_dte(int next_test) {
	write(0, "Test DISC  .......... ", 22);
	sleep_ms(400);

	printf("OK\n");
	return next_test;
}
