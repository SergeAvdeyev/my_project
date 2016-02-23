
#include <string.h>
#include <signal.h>

#include "lapb_iface.h"

#include "tcp_server.h"
#include "my_timer.h"
#include "common.h"
#include "logger.h"


struct lapb_cs * lapb_server = NULL;

char in_buffer[4096];
int data_block = FALSE;
int block_size = 0;


int AutomaticMode = TRUE;

int manual_process();
int man_decode(struct lapb_cs *lapb, char *data, int data_size, struct lapb_frame *frame);
void print_man_commands();



void hex_dump(char * data, int data_size) {
	int i = 0;
	while (i < data_size) {
		printf("%02X ", (_uchar)data[i]);
		i++;
		if ((i % 32) == 0)
			printf("\n");
	};
	printf("\n");
}



///*
// * LAPB callback functions for X.25
// *

/* Called by LAPB to transmit data via physical connection */
void transmit_data(struct lapb_cs * lapb, char *data, int data_size) {
	(void)lapb;
	//lapb_debug(NULL, 0, "[LAPB] data_transmit is called");

	char buffer[1024];
	buffer[0] = 0x7E; /* Open flag */
	buffer[1] = 0x7E; /* Open flag */
	memcpy(&buffer[2], data, data_size);
	int if_nr_present = *(_uchar *)&buffer[3];
	if_nr_present = if_nr_present & 0x03;
	if ((if_nr_present != 0x03) && (fcs == 0))
		*(_ushort *)&buffer[data_size + 2] = 1; /* Bad FCS */
		//*(_uchar *)&buffer[3] |= 0xE0; /* Bad N(R) */
	else
		*(_ushort *)&buffer[data_size + 2] = 0; /* Good FCS */
	buffer[data_size + 4] = 0x7E; /* Close flag */
	buffer[data_size + 5] = 0x7E; /* Close flag */
	int n = send(tcp_client_socket(), buffer, data_size + 6, MSG_NOSIGNAL);
	if (n < 0)
		lapb_debug(NULL, 0, "[LAPB] ERROR writing to socket, %s", strerror(errno));
}






/*
 * TCP server callback functions
 *
*/
void manual_received(char * data, int data_size) {
	int i;
	struct lapb_frame frame;
	if (man_decode(lapb_server, data, data_size, &frame) == 0) {
		if (frame.type == LAPB_I) {
			printf("I(%d) S%d R%d '%s'", frame.pf, frame.ns, frame.nr, buf_to_str(&data[2], data_size -2));
			lapb_server->vr = frame.ns + 1;
		} else {
			for (i = 0; i < data_size; i++)
				printf(" %02X", (_uchar)data[i]);
			switch (frame.type) {
				case LAPB_SABM:
					printf(" - SABM(%d)", frame.pf);
					break;
				case LAPB_SABME:
					printf(" - SABME(%d)", frame.pf);
					break;
				case LAPB_DISC:
					printf(" - DISC(%d)", frame.pf);
					break;
				case LAPB_UA:
					printf(" - UA(%d)", frame.pf);
					break;
				case LAPB_DM:
					printf(" - DM(%d)", frame.pf);
					break;
				default:
					break;
			};
		};
	};

}

void new_data_received(char * data, int data_size) {
	int i = 0;
	_ushort rcv_fcs;

	//hex_dump(data, data_size);

	if (AutomaticMode) {
		while (i < data_size) {
			if (data[i] == 0x7E) { /* Flag */
				if (data[i + 1] == 0x7E) {
					if (data_block) { /* Close flag */
						data_block = FALSE;
						block_size -= 2; /* 2 bytes for FCS */
						rcv_fcs = *(_ushort *)&in_buffer[block_size];
						//lapb_debug(NULL, 0, "[PHYS_CB] data_received is called(%d bytes)", block_size);
						lapb_data_received(lapb_server, in_buffer, block_size, rcv_fcs);
					} else {
						/* Open flag */
						bzero(in_buffer, 1024);
						block_size = 0;
						data_block = TRUE;
					};
					i += 2;
					continue;
				} else {
					if (data_block) {
						in_buffer[block_size] = data[i];
						block_size++;
					} else {
						lapb_debug(NULL, 0, "[PHYS_CB] data error");
						break;
					};
				}

				i++;
			} else if (data_block) {
				in_buffer[block_size] = data[i];
				block_size++;
				i++;
			};
		};
	} else {
		printf("\nData received:");

		//bzero(buffer, 1024);
		while (i < data_size) {
			if (data[i] == 0x7E) { /* Flag */
				if (data_block) { /* Close flag */
					manual_received(in_buffer, block_size);
					data_block = FALSE;
					i++;
					continue;
				};
				/* Open flag */
				data_block = TRUE;
				bzero(in_buffer, 1024);
				block_size = 0;
				i++;
				continue;
			};
			if (data_block) {
				in_buffer[block_size] = data[i];
				block_size++;
			};
			i++;
		};


		printf("\n\n");
		print_man_commands();
	};
}

