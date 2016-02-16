
#include "tcp_server.h"


int server_exit_flag = FALSE;
int server_socket = -1;
int client_socket = -1;
int is_started = FALSE;
int is_accepted = FALSE;



///////////////////////////////
//
//		PUBLIC
//
///////////////////////////////

int tcp_client_socket() {
	return client_socket;
}

int is_server_started() {
	int result;
	main_lock();
	result = is_started;
	main_unlock();
	return result;
}

int is_server_accepted() {
	int result;
	main_lock();
	result = is_accepted;
	main_unlock();
	return result;
}

void terminate_tcp_server() {
	main_lock();
	server_exit_flag = TRUE;
	if (server_socket != -1) {
		shutdown(server_socket, SHUT_RDWR);
		//close(listen_sd);
	};
	main_unlock();
}









///////////////////////////////
//
//		PRIVATE
//
///////////////////////////////

void server_started() {
	main_lock();
	is_started = TRUE;
	main_unlock();
}

void server_stopped() {
	main_lock();
	is_started = FALSE;
	main_unlock();
}

void server_accepted() {
	main_lock();
	is_accepted = TRUE;
	main_unlock();
}

void server_listening() {
	main_lock();
	is_accepted = FALSE;
	main_unlock();
}



int get_server_exit_flag() {
	int result;
	main_lock();
	result = server_exit_flag;
	main_unlock();
	return result;
}





void * server_function(void *ptr) {
	int		port_no = ((struct tcp_server_struct *)ptr)->port;
	int		i, rc, on = 1;
	int		max_sd;
	int		desc_ready;
	int		close_conn;
	char	buffer[2048];
	struct sockaddr_in6 addr;
	struct timeval      timeout;
	fd_set              master_set, working_set;

	int * result = calloc(1, sizeof(int));

	server_started();

	/* Create an AF_INET6 stream socket to receive incoming      */
	/* connections on                                            */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)    {
		syslog(LOG_ERR, "socket() failed, %s", strerror(errno));
		*result = -1;
		return result;
	};

	/* Allow socket descriptor to be reuseable                   */
	rc = setsockopt(server_socket, SOL_SOCKET,  SO_REUSEADDR,
	(char *)&on, sizeof(on));
	if (rc < 0) {
		syslog(LOG_ERR, "setsockopt() failed, %s", strerror(errno));
		close(server_socket);
		*result = -1;
		return result;
	};

	/* Set socket to be nonblocking. All of the sockets for      */
	/* the incoming connections will also be nonblocking since   */
	/* they will inherit that state from the listening socket.   */
	rc = ioctl(server_socket, FIONBIO, (char *)&on);
	if (rc < 0) {
		syslog(LOG_ERR, "ioctl() failed, %s", strerror(errno));
		close(server_socket);
		*result = -1;
		return result;
	};

	/* Bind the socket  */
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET;
	memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
	addr.sin6_port = htons(port_no);
	rc = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
	if (rc < 0) {
		syslog(LOG_ERR, "bind() failed, %s", strerror(errno));
		close(server_socket);
		*result = -1;
		return result;
	};

	/* Set the listen back log	*/
	rc = listen(server_socket, 32);
	if (rc < 0) {
		syslog(LOG_ERR, "listen() failed, %s", strerror(errno));
		close(server_socket);
		*result = -1;
		return result;
	};

	/* Initialize the master fd_set	*/
	FD_ZERO(&master_set);
	max_sd = server_socket;
	FD_SET(server_socket, &master_set);

	/* Initialize the timeval struct to 3 minutes.  If no		*/
	/* activity after 3 minutes this program will end.			*/
	timeout.tv_sec  = 5;
	timeout.tv_usec = 0;

	/* Loop waiting for incoming connects or for incoming data	*/
	/* on any of the connected sockets.							*/
	while (!get_server_exit_flag()) {
		// Copy the master fd_set over to the working fd_set.
		memcpy(&working_set, &master_set, sizeof(master_set));

		/* Call select() and wait 3 minutes for it to complete.	*/
		timeout.tv_sec  = 5;
		timeout.tv_usec = 0;
		rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

		if (get_server_exit_flag()) break;
		/* Check to see if the select call failed.				*/
		if (rc < 0) {
			syslog(LOG_ERR, "select() failed, %s", strerror(errno));
			break;
		};

		/* Check to see if the 3 minute time out expired.		*/
		if (rc == 0) {
			//printf("Waiting on select() is timed out.\n");
			continue;
		};

		/* One or more descriptors are readable.  Need to		*/
		/* determine which ones they are.						*/
		desc_ready = rc;
		for (i = 0; i <= max_sd  &&  desc_ready > 0; ++i) {
			/* Check to see if this descriptor is ready			*/
			if (FD_ISSET(i, &working_set)) {
				desc_ready -= 1;

				/* Check to see if this is the listening socket     */
				if (i == server_socket) {
					do {
						int client_socket_tmp = accept(server_socket, NULL, NULL);
						if (client_socket_tmp < 0) {
							if (errno != EWOULDBLOCK) {
								syslog(LOG_ERR, "accept() failed, %s", strerror(errno));
								*result = -1;
								return result;
							};
							break;
						};

						syslog(LOG_INFO, "New incoming connection - %d", client_socket_tmp);
						if (is_server_accepted()) {
							// already established one connection
							syslog(LOG_NOTICE, "One connection already established. Close it");
							close(client_socket_tmp);
							break;
						};
						client_socket = client_socket_tmp;
						FD_SET(client_socket, &master_set);
						if (client_socket > max_sd)
							max_sd = client_socket;
						server_accepted();

					} while (client_socket != -1);
				}

				/* This is not the listening socket, therefore an   */
				/* existing connection must be readable             */
				else {
					close_conn = FALSE;

					rc = recv(i, buffer, sizeof(buffer), 0);
					if (rc < 0) {
						if (errno != EWOULDBLOCK) {
							syslog(LOG_ERR, "Client(%d): recv() failed, %s", i, strerror(errno));
							syslog(LOG_ERR, "Client(%d): Connection will be closed", i);
							close_conn = TRUE;
						};
					} else if (rc == 0) {
						/* Connection has been closed by the client  */
						syslog(LOG_NOTICE, "Client(%d): Connection closed by client\n", i);
						close_conn = TRUE;
					} else {
						/* Data was received                          */
						//len = rc;
						//syslog(LOG_INFO, "Client(%d): %d bytes received\n", i, len);

						if (((struct tcp_server_struct *)ptr)->new_data_received)
							((struct tcp_server_struct *)ptr)->new_data_received(buffer, rc);
					};

					if (close_conn) {
						close(i);
						FD_CLR(i, &master_set);
						if (i == max_sd) {
							while (FD_ISSET(max_sd, &master_set) == FALSE)
								max_sd -= 1;
						};
						server_listening();
						if (((struct tcp_server_struct *)ptr)->no_active_connection)
							((struct tcp_server_struct *)ptr)->no_active_connection();
					};
				} /* End of existing connection is readable */
			} /* End of if (FD_ISSET(i, &working_set)) */
		} /* End of loop through selectable descriptors */

	};

	/* Clean up all of the sockets that are open                 */
	for (i = 0; i <= max_sd; ++i) {
		if (FD_ISSET(i, &master_set))
			close(i);
	};

	server_stopped();
	printf("TCP server thread is terminated\n");

	*result = 0;
	return result;
}
