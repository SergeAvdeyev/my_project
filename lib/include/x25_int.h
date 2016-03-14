/*
 *	LAPB release 001
 *
 *  By Serge.V.Avdeyev
 *
 *  2016-02-01: Start Coding
 *
 *
 */

#ifndef X25_INT_H
#define X25_INT_H

#include <stdio.h>

#include "x25_iface.h"
//#include "queue.h"




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

/* Define the various conditions that may exist */
#define	X25_COND_ACK_PENDING	0x01
#define	X25_COND_OWN_RX_BUSY	0x02
#define	X25_COND_PEER_RX_BUSY	0x04

/* Bitset in x25_cs->flags for misc flags */
#define X25_Q_BIT_FLAG			0
#define X25_INTERRUPT_FLAG		1
#define X25_ACCPT_APPRV_FLAG	2



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


/* Define Link State constants. */
enum {
	X25_LINK_STATE_0,
	X25_LINK_STATE_1,
	X25_LINK_STATE_2,
	X25_LINK_STATE_3
};


/* Private lapb properties */
struct x25_cs_internal {
	_uchar		state;
	_uchar		condition;
	_ushort		vs;
	_ushort		vr;
	_ushort		va;
	_ushort		vl;

	struct x25_timer	T20;
//	_ushort				N20;	/* Maximum number of retries for T20 */
	_ushort				N20_RC;	/* Retry counter for T20 */
	struct x25_timer	T21;
	struct x25_timer	T22;
//	_ushort				N22;	/* Maximum number of retries for T22 */
	_ushort				N22_RC;	/* Retry counter for T22 */
	struct x25_timer	T23;

	struct circular_buffer	ack_queue;
	struct circular_buffer	write_queue;
	struct circular_buffer	receive_queue;

	char		fragment_buffer[65536];
	_ushort		fragment_len;


	struct circular_buffer	interrupt_in_queue;
	struct circular_buffer	interrupt_out_queue;

	_ulong		flags;

	struct x25_causediag	causediag;
	struct x25_facilities	facilities;
	struct x25_dte_facilities dte_facilities;
	struct x25_calluserdata	calluserdata;
	_ulong 		vc_facil_mask;	/* inc_call facilities mask */

//#if INTERNAL_SYNC
//	pthread_mutex_t		_mutex;
//#endif
};

/* x25_iface.c */



/* x25_in.c */
int x25_process_rx_frame(struct x25_cs *x25, char * data, int data_size);


/* x25_out.c */
int x25_pacsize_to_bytes(_uint pacsize);
int x25_output(struct x25_cs * x25, char * data, int data_size, int q_bit_flag);
void x25_send_iframe(struct x25_cs * x25, char * data, int data_size);
void x25_kick(struct x25_cs * x25);
void x25_enquiry_response(struct x25_cs * x25);

/* x25_link.c */
void x25_transmit_link(struct x25_cs * x25, char * data, int data_size);
void x25_transmit_restart_request(struct x25_cs * x25);
void x25_transmit_restart_confirmation(struct x25_cs *x25);
void x25_link_control(struct x25_cs *x25, char * data, int data_size, _uchar frametype);
void x25_transmit_clear_request(struct x25_cs *x25, _uint lci, _uchar cause);

/* x25_facilities.c */
int x25_parse_facilities(struct x25_cs *x25,
						 char * data,
						 int data_size,
						 struct x25_facilities *facilities,
						 struct x25_dte_facilities *dte_facs,
						 _ulong *vc_fac_mask);
int x25_create_facilities(//struct x25_cs * x25,
						  _uchar *buffer,
						  struct x25_facilities *facilities,
						  struct x25_dte_facilities *dte_facs,
						  _ulong facil_mask);
int x25_negotiate_facilities(struct x25_cs *x25,
							 char * data,
							 int data_size,
							 struct x25_dte_facilities *dte);
void x25_limit_facilities(struct x25_cs *x25);

/* x25_subr.c */
void lock(struct x25_cs *x25);
void unlock(struct x25_cs *x25);
void set_bit(long nr, _ulong * addr);
void clear_bit(long nr, _ulong * addr);
int test_bit(long nr, _ulong * addr);
int test_and_set_bit(long nr, _ulong * addr);
struct x25_cs_internal * x25_get_internal(struct x25_cs *x25);
/* Redefine 'malloc' and 'free' */
void * x25_mem_get(_ulong size);
void x25_mem_free(void *ptr);
void * x25_mem_copy(void *dest, const void *src, _ulong n);
void x25_mem_zero(void *src, _ulong n);
int x25_parse_address_block(char * data, int data_size, struct x25_address *called_addr, struct x25_address *calling_addr);
int x25_addr_ntoa(_uchar *p, struct x25_address *called_addr, struct x25_address *calling_addr);
int x25_addr_aton(_uchar *p, struct x25_address *called_addr, struct x25_address *calling_addr);
void x25_clear_queues(struct x25_cs * x25);
void x25_frames_acked(struct x25_cs * x25, _ushort nr);
void x25_requeue_frames(struct x25_cs * x25);
int x25_validate_nr(struct x25_cs * x25, _ushort nr);
void x25_write_internal(struct x25_cs *x25, int frametype);
void x25_disconnect(void * x25_ptr, int reason, _uchar cause, _uchar diagnostic);
int x25_decode(struct x25_cs * x25, char * data, int data_size, int *ns, int *nr, int *q, int *d, int *m);
int x25_rx_call_request(struct x25_cs * x25, char * data, int data_size, _uint lci);

/* x25_timer.c */
void x25_start_heartbeat(struct x25_cs *x25);
void x25_stop_heartbeat(struct x25_cs *x25);

void x25_start_timer(struct x25_cs *x25, struct x25_timer * _timer);
void x25_stop_timer(struct x25_cs *x25, struct x25_timer * _timer);
int x25_timer_running(struct x25_timer * _timer);
void x25_stop_timers(struct x25_cs *x25);
void x25_t20timer_expiry(void * x25_ptr);
void x25_t21timer_expiry(void * x25_ptr);
void x25_t22timer_expiry(void * x25_ptr);
void x25_t23timer_expiry(void * x25_ptr);
void x25_t2timer_expiry(void * x25_ptr);

#endif // X25_INT_H

