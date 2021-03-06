
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



void * timer_add(int interval, void * lapb_ptr, void *timer_expiry) {
	if (timers_count >= sizeof(timers_list)/sizeof(void *)) return NULL;
	if (!timer_expiry) return NULL;

	struct timer_descr * timer_tmp;

	timer_tmp = malloc(sizeof(struct timer_descr));
	timer_tmp->interval = interval;
	timer_tmp->active = FALSE;
	timer_tmp->interval_tmp = 0;
	timer_tmp->lapb_ptr = lapb_ptr;
	timer_tmp->timer_expiry = timer_expiry;
	int array_size = sizeof(timers_list)/sizeof(struct timer_descr *);
	int i = 0;
	while (i < array_size) {
		if (timers_list[i] == NULL) {
			timers_list[i] = timer_tmp;
			break;
		};
		i++;
	};
	timers_count++;

	return timer_tmp;
}

void timer_del(void *timer) {
	if (timer == NULL) return;

	int array_size = sizeof(timers_list)/sizeof(struct timer_descr *);
	int i = 0;
	while (i < array_size) {
		if (timers_list[i] == timer) {
			timers_list[i] = NULL;
			free(timer);
			timers_count--;
			break;
		};
		i++;
	};
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

	i = sizeof(timers_list);
	bzero(timers_list, i);

	_timer_mutex_init();

	timer_thread_started();

	ts.tv_sec = struct_ptr->interval / 1000;
	ts.tv_nsec = (struct_ptr->interval % 1000) * 1000000;
	int array_size = i/sizeof(struct timer_descr *);

	while (!get_timer_thread_exit_flag()) {
		ts2.tv_sec = 0;
		ts2.tv_nsec = 0;
		nanosleep(&ts, &ts2);
		for (i = 0; i < array_size; i++) {
			timer = timers_list[i];
			if (!timer)
				continue;
			if (!get_timer_state(timer))
				continue;
			if (timer_dec(timer, struct_ptr->interval) > 0)
				continue;
			struct_ptr->main_lock();
			timer->timer_expiry(timer->lapb_ptr, timer);
			struct_ptr->main_unlock();
		};
	};

	timer_thread_stopped();
	printf("Timer thread is terminated\n");

	*result = 0;
	return result;

}
