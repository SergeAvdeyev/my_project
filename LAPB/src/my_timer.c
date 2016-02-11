
#include "my_timer.h"

pthread_mutex_t timer_mutex;
int timer_exit_flag = FALSE;

int timer_state = FALSE;

int t1_value = 0;
int t1_active = FALSE;

int t2_value = 0;
int t2_active = FALSE;

// Private prototipes
void _timer_mutex_init();
void _timer_mutex_lock();
void _timer_mutex_unlock();


///////////////////////////////
//
//		PUBLIC
//
///////////////////////////////

void timer_init() {
	_timer_mutex_init();
}

int is_timer_started() {
	int result;
	_timer_mutex_lock();
	result = timer_state;
	_timer_mutex_unlock();
	return result;
}


void terminate_timer() {
	_timer_mutex_lock();
	timer_exit_flag = TRUE;
	_timer_mutex_unlock();
}


void set_t1_state(int state) {
	_timer_mutex_lock();
	t1_active = state;
	_timer_mutex_unlock();
}

void set_t1_value(int value) {
	_timer_mutex_lock();
	t1_value = value;
	_timer_mutex_unlock();
}

int get_t1_state() {
	int result;
	_timer_mutex_lock();
	result = t1_active;
	_timer_mutex_unlock();
	return result;
}

void set_t2_state(int state) {
	_timer_mutex_lock();
	t2_active = state;
	_timer_mutex_unlock();
}

void set_t2_value(int value) {
	_timer_mutex_lock();
	t2_value = value;
	_timer_mutex_unlock();
}



///////////////////////////////
//
//		PRIVATE
//
///////////////////////////////

/* Mutex section */

void _timer_mutex_init() {
	if (pthread_mutex_init(&timer_mutex, NULL) != 0)
		perror("Timer mutex init failed, ");
}

void _timer_mutex_lock() {
	pthread_mutex_lock(&timer_mutex);
}

void _timer_mutex_unlock() {
	pthread_mutex_unlock(&timer_mutex);
}

void timer_started() {
	_timer_mutex_lock();
	timer_state = TRUE;
	_timer_mutex_unlock();
}

void timer_stopped() {
	_timer_mutex_lock();
	timer_state = FALSE;
	_timer_mutex_unlock();
}



int get_timer_exit_flag() {
	int result;
	_timer_mutex_lock();
	result = timer_exit_flag;
	_timer_mutex_unlock();
	return result;
}



int dec_t1_value(int value) {
	int result;
	_timer_mutex_lock();
	t1_value -= value;
	result = t1_value;
	_timer_mutex_unlock();
	return result;
}

int get_t2_state() {
	int result;
	_timer_mutex_lock();
	result = t2_active;
	_timer_mutex_unlock();
	return result;
}

int dec_t2_value(int value) {
	int result;
	_timer_mutex_lock();
	t2_value -= value;
	result = t2_value;
	_timer_mutex_unlock();
	return result;
}



void * timer_function(void *ptr) {
	struct timer_struct * struct_ptr = (struct timer_struct *)ptr;
	(void)struct_ptr;
	struct timespec ts, ts2;

	int * result = calloc(1, sizeof(int));

	timer_started();

	ts.tv_sec = struct_ptr->interval / 1000;
	ts.tv_nsec = (struct_ptr->interval % 1000) * 1000000;

	while (!get_timer_exit_flag()) {
		ts2.tv_sec = 0;
		ts2.tv_nsec = 0;
		nanosleep(&ts, &ts2);
		if (get_t1_state()) {
			if (dec_t1_value(struct_ptr->interval) <= 0) {
				set_t1_state(FALSE);
				if (struct_ptr->t1timer_expiry)
					struct_ptr->t1timer_expiry(struct_ptr->lapb_addr);
			};
		};
		if (get_t2_state()) {
			if (dec_t2_value(struct_ptr->interval) <= 0) {
				set_t2_state(FALSE);
				if (struct_ptr->t2timer_expiry)
					struct_ptr->t2timer_expiry(struct_ptr->lapb_addr);
			};
		};

	};


	timer_stopped();
	printf("Timer thread is terminated\n");

	*result = 0;
	return result;

}
