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

#include <stdarg.h>

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



/* Called by LAPB to inform X25 that SABM(SABME) is confirmed */
void connect_confirmation(struct lapb_cb * lapb, int reason);
/* Called by LAPB to inform X25 that SABM(SABME) is received and UA sended */
void connect_indication(struct lapb_cb * lapb, int reason);
/* Called by LAPB to inform X25 that DISC is confirmed */
void disconnect_confirmation(struct lapb_cb * lapb, int reason);
/* Called by LAPB to inform X25 that DISC is received and UA sended */
void disconnect_indication(struct lapb_cb * lapb, int reason);
/* Called by LAPB to inform X25 about new data */
int data_indication(struct lapb_cb * lapb, unsigned char * data, int data_size);
///* Called by LAPB to transmit data via physical connection */
//void data_transmit(struct lapb_cb * lapb, char *data, int data_size);
/* Called by LAPB to start timer T1 */
void start_t1timer(struct lapb_cb * lapb);
/* Called by LAPB to stop timer T1 */
void stop_t1timer();
/* Called by LAPB to check timer T1 state */
int t1timer_running();
/* Called by LAPB to start timer T2 */
void start_t2timer(struct lapb_cb * lapb);
/* Called by LAPB to stop timer T1 */
void stop_t2timer();
/* Called by LAPB to write debug info */
void lapb_debug(struct lapb_cb *lapb, int level, const char * format, ...);


void t1timer_expiry(unsigned long int lapb_addr);
void t2timer_expiry(unsigned long int lapb_addr);



void setup_signals_handler();
int sleep_ms(int milliseconds);
void main_loop(struct lapb_cb *lapb, const struct main_callbacks * callbacks, unsigned char lapb_equipment_type, unsigned char lapb_modulo);
int wait_stdin(struct lapb_cb * lapb, unsigned char break_condition, int run_once);

char * lapb_error_str(int error);

void main_lock();
void main_unlock();

#endif // COMMON_H

