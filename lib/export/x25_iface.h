/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */

#ifndef X25_IFACE_H
#define X25_IFACE_H


#ifdef __GNUC__
#include <string.h>
#include <stdlib.h>
#endif

#define INTERNAL_SYNC 1 /* For including pthread.h and make library thread-safe */

#if INTERNAL_SYNC
#include <pthread.h>
#endif

#include "queue.h"




#define	X25_OK				0
#define	X25_BADTOKEN		1
#define	X25_INVALUE			2
#define	X25_CONNECTED		3
#define	X25_NOTCONNECTED	4
#define	X25_REFUSED			5
#define	X25_TIMEDOUT		6
#define	X25_NOMEM			7
#define X25_BUSY			8
#define X25_BADFCS			9
#define X25_ROOT_UNREACH	10

#define	X25_OK_STR				"OK"
#define	X25_BADTOKEN_STR		"Bad token"
#define	X25_INVALUE_STR		""
#define	X25_CONNECTED_STR		"Connected"
#define	X25_NOTCONNECTED_STR	"Not connected"
#define	X25_REFUSED_STR		"Refused"
#define	X25_TIMEDOUT_STR		"Timed out"
#define	X25_NOMEM_STR			"No memory"
#define	X25_BUSY_STR			"Transmitter busy"
#define	X25_BADFCS_STR			"Bad checksum"


#define X25_DEFAULT_T20		200		/* Default restart_request_timeout value 200ms */
#define X25_DEFAULT_T21		200		/* Default call_request_timeout value 200ms */
#define X25_DEFAULT_T22		200		/* Default reset_request_timeout value 200ms */
#define	X25_DEFAULT_T23		200		/* Default clear_request_timeout value 200ms */
#define	X25_DEFAULT_T2		1000	/* Default ack_holdback_timeout value 1s */

/*
 *	X.25 Packet Size values.
 */
#define	X25_PS16		4
#define	X25_PS32		5
#define	X25_PS64		6
#define	X25_PS128		7
#define	X25_PS256		8
#define	X25_PS512		9
#define	X25_PS1024		10
#define	X25_PS2048		11
#define	X25_PS4096		12

#define	X25_DEFAULT_WINDOW_SIZE	2			/* Default Window Size	*/
#define	X25_DEFAULT_PACKET_SIZE	X25_PS128	/* Default Packet Size */
#define	X25_DEFAULT_THROUGHPUT	0x0A		/* Deafult Throughput */
#define	X25_DEFAULT_REVERSE		0x00		/* Default Reverse Charging */

#define X25_SMODULUS 		8
#define	X25_EMODULUS		128

/*
 *	X.25 Facilities constants.
 */

#define	X25_FAC_CLASS_MASK	0xC0

#define	X25_FAC_CLASS_A		0x00
#define	X25_FAC_CLASS_B		0x40
#define	X25_FAC_CLASS_C		0x80
#define	X25_FAC_CLASS_D		0xC0

#define	X25_FAC_REVERSE		0x01			/* also fast select */
#define	X25_FAC_THROUGHPUT	0x02
#define	X25_FAC_PACKET_SIZE	0x42
#define	X25_FAC_WINDOW_SIZE	0x43

#define X25_MAX_FAC_LEN 	60
#define	X25_MAX_CUD_LEN		128

#define X25_FAC_CALLING_AE 	0xCB
#define X25_FAC_CALLED_AE 	0xC9

#define X25_MARKER				0x00
#define X25_DTE_SERVICES		0x0F
#define X25_MAX_AE_LEN			40			/* Max num of semi-octets in AE - OSI Nw */
#define X25_MAX_DTE_FACIL_LEN	21			/* Max length of DTE facility params */

/* values for above global_facil_mask */

#define	X25_MASK_REVERSE	0x01
#define	X25_MASK_THROUGHPUT	0x02
#define	X25_MASK_PACKET_SIZE	0x04
#define	X25_MASK_WINDOW_SIZE	0x08

#define X25_MASK_CALLING_AE 0x10
#define X25_MASK_CALLED_AE 0x20


#define	X25_ADDR_LEN			16

#define	X25_STD_MIN_LEN			3
#define	X25_EXT_MIN_LEN			4

#define	X25_GFI_SEQ_MASK		0x30
#define	X25_GFI_STDSEQ			0x10
#define	X25_GFI_EXTSEQ			0x20

#define	X25_Q_BIT				0x80
#define	X25_D_BIT				0x40
#define	X25_STD_M_BIT			0x10
#define	X25_EXT_M_BIT			0x01

#define	X25_CALL_REQUEST				0x0B
#define	X25_CALL_ACCEPTED				0x0F
#define	X25_CLEAR_REQUEST				0x13
#define	X25_CLEAR_CONFIRMATION			0x17
#define	X25_DATA						0x00
#define	X25_INTERRUPT					0x23
#define	X25_INTERRUPT_CONFIRMATION		0x27
#define	X25_RR							0x01
#define	X25_RNR							0x05
#define	X25_REJ							0x09
#define	X25_RESET_REQUEST				0x1B
#define	X25_RESET_CONFIRMATION			0x1F
#define	X25_REGISTRATION_REQUEST		0xF3
#define	X25_REGISTRATION_CONFIRMATION	0xF7
#define	X25_RESTART_REQUEST				0xFB
#define	X25_RESTART_CONFIRMATION		0xFF
#define	X25_DIAGNOSTIC					0xF1
#define	X25_ILLEGAL						0xFD







#define TRUE  1
#define FALSE 0


