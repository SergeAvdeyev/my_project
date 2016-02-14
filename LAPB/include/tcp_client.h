#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>

#include "common.h"

#define TRUE             1
#define FALSE            0

struct tcp_client_struct {
	char * server_address;
	int server_port;
	void (*new_data_received)(char * data, int data_size);
	void (*connection_lost)();
};


void tcp_client_init();
void *client_function(void * ptr);
int tcp_client_socket();

void terminate_tcp_client();
int is_client_started();
int is_client_connected();

//int sleep_ms(int milliseconds);

#endif // TCP_CLIENT_H
