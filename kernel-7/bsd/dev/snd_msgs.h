/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/* 
 * Copyright (c) 1988 NeXT, Inc.
 */
/*
 * HISTORY
 * 01-Mar-93 Mike Minnick (mminnick) at NeXT
 *	Added generic stream format get/set.
 *
 * 29-Oct-91 Mike Minnick (mminnick) at NeXT
 *	Added stream owner port to snd_stream_msg_t.
 *
 * 23-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Added SND_DSP_PROTO_TXD for 040 TXD interrupt support.
 *
 * 23-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Support message passing protocol change so the reply port isn't
 *	used for passing permissions (set_owner, get_stream, dsp_proto,
 *	and get_dsp_cmd_port).
 *
 * 21-Aug-90  Julius Smith (jos) at NeXT
 *	Added protocol message type for setting the protocol from the
 *	dsp command queue.
 *
 * 10-Dec-88  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 
#ifdef	DRIVER_PRIVATE

#ifndef	_M68K_DEV_SND_MSGS_PRIV_
#define _M68K_DEV_SND_MSGS_PRIV_

#import <mach/mach_types.h>
#import <mach/message.h>

/*
 * Message component type codes on stream port.
 */
typedef struct {
	msg_type_t	typeType;
	unsigned int	type;
#define SND_MT_PLAY_DATA	0	// data to be played (with size)
#define SND_MT_RECORD_DATA	1	// number of bytes to record
#define SND_MT_OPTIONS		2	// high/low water and dma size options
#define SND_MT_CONTROL		3	// await/abort/pause/continue
#define SND_MT_NDMA		4	// number of dma descs in stream.
/* New for 3.1 */
#define SND_MT_FORMAT		5	// rate, encoding, chan count
} snd_msg_type_t;

/*
 * Interface message formats
 */

/*
 * Messages sent from the kernel
 */
typedef struct {
	msg_header_t	header;
	msg_type_t	data_tagType;
	int		data_tag;
	msg_type_long_t	dataType;
	pointer_t	recorded_data;
} snd_recorded_data_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	data_tagType;
	int		data_tag;
} snd_taged_reply_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	dataType;
	int		nsamples;
	int		timeStamp;
} snd_ret_samples_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	dataType;
	unsigned int	parms;
#define SND_PARM_LOWPASS	0x1
#define SND_PARM_SPEAKER	0x2
#define SND_PARM_ZEROFILL	0x4
} snd_ret_parms_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	dataType;
	unsigned int	volume;
#define SND_VOLUME_RCHAN_MASK	0xff
#define SND_VOLUME_RCHAN_SHIFT	0
#define SND_VOLUME_LCHAN_MASK	0xff00
#define SND_VOLUME_LCHAN_SHIFT	8
} snd_ret_volume_t;

/* New for 3.1 */
typedef struct {
	msg_header_t	header;
	msg_type_t	Type;
	unsigned int	rate;
#define	SND_STREAM_FORMAT_RATE_CONTINUOUS	(1<<0)
#define	SND_STREAM_FORMAT_RATE_8000		(1<<1)
#define	SND_STREAM_FORMAT_RATE_11025		(1<<2)
#define	SND_STREAM_FORMAT_RATE_16000		(1<<3)
#define	SND_STREAM_FORMAT_RATE_22050		(1<<4)
#define	SND_STREAM_FORMAT_RATE_32000		(1<<5)
#define	SND_STREAM_FORMAT_RATE_44100		(1<<6)
#define	SND_STREAM_FORMAT_RATE_48000		(1<<7)
	unsigned int	low_rate;
	unsigned int	high_rate;
	unsigned int	encoding;
#define	SND_STREAM_FORMAT_ENCODING_MULAW_8	(1<<0)
#define	SND_STREAM_FORMAT_ENCODING_LINEAR_8	(1<<1)
#define	SND_STREAM_FORMAT_ENCODING_LINEAR_16	(1<<2)
	unsigned int	chan_count;
} snd_ret_stream_formats_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	dataType;
	int		dsp_regs;
} snd_ret_dsp_regs_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	dataType;
	unsigned int	ill_msgid;	// message id of bad message.
	unsigned int	ill_error;	// error returned
