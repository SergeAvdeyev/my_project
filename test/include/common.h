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

//#define	LIB_DEBUG	3

volatile sig_atomic_t exit_flag;

_ushort error_type; /* 0 - no error; 1 - FCS error; 2 - N(R) error */
_ushort error_counter;

void main_lock();
void main_unlock();

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

/* Called by X25 to inform App that Call request was initiated by the remote system */
void x25_call_indication_cb(struct x25_cs * x25);
/* Called by X25 to inform App that Call request is confirmed */
void x25_call_accepted_cb(struct x25_cs * x25);
/* Called by X25 to inform App that Clear request was initiated by the remote system */
void x25_clear_indication_cb(struct x25_cs * x25);
/* Called by X25 to inform App that Clear request is confirmed */
void x25_clear_accepted_cb(struct x25_cs * x25);
/* Called by X25 to inform App that Call request is confirmed */
int x25_data_indication_cb(struct x25_cs * x25, char * data, int data_size);

void setup_signals_handler();
int sleep_ms(int milliseconds);
char * buf_to_str(char * data, int data_size);
void lapb_debug(int level, const char * format, ...);
void x25_debug(int level, const char * format, ...);
void main_loop(struct x25_cs *x25, struct x25_address *dest_addr);


#endif // COMMON_H

