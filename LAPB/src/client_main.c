#include <signal.h>     /* for signal */
#include <syslog.h>		/* for syslog writing */
#include <stdarg.h>

#define INTERVAL 1000        /* number of milliseconds to go off */

#include "net_lapb.h"
#include "tcp_client.h"
#include "my_timer.h"

#include "common.h"

struct lapb_cb * lapb_client = NULL;








/*
 * LAPB callback functions for X.25
 *
*/
void connect_confirmation(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "connect_confirmation is called(%s)", lapb_error_str(reason));
}

void connect_indication(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "connect_indication is called(%s)", lapb_error_str(reason));
}

void disconnect_confirmation(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "disconnect_confirmation is called(%s)", lapb_error_str(reason));
}

void disconnect_indication(struct lapb_cb * lapb, int reason) {
	(void)lapb;
	syslog(LOG_NOTICE, "disconnect_indication is called(%s)", lapb_error_str(reason));
	//printf("DTE disconnected(%s)\n\n", lapb_error_str(reason));
}

int  data_indication(struct lapb_cb * lapb, char * data, int data_size) {
	(void)lapb;
	(void)data;
	(void)data_size;
	syslog(LOG_NOTICE, "data_indication is called");
	return 0;
}

void data_transmit(struct lapb_cb * lapb, struct lapb_buff *skb) {
	(void)lapb;
	syslog(LOG_NOTICE, "data_transmit is called");

	int n = write(tcp_client_socket(), skb->data, skb->data_size);
	if (n < 0)
		syslog(LOG_ERR, "ERROR writing to socket, %s", strerror(errno));
}


void start_t1timer(struct lapb_cb * lapb) {
	set_t1_value(lapb->t1 * 1000);
	set_t1_state(TRUE);
	syslog(LOG_NOTICE, "start_t1timer is called");
}

void stop_t1timer() {
	set_t1_state(FALSE);
	set_t1_value(0);
	syslog(LOG_NOTICE, "stop_t1timer is called");
}

void start_t2timer(struct lapb_cb * lapb) {
	set_t2_value(lapb->t2 * 1000);
	set_t2_state(TRUE);
	syslog(LOG_NOTICE, "start_t2timer is called");
}

void stop_t2timer() {
	set_t2_state(FALSE);
	set_t2_value(0);
	syslog(LOG_NOTICE, "stop_t2timer is called");
}

void lapb_debug(struct lapb_cb *lapb, int level, const char * format, ...) {
	(void)lapb;
	if (level < LAPB_DEBUG) {
		char buf[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buf, 256, format, argList);
		//sprintf(buf, format, argList);
		va_end(argList);
		syslog(LOG_NOTICE, buf);
	};
}






/*
 * Timer callback functions
 *
*/
void t1timer_expiry(unsigned long int lapb_addr) {
	struct lapb_cb * lapb = (struct lapb_cb *)lapb_addr;
	syslog(LOG_NOTICE, "Timer_1 expired");
	lapb_t1timer_expiry(lapb);
}

void t2timer_expiry(unsigned long int lapb_addr) {
	struct lapb_cb * lapb = (struct lapb_cb *)lapb_addr;
	syslog(LOG_NOTICE, "Timer_2 expired");
	lapb_t2timer_expiry(lapb);
}

/*
 * TCP client callback functions
 *
*/
void new_data_received(char * data, int data_size) {
	main_lock();
	syslog(LOG_NOTICE, "data_received is called");
	lapb_data_received(lapb_client, data, data_size);
	main_unlock();
}

void connection_lost() {
	lapb_reset(lapb_client, LAPB_NOT_READY);
	//lapb_client->state = LAPB_NOT_READY;
//	printf("\nPhysical connection lost\n");
//	printf("Reconnecting\n\n");
}




int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	int ret;
	char buffer[2048];

	unsigned char lapb_equipment_type = LAPB_DTE;
	unsigned char lapb_modulo = LAPB_STANDARD;

	struct lapb_register_struct callbacks;
	int lapb_res;

	pthread_t client_thread;
	struct tcp_client_struct * client_struct = NULL;

	pthread_t timer_thread;
	struct timer_struct * timer_struct = NULL;


	printf("*******************************************\n");
	printf("******                               ******\n");
	printf("******  LAPB EMULATOR (client side)  ******\n");
	printf("******                               ******\n");
	printf("*******************************************\n");

	/* Initialize syslog */
	setlogmask (LOG_UPTO (LOG_DEBUG));
	openlog ("client_app", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	syslog (LOG_NOTICE, "Program started by User %d", getuid ());

	/* Setup signal handler */
	setup_signals_handler();

	/* Init mutex for sinchronization */
	pthread_mutex_init(&main_mutex, NULL);

	/* Set up equipment type: DTE or DCE */
	printf("\nSelect equipment type:\n1. DTE\n2. DCE\n");
	write(0, ">", 1);
	while (read(0, buffer, sizeof(buffer)) <= 1)
		write(0, ">", 1);
	if (atoi(buffer) == 2)
		lapb_equipment_type = LAPB_DCE;

	/* Set up lapb modulo: STANDARD or EXTENDED */
	printf("\nSelect modulo value:\n1. STANDARD(8)\n2. EXTENDED(128)\n");
	write(0, ">", 1);
	while (read(0, buffer, sizeof(buffer)) <= 1)
		write(0, ">", 1);
	if (atoi(buffer) == 2)
		lapb_modulo = LAPB_EXTENDED;

	/* Create TCP client */
	client_struct = malloc(sizeof(struct tcp_client_struct));
	printf("\nEnter remote server address[127.0.0.1]: ");
	client_struct->server_address = calloc(16, 1);
	bzero(client_struct->server_address, 16);
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
	/* TCP client callbacks */
	client_struct->new_data_received = new_data_received;
	client_struct->connection_lost = connection_lost;

	tcp_client_init();
	ret = pthread_create(&client_thread, NULL, client_function, (void*)client_struct);
	if (ret) {
		syslog(LOG_ERR, "Error - pthread_create() return code: %d\n", ret);
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("TCP client thread created(code %d)\n", ret);
	while (!is_client_started())
		sleep_ms(200);
	printf("TCP client started\n");



	/* LAPB init */
	bzero(&callbacks, sizeof(struct lapb_register_struct));
	callbacks.connect_confirmation = connect_confirmation;
	callbacks.connect_indication = connect_indication;
	callbacks.disconnect_confirmation = disconnect_confirmation;
	callbacks.disconnect_indication = disconnect_indication;
	callbacks.data_indication = data_indication;
	callbacks.data_transmit = data_transmit;
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

	/* Create timer */
	timer_struct = malloc(sizeof(struct timer_struct));
	timer_struct->interval = 100; // milliseconds
	timer_struct->lapb_addr = (unsigned long int)lapb_client;

	/* Timer callbacks */
	timer_struct->t1timer_expiry = t1timer_expiry;
	timer_struct->t2timer_expiry = t2timer_expiry;

	timer_init();
	ret = pthread_create(&timer_thread, NULL, timer_function, (void*)timer_struct);
	if (ret) {
		syslog(LOG_ERR, "Error - pthread_create() return code: %d\n", ret);
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
	main_loop(lapb_client, &m_callbacks, lapb_equipment_type, lapb_modulo);


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


	/* Close syslog */
	closelog();
	printf("Exit application\n\n");

	return EXIT_SUCCESS;

}

