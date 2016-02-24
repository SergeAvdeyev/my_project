
#include "tcp_client.h"


pthread_mutex_t client_mutex;
int client_exit_flag = FALSE;

int client_socket = -1;
int is_started = FALSE;
int is_connected = FALSE;

// Private prototipes
void _mutex_init();
void _mutex_lock();
void _mutex_unlock();


///////////////////////////////
//
//		PUBLIC
//
///////////////////////////////

//void tcp_client_init() {
//	_mutex_init();
//}

int tcp_client_socket() {
	return client_socket;
}

int is_client_started() {
	int result;
	_mutex_lock();
	result = is_started;
	_mutex_unlock();
	return result;
}

int is_client_connected() {
	int result;
	_mutex_lock();
	result = is_connected;
	_mutex_unlock();
	return result;
}

void terminate_tcp_client() {
	_mutex_lock();
	client_exit_flag = TRUE;
	if (client_socket != -1) {
		shutdown(client_socket, SHUT_RDWR);
		//close(listen_sd);
	};
	_mutex_unlock();
}



///////////////////////////////
//
//		PRIVATE
//
///////////////////////////////

/* Mutex section */

void _mutex_init() {
	if (pthread_mutex_init(&client_mutex, NULL) != 0)
		perror("\nClient mutex init failed\n");
}

void _mutex_lock() {
	pthread_mutex_lock(&client_mutex);
}

void _mutex_unlock() {
	pthread_mutex_unlock(&client_mutex);
}

void client_started() {
	_mutex_lock();
	is_started = TRUE;
	_mutex_unlock();
}

void client_stopped() {
	_mutex_lock();
	is_started = FALSE;
	_mutex_unlock();
}

void client_connected() {
	_mutex_lock();
	is_connected = TRUE;
	_mutex_unlock();
}

void client_disconnected() {
	_mutex_lock();
	is_connected = FALSE;
	_mutex_unlock();
}


int get_client_exit_flag() {
	int result;
	_mutex_lock();
	result = client_exit_flag;
	_mutex_unlock();
	return result;
}





//int sleep_ms(int milliseconds) { // cross-platform sleep function
//	int result = 0;

//	struct timespec ts, ts2;
//	ts.tv_sec = milliseconds / 1000;
//	ts.tv_nsec = (milliseconds % 1000) * 1000000;
//	ts2.tv_sec = 0;
//	ts2.tv_nsec = 0;
//	result = nanosleep(&ts, &ts2);
//	if ((ts.tv_sec != ts2.tv_sec) || (ts.tv_nsec > ts2.tv_nsec))
//		if (ts2.tv_nsec != -1)
//			result = -1;

//	return result;
//}





int create_client_socket() {
	int	result = -1;
	int	rc, on = 1;

	result = socket(AF_INET, SOCK_STREAM, 0);

	rc = ioctl(result, FIONBIO, (char *)&on);
	if (rc < 0) {
		lapb_debug(NULL, 0, "ioctl() failed, %s", strerror(errno));
		close(result);
		return result;
	};

	if (result < 0) {
		lapb_debug(NULL, 0, "ERROR opening socket, %s", strerror(errno));
		return -1;
	};

	return result;
}


void * client_function(void *ptr) {
	struct sockaddr_in	serv_addr;
	struct hostent	*	server;
	int				rc;

	char			buffer[4096];
	int				max_sd = 0;
	struct timeval	timeout;
	fd_set			master_set, working_set;

	int * result = calloc(1, sizeof(int));

	_mutex_init();

	client_started();

	server = gethostbyname(((struct tcp_client_struct *)ptr)->server_address);
	if (server == NULL) {
		lapb_debug(NULL, 0, "ERROR, no such host %s", ((struct tcp_client_struct *)ptr)->server_address);
		*result = 0;
		return result;
	};
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(((struct tcp_client_struct *)ptr)->server_port);


	int k = 50; // 5 seconds
	while (!get_client_exit_flag()) {

		if (client_socket == -1) {
			client_socket = create_client_socket();
			if (client_socket == -1)
				exit(0);
			/* Initialize the master fd_set	*/
			FD_ZERO(&master_set);
			max_sd = client_socket;
			FD_SET(client_socket, &master_set);
		};

		if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			k--;
			if (k <= 0) {
				lapb_debug(NULL, 0, "ERROR connecting, %s", strerror(errno));
				k = 50;
			};
			sleep_ms(100);
			continue;
		};
		k = 50; // 5 seconds
		client_connected();


		while (1) {
			// Copy the master fd_set over to the working fd_set.
			memcpy(&working_set, &master_set, sizeof(master_set));

			/* Call select() and wait 3 minutes for it to complete.	*/
			timeout.tv_sec  = 5;
			timeout.tv_usec = 0;
			rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
			//rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

			if (get_client_exit_flag()) break;
			/* Check to see if the select call failed.				*/
			if (rc < 0) {
				lapb_debug(NULL, 0, "select() failed, %s", strerror(errno));
				lapb_debug(NULL, 0, "Connection will be closed");
				//close(client_socket);
				break;
			};
			/* Check to see if the 3 minute time out expired.		*/
			if (rc == 0) {
				//printf("Waiting on select() is timed out.\n");
				continue;
			};

			if (FD_ISSET(client_socket, &working_set)) {
				rc = recv(client_socket, buffer, sizeof(buffer), 0);
				if (rc < 0) {
					if (errno != EWOULDBLOCK) {
						lapb_debug(NULL, 0, "recv() failed, %s", strerror(errno));
						lapb_debug(NULL, 0, "Connection will be closed");
						close(client_socket);
						client_socket = -1;
						if (((struct tcp_client_struct *)ptr)->connection_lost)
							((struct tcp_client_struct *)ptr)->connection_lost();
						client_disconnected();
						break;
					};
				} else if (rc == 0) {
					/* Connection has been closed by the client  */
					lapb_debug(NULL, 0, "Connection closed by server");
					close(client_socket);
					client_socket = -1;
					client_disconnected();
					if (((struct tcp_client_struct *)ptr)->connection_lost)
						((struct tcp_client_struct *)ptr)->connection_lost();
					break;
				} else {
					/* Data was received                          */
					//lapb_debug(NULL, 0, "%d bytes received", rc);

					if (((struct tcp_client_struct *)ptr)->new_data_received)
						((struct tcp_client_struct *)ptr)->new_data_received(buffer, rc);
				};

			};

		};
	};

	if (client_socket != -1)
		close(client_socket);
	client_socket = -1;

	client_stopped();
	printf("TCP client thread is terminated\n");

	*result = 0;
	return result;

}
