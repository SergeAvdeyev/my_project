/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */

#ifndef LAPB_IFACE_H
#define LAPB_IFACE_H


//#define USE_GNU_C_Library 0

#ifdef __GNUC__
#include <string.h>
#include <stdlib.h>
#endif

#define INTERNAL_SYNC 1 /* For including pthread.h and make library thread-safe */

#if INTERNAL_SYNC
#include <pthread.h>
#endif

#define	LAPB_ACK_PENDING_CONDITION	0x01
#define	LAPB_REJECT_CONDITION		0x02
#define	LAPB_PEER_RX_BUSY_CONDITION	0x04
#define	LAPB_FRMR_CONDITION			0x08

/* Control field templates */
#define	LAPB_I		0x00	/* Information frames */
#define	LAPB_S		0x01	/* Supervisory frames */
#define	LAPB_U		0x03	/* Unnumbered frames */

#define	LAPB_RR		0x01	/* Receiver ready */
#define	LAPB_RNR	0x05	/* Receiver not ready */
#define	LAPB_REJ	0x09	/* Reject */

#define	LAPB_SABM	0x2F	/* Set Asynchronous Balanced Mode */
#define	LAPB_SABME	0x6F	/* Set Asynchronous Balanced Mode Extended */
#define	LAPB_DISC	0x43	/* Disconnect */
#define	LAPB_DM		0x0F	/* Disconnected mode */
#define	LAPB_UA		0x63	/* Unnumbered acknowledge */
#define	LAPB_FRMR	0x87	/* Frame reject */

#define LAPB_ILLEGAL	0x100	/* Impossible to be a real frame type */

#define	LAPB_SPF	0x10	/* Poll/final bit for standard LAPB */
#define	LAPB_EPF	0x01	/* Poll/final bit for extended LAPB */

#define	LAPB_FRMR_W	0x01	/* Control field invalid	*/
#define	LAPB_FRMR_X	0x02	/* I field invalid		*/
#define	LAPB_FRMR_Y	0x04	/* I field too long		*/
#define	LAPB_FRMR_Z	0x08	/* Invalid N(R)			*/

#define	LAPB_POLLOFF	0
#define	LAPB_POLLON		1

/* LAPB C-bit */
#define LAPB_COMMAND	1
#define LAPB_RESPONSE	2

#define	LAPB_ADDR_A	0x03
#define	LAPB_ADDR_B	0x01
#define	LAPB_ADDR_C	0x0F
#define	LAPB_ADDR_D	0x07

/* Define Link State constants. */
enum {
	LAPB_STATE_0,	/* Disconnected State */
	LAPB_STATE_1,	/* Awaiting Connection State */
	LAPB_STATE_2,	/* Awaiting Disconnection State	*/
	LAPB_STATE_3,	/* Data Transfer State */
	LAPB_STATE_4	/* Frame Reject State */
};


#define	LAPB_SLP		0x00
#define	LAPB_MLP		0x02

#define	LAPB_DTE		0x00
#define	LAPB_DCE		0x04


#define	LAPB_STANDARD	0x00
#define	LAPB_SMODULUS	8
#define	LAPB_DEFAULT_SMODE		(LAPB_STANDARD | LAPB_SLP | LAPB_DTE)
#define	LAPB_DEFAULT_SWINDOW	7		/* Window=7 for standard modulo */

#define	LAPB_EXTENDED	0x01
#define	LAPB_EMODULUS	128
#define	LAPB_DEFAULT_EMODE		(LAPB_EXTENDED | LAPB_SLP | LAPB_DTE)
#define	LAPB_DEFAULT_EWINDOW	127		/* Window=127 for extended modulo */

#define	LAPB_DEFAULT_T201		5000	/* T1=5s    */
#define	LAPB_DEFAULT_T202		1000	/* T2=1s    */
#define	LAPB_DEFAULT_N202		20		/* N202=20 times */
#define	LAPB_DEFAULT_N1			135		/* Default I frame maximal size 135 bytes */





#define	LAPB_OK				0
#define	LAPB_BADTOKEN		1
#define	LAPB_INVALUE		2
#define	LAPB_CONNECTED		3
#define	LAPB_NOTCONNECTED	4
#define	LAPB_REFUSED		5
#define	LAPB_TIMEDOUT		6
#define	LAPB_NOMEM			7
#define LAPB_BUSY			8
#define LAPB_BADFCS			9

#define	LAPB_OK_STR				"OK"
#define	LAPB_BADTOKEN_STR		"Bad token"
#define	LAPB_INVALUE_STR		""
#define	LAPB_CONNECTED_STR		"Connected"
#define	LAPB_NOTCONNECTED_STR	"Not connected"
#define	LAPB_REFUSED_STR		"Refused"
#define	LAPB_TIMEDOUT_STR		"Timed out"
#define	LAPB_NOMEM_STR			"No memory"
#define	LAPB_BUSY_STR			"Transmitter busy"
#define	LAPB_BADFCS_STR			"Bad checksum"

#define TRUE  1
#define FALSE 0

#ifndef UNSIGNED_TYPES
#define UNSIGNED_TYPES
typedef unsigned char	_uchar;
typedef unsigned short	_ushort;
typedef unsigned int	_uint;
typedef unsigned long	_ulong;
#endif

