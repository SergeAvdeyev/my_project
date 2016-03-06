#ifndef LOGGER_H
#define LOGGER_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <syslog.h>
#include <string.h>

#include "common.h"

void *logger_function(void * ptr);
void logger_enqueue(char * data, int data_size);

void terminate_logger();
int is_logger_started();

#endif // LOGGER_H
