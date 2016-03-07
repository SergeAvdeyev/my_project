#include <signal.h>     /* for signal */


#include "x25_iface.h"

#include "tcp_client.h"
#include "my_timer.h"
#include "logger.h"

#include "common.h"

struct x25_cs * x25_client = NULL;

char in_buffer[4096];
int data_block = FALSE;
int block_size = 0;




/*
 * callback functions for X.25
 *
*/

void transmit_data(struct x25_cs * lapb, char *data, int data_size) {
	(void)lapb;
	(void)data;
	(void)data_size;
}




/*
 * TCP client callback functions
 *
*/
void new_data_received(char * data, int data_size) {
	(void)data;
	int i = 0;
	//_ushort rcv_fcs;

	while (i < data_size) {

		i++;
	};
}

void connection_lost() {
//	char * buffer;
//	int buffer_size;

	printf("\nUnacked data:\n");
//	while ((buffer = x25_dequeue(x25_client, &buffer_size)) != NULL)
//		printf("%s\n", buf_to_str(buffer, buffer_size));

//	x25_reset(x25_client, LAPB_STATE_0);
}




int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	int ret;
	char buffer[2048];

	int dbg = FALSE;

//	unsigned char lapb_equipment_type = LAPB_DTE;
//	unsigned char lapb_modulo = LAPB_STANDARD;

	struct x25_callbacks callbacks;
	int x25_res;

	pthread_t client_thread;
	struct tcp_client_struct * client_struct = NULL;

	pthread_t timer_thread;
	struct timer_thread_struct * timer_struct = NULL;

	pthread_t logger_thread;

	printf("*******************************************\n");
	printf("******                               ******\n");
	printf("******  X25 EMULATOR (client side)  ******\n");
	printf("******                               ******\n");
	printf("*******************************************\n");

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

	ret = pthread_create(&logger_thread, NULL, logger_function, NULL);
	if (ret) {
		perror("Error - pthread_create()");
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("Logger thread created(code %d)\n", ret);
	while (!is_logger_started())
		sleep_ms(200);
	printf("Logger started\n\n");

	//x25_debug(0, "Program started by User %d", getuid ());

	/* Setup signal handler */
	setup_signals_handler();


	if (dbg)
		goto label_1;

//	/* Set up equipment type: DTE or DCE */
//	fprintf(stderr, "\nSelect equipment type:\n1. DTE\n2. DCE\n>");
//	while (read(0, buffer, sizeof(buffer)) <= 1)
//		fprintf(stderr, ">");
//	if (atoi(buffer) == 2)
//		lapb_equipment_type = LAPB_DCE;

//	/* Set up lapb modulo: STANDARD or EXTENDED */
//	fprintf(stderr, "\nSelect modulo value:\n1. STANDARD(8)\n2. EXTENDED(128)\n>");
//	while (read(0, buffer, sizeof(buffer)) <= 1)
//		fprintf(stderr, ">");
//	if (atoi(buffer) == 2)
//		lapb_modulo = LAPB_EXTENDED;

label_1:

	/* Create TCP client */
	client_struct = malloc(sizeof(struct tcp_client_struct));
	client_struct->server_address = calloc(16, 1);

	if (dbg) {
		sprintf(client_struct->server_address, "127.0.0.1");
		client_struct->server_port = 1234;
		goto label_2;
	};

	printf("\nEnter remote server address[127.0.0.1]: ");
	fgets(client_struct->server_address, 16, stdin);
	int tmp_len = strlen(client_struct->server_address);
	if (tmp_len == 1)
		sprintf(client_struct->server_address, "127.0.0.1");
	else
		client_struct->server_address[tmp_len - 1] = 0;

	printf("\nEnter remote server port[1234]: ");
	fgets(buffer, sizeof(buffer) - 1, stdin);
	tmp_len = strlen(buffer);
	if (tmp_len == 1)
		client_struct->server_port = 1234;
	else {
		buffer[strlen(buffer) - 1] = 0;
		client_struct->server_port = atoi(buffer);
	};

label_2:

	/* TCP client callbacks */
	client_struct->new_data_received = new_data_received;
	client_struct->connection_lost = connection_lost;

	ret = pthread_create(&client_thread, NULL, client_function, (void*)client_struct);
	if (ret) {
		//x25_debug(0, "Error - pthread_create() return code: %d\n", ret);
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("TCP client thread created(code %d)\n", ret);
	while (!is_client_started())
		sleep_ms(200);
	printf("TCP client started\n");

	/* Create timer */
	timer_struct = malloc(sizeof(struct timer_thread_struct));
	timer_struct->interval = 10; /* milliseconds */
	//timer_struct->lapb_addr = (unsigned long int)lapb_client;
	//bzero(timer_struct->timers_list, sizeof(timer_struct->timers_list));
	ret = pthread_create(&timer_thread, NULL, timer_thread_function, (void*)timer_struct);
	if (ret) {
		//x25_debug(0, "Error - pthread_create() return code: %d\n", ret);
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("Timer thread created(code %d)\n", ret);
	while (!is_timer_thread_started())
		sleep_ms(200);
	printf("Timer started\n\n");


	/* X25 init */
	bzero(&callbacks, sizeof(struct x25_callbacks));
//	callbacks.connect_confirmation = connect_confirmation;
//	callbacks.connect_indication = connect_indication;
//	callbacks.disconnect_confirmation = disconnect_confirmation;
//	callbacks.disconnect_indication = disconnect_indication;
//	callbacks.data_indication = data_indication;
//	callbacks.transmit_data = transmit_data;

//	callbacks.add_timer = timer_add;
//	callbacks.del_timer = timer_del;
//	callbacks.start_timer = timer_start;
//	callbacks.stop_timer = timer_stop;

	callbacks.debug = custom_debug;

	/* Define X25 values */
	struct x25_params params;
//	params.mode = lapb_modulo | LAPB_SLP | lapb_equipment_type;
//	params.window = LAPB_DEFAULT_SWINDOW;
//	params.N1 = LAPB_DEFAULT_N1;	/* I frame size is 135 bytes */
//	params.T201_interval = 1000;	/* 1s */
//	params.T202_interval = 100;		/* 0.1s */
//	params.N201 = 10;					/* T201 timer will repeat for 10 times */
//	params.low_order_bits = FALSE;
//	params.auto_connecting = TRUE;

	x25_res = x25_register(&callbacks, &params, &x25_client);
	if (x25_res != X25_OK) {
		printf("x25_register return %d\n", x25_res);
		exit(EXIT_FAILURE);
	};

	/* Start endless loop */
	printf("Run Main loop\n\n");
	main_loop();

	printf("Main loop ended\n");

	terminate_tcp_client();
	while (is_client_started())
		sleep_ms(200);
	printf("TCP client stopped\n");
	int * thread_result;
	pthread_join(client_thread, (void **)&thread_result);
	printf("TCP client thread exit(code %d)\n", *thread_result);
	free(thread_result);
	if (client_struct != NULL) {
		free(client_struct->server_address);
		free(client_struct);
	};

	terminate_timer_thread();
	while (is_timer_thread_started())
		sleep_ms(200);
	printf("Timer stopped\n");
	pthread_join(timer_thread, (void **)&thread_result);
	printf("Timer thread exit(code %d)\n", *thread_result);
	free(thread_result);
	if (timer_struct != NULL)
		free(timer_struct);

	x25_unregister(x25_client);

	terminate_logger();
	while (is_logger_started())
		sleep_ms(200);
	printf("Logger stopped\n");
	pthread_join(logger_thread, (void **)&thread_result);
	printf("Logger thread exit(code %d)\n", *thread_result);
	free(thread_result);

	/* Close syslog */
	closelog();
	printf("Exit application\n\n");

	return EXIT_SUCCESS;
}
