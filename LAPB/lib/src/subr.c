/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */


#include "subr.h"

char str_buf[1024];

char * buf_to_str(char * data, int data_size) {
	str_buf[0] = '\0';
	if (data_size < 1024) {/* 1 byte for null-terminating */
		mem_copy(str_buf, data, data_size);
		str_buf[data_size] = '\0';
	};
	return str_buf;
}

void * mem_get(_ulong size) {
#ifdef __GNUC__
	return malloc(size);
#else
	size = 0;
	return (void *)size;
#endif
}

void mem_free(void * ptr) {
#ifdef __GNUC__
	free(ptr);
#else
	*(int *)ptr = 0;
#endif
}

void * mem_copy(void *dest, const void *src, _ulong n) {
#ifdef __GNUC__
	return memcpy(dest, src, n);
#else
	_ulong i = 0;
	while (i < n) {
		*(char *)dest = *(char *)src;
		i++;
	};
	return dest;
#endif
}

void mem_zero(void *src, _ulong n) {
#ifdef __GNUC__
	bzero(src, n);
#else
	_ulong i = 0;
	while (i < n) {
		*(char *)src = 0;
		i++;
	};
#endif
}

