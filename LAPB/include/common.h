#ifndef COMMON_H
#define COMMON_H

#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>

#include "net_lapb.h"

#define TRUE	1
#define FALSE	0

volatile sig_atomic_t exit_flag;
pthread_mutex_t main_mutex;

//unsigned char lapb_equipment_type = LAPB_DTE;
//unsigned char lapb_modulo = LAPB_STANDARD;

struct main_callbacks {
	int (*is_connected)();

	//void (*print_commands_0)(struct lapb_cb * lapb);
	//void (*print_commands_1)(struct lapb_cb * lapb);
	//void (*print_commands_2)(struct lapb_cb * lapb);
	//void (*print_commands_3)(struct lapb_cb * lapb);
};

void setup_signals_handler();
int sleep_ms(int milliseconds);
void main_loop(struct lapb_cb *lapb, const struct main_callbacks * callbacks, unsigned char lapb_equipment_type, unsigned char lapb_modulo);
int wait_stdin(struct lapb_cb * lapb, unsigned char break_condition, int run_once);

char * lapb_error_str(int error);

void main_lock();
void main_unlock();

#endif // COMMON_H

