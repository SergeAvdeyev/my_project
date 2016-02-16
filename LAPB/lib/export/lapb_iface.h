#ifndef LAPB_IFACE_H
#define LAPB_IFACE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define	LAPB_ACK_PENDING_CONDITION	0x01
#define	LAPB_REJECT_CONDITION		0x02
#define	LAPB_PEER_RX_BUSY_CONDITION	0x04

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
#define	LAPB_POLLON	1

/* LAPB C-bit */
#define LAPB_COMMAND	1
#define LAPB_RESPONSE	2

#define	LAPB_ADDR_A	0x03
#define	LAPB_ADDR_B	0x01
#define	LAPB_ADDR_C	0x0F
#define	LAPB_ADDR_D	0x07

/* Define Link State constants. */
enum {
	LAPB_STATE_0,	/* Disconnected State		*/
	LAPB_STATE_1,	/* Awaiting Connection State	*/
	LAPB_STATE_2,	/* Awaiting Disconnection State	*/
	LAPB_STATE_3,	/* Data Transfer State		*/
	LAPB_STATE_4,	/* Frame Reject State		*/
	LAPB_NOT_READY	/* Physical layer not ready */
};

#define	LAPB_STANDARD	0x00
#define	LAPB_EXTENDED	0x01

#define	LAPB_SLP		0x00
#define	LAPB_MLP		0x02

#define	LAPB_DTE		0x00
#define	LAPB_DCE		0x04


#define	LAPB_DEFAULT_MODE		(LAPB_STANDARD | LAPB_SLP | LAPB_DTE)
#define	LAPB_DEFAULT_WINDOW		7		/* Window=7 */
#define	LAPB_DEFAULT_T1			5000	/* T1=5s    */
#define	LAPB_DEFAULT_T2			1000	/* T2=1s    */
#define	LAPB_DEFAULT_N2			20		/* N2=20    */
#define	LAPB_DEFAULT_N1			135		/* Default I frame maximal size */

#define	LAPB_SMODULUS	8
#define	LAPB_EMODULUS	128




#define	LAPB_OK				0
#define	LAPB_BADTOKEN		1
#define	LAPB_INVALUE		2
#define	LAPB_CONNECTED		3
#define	LAPB_NOTCONNECTED	4
#define	LAPB_REFUSED		5
#define	LAPB_TIMEDOUT		6
#define	LAPB_NOMEM			7
#define LAPB_NOTREADY		8
#define LAPB_BADFCS			9

#define	LAPB_OK_STR				"OK"
#define	LAPB_BADTOKEN_STR		"Bad token"
#define	LAPB_INVALUE_STR		""
#define	LAPB_CONNECTED_STR		"Connected"
#define	LAPB_NOTCONNECTED_STR	"Not connected"
#define	LAPB_REFUSED_STR		"Refused"
#define	LAPB_TIMEDOUT_STR		"Timed out"
#define	LAPB_NOMEM_STR			"No mem"
#define	LAPB_NOTREADY_STR		"Phys layer not ready"


typedef unsigned char	_uchar;
typedef unsigned short	_ushort;
typedef unsigned int	_uint;
typedef unsigned long	_ulong;

struct circular_buffer {
        char * buffer;		// data buffer
        char * buffer_end;	// end of data buffer
        size_t capacity;	// maximum number of items in the buffer
        size_t count;		// number of items in the buffer
        size_t sz;			// size of each item in the buffer
        char * head;		// pointer to head
        char * tail;		// pointer to tail
};


/*
 *	Information about the current frame.
 */
struct lapb_frame {
	_ushort		type;		/* Parsed type		*/
	_ushort		nr, ns;		/* N(R), N(S)		*/
	_uchar		cr;		/* Command/Response	*/
	_uchar		pf;		/* Poll/Final		*/
	_uchar		control[2];	/* Original control data*/
};

/*
 *	The per LAPB connection control structure.
 */
struct lapb_cb {
	/* Link status fields */
	_uchar		mode;	/* Bit mask for STANDARD-EXTENDED|SLP-MLP|DTE-DCE */
	_uchar		state;
	_ushort		vs, vr, va;
	_uchar		condition;
	_ushort		N2;
	_ushort		N2count;
	_ushort		T1, T2;

	/* Internal control information */
	_ushort		N1; /* Maximal I frame size */

	struct circular_buffer	write_queue;
	struct circular_buffer	ack_queue;

	_uchar		window;
	const struct lapb_register_struct *callbacks;

	/* FRMR control information */
	struct lapb_frame	frmr_data;
	_uchar		frmr_type;
};



struct lapb_register_struct {
	void (*on_connected)(struct lapb_cb * lapb, int reason);
	void (*on_disconnected)(struct lapb_cb * lapb, int reason);
	int  (*on_new_data)(struct lapb_cb * lapb, char * data, int data_size);
	void (*transmit_data)(struct lapb_cb * lapb, char *data, int data_size);

	void (*start_t1timer)(struct lapb_cb * lapb);
	void (*stop_t1timer)();
	void (*start_t2timer)(struct lapb_cb * lapb);
	void (*stop_t2timer)();
	int  (*t1timer_running)();
	int  (*t2timer_running)();

	void (*debug)(struct lapb_cb *lapb, int level, const char * format, ...);
};

struct lapb_parms_struct {
	_uint T1;
	_uint t1timer;
	_uint T2;
	_uint t2timer;
	_uint N1;
	_uint N2;
	_uint N2count;
	_uint window;
	_uint state;
	_uint mode;
};

/* lapb_iface.c */
extern int lapb_register(struct lapb_register_struct *callbacks, struct lapb_cb ** lapb);
extern int lapb_unregister(struct lapb_cb * lapb);
extern char * lapb_dequeue(struct lapb_cb * lapb, int * buffer_size);
extern int lapb_reset(struct lapb_cb * lapb, _uchar init_state);
extern int lapb_getparms(struct lapb_cb * lapb, struct lapb_parms_struct *parms);
extern int lapb_setparms(struct lapb_cb * lapb, struct lapb_parms_struct *parms);
extern int lapb_connect_request(struct lapb_cb *lapb);
extern int lapb_disconnect_request(struct lapb_cb *lapb);
extern int lapb_data_request(struct lapb_cb *lapb, char * data, int data_size);

/* Executing by physical leyer (when new incoming data received) */
extern int lapb_data_received(struct lapb_cb *lapb, char * data, int data_size, _ushort fcs);


/* lapb_subr.c */
extern void lapb_send_control(struct lapb_cb *lapb, int, int, int);

/* lapb_timer.c */
extern void lapb_t1timer_expiry(struct lapb_cb *lapb);
extern void lapb_t2timer_expiry(struct lapb_cb *lapb);


/*
 * Debug levels.
 *	0 = Off
 *	1 = State Changes
 *	2 = Packets I/O and State Changes
 *	3 = Hex dumps, Packets I/O and State Changes.
 */
#define	LAPB_DEBUG	3

#endif