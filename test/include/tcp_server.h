#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>

#include "common.h"

struct tcp_server_struct {
	int port;
	void (*new_data_received)(char * data, int data_size);
	void (*no_active_connection)();
};


//void tcp_server_init();

void *server_function(void * ptr);
int tcp_client_socket();
//int get_tcp_server_exit();
void terminate_tcp_server();
int is_server_started();
int is_server_accepted();

//int sleep_ms(int milliseconds);

#endif // TCP_SERVER_H