#define SND_NO_ERROR	100	// non-error ack.
#define SND_BAD_PORT	101	// message sent to wrong port
#define SND_BAD_MSG	102	// unknown message id
#define SND_BAD_PARM	103	// bad parameter list in message
#define SND_NO_MEMORY	104	// can't allocate memory (record)
#define SND_PORT_BUSY	105	// access req'd to existing excl access port
#define SND_NOT_OWNER	106	// must be owner to do this
#define SND_BAD_CHAN	107	// dsp channel hasn't been inited
#define SND_SEARCH	108	// couldn't find requested resource
#define SND_NODATA	109	// can't send data commands to dsp in this mode
#define SND_NOPAGER	110	// can't allocate from external pager (record).
#define SND_NOTALIGNED	111	// bad data alignment.
#define SND_BAD_HOST_PRIV 112	// bad host privilege port passed.
#define SND_BAD_PROTO 	113	// can't do requested operation given protocol
} snd_illegal_msg_t;

typedef struct {msg_header_t header;} snd_ret_port_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	dataType;
	unsigned int	mask;
	unsigned int	flags;
	unsigned int	value;
} snd_dsp_cond_true_t;

/*
 * Messages sent from the user to the device
 */
typedef struct {
	msg_header_t	header;
	msg_type_t	Type;
	unsigned int	stream;
	msg_type_t	ownerType;
	port_t		owner;
} snd_get_stream_t;

#define SND_GD_CHAN_MASK	0x7f
#define SND_GD_CHAN_SHIFT	0
#define SND_GD_DEVICE_MASK	0x80
#define SND_GD_DEVICE_SHIFT	7

#define snd_gd_bitmap(chan, device) \
	(((chan)<<SND_GD_CHAN_SHIFT) | ((device)<<SND_GD_DEVICE_SHIFT))

#define snd_gd_isdevice(bitmap)  ((bitmap)&SND_GD_DEVICE_MASK)
#define snd_gd_chan(bitmap) ((bitmap)&SND_GD_CHAN_MASK)

#define SND_GD_SOUT_44	snd_gd_bitmap(0, TRUE)
#define SND_GD_SOUT_22	snd_gd_bitmap(1, TRUE)
#define SND_GD_SIN	snd_gd_bitmap(2, TRUE)
#define SND_GD_DSP_OUT	snd_gd_bitmap(DSP_SO_CHAN, FALSE) // sound out from dsp
#define SND_GD_DSP_IN	snd_gd_bitmap(DSP_SI_CHAN, FALSE) // sound in to dsp

typedef struct {
	msg_header_t	header;
	msg_type_t	Type;
	unsigned int	parms;
} snd_set_parms_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	Type;
	unsigned int	volume;
} snd_set_volume_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	Type;
	unsigned int	proto;
	msg_type_t	ownerType;
	port_t		owner;
} snd_dsp_proto_t;

#define SND_DSP_PROTO_DSPERR	0x1	// DSP error messages enabled
#define SND_DSP_PROTO_C_DMA	0x2	// complex dma mode
#define SND_DSP_PROTO_S_DMA	0x4	// simple dma mode
#define SND_DSP_PROTO_LINKOUT	0x8	// link directly to sound out
#define SND_DSP_PROTO_LINKIN	0x10	// link directly to sound out
#define SND_DSP_PROTO_SOUNDDATA	0x20	// all data to dsp is sound samples
#define SND_DSP_PROTO_HIGH	0x40	// 44khz sound out
#define SND_DSP_PROTO_HFABORT	0x80	// DSP abort indicated by HF2 & HF3
#define SND_DSP_PROTO_DSPMSG	0x100	// DSP messages enabled
#define SND_DSP_PROTO_RAW	0x200	// DSP messages enabled
#define SND_DSP_PROTO_TXD	0x400	// DSP txd interrupt enable (040 only)

typedef msg_header_t	snd_get_parms_t;
typedef msg_header_t	snd_get_volume_t;
/* New for 3.1 */
typedef msg_header_t	snd_get_stream_formats_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	ownerType;
	port_t		owner;		// owner port
} snd_get_dsp_cmd_port_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	negType;
	port_t		negotiation;	// negotiation port
	msg_type_t	ownerType;
	port_t		owner;		// owner port
} snd_set_owner_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	old_ownerType;
	port_t		old_owner;
	msg_type_t	new_ownerType;
	port_t		new_owner;
	msg_type_t	new_negotiationType;
	port_t		new_negotiation;
} snd_reset_owner_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	privType;
	port_t		priv;
} snd_new_device_t;

