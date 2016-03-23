
#include "common.h"
#include "my_timer.h"

pthread_mutex_t timer_mutex;
int timer_thread_exit_flag = FALSE;

int timer_thread_state = FALSE;

// Private prototipes

struct timer_descr * timers_list[10];
_uchar	timers_count = 0;

void _timer_mutex_init();
void _timer_mutex_lock();
void _timer_mutex_unlock();


///////////////////////////////
//
//		PUBLIC
//
///////////////////////////////

int is_timer_thread_started() {
	int result;
	_timer_mutex_lock();
	result = timer_thread_state;
	_timer_mutex_unlock();
	return result;
}


void terminate_timer_thread() {
	_timer_mutex_lock();
	timer_thread_exit_flag = TRUE;
	_timer_mutex_unlock();
}



void * timer_add(int interval, void * owner, void *timer_expiry_sub) {
	if (timers_count >= sizeof(timers_list)/sizeof(void *)) return NULL;
	if (!timer_expiry_sub) return NULL;

	struct timer_descr * timer_tmp;

	timer_tmp = malloc(sizeof(struct timer_descr));
	timer_tmp->interval = interval;
	timer_tmp->active = FALSE;
	timer_tmp->interval_tmp = 0;
	timer_tmp->owner = owner;
	timer_tmp->timer_expiry_sub = timer_expiry_sub;
	timers_list[timers_count] = timer_tmp;
	timers_count++;

	return timer_tmp;
}

void timer_del(void *timer) {
	free(timer);
}

void timer_start(void * timer) {
	_timer_mutex_lock();
	struct timer_descr * timer_tmp = (struct timer_descr *)timer;
	timer_tmp->interval_tmp = timer_tmp->interval;
	timer_tmp->active = TRUE;
	_timer_mutex_unlock();
}

void timer_stop(void * timer) {
	_timer_mutex_lock();
	struct timer_descr * timer_tmp = (struct timer_descr *)timer;
	timer_tmp->active = FALSE;
	timer_tmp->interval_tmp = timer_tmp->interval;
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

void timer_thread_started() {
	_timer_mutex_lock();
	timer_thread_state = TRUE;
	_timer_mutex_unlock();
}

void timer_thread_stopped() {
	_timer_mutex_lock();
	timer_thread_state = FALSE;
	_timer_mutex_unlock();
}



int get_timer_thread_exit_flag() {
	int result;
	_timer_mutex_lock();
	result = timer_thread_exit_flag;
	_timer_mutex_unlock();
	return result;
}



int timer_dec(struct timer_descr * timer, int value) {
	int result;
	_timer_mutex_lock();
	timer->interval_tmp -= value;
	result = timer->interval_tmp;
	if (timer->interval_tmp <= 0)
		timer->interval_tmp = timer->interval;
	_timer_mutex_unlock();
	return result;
}

int get_timer_state(struct timer_descr * timer) {
	int result;
	_timer_mutex_lock();
	result = timer->active;
	_timer_mutex_unlock();
	return result;
}


void * timer_thread_function(void *ptr) {
	struct timer_thread_struct * struct_ptr = (struct timer_thread_struct *)ptr;
	struct timer_descr * timer;
	struct timespec ts, ts2;
	int i;

	int * result = calloc(1, sizeof(int));

	_timer_mutex_init();

	timer_thread_started();

	ts.tv_sec = struct_ptr->interval / 1000;
	ts.tv_nsec = (struct_ptr->interval % 1000) * 1000000;

	while (!get_timer_thread_exit_flag()) {
		ts2.tv_sec = 0;
		ts2.tv_nsec = 0;
		nanosleep(&ts, &ts2);
		for (i = 0; i < timers_count; i++) {
			timer = timers_list[i];
			if (!get_timer_state(timer))
				continue;
			if (timer_dec(timer, struct_ptr->interval) > 0)
				continue;
			timer->timer_expiry_sub(timer->owner);
		};
	};

	timer_thread_stopped();
	printf("Timer thread is terminated\n");

	*result = 0;
	return result;

}