void no_active_connection() {
	char * buffer;
	int buffer_size;

	printf("\nUnacked data:\n");
	while ((buffer = lapb_dequeue(lapb_server, &buffer_size)) != NULL)
		printf("%s\n", buf_to_str(buffer, buffer_size));

	lapb_reset(lapb_server, LAPB_NOT_READY);
}






int main (int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	int ret;
	int dbg = TRUE;

	struct lapb_callbacks callbacks;
	int lapb_res;

	char buffer[2048];

	unsigned char lapb_equipment_type = LAPB_DCE;
	unsigned char lapb_modulo = LAPB_STANDARD;

	pthread_t server_thread;
	struct tcp_server_struct * server_struct = NULL;

	pthread_t timer_thread;
	struct timer_struct * timer_struct = NULL;

	pthread_t logger_thread;

	printf("*******************************************\n");
	printf("******                               ******\n");
	printf("******  LAPB EMULATOR (server side)  ******\n");
	printf("******                               ******\n");
	printf("*******************************************\n");

	/* Initialize syslog */
	setlogmask (LOG_UPTO (LOG_DEBUG));
	openlog ("server_app", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

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

	if (dbg)
		goto label_1;

	/* Select program mode */
	printf("\nSelect program mode:\n1. Automatic\n2. Manual\n");
	write(0, ">", 1);
	while (read(0, buffer, sizeof(buffer)) <= 1)
		write(0, ">", 1);
	if (atoi(buffer) == 2)
		AutomaticMode = FALSE;
	else {
		/* Set up equipment mode: DTE or DCE */
		printf("\nSelect equipment type:\n1. DTE\n2. DCE\n");
		write(0, ">", 1);
		while (read(0, buffer, sizeof(buffer)) <= 1)
			write(0, ">", 1);
		if (atoi(buffer) == 1)
			lapb_equipment_type = LAPB_DTE;

		/* Set up lapb modulo: STANDARD or EXTENDED */
		printf("\nSelect modulo value:\n1. STANDARD(8)\n2. EXTENDED(128)\n");
		write(0, ">", 1);
		while (read(0, buffer, sizeof(buffer)) <= 1)
			write(0, ">", 1);
		if (atoi(buffer) == 2)
			lapb_modulo = LAPB_EXTENDED;
	};

label_1:

	/* Create TCP server */
	server_struct = malloc(sizeof(struct tcp_server_struct));

	if (dbg) {
		server_struct->port = 1234;
		goto label_2;
	};

	printf("\nEnter server port[1234]: ");
	fgets(buffer, sizeof(buffer) - 1, stdin);
	int tmp_len = strlen(buffer);
	if (tmp_len == 1)
		server_struct->port = 1234;
	else {
		buffer[strlen(buffer) - 1] = 0;
		server_struct->port = atoi(buffer);
	};

label_2:

	/* TCP server callbacks */
	server_struct->new_data_received = new_data_received;
	server_struct->no_active_connection = no_active_connection;

	ret = pthread_create(&server_thread, NULL, server_function, (void*)server_struct);
	if (ret) {
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret);
		exit(EXIT_FAILURE);
	};
	printf("TCP server thread created(code %d)\n", ret);
	while (!is_server_started())
		sleep_ms(200);
	printf("TCP server started\n\n");

	if (!AutomaticMode)
		return manual_process();

	/* LAPB init */
	bzero(&callbacks, sizeof(struct lapb_callbacks));
	callbacks.on_connected = on_connected;
	callbacks.on_disconnected = on_disconnected;
	callbacks.on_new_data = on_new_incoming_data;
	callbacks.transmit_data = transmit_data;
	callbacks.start_t1timer = start_t1timer;
	callbacks.stop_t1timer = stop_t1timer;
	callbacks.start_t2timer = start_t2timer;
	callbacks.stop_t2timer = stop_t2timer;
	callbacks.debug = lapb_debug;
	lapb_res = lapb_register(&callbacks, lapb_modulo, LAPB_SLP, lapb_equipment_type, &lapb_server);
	if (lapb_res != LAPB_OK) {
		printf("lapb_register return %d\n", lapb_res);
		exit(EXIT_FAILURE);
	};
	//lapb_server->mode = lapb_modulo | LAPB_SLP | lapb_equipment_type;

	/* Redefine some default values */
	lapb_server->T1 = 1000; /* 1s */
	lapb_server->T2 = 100;  /* 0.5s */
	lapb_server->N2 = 10;	/* Try 10 times */
	//lapb_server->low_order_bits = TRUE;

	/* Create timer */
	timer_struct = malloc(sizeof(struct timer_struct));
	timer_struct->interval = 10; // milliseconds
	timer_struct->lapb_addr = (unsigned long int)lapb_server;

	/* Timer callbacks */
	timer_struct->t1timer_expiry = t1timer_expiry;
	timer_struct->t2timer_expiry = t2timer_expiry;

	ret = pthread_create(&timer_thread, NULL, timer_function, (void*)timer_struct);
	if (ret) {
		lapb_debug(NULL, 0, "Error - pthread_create() return code: %d\n", ret);
		closelog();
		exit(EXIT_FAILURE);
	};
	printf("Timer thread created(code %d)\n", ret);
	while (!is_timer_started())
		sleep_ms(200);
	printf("Timer started\n");

	struct main_callbacks m_callbacks;
	bzero(&m_callbacks, sizeof(struct main_callbacks));
	m_callbacks.is_connected = is_server_accepted;
	//m_callbacks.print_commands_0 = print_commands_0;
	//m_callbacks.print_commands_1 = print_commands_1;
	//m_callbacks.print_commands_2 = print_commands_2;
	//m_callbacks.print_commands_3 = print_commands_3;
	main_loop(lapb_server, &m_callbacks);

	printf("Main loop ended\n");

	terminate_tcp_server();
	while (is_server_started())
		sleep_ms(200);
	printf("TCP server stopped\n");
	int * thread_result;
	pthread_join(server_thread, (void **)&thread_result);
	printf("TCP server thread exit(code %d)\n", *thread_result);
	free(thread_result);
	if (server_struct != NULL)
		free(server_struct);

	terminate_timer();
	while (is_timer_started())
		sleep_ms(200);
	printf("Timer stopped\n");
	pthread_join(timer_thread, (void **)&thread_result);
	printf("Timer thread exit(code %d)\n", *thread_result);
	free(thread_result);
	if (timer_struct != NULL)
		free(timer_struct);

	lapb_unregister(lapb_server);

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


void print_man_commands() {
	printf("Manual commands:\n");
	printf("1 Send SABM\n");
	printf("2 Send SABME\n");
	printf("3 Send DISC\n");
	printf("4 Send UA\n");
	printf("5 Send DM\n");
	printf("6 Send RR(good)\n");
	printf("7 Send RR(bad)\n");
	printf("\n");
	printf("0 Exit application\n");
	write(0, ">", 1);
}


int manual_process() {
	struct lapb_callbacks callbacks;
	int lapb_res;

	fd_set			read_set;
	struct timeval	timeout;
	int sr = 0;

	char buffer[4];

	/* LAPB init */
	bzero(&callbacks, sizeof(struct lapb_callbacks));
	callbacks.transmit_data = transmit_data;
	callbacks.debug = lapb_debug;
	lapb_res = lapb_register(&callbacks, LAPB_SMODULUS, LAPB_SLP, LAPB_DCE, &lapb_server);
	if (lapb_res != LAPB_OK) {
		printf("lapb_register return %d\n", lapb_res);
		exit(EXIT_FAILURE);
	};
	//lapb_server->mode = LAPB_DCE;

	print_man_commands();

	while (!exit_flag) {
		FD_ZERO(&read_set);
		FD_SET(fileno(stdin), &read_set);
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		sr = select(fileno(stdin) + 1, &read_set, NULL, NULL, &timeout);
		if (sr > 0) {
			bzero(buffer, sizeof(buffer));
			while (read(0, buffer, sizeof(buffer)) <= 1)
				write(0, ">", 1);
			//printf("\n");
			int action = atoi(buffer);
			switch (action) {
				case 1: /* SABM */
					//lapb_send_control(lapb_server, LAPB_SABM, LAPB_POLLON, LAPB_COMMAND);
					//
					break;
				case 2: /* SABME */
					//lapb_send_control(lapb_server, LAPB_SABME, LAPB_POLLON, LAPB_COMMAND);
					//
					break;
				case 3: /* DISC */
					//lapb_send_control(lapb_server, LAPB_DISC, LAPB_POLLON, LAPB_COMMAND);
					//
					break;
				case 4: /* UA */
					//lapb_send_control(lapb_server, LAPB_UA, LAPB_POLLON, LAPB_RESPONSE);
					//
					break;
				case 5: /* DM */
					//lapb_send_control(lapb_server, LAPB_DM, LAPB_POLLON, LAPB_RESPONSE);
					//
					break;
				case 6: /* RR good*/
					//lapb_send_control(lapb_server, LAPB_RR, LAPB_POLLON, LAPB_RESPONSE);
					//
					break;
				case 7: /* RR bad*/
					//lapb_server->vr++;
					//lapb_send_control(lapb_server, LAPB_RR, LAPB_POLLON, LAPB_RESPONSE);
					//
					break;
				case 0:
					exit_flag = TRUE;
					break;
				default:
					printf("Command is not supported\n\n");
					break;
			};
		};

	};


	lapb_unregister(lapb_server);

	return EXIT_SUCCESS;
}


int man_decode(struct lapb_cs *lapb, char *data, int data_size, struct lapb_frame *frame) {
	frame->type = LAPB_ILLEGAL;


	/* We always need to look at 2 bytes, sometimes we need
	 * to look at 3 and those cases are handled below.
	 */
	if (data_size < 2)
		return -1;

	if (lapb->mode & LAPB_MLP) {
		if (lapb->mode & LAPB_DCE) {
			if (data[0] == LAPB_ADDR_D)
				frame->cr = LAPB_COMMAND;
			if (data[0] == LAPB_ADDR_C)
				frame->cr = LAPB_RESPONSE;
		} else {
			if (data[0] == LAPB_ADDR_C)
				frame->cr = LAPB_COMMAND;
			if (data[0] == LAPB_ADDR_D)
				frame->cr = LAPB_RESPONSE;
		};
	} else {
		if (lapb->mode & LAPB_DCE) {
			if (data[0] == LAPB_ADDR_B)
				frame->cr = LAPB_COMMAND;
			if (data[0] == LAPB_ADDR_A)
				frame->cr = LAPB_RESPONSE;
		} else {
			if (data[0] == LAPB_ADDR_A)
				frame->cr = LAPB_COMMAND;
			if (data[0] == LAPB_ADDR_B)
				frame->cr = LAPB_RESPONSE;
		};
	};

	if (lapb->mode & LAPB_EXTENDED) {
		if (!(data[1] & LAPB_S)) {
			/*
			 * I frame - carries NR/NS/PF
			 */
			frame->type       = LAPB_I;
			frame->ns         = (data[1] >> 1) & 0x7F;
			frame->nr         = (data[2] >> 1) & 0x7F;
			frame->pf         = data[2] & LAPB_EPF;
			frame->control[0] = data[1];
			frame->control[1] = data[2];
		} else if ((data[1] & LAPB_U) == 1) {
			/*
			 * S frame - take out PF/NR
			 */
			frame->type       = data[1] & 0x0F;
			frame->nr         = (data[2] >> 1) & 0x7F;
			frame->pf         = data[2] & LAPB_EPF;
			frame->control[0] = data[1];
			frame->control[1] = data[2];
		} else if ((data[1] & LAPB_U) == 3) {
			/*
			 * U frame - take out PF
			 */
			frame->type       = data[1] & ~LAPB_SPF;
			frame->pf         = data[1] & LAPB_SPF;
			frame->control[0] = data[1];
			frame->control[1] = 0x00;
		};
	} else {
		if (!(data[1] & LAPB_S)) {
			/*
			 * I frame - carries NR/NS/PF
			 */
			frame->type = LAPB_I;
			frame->ns   = (data[1] >> 1) & 0x07;
			frame->nr   = (data[1] >> 5) & 0x07;
			frame->pf   = (data[1] & LAPB_SPF) >> 4;
		} else if ((data[1] & LAPB_U) == 1) {
			/*
			 * S frame - take out PF/NR
			 */
			frame->type = data[1] & 0x0F;
			frame->nr   = (data[1] >> 5) & 0x07;
			frame->pf   = (data[1] & LAPB_SPF) >> 4;
		} else if ((data[1] & LAPB_U) == 3) {
			/*
			 * U frame - take out PF
			 */
			frame->type = data[1] & ~LAPB_SPF;
			frame->pf   = (data[1] & LAPB_SPF) >> 4;
		};

		frame->control[0] = data[1];
	};

	return 0;
}
