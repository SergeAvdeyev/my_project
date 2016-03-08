#include <signal.h>     /* for signal */


#include "x25_iface.h"
#include "lapb_iface.h"

#include "tcp_client.h"
#include "my_timer.h"
#include "logger.h"

#include "common.h"

struct x25_cs * x25_client = NULL;
struct lapb_cs * lapb_client = NULL;


char in_buffer[4096];
int data_block = FALSE;
int block_size = 0;




/*
 * LAPB callback functions for X.25
 *
*/

void lapb_transmit_data(struct lapb_cs * lapb, char *data, int data_size) {
	(void)lapb;
	if (!is_client_connected()) return;
	//lapb_debug(lapb, 0, "[LAPB] data_transmit is called");

	char buffer[1024];
	buffer[0] = 0x7E; /* Open flag */
	buffer[1] = 0x7E; /* Open flag */
	memcpy(&buffer[2], data, data_size);
	if (error_type == 1) { /* Bad FCS (for I frames) */
		int i_frame = ~(*(_uchar *)&buffer[3] & 0x01);
		if (i_frame && (error_counter == 0))
			*(_ushort *)&buffer[data_size + 2] = 1; /* Bad FCS */
		else
			*(_ushort *)&buffer[data_size + 2] = 0; /* Good FCS */
	} else if (error_type == 2) { /* Bad N(R) (for I or S frames) */
		int if_nr_present = *(_uchar *)&buffer[3];
		if_nr_present = if_nr_present & 0x03;
		if ((if_nr_present != 0x03) && (error_counter == 0))
			*(_uchar *)&buffer[3] |= 0xE0; /* Bad N(R) */
		*(_ushort *)&buffer[data_size + 2] = 0; /* Good FCS */
	} else
		*(_ushort *)&buffer[data_size + 2] = 0; /* Good FCS */
	buffer[data_size + 4] = 0x7E; /* Close flag */
	buffer[data_size + 5] = 0x7E; /* Close flag */

	int n = send(tcp_client_socket(), buffer, data_size + 6, MSG_NOSIGNAL);
	if (n < 0)
		custom_debug(0, "[LAPB] ERROR writing to socket, %s", strerror(errno));
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

	int dbg = TRUE;

	unsigned char lapb_equipment_type = LAPB_DTE;
	unsigned char lapb_modulo = LAPB_STANDARD;

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

	custom_debug(0, "Program started by User %d", getuid ());

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
		custom_debug(0, "Error - pthread_create() return code: %d\n", ret);
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
		custom_debug(0, "Error - pthread_create() return code: %d\n", ret);
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("Timer thread created(code %d)\n", ret);
	while (!is_timer_thread_started())
		sleep_ms(200);
	printf("Timer started\n\n");


	/* LAPB init */
	struct lapb_callbacks lapb_callbacks;
	int res;
	bzero(&lapb_callbacks, sizeof(struct lapb_callbacks));
	lapb_callbacks.connect_confirmation = lapb_connect_confirmation_cb;
	lapb_callbacks.connect_indication = lapb_connect_indication_cb;
	lapb_callbacks.disconnect_confirmation = lapb_disconnect_confirmation_cb;
	lapb_callbacks.disconnect_indication = lapb_disconnect_indication_cb;
	lapb_callbacks.data_indication = lapb_data_indication_cb;
	lapb_callbacks.transmit_data = lapb_transmit_data;

	lapb_callbacks.add_timer = timer_add;
	lapb_callbacks.del_timer = timer_del;
	lapb_callbacks.start_timer = timer_start;
	lapb_callbacks.stop_timer = timer_stop;

	lapb_callbacks.debug = custom_debug;

	/* Define LAPB values */
	struct lapb_params lapb_params;
	lapb_params.mode = lapb_modulo | LAPB_SLP | lapb_equipment_type;
	lapb_params.window = LAPB_DEFAULT_SWINDOW;
	lapb_params.N1 = LAPB_DEFAULT_N1;	/* I frame size is 135 bytes */
	lapb_params.T201_interval = 1000;	/* 1s */
	lapb_params.T202_interval = 100;		/* 0.1s */
	lapb_params.N201 = 10;					/* T201 timer will repeat for 10 times */
	lapb_params.low_order_bits = FALSE;
	lapb_params.auto_connecting = TRUE;

	res = lapb_register(&lapb_callbacks, &lapb_params, &lapb_client);
	if (res != LAPB_OK) {
		printf("lapb_register return %d\n", res);
		exit(EXIT_FAILURE);
	};




	/* X25 init */
	struct x25_callbacks x25_callbacks;
	bzero(&x25_callbacks, sizeof(struct x25_callbacks));
//	x25_callbacks.connect_confirmation = connect_confirmation;
//	x25_callbacks.connect_indication = connect_indication;
//	x25_callbacks.disconnect_confirmation = disconnect_confirmation;
//	x25_callbacks.disconnect_indication = disconnect_indication;
//	x25_callbacks.data_indication = data_indication;
//	x25_callbacks.transmit_data = transmit_data;

	x25_callbacks.add_timer = timer_add;
	x25_callbacks.del_timer = timer_del;
	x25_callbacks.start_timer = timer_start;
	x25_callbacks.stop_timer = timer_stop;

	x25_callbacks.debug = custom_debug;

	/* Define X25 values */
	struct x25_params x25_params;

	res = x25_register(&x25_callbacks, &x25_params, &x25_client);
	if (res != X25_OK) {
		printf("x25_register return %d\n", res);
		exit(EXIT_FAILURE);
	};
	x25_add_link(x25_client, lapb_client, 0);

	struct x25_address addr;
	sprintf(addr.x25_addr, "7654321");
	//x25_client->lci = 1 << 8 | 1;
	x25_client->lci = 1;
	sprintf((char *)&x25_client->source_addr, "1234567");
	x25_connect_request(x25_client, &addr);

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

