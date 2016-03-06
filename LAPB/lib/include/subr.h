/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */

#ifndef SUBR_H
#define SUBR_H

#ifdef __GNUC__
#include <string.h>
#include <stdlib.h>
#endif

char * buf_to_str(char * data, int data_size);
/* Redefine 'malloc' and 'free' */
extern void * mem_get(unsigned long size);
void mem_free(void *ptr);
void * mem_copy(void *dest, const void *src, unsigned long n);
void mem_zero(void *src, unsigned long n);

#endif // LAPB_INT_H

