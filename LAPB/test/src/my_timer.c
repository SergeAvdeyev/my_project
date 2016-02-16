
#include "common.h"
#include "my_timer.h"

//pthread_mutex_t timer_mutex;
int timer_exit_flag = FALSE;

int timer_state = FALSE;

int t1_interval = 0;
int t1_interval_tmp = 0;
int t1_active = FALSE;

int t2_interval = 0;
int t2_interval_tmp = 0;
int t2_active = FALSE;

// Private prototipes
//void _timer_mutex_init();
//void _timer_mutex_lock();
//void _timer_mutex_unlock();


///////////////////////////////
//
//		PUBLIC
//
///////////////////////////////

int is_timer_started() {
	int result;
	main_lock();
	result = timer_state;
	main_unlock();
	return result;
}


void terminate_timer() {
	main_lock();
	timer_exit_flag = TRUE;
	main_unlock();
}




void timer_t1_start() {
	main_lock();
	t1_interval_tmp = t1_interval;
	t1_active = TRUE;
	main_unlock();
}

void timer_t1_stop() {
	main_lock();
	t1_active = FALSE;
	main_unlock();
}

void timer_t1_set_interval(int value) {
	main_lock();
	t1_interval = value;
	main_unlock();
}

int timer_t1_get_state() {
	int result;
	main_lock();
	result = t1_active;
	main_unlock();
	return result;
}




void timer_t2_start() {
	main_lock();
	t2_interval_tmp = t2_interval;
	t2_active = TRUE;
	main_unlock();
}

void timer_t2_stop() {
	main_lock();
	t2_active = FALSE;
	main_unlock();
}

void timer_t2_set_interval(int value) {
	main_lock();
	t2_interval = value;
	main_unlock();
}

int timer_t2_get_state() {
	int result;
	main_lock();
	result = t2_active;
	main_unlock();
	return result;
}



///////////////////////////////
//
//		PRIVATE
//
///////////////////////////////

///* Mutex section */

//void _timer_mutex_init() {
//	if (pthread_mutex_init(&timer_mutex, NULL) != 0)
//		perror("Timer mutex init failed, ");
//}

//void _timer_mutex_lock() {
//	pthread_mutex_lock(&timer_mutex);
//}

//void _timer_mutex_unlock() {
//	pthread_mutex_unlock(&timer_mutex);
//}

void timer_started() {
	main_lock();
	timer_state = TRUE;
	main_unlock();
}

void timer_stopped() {
	main_lock();
	timer_state = FALSE;
	main_unlock();
}



int get_timer_exit_flag() {
	int result;
	main_lock();
	result = timer_exit_flag;
	main_unlock();
	return result;
}



int timer_t1_dec(int value) {
	int result;
	main_lock();
	t1_interval_tmp -= value;
	result = t1_interval_tmp;
	main_unlock();
	return result;
}

//int get_t2_state() {
//	int result;
//	_timer_mutex_lock();
//	result = t2_active;
//	_timer_mutex_unlock();
//	return result;
//}

int timer_t2_dec(int value) {
	int result;
	main_lock();
	t2_interval_tmp -= value;
	result = t2_interval_tmp;
	main_unlock();
	return result;
}



void * timer_function(void *ptr) {
	struct timer_struct * struct_ptr = (struct timer_struct *)ptr;
	(void)struct_ptr;
	struct timespec ts, ts2;

	int * result = calloc(1, sizeof(int));

	//_timer_mutex_init();

	timer_started();

	ts.tv_sec = struct_ptr->interval / 1000;
	ts.tv_nsec = (struct_ptr->interval % 1000) * 1000000;

	while (!get_timer_exit_flag()) {
		ts2.tv_sec = 0;
		ts2.tv_nsec = 0;
		nanosleep(&ts, &ts2);
		if (timer_t1_get_state()) {
			if (timer_t1_dec(struct_ptr->interval) <= 0) {
				//set_t1_state(FALSE);
				if (struct_ptr->t1timer_expiry)
					struct_ptr->t1timer_expiry(struct_ptr->lapb_addr);

			};
		};
		if (timer_t2_get_state()) {
			if (timer_t1_dec(struct_ptr->interval) <= 0) {
				//set_t2_state(FALSE);
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
