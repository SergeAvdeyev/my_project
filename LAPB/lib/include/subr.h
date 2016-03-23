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

#ifndef UNSIGNED_TYPES
#define UNSIGNED_TYPES
typedef unsigned char	_uchar;
typedef unsigned short	_ushort;
typedef unsigned int	_uint;
typedef unsigned long	_ulong;
#endif

char * buf_to_str(char * data, int data_size);
/* Redefine 'malloc' and 'free' */
extern void * mem_get(_ulong size);
void mem_free(void *ptr);
void * mem_copy(void *dest, const void *src, _ulong n);
void mem_zero(void *src, _ulong n);

#endif // LAPB_INT_H

