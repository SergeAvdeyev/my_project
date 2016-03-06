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

#include "x25_iface.h"

#define TRUE	1
#define FALSE	0

volatile sig_atomic_t exit_flag;

_ushort error_type; /* 0 - no error; 1 - FCS error; 2 - N(R) error */
_ushort error_counter;


//struct main_callbacks {
//	int (*is_connected)();

//};



/* Called by LAPB to inform X25 that Connect Request is confirmed */
void connect_confirmation(struct x25_cs * lapb, int reason);
/* Called by LAPB to inform X25 that connection was initiated by the remote system */
void connect_indication(struct x25_cs * lapb, int reason);
/* Called by LAPB to inform X25 that Disconnect Request is confirmed */
void disconnect_confirmation(struct x25_cs * lapb, int reason);
/* Called by LAPB to inform X25 that connection was terminated by the remote system */
void disconnect_indication(struct x25_cs * lapb, int reason);
/* Called by LAPB to inform X25 about new data */
int data_indication(struct x25_cs * lapb, char * data, int data_size);

/* Called by LAPB to write debug info */
void x25_debug(int level, const char * format, ...);


void setup_signals_handler();
int sleep_ms(int milliseconds);
char * buf_to_str(char * data, int data_size);
void main_loop(struct x25_cs *lapb);
int wait_stdin(struct x25_cs * lapb, unsigned char break_condition, int run_once);

char * x25_error_str(int error);

#endif // COMMON_H