typedef struct {
	msg_header_t	header;
	msg_type_long_t	dataType;
#define DSP_DEF_BUFSIZE	512			// default #words in each buf
	unsigned int	data[DSP_DEF_BUFSIZE];
} snd_dsp_msg_t;

typedef struct {
	msg_header_t	header;
	msg_type_long_t	dataType;
#define DSP_DEF_EBUFSIZE 32			// default #words in each buf
	unsigned int	data[DSP_DEF_EBUFSIZE];
} snd_dsp_err_t;

/*
 * Messages sent from the user to a stream port
 * There's only one message, but it can contain some combination
 * of several different sub-messages.
 */
typedef struct snd_stream_msg {
	msg_header_t	header;
	msg_type_t	data_tagType;
	int		data_tag;		// tag for this request, 0 def
	msg_type_t	ownerType;
	port_t		streamOwner;		// stream owner port for 3.0
} snd_stream_msg_t;

typedef struct {
	snd_msg_type_t	msgtype;
	msg_type_t	optionsType;
	unsigned int	options;
#define SND_DM_STARTED_MSG	0x01
#define SND_DM_COMPLETED_MSG	0x02
#define SND_DM_ABORTED_MSG	0x04
#define SND_DM_PAUSED_MSG	0x08
#define SND_DM_RESUMED_MSG	0x10
#define SND_DM_OVERFLOW_MSG	0x20
#define SND_DM_PREEMPT		0x40
	msg_type_t	reg_portType;
	port_t		reg_port;	// remote port for region messages
	msg_type_long_t	dataType;
	pointer_t	data;
} snd_stream_play_data_t;

typedef struct {
	snd_msg_type_t	msgtype;
	msg_type_t	optionsType;
	unsigned int	options;
	int		nbytes;
	msg_type_t	reg_portType;
	port_t		reg_port;	// remote port for region messages
	msg_type_t	filenameType;	// filename string follows inline
} snd_stream_record_data_t;

typedef struct {
	snd_msg_type_t	msgtype;
	msg_type_t	controlType;
	unsigned int	snd_control;
#define SND_DC_AWAIT	0x1
#define SND_DC_ABORT	0x2
#define SND_DC_PAUSE	0x4
#define SND_DC_RESUME	0x8
} snd_stream_control_t;

typedef struct {
	snd_msg_type_t	msg_type;
	msg_type_t	optionsType;
	unsigned int	high_water;
	unsigned int	low_water;
	unsigned int	dma_size;
} snd_stream_options_t;

/* New for 3.1 */
typedef struct {
	snd_msg_type_t	msg_type;
	msg_type_t	formatType;
	unsigned int	rate;
	unsigned int	encoding;
	unsigned int	chan_count;
} snd_stream_format_t;

/*
 * Number of dma descriptors used in stream.
 */
typedef struct {
	snd_msg_type_t	msg_type;
	msg_type_t	ndmaType;
	unsigned int	ndma;
} snd_stream_ndma_t;

/*
 * Nsamples is another message on a stream port.
 */
typedef struct {
	msg_header_t	header;
} snd_stream_nsamples_t;

/*
 * Messages sent by user to dsp command port.
 */
typedef struct {
	msg_header_t	header;
	msg_type_t	reg_maskType;
	unsigned int	mask;		// mask of flags to inspect
	unsigned int	flags;		// set of flags that must be on
	msg_type_t	ret_portType;
	port_t		ret_port;	// remote port for ret_msg
	msg_type_long_t	ret_msgType;
	/*
	 * follows is the body of the (simple) message to send
	 * either to msg_remote_port, or to snd_var.dsp  (if PORT_NULL).
	 */
	msg_header_t	ret_msg;	// possibly longer than this.
} snd_dspcmd_event_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	dataType;
	int		addr;		// .. of dsp buffer
	int		size;		// .. of dsp buffer
	int		skip;		// dma skip factor
#define DSP_SKIP_0	1
#define DSP_SKIP_CONTIG	DSP_SKIP_0
#define DSP_SKIP_1	2
#define DSP_SKIP_2	3
#define DSP_SKIP_3	4
#define DSP_SKIP_4	5
	int		space;		// dsp space of buffer
