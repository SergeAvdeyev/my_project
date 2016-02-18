
#include "logger.h"


pthread_mutex_t logger_mutex;
int logger_exit_flag = FALSE;

int logger_state = FALSE;


struct node {
	char *	data;
	int		data_size;
	struct node *ptr;
} *front, *rear, *temp, *front1;


int count = 0;


// Private prototipes
void _logger_mutex_init();
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
	_logger_mutex_unlock();
}




///////////////////////////////
//
//		PRIVATE
//
///////////////////////////////

/* Mutex section */

void _logger_mutex_init() {
	if (pthread_mutex_init(&logger_mutex, NULL) != 0)
		perror("Logger mutex init failed, ");
}

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

	char * buf_tmp = malloc(data_size + 1);
	memcpy(buf_tmp, data, data_size);
	buf_tmp[data_size] = 0;
	if (rear == NULL) {
		rear = (struct node *)malloc(sizeof(struct node));
		rear->ptr = NULL;
		rear->data = buf_tmp;
		rear->data_size = data_size + 1;
		front = rear;
	} else {
		temp = (struct node *)malloc(1*sizeof(struct node));
		rear->ptr = temp;
		temp->data = buf_tmp;
		temp->data_size = data_size + 1;
		temp->ptr = NULL;

		rear = temp;
	};
	count++;

	_logger_mutex_unlock();
}


/* Dequeing the queue */
char * logger_dequeue(int *ret_size) {
	char * result = NULL;

	_logger_mutex_lock();
	front1 = front;

	if (front1 == NULL) {
		*ret_size = 0;
		_logger_mutex_unlock();
		return NULL;
	} else {
		if (front1->ptr != NULL) {
			front1 = front1->ptr;
			//printf("\n Dequed value : %d", front->info);
			result = front->data;
			*ret_size = front->data_size;
			free(front);
			front = front1;
		} else {
			//printf("\n Dequed value : %d", front->info);
			result = front->data;
			*ret_size = front->data_size;
			free(front);
			front = NULL;
			rear = NULL;
		};
		count--;
	};
	_logger_mutex_unlock();
	return result;
}



void * logger_function(void *ptr) {
	(void)ptr;
	int * result = calloc(1, sizeof(int));

	char * buffer;
	int buffer_size;

	_logger_mutex_init();

	create_queue();
	logger_started();


	while (!get_logger_exit_flag()) {
		if ((buffer = logger_dequeue(&buffer_size)) != NULL) {
			syslog(LOG_ALERT, "%s", buffer);
			free(buffer);
		} else
			usleep(1000);
	};


	logger_stopped();
	printf("Timer thread is terminated\n");

	*result = 0;
	return result;

}




