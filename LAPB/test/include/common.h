#ifndef COMMON_H
#define COMMON_H

#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdarg.h>

#include "lapb_iface.h"

#define TRUE	1
#define FALSE	0

volatile sig_atomic_t exit_flag;

_ushort fcs;


struct main_callbacks {
	int (*is_connected)();

};



/* Called by LAPB to inform X25 that SABM(SABME) is confirmed or UA sended on SABM(SABME) command */
void on_connected(struct lapb_cb * lapb, int reason);
/* Called by LAPB to inform X25 that DISC is received or UA sended on DISC command */
void on_disconnected(struct lapb_cb * lapb, int reason);
/* Called by LAPB to inform X25 about new data */
int on_new_incoming_data(struct lapb_cb * lapb, char * data, int data_size);
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
/* Called by LAPB to check timer T2 state */
int t2timer_running();
/* Called by LAPB to write debug info */
void lapb_debug(struct lapb_cb *lapb, int level, const char * format, ...);


void t1timer_expiry(unsigned long int lapb_addr);
void t2timer_expiry(unsigned long int lapb_addr);



void setup_signals_handler();
int sleep_ms(int milliseconds);
char * buf_to_str(char * data, int data_size);
void main_loop(struct lapb_cb *lapb, const struct main_callbacks * callbacks);
int wait_stdin(struct lapb_cb * lapb, unsigned char break_condition, int run_once);

char * lapb_error_str(int error);

//void main_lock();
//void main_unlock();

#endif // COMMON_H