#define	DSP_SPACE_X	1
#define DSP_SPACE_Y	2
#define DSP_SPACE_L	3
#define DSP_SPACE_P	4
	int		mode;		// mode of dma [1..5]
#define DSP_MODE8	1
#define DSP_MODE16	2
#define DSP_MODE24	3
#define DSP_MODE32	4
#define DSP_MODE2416	5		/* Pseudo-dma shift 24 to 16 bits */
#define SND_MODE_MIN	1		/* Minimum value for mode */
#define SND_MODE_MAX	5		/* Maximum value for mode */
	int		chan;		// channel for dma
#define DSP_USER_REQ_CHAN 0	// for user-requested dma's
#define DSP_SO_CHAN	1	// for sound-out dma's.
#define DSP_SI_CHAN	2	// for sound-in dma's
#define DSP_USER_CHAN	3	// first user chan

#define DSP_N_USER_CHAN	16	// 0..15 for user dma.
} snd_dspcmd_chandata_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	chandataType;
	int		addr;		// .. of dsp buffer
	int		size;		// .. of dsp buffer
	int		skip;		// dma skip factor
	int		space;		// dsp space of buffer
	int		mode;		// mode of dma [1..5]
	msg_type_long_t	dataType;
	pointer_t	data;		// data to output
} snd_dspcmd_dma_t;

typedef msg_header_t	snd_dspcmd_abortdma_t;
typedef msg_header_t	snd_dspcmd_req_err_t;
typedef msg_header_t	snd_dspcmd_req_msg_t;

/*
 * Multi-part dspcmd_msg.
 */
/*
 * Message component type codes on stream port.
 */
typedef struct {
	msg_type_t	typeType;
	unsigned int	type;
#define SND_DSP_MT_DATA		1	// 1, 2, or 4 byte data
#define SND_DSP_MT_HOST_COMMAND	2	// host command
#define SND_DSP_MT_HOST_FLAG	3	// host flag(s) to set
#define SND_DSP_MT_RET_MSG	4	// (simple) message to send
#define SND_DSP_MT_RESET	5	// hard reset the DSP
#define SND_DSP_MT_GET_REGS	6	// return DSP host I/F registers
#define SND_DSP_MT_CONDITION	7	// wait for condition (return msg)
#define SND_DSP_MT_RDATA	8	// read 1, 2, or 4 byte data
#define SND_DSP_MT_PROTO	9	// DSP protocol
} snd_dsp_type_t;

typedef struct {
	msg_header_t	header;
	msg_type_t	priType;
	int		pri;		// Priority of message group
#define DSP_MSG_HIGH		0
#define DSP_MSG_MED		1
#define DSP_MSG_LOW		2
	boolean_t	atomic;		// don't preempt this msg with another
} snd_dspcmd_msg_t;

typedef struct {
	snd_dsp_type_t	msgtype;
	msg_type_t	conditionType;
	unsigned int	mask;		// mask of flags to inspect
	unsigned int	flags;		// set of flags that must be on
	msg_type_t	reply_portType;
	port_t		reply_port;	// were to send device regs to
} snd_dsp_condition_t;

typedef struct {
	snd_dsp_type_t	msgtype;
	msg_type_long_t	dataType;
	pointer_t	data;		// data to send
} snd_dsp_data_t;

typedef struct {
	snd_dsp_type_t	msgtype;
	msg_type_t	hcType;
	unsigned int	hc;		// host command
} snd_dsp_host_command_t;

typedef struct {
	snd_dsp_type_t	msgtype;
	msg_type_t	protoType;	// protocol modification
	unsigned int	proto;
} snd_dsp_mt_proto_t;

typedef struct {
	snd_dsp_type_t	msgtype;
	msg_type_t	hfType;
	unsigned int	mask;		// mask of HF0|HF1
	unsigned int	flags;		// flags to set
} snd_dsp_host_flag_t;

typedef struct {
	snd_dsp_type_t	msgtype;
	msg_type_t	ret_portType;
	port_t		ret_port;	// remote port for ret_msg
	msg_type_long_t	ret_msgType;
	/*
	 * follows is the body of the (simple) message to send
	 * either to msg_remote_port, or to snd_var.dspowner (if PORT_NULL).
	 */
	msg_header_t	ret_msg;	// possibly longer than this.
} snd_dsp_ret_msg_t;