#ifndef UNSIGNED_TYPES
#define UNSIGNED_TYPES
typedef unsigned char	_uchar;
typedef unsigned short	_ushort;
typedef unsigned int	_uint;
typedef unsigned long	_ulong;
#endif


enum {
	X25_STATE_0,	/* Ready */
	X25_STATE_1,	/* Awaiting Call Accepted */
	X25_STATE_2,	/* Awaiting Clear Confirmation */
	X25_STATE_3,	/* Data Transfer */
	X25_STATE_4		/* Awaiting Reset Confirmation */
};

struct sk_buff {
	_uchar * data;
	int	   data_size;
};

struct x25_callbacks {
	int (*link_connect_request)(void * link_ptr);
	int (*link_send_frame)(void * link_ptr, char * data, int data_size);
	void * (*add_timer)(int interval, void * lapb_ptr, void (*timer_expiry));
	void (*del_timer)(void * timer);
	void (*start_timer)(void * timer);
	void (*stop_timer)(void * timer);

	void (*debug)(int level, const char * format, ...);
};

struct x25_link_callbacks {
	void * (*add_timer)(int interval, void * lapb_ptr, void (*timer_expiry));
	void (*del_timer)(void * timer);
	void (*start_timer)(void * timer);
	void (*stop_timer)(void * timer);

	void (*debug)(int level, const char * format, ...);
};

struct x25_params {
};


/*
 * An X.121 address, it is held as ASCII text, null terminated, up to 15
 * digits and a null terminator.
 */
struct x25_address {
	char x25_addr[16];
};

/*
 *	Call clearing Cause and Diagnostic structure.
 */
struct x25_causediag {
	_uchar	cause;
	_uchar	diagnostic;
};

/*
 *	Facilities structure.
 */
struct x25_facilities {
	_uint	winsize_in;
	_uint	winsize_out;
	_uint	pacsize_in;
	_uint	pacsize_out;
	_uint	throughput;
	_uint	reverse;
};

/*
* ITU DTE facilities
* Only the called and calling address
* extension are currently implemented.
* The rest are in place to avoid the struct
* changing size if someone needs them later
*/
struct x25_dte_facilities {
	_ushort	delay_cumul;
	_ushort	delay_target;
	_ushort	delay_max;
	_uchar	min_throughput;
	_uchar	expedited;
	_uchar	calling_len;
	_uchar	called_len;
	_uchar	calling_ae[20];
	_uchar	called_ae[20];
};

/*
 *	Call User Data structure.
 */
struct x25_calluserdata {
	_uint	cudlength;
	_uchar	cuddata[128];
};

struct x25_timer {
	_ulong		interval;
	void *		timer_ptr;
	_uchar		state;
};


struct x25_link {
	void *		link_ptr;
	_uint		state;
	_uint		extended;
	struct circular_buffer	queue;
	struct x25_timer	T20;
	//_ulong		t20;
	//void *	t20timer;
	_ulong		global_facil_mask;
};

/* X.25 Control structure */
struct x25_cs {
	//struct sock		sk;
	struct x25_address	source_addr;
	struct x25_address	dest_addr;
	struct x25_link		link;
	_uint		lci;
	_uint		cudmatchlength;
	_uchar		state;
	_uchar		condition;
	_ushort		vs;
	_ushort		vr;
	_ushort		va;
	_ushort		vl;

	struct x25_timer	T2;
	struct x25_timer	T21;
	struct x25_timer	T22;
	struct x25_timer	T23;

//	_ulong		t2_interval;
//	void *		t2_timer_ptr;
//	_uchar		t2_state;

//	_ulong		t21_interval;
//	void *		t21_timer_ptr;
//	_uchar		t21_state;

//	_ulong		t22_interval;
//	void *		t22_timer_ptr;
//	_uchar		t22_state;

//	_ulong		t23_interval;
//	void *		t23_timer_ptr;
//	_uchar		t23_state;

	_ushort		fraglen;
	_ulong		flags;

	struct circular_buffer	ack_queue;
	struct circular_buffer	fragment_queue;
	struct circular_buffer	interrupt_in_queue;
	struct circular_buffer	interrupt_out_queue;

	//void *	timer;
	struct x25_causediag	causediag;
	struct x25_facilities	facilities;
	struct x25_dte_facilities dte_facilities;
	struct x25_calluserdata	calluserdata;
	_ulong 		vc_facil_mask;	/* inc_call facilities mask */

	const struct x25_callbacks *callbacks;
	/* Internal control information */
	void * internal_struct;
};





/* x25_iface.c */
extern int x25_register(struct x25_callbacks *callbacks, struct x25_params * params, struct x25_cs ** x25);
extern int x25_unregister(struct x25_cs * x25);
extern int x25_get_params(struct x25_cs * x25, struct x25_params * params);
extern int x25_set_params(struct x25_cs * x25, struct x25_params * params);

//int x25_connect_request(struct x25_cs * x25, struct x25_address *uaddr, int addr_len);
void x25_add_link(struct x25_cs *x25, void * link, int extended);
int x25_connect_request(struct x25_cs * x25, struct x25_address *dest_addr);


/* x25_link.c */
void x25_link_established(void *x25_ptr);

/* lapb_subr.c */

/* lapb_timer.c */


/*
 * Debug levels.
 *	0 = Off
 *	1 = State Changes
 *	2 = Packets I/O and State Changes
 *	3 = Hex dumps, Packets I/O and State Changes.
 */
#define	X25_DEBUG	3

#endif
