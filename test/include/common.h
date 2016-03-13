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
#include "lapb_iface.h"

#define TRUE	1
#define FALSE	0

#define	LIB_DEBUG	3

volatile sig_atomic_t exit_flag;

_ushort error_type; /* 0 - no error; 1 - FCS error; 2 - N(R) error */
_ushort error_counter;

/* Called by LAPB to inform X25 that Connect Request is confirmed */
void lapb_connect_confirmation_cb(struct lapb_cs * lapb, int reason);
/* Called by LAPB to inform X25 that connection was initiated by the remote system */
void lapb_connect_indication_cb(struct lapb_cs * lapb, int reason);
/* Called by LAPB to inform X25 that Disconnect Request is confirmed */
void lapb_disconnect_confirmation_cb(struct lapb_cs * lapb, int reason);
/* Called by LAPB to inform X25 that connection was terminated by the remote system */
void lapb_disconnect_indication_cb(struct lapb_cs * lapb, int reason);
/* Called by LAPB to inform X25 about new data */
int lapb_data_indication_cb(struct lapb_cs * lapb, char * data, int data_size);
char * lapb_error_str(int error);

void x25_call_indication_cb(struct x25_cs * x25);
void x25_call_accepted_cb(struct x25_cs * x25);

void setup_signals_handler();
int sleep_ms(int milliseconds);
char * buf_to_str(char * data, int data_size);
void custom_debug(int level, const char * format, ...);
void main_loop(struct x25_cs *x25, struct x25_address *dest_addr);


#endif // COMMON_H