typedef struct {
	snd_dsp_type_t	msgtype;
} snd_dsp_reset_t, snd_dsp_get_regs_t;

/*
 * Message Id's
 */
#define SND_MSG_BASE		0

/*
 * User messages on stream port
 */
#define SND_MSG_STREAM_BASE	SND_MSG_BASE+0
#define SND_MSG_STREAM_MSG	SND_MSG_STREAM_BASE+0
#define SND_MSG_STREAM_NSAMPLES	SND_MSG_STREAM_BASE+1

/*
 * User messages on device port
 */
#define SND_MSG_DEVICE_BASE		SND_MSG_BASE+100
#define SND_MSG_GET_STREAM		SND_MSG_DEVICE_BASE+0
#define SND_MSG_SET_PARMS		SND_MSG_DEVICE_BASE+1
#define SND_MSG_GET_PARMS		SND_MSG_DEVICE_BASE+2
#define SND_MSG_SET_VOLUME		SND_MSG_DEVICE_BASE+3
#define SND_MSG_GET_VOLUME		SND_MSG_DEVICE_BASE+4
#define SND_MSG_SET_DSPOWNER		SND_MSG_DEVICE_BASE+5
#define SND_MSG_SET_SNDINOWNER		SND_MSG_DEVICE_BASE+6
#define SND_MSG_SET_SNDOUTOWNER		SND_MSG_DEVICE_BASE+7
#define SND_MSG_DSP_PROTO		SND_MSG_DEVICE_BASE+8
#define SND_MSG_GET_DSP_CMD_PORT	SND_MSG_DEVICE_BASE+9
#define SND_MSG_NEW_DEVICE_PORT		SND_MSG_DEVICE_BASE+10
#define SND_MSG_RESET_DSPOWNER		SND_MSG_DEVICE_BASE+11
#define SND_MSG_RESET_SNDINOWNER	SND_MSG_DEVICE_BASE+12
#define SND_MSG_RESET_SNDOUTOWNER	SND_MSG_DEVICE_BASE+13
#define SND_MSG_SET_RAMP		SND_MSG_DEVICE_BASE+14
/*
 * Set ramp parameter
 */
#define SND_PARM_RAMPUP		0x1	/* Ramp sound up */
#define SND_PARM_RAMPDOWN	0x2	/* Ramp sound down */

/* New for 3.1 */
#define SND_MSG_GET_SNDOUT_FORMATS	SND_MSG_DEVICE_BASE+15
#define SND_MSG_GET_SNDIN_FORMATS	SND_MSG_DEVICE_BASE+16

/*
 * User messages on dsp command port
 */
#define SND_MSG_DSP_BASE	SND_MSG_BASE+200
#define SND_MSG_DSP_MSG		SND_MSG_DSP_BASE+0
#define SND_MSG_DSP_EVENT	SND_MSG_DSP_BASE+1
#define SND_MSG_DSP_CHANDATA	SND_MSG_DSP_BASE+2
#define SND_MSG_DSP_DMAOUT	SND_MSG_DSP_BASE+3
#define SND_MSG_DSP_DMAIN	SND_MSG_DSP_BASE+4
#define SND_MSG_DSP_ABORTDMA	SND_MSG_DSP_BASE+5
#define SND_MSG_DSP_REQ_MSG	SND_MSG_DSP_BASE+6
#define SND_MSG_DSP_REQ_ERR	SND_MSG_DSP_BASE+7

/*
 * Kernel messages returned.
 */
