#include <signal.h>     /* for signal */


#include "lapb_iface.h"

#include "tcp_client.h"
#include "my_timer.h"
#include "logger.h"

#include "common.h"

struct lapb_cb * lapb_client = NULL;





/*
 * LAPB callback functions for X.25
 *
*/

void transmit_data(struct lapb_cb * lapb, char *data, int data_size) {
	(void)lapb;
	lapb_debug(lapb, 0, "[LAPB] data_transmit is called");

	char buffer[1024];
	buffer[0] = 0x7E; /* Open flag */
	memcpy(&buffer[1], data, data_size);
	buffer[data_size + 1] = 0x7E; /* Close flag */
	int n = send(tcp_client_socket(), buffer, data_size + 2, MSG_NOSIGNAL);
	if (n < 0)
		lapb_debug(lapb, 0, "[LAPB] ERROR writing to socket, %s", strerror(errno));
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
				main_lock();
				lapb_debug(NULL, 0, "[PHYS_CB] data_received is called(%d bytes)", block_size);
				lapb_data_received(lapb_client, buffer, block_size, 0);
				main_unlock();
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

void connection_lost() {
	char * buffer;
	int buffer_size;
	main_lock();
	//lapb_debug(NULL, 0, "[X25_CB] disconnected event is called(%s)", lapb_error_str(reason));
	if (lapb_client->write_queue.count) {
		printf("\n\nUnacked data:\n");
		while ((buffer = lapb_dequeue(lapb_client, &buffer_size)) != NULL) {
			printf("%s\n", buf_to_str(buffer, buffer_size));
		};
	};
	lapb_reset(lapb_client, LAPB_NOT_READY);
	main_unlock();
}




int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	int ret;
	char buffer[2048];

	int dbg = TRUE;

	unsigned char lapb_equipment_type = LAPB_DTE;
	unsigned char lapb_modulo = LAPB_STANDARD;

	struct lapb_register_struct callbacks;
	int lapb_res;

	pthread_t client_thread;
	struct tcp_client_struct * client_struct = NULL;

	pthread_t timer_thread;
	struct timer_struct * timer_struct = NULL;

	pthread_t logger_thread;


	printf("*******************************************\n");
	printf("******                               ******\n");
	printf("******  LAPB EMULATOR (client side)  ******\n");
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

	lapb_debug(NULL, 0, "Program started by User %d", getuid ());

	/* Setup signal handler */
	setup_signals_handler();

	/* Init mutex for sinchronization */
	pthread_mutex_init(&main_mutex, NULL);

	if (dbg)
		goto label_1;

	/* Set up equipment type: DTE or DCE */
	fprintf(stderr, "\nSelect equipment type:\n1. DTE\n2. DCE\n>");
	while (read(0, buffer, sizeof(buffer)) <= 1)
		fprintf(stderr, ">");
	if (atoi(buffer) == 2)
		lapb_equipment_type = LAPB_DCE;

	/* Set up lapb modulo: STANDARD or EXTENDED */
	fprintf(stderr, "\nSelect modulo value:\n1. STANDARD(8)\n2. EXTENDED(128)\n>");
	while (read(0, buffer, sizeof(buffer)) <= 1)
		fprintf(stderr, ">");
	if (atoi(buffer) == 2)
		lapb_modulo = LAPB_EXTENDED;

label_1:

	/* Create TCP client */
	client_struct = malloc(sizeof(struct tcp_client_struct));
	client_struct->server_address = calloc(16, 1);
	//bzero(client_struct->server_address, 16);

	if (dbg) {
		memcpy(client_struct->server_address, "127.0.0.1", 9);
		client_struct->server_port = 1234;
		goto label_2;
	};

	printf("\nEnter remote server address[127.0.0.1]: ");
	fgets(client_struct->server_address, 16, stdin);
	int tmp_len = strlen(client_struct->server_address);
	if (tmp_len == 1)
		memcpy(client_struct->server_address, "127.0.0.1", 9);
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

	//tcp_client_init();
	ret = pthread_create(&client_thread, NULL, client_function, (void*)client_struct);
	if (ret) {
		lapb_debug(NULL, 0, "Error - pthread_create() return code: %d\n", ret);
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("TCP client thread created(code %d)\n", ret);
	while (!is_client_started())
		sleep_ms(200);
	printf("TCP client started\n");



	/* LAPB init */
	bzero(&callbacks, sizeof(struct lapb_register_struct));
	callbacks.on_connected = on_connected;
	//callbacks.connect_indication = connect_indication;
	//callbacks.disconnect_confirmation = disconnect_confirmation;
	callbacks.on_disconnected = on_disconnected;
	callbacks.on_new_data = on_new_incoming_data;
	callbacks.transmit_data = transmit_data;
	callbacks.start_t1timer = start_t1timer;
	callbacks.stop_t1timer = stop_t1timer;
	callbacks.start_t2timer = start_t2timer;
	callbacks.stop_t2timer = stop_t2timer;
	callbacks.debug = lapb_debug;
	lapb_res = lapb_register(&callbacks, lapb_modulo, LAPB_SLP, lapb_equipment_type, &lapb_client);
	if (lapb_res != LAPB_OK) {
		printf("lapb_register return %d\n", lapb_res);
		exit(EXIT_FAILURE);
	};
	//lapb_client->mode = lapb_modulo | LAPB_SLP | lapb_equipment_type;
	/* Redefine some default values */
	lapb_client->T1 = 5000; /* 5s */
	lapb_client->T2 = 500;  /* 0.5s */
	lapb_client->N2 = 3; /* Try 3 times */

	/* Create timer */
	timer_struct = malloc(sizeof(struct timer_struct));
	timer_struct->interval = 50; // milliseconds
	timer_struct->lapb_addr = (unsigned long int)lapb_client;

	/* Timer callbacks */
	timer_struct->t1timer_expiry = t1timer_expiry;
	timer_struct->t2timer_expiry = t2timer_expiry;

	//timer_init();
	ret = pthread_create(&timer_thread, NULL, timer_function, (void*)timer_struct);
	if (ret) {
		lapb_debug(NULL, 0, "Error - pthread_create() return code: %d\n", ret);
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("Timer thread created(code %d)\n", ret);
	while (!is_timer_started())
		sleep_ms(200);
	printf("Timer started\n\n");

	struct main_callbacks m_callbacks;
	bzero(&m_callbacks, sizeof(struct main_callbacks));
	m_callbacks.is_connected = is_client_connected;
	//m_callbacks.print_commands_0 = print_commands_0;
	//m_callbacks.print_commands_1 = print_commands_1;
	//m_callbacks.print_commands_2 = print_commands_2;
	//m_callbacks.print_commands_3 = print_commands_3;
	//main_loop(lapb_client, &m_callbacks, lapb_equipment_type, lapb_modulo);
	main_loop(lapb_client, &m_callbacks);


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

	terminate_timer();
	while (is_timer_started())
		sleep_ms(200);
	printf("Timer stopped\n");
	pthread_join(timer_thread, (void **)&thread_result);
	printf("Timer thread exit(code %d)\n", *thread_result);
	free(thread_result);
	if (timer_struct != NULL)
		free(timer_struct);

	lapb_unregister(lapb_client);

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

