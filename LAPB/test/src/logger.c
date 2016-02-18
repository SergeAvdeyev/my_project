
#include "logger.h"


pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t logger_cond = PTHREAD_COND_INITIALIZER;
int logger_exit_flag = FALSE;

int logger_state = FALSE;


struct node {
	char *	data;
	int		data_size;
	struct node *next;
} *front, *rear;


int count = 0;


// Private prototipes
//void _logger_mutex_init();
void _logger_mutex_lock();
void _logger_mutex_unlock();



///////////////////////////////
//
//		PUBLIC
//
///////////////////////////////


int is_logger_started() {
	int result;
	_logger_mutex_lock();
	result = logger_state;
	_logger_mutex_unlock();
	return result;
}


void terminate_logger() {
	_logger_mutex_lock();
	logger_exit_flag = TRUE;
	pthread_cond_signal(&logger_cond);
	_logger_mutex_unlock();
}




///////////////////////////////
//
//		PRIVATE
//
///////////////////////////////

/* Mutex section */

//void _logger_mutex_init() {
//	if (pthread_mutex_init(&logger_mutex, NULL) != 0)
//		perror("Logger mutex init failed, ");
//}

void _logger_mutex_lock() {
	pthread_mutex_lock(&logger_mutex);
}

void _logger_mutex_unlock() {
	pthread_mutex_unlock(&logger_mutex);
}

void logger_started() {
	_logger_mutex_lock();
	logger_state = TRUE;
	_logger_mutex_unlock();
}

void logger_stopped() {
	_logger_mutex_lock();
	logger_state = FALSE;
	_logger_mutex_unlock();
}



int get_logger_exit_flag() {
	int result;
	_logger_mutex_lock();
	result = logger_exit_flag;
	_logger_mutex_unlock();
	return result;
}



/* Create an empty queue */
void create_queue() {
	front = rear = NULL;
}

/* Returns queue size */
int queue_size() {
	int result;
	_logger_mutex_lock();
	result = count;
	_logger_mutex_unlock();
	return result;
}

/* Enqueing the queue */
void logger_enqueue(char *data, int data_size) {

	_logger_mutex_lock();

	struct node *tmp = (struct node *)malloc(sizeof(struct node));
	char * buf_tmp = malloc(data_size);
	memcpy(buf_tmp, data, data_size);

	tmp->data = buf_tmp;
	tmp->data_size = data_size;
	tmp->next = NULL;

	if (rear == NULL) {
		rear = tmp;
		front = tmp;
	} else {
		rear->next = tmp;
		rear = tmp;
	};
	count++;
	pthread_cond_signal(&logger_cond);

	_logger_mutex_unlock();
}


/* Dequeing the queue */
char * logger_dequeue(int *ret_size) {
	char * result = NULL;

	//_logger_mutex_lock();
	if (front == NULL) {
		*ret_size = 0;
		//_logger_mutex_unlock();
		return NULL;
	};
	result = front->data;
	*ret_size = front->data_size;
	struct node *tmp = front;
	if (front->next != NULL)
		front = front->next;
	else {
		front = NULL;
		rear = NULL;
	};
	free(tmp);
	count--;
	//_logger_mutex_unlock();
	return result;
}



void * logger_function(void *ptr) {
	(void)ptr;
	int * result = calloc(1, sizeof(int));

	char * buffer;
	int buffer_size;

	//_logger_mutex_init();

	create_queue();
	logger_started();

	while (!get_logger_exit_flag()) {
		_logger_mutex_lock();
		pthread_cond_wait(&logger_cond, &logger_mutex);
		while ((buffer = logger_dequeue(&buffer_size)) != NULL) {
			syslog(LOG_ALERT, "%s", buffer);
			free(buffer);
		};
//		if ((buffer = logger_dequeue(&buffer_size)) != NULL) {
//			syslog(LOG_ALERT, "%s", buffer);
//			free(buffer);
//		} else
//			usleep(1000);
		_logger_mutex_unlock();
	};


	logger_stopped();
	printf("Timer thread is terminated\n");

	*result = 0;
	return result;

}