#define SND_MSG_KERN_BASE	SND_MSG_BASE+300
#define SND_MSG_RECORDED_DATA	SND_MSG_KERN_BASE+0
#define SND_MSG_TIMED_OUT	SND_MSG_KERN_BASE+1
#define SND_MSG_RET_SAMPLES	SND_MSG_KERN_BASE+2
#define SND_MSG_RET_DEVICE	SND_MSG_KERN_BASE+3
#define SND_MSG_RET_STREAM	SND_MSG_KERN_BASE+4
#define SND_MSG_RET_PARMS	SND_MSG_KERN_BASE+5
#define SND_MSG_RET_VOLUME	SND_MSG_KERN_BASE+6
#define SND_MSG_OVERFLOW	SND_MSG_KERN_BASE+7
#define SND_MSG_STARTED		SND_MSG_KERN_BASE+9
#define SND_MSG_COMPLETED	SND_MSG_KERN_BASE+10
#define SND_MSG_ABORTED		SND_MSG_KERN_BASE+11
#define SND_MSG_PAUSED		SND_MSG_KERN_BASE+12
#define SND_MSG_RESUMED		SND_MSG_KERN_BASE+13
#define SND_MSG_ILLEGAL_MSG	SND_MSG_KERN_BASE+14
#define SND_MSG_RET_DSP_ERR	SND_MSG_KERN_BASE+15
#define SND_MSG_RET_DSP_MSG	SND_MSG_KERN_BASE+16
#define SND_MSG_RET_CMD		SND_MSG_KERN_BASE+17
#define SND_MSG_DSP_REGS	SND_MSG_KERN_BASE+18
#define SND_MSG_DSP_COND_TRUE	SND_MSG_KERN_BASE+19
/* New for 3.1 */
#define SND_MSG_RET_STREAM_FORMATS	SND_MSG_KERN_BASE+20

/*
 * Ioctl for retrieving device port.
 */
#ifdef	_IO
#define SOUNDIOCDEVPORT	_IO('A', 8)
#endif	/* _IO */

/*
 * Routine prototypes
 */
#if	KERNEL
#ifdef	m68k
kern_return_t snd_reply_recorded_data (
	port_t		remote_port,	// who to reply to
	int		data_tag,	// tag from region
	pointer_t	data,		// recorded data
	int		nbytes,		// number of bytes of data to send
	int		in_line);	// "Send data inline" flag.
kern_return_t snd_reply_timed_out (
	port_t		remote_port,	// who to send it to.
	int		data_tag);	// tag from region
kern_return_t snd_reply_ret_samples (
	port_t		remote_port,	// who to send it to.
	int		nsamples,	// number of bytes of data to record
	int 		timeStamp);	
kern_return_t snd_reply_ret_device (
	port_name_t	remote_port,	// who to send it to.
	port_name_t	device_port);	// returned port.
kern_return_t snd_reply_ret_stream (
	port_t		remote_port,	// who to send it to.
	port_t		stream_port);	// returned port.
kern_return_t snd_reply_ret_parms (
	port_t		remote_port,	// who to send it to.
	unsigned int	parms);
kern_return_t snd_reply_ret_volume (
	port_t		remote_port,	// who to send it to.
	unsigned int	volume);
/* New for 3.1 */
kern_return_t snd_reply_ret_formats (
	port_name_t	remote_port,	// who to send it to.
	u_int		rates,
        u_int		low_rate,
        u_int		high_rate,
        u_int           encodings,
        u_int           chan_count);
kern_return_t snd_reply_overflow (
	port_t		remote_port,	// who to send it to.
	int		data_tag);	// from region
kern_return_t snd_reply_started (
	port_t		remote_port,	// who to send it to.
	int		data_tag);	// from region
kern_return_t snd_reply_completed (
	port_t		remote_port,	// who to send it to.
	int		data_tag);	// from region
kern_return_t snd_reply_aborted (
	port_t		remote_port,	// who to send it to.
	int		data_tag);	// from region
kern_return_t snd_reply_paused (
	port_t		remote_port,	// who to send it to.
	int		data_tag);	// from region
kern_return_t snd_reply_resumed (
	port_t		remote_port,	// who to send it to.
	int		data_tag);	// from region
kern_return_t snd_reply_illegal_msg (
	port_t		local_port,	// returned port of interest
	port_t		remote_port,	// who to send it to.
	int		msg_id,		// message id with illegal syntax	
	int		error);		// error code
kern_return_t snd_reply_dsp_err (
	port_t		remote_port);	// who to send it to.
kern_return_t snd_reply_dsp_msg (
	port_t		remote_port);	// who to send it to.
kern_return_t snd_reply_dsp_cmd_port(
	port_t		cmd_port,	// port to return.
	port_t		remote_port);	// where to return it
kern_return_t snd_reply_dsp_regs (
	int		regs);		// DSP host I/F registers (not recieve)
kern_return_t snd_reply_dsp_cond_true (	// reply indicating condition true
	vm_address_t	ret_cond);	// reply_port, status and conditions
#endif	/* m68k */
#endif	/* KERNEL */

#endif	/* _M68K_DEV_SND_MSGS_PRIV_ */

#endif	/* DRIVER_PRIVATE */
