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

//#define INTERNAL_SYNC 1 /* For including pthread.h and make library thread-safe */

//#if INTERNAL_SYNC
//#include <pthread.h>
//#endif

#include "queue.h"



/* X25 function results */
#define	X25_OK				0
#define	X25_BADTOKEN		1
#define	X25_INVALUE			2
#define	X25_CONNECTED		3
#define	X25_NOTCONNECTED	4
#define	X25_REFUSED			5
#define	X25_TIMEDOUT		6
#define	X25_NOMEM			7
#define X25_BUSY			8
//#define X25_BADFCS			9
#define X25_ROOT_UNREACH	10
#define X25_CALLBACK_ERR	11
#define X25_MSGSIZE			12

/* X25 fuction result descriptions */
#define	X25_OK_STR				"OK"
#define	X25_BADTOKEN_STR		"Bad token"
#define	X25_INVALUE_STR			"Invalid value"
#define	X25_CONNECTED_STR		"Connected"
#define	X25_NOTCONNECTED_STR	"Not connected"
#define	X25_REFUSED_STR			"Refused"
#define	X25_TIMEDOUT_STR		"Timed out"
#define	X25_NOMEM_STR			"No memory"
#define	X25_BUSY_STR			"Transmitter busy"
//#define	X25_BADFCS_STR			"Bad checksum"
#define X25_ROOT_UNREACH_STR	"Destination unreacheble"
#define X25_CALLBACK_ERR_STR	"Bad callback pointer"
#define X25_MSGSIZE_STR			"Message size is too long"


#define X25_DEFAULT_RESTART_TIMER	180000	/* Default restart_request_timeout value 180s */
#define X25_DEFAULT_CALL_TIMER		200000	/* Default call_request_timeout value 200s */
#define X25_DEFAULT_RESET_TIMER		180000	/* Default reset_request_timeout value 180s */
#define	X25_DEFAULT_CLEAR_TIMER		180000	/* Default clear_request_timeout value 180s */
#define	X25_DEFAULT_ACK_TIMER		3000	/* Default ack holdback value 3s */
#define	X25_DEFAULT_DATA_TIMER		1000	/* Default data retransmition timeout value 1s */
//#define	X25_DEFAULT_T_LINK	1000	/* Default Link timer value 1s */

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

#define	X25_DTE			0x00
#define	X25_DCE			0x01

#define	X25_STANDARD	0x00
#define	X25_EXTENDED	0x02

#define X25_SMODULUS 	8
#define	X25_EMODULUS	128

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

#define	X25_MASK_REVERSE		0x01
#define	X25_MASK_THROUGHPUT		0x02
#define	X25_MASK_PACKET_SIZE	0x04
#define	X25_MASK_WINDOW_SIZE	0x08

#define X25_MASK_CALLING_AE		0x10
#define X25_MASK_CALLED_AE		0x20


#define	X25_ADDR_LEN			16



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


struct x25_cs;

struct x25_callbacks {
	void (*call_indication)(struct x25_cs * x25);
	void (*call_accepted)(struct x25_cs * x25);
	void (*clear_indication)(struct x25_cs * x25);
	void (*clear_accepted)(struct x25_cs * x25);
	int  (*data_indication)(struct x25_cs * x25, char * data, int data_size);	/* Data from the remote system has been received. */

	int (*link_connect_request)(void * link_ptr);
	int (*link_disconnect_request)(void * link_ptr);
	int (*link_send_frame)(void * link_ptr, char * data, int data_size);
	void * (*add_timer)(int interval, void * lapb_ptr, void (*timer_expiry));
	void (*del_timer)(void * timer);
	void (*start_timer)(void * timer);
	void (*stop_timer)(void * timer);

	void (*debug)(int level, const char * format, ...);
};


struct x25_params {
	_ulong RestartTimerInterval;
	int RestartTimerNR;
	_ulong CallTimerInterval;
	int CallTimerNR;
	_ulong ResetTimerInterval;
	int ResetTimerNR;
	_ulong ClearTimerInterval;
	int ClearTimerNR;
	_ulong AckTimerInterval;
	int AckTimerNR;
	_ulong DataTimerInterval;
	int DataTimerNR;

	_uint WinsizeIn;
	_uint WinsizeOut;
	_uint PacsizeIn;
	_uint PacsizeOut;
};


/*
 * An X.121 address, it is held as ASCII text, null terminated, up to 15
 * digits and a null terminator.
 */
struct x25_address {
	char x25_addr[16];	/* Null-terminated address */
};


struct x25_timer {
	void *		timer_ptr;	/* Pointer to timer object */
	_ulong		interval;	/* Interval for given timer in msec */
	_uchar		state;		/* Current state for given timer */
	_uchar		RC;			/* Retry counter for given timer */
};



struct x25_link {
	void *		link_ptr;
	_uint		state;
	struct circular_buffer	queue;
	_ulong		global_facil_mask;
};

/* X.25 Control structure */
struct x25_cs {
	_uchar				mode;	/* Bit mask for STANDARD-EXTENDED|DTE-DCE */
	struct x25_address	source_addr;
	struct x25_address	dest_addr;
	struct x25_link		link;
	_uint				lci;
	_uint				peer_lci;
	_uint				cudmatchlength;

	_uchar				RestartTimer_NR;	/* Maximum number of retries for Restart timer */
	_uchar				CallTimer_NR;		/* Maximum number of retries for Call timer */
	_uchar				ResetTimer_NR;		/* Maximum number of retries for Reset timer */
	_uchar				ClearTimer_NR;		/* Maximum number of retries for Clear timer */
	_uchar				AckTimer_NR;		/* Maximum number of retries for Ack timer */
	_uchar				DataTimer_NR;		/* Maximum number of retries for Data timer */

	const struct x25_callbacks *callbacks;
	/* Internal control information */
	void * internal_struct;
};





/* x25_iface.c */
int x25_register(struct x25_callbacks *callbacks, struct x25_params * params, struct x25_cs ** x25);
int x25_unregister(struct x25_cs * x25);
int x25_get_params(struct x25_cs * x25, struct x25_params * params);
int x25_set_params(struct x25_cs * x25, struct x25_params * params);
void x25_add_link(struct x25_cs *x25, void * link);
int x25_get_state(struct x25_cs *x25);

int x25_call_request(struct x25_cs * x25, struct x25_address *dest_addr);
int x25_clear_request(struct x25_cs * x25);
int x25_call_request(struct x25_cs * x25, struct x25_address *dest_addr);
int x25_sendmsg(struct x25_cs * x25, char * data, int data_size, _uchar out_of_band, _uchar q_bit_flag);


/* x25_link.c */
void x25_link_established(void *x25_ptr);
void x25_link_terminated(void *x25_ptr);
int x25_link_receive_data(void *x25_ptr, char * data, int data_size);


/* x25_subr.c */
char * x25_error_str(int error);


/* x25_out.c */
void x25_check_iframes_acked(struct x25_cs *x25, _ushort nr);






/*
 * Debug levels.
 *	0 = Off
 *	1 = State Changes
 *	2 = Packets I/O and State Changes
 *	3 = Hex dumps, Packets I/O and State Changes.
 */
#define	X25_DEBUG	1

#endif
