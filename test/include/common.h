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

#define	LIB_DEBUG	3

volatile sig_atomic_t exit_flag;

_ushort error_type; /* 0 - no error; 1 - FCS error; 2 - N(R) error */
_ushort error_counter;



void setup_signals_handler();
int sleep_ms(int milliseconds);
char * buf_to_str(char * data, int data_size);
void custom_debug(int level, const char * format, ...);
void main_loop();


#endif // COMMON_H