/*
 *	Information about the current frame.
 */
struct lapb_frame {
	_ushort		type;		/* Parsed type		*/
	_ushort		nr, ns;		/* N(R), N(S)		*/
	_uchar		cr;			/* Command/Response	*/
	_uchar		pf;			/* Poll/Final		*/
	_uchar		control[2];	/* Original control data*/
};

struct lapb_params {
	_uchar		mode;				/* See struct lapb_cs. Applies only in LAPB_STATE_0 */
	_uchar		window;				/* See struct lapb_cs. Applies only in LAPB_STATE_0 */
	_ushort		N1;					/* See struct lapb_cs */
	_ushort		T201_interval;		/* See struct lapb_cs */
	_ushort		N201;				/* See struct lapb_cs */
	_ushort		T202_interval;		/* See struct lapb_cs */
	_uchar		low_order_bits;		/* See struct lapb_cs */
	_uchar		auto_connecting;	/* See struct lapb_cs */
};

/*
 *	The per LAPB connection control structure.
 */
struct lapb_cs {
	/* Link status fields */
	_uchar		mode;	/* Bit mask for STANDARD-EXTENDED|SLP-MLP|DTE-DCE */
	_uchar		window;
	_ushort		N1;					/* Maximal I frame size */
	_ushort		T201_interval;		/* Timer T201 interval */
	_ushort		N201;				/* Maximum number of retries for T201 */
	_ushort		T202_interval;		/* Timer T202 interval */
	_uchar		low_order_bits;		/* If TRUE - use low-order bit first orientation for addresses,
									 * commands, responses and sequence numbers */
	_uchar		auto_connecting;	/* If TRUE - Automatic send SABM(E) if we are DTE, state=0 and receive DM from DCE
									 * Note: Resets after calling lapb_disconnect_request() */

	const struct lapb_callbacks *callbacks;
	/* Internal control information */
	void * internal_struct;
	void * L3_ptr;  /* Pointer to packet layer struct */
};



struct lapb_callbacks {
	void (*connect_confirmation)(struct lapb_cs * lapb, int reason);	/* Connection to the remote system has been established, it
																		   was initiated by the local system due to the DL Connect
																		   Request message. */
	void (*connect_indication)(struct lapb_cs * lapb, int reason);		/* Connection to the remote system has been established, it
																		   was initiated by the remote system. */
	void (*disconnect_confirmation)(struct lapb_cs * lapb, int reason);	/* Connection with the remote system has been terminated, it
																		   was terminated by the local system due to the DL
																		   Disconnect Request message. */
	void (*disconnect_indication)(struct lapb_cs * lapb, int reason);	/* Connection with the remote system has been terminated, it
																		   was terminated by the remote system, or the connection
																		   initiation requested by the DL Connect Request message
																		   has been refused by the remote system. */
	int  (*data_indication)(struct lapb_cs * lapb, char * data, int data_size);	/* Data from the remote system has been received. */
	void (*transmit_data)(struct lapb_cs * lapb, char *data, int data_size);

	void * (*add_timer)(int interval, void * lapb_ptr, void (*timer_expiry));
	void (*del_timer)(void * timer);
	void (*start_timer)(void * timer);
	void (*stop_timer)(void * timer);

	//void (*debug)(struct lapb_cs *lapb, int level, const char * format, ...);
	void (*debug)(int level, const char * format, ...);
};


/* lapb_iface.c */
extern int lapb_register(struct lapb_callbacks *callbacks, struct lapb_params * params, struct lapb_cs ** lapb);
extern int lapb_unregister(struct lapb_cs * lapb);
extern int lapb_get_params(struct lapb_cs * lapb, struct lapb_params * params);
extern int lapb_set_params(struct lapb_cs * lapb, struct lapb_params * params);
extern char * lapb_dequeue(struct lapb_cs * lapb, int * buffer_size);
extern int lapb_reset(struct lapb_cs * lapb, _uchar init_state);
extern int lapb_connect_request(void *lapb_ptr);
extern int lapb_disconnect_request(void *lapb_ptr);
extern int lapb_data_request(void *lapb_ptr, char * data, int data_size);
extern int lapb_get_state(struct lapb_cs *lapb);

/* Executing by physical leyer (when new incoming data received) */
extern int lapb_data_received(struct lapb_cs *lapb, char * data, int data_size, _ushort fcs);


/* lapb_subr.c */
//extern void lapb_send_control(struct lapb_cb *lapb, int, int, int);
char * lapb_error_str(int error);
int lapb_is_dce(struct lapb_cs *lapb);
int lapb_is_extended(struct lapb_cs *lapb);
int lapb_is_slp(struct lapb_cs *lapb);

/* lapb_timer.c */
extern void lapb_t201timer_expiry(void * lapb_ptr);
extern void lapb_t202timer_expiry(void * lapb_ptr);


/*
 * Debug levels.
 *	0 = Off
 *	1 = State Changes
 *	2 = Packets I/O and State Changes
 *	3 = Hex dumps, Packets I/O and State Changes.
 */
#define	LAPB_DEBUG	3

#endif
