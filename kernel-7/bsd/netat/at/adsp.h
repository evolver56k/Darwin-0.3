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
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

#ifndef __ADSP__
#define __ADSP__

/* ADSP flags for read, write, and close routines */

#define	ADSP_EOM	0x01	/* Sent or received EOM with data */
#define ADSP_FLUSH	0x02	/* Send all data in send queue */
#define	ADSP_WAIT	0x04	/* Graceful close, wait 'til snd queue emptys */


/* ADSP events to be fielded by the user event handler */

#define	ADSP_EV_ATTN 	0x02	/* Attention data recvd. */
#define	ADSP_EV_RESET	0x04	/* Forward reset recvd. */
#define	ADSP_EV_CLOSE	0x08	/* Close advice recvd. */


/* ADSP packet control codes */

#define ADSP_PROBEACK 0			/* Probe or acknowledgement */
#define ADSP_OPENCONREQUEST 1		/* Open connection request */
#define ADSP_OPENCONACK 2		/* Open connection acknowledgement */
#define ADSP_OPENCONREQACK 3		/* Open connection request + ack */
#define ADSP_OPENCONDENIAL 4		/* Open connection denial */
#define ADSP_CLOSEADVICE 5		/* Close connection advice */
#define ADSP_FORWARDRESET 6		/* Forward reset */
#define ADSP_FORWARDRESETACK 7		/* Forward reset acknowledgement */
#define ADSP_RETRANSADVICE 8		/* Retransmit advice */


/* Miscellaneous constants */

#define ADSP_MAXDATA		572	/* Maximum data bytes in ADSP packet */
#define ADSP_MAXATTNDATA	570	/* Maximum data bytes in attn msg */
#define ADSP_DDPTYPE		7	/* DDP protocol type for ADSP */
#define ADSP_VERSION		0x0100	/* ADSP version */


/* Some additional ADSP error codes */

#define	EQUEWASEMP	10001
#define EONEENTQUE	10002
#define	EQUEBLOCKED	10003
#define	EFWDRESET	10004
#define	EENDOFMSG	10005
#define	EADDRNOTINUSE	10006



/* Tuning Parameter Block */

struct tpb {
   unsigned Valid : 1;			/* Tuning parameter block is valid */
   unsigned short TransThresh;		/* Transmit threshold */
   unsigned TransTimerIntrvl;		/* Transmit timer interval */
   unsigned short SndWdwCloThresh;	/* Send window closing threshold */
   unsigned SndWdwCloIntrvl;		/* Send window closed interval */
   unsigned char SndWdwCloBckoff;	/* Send window closed backoff rate */
   unsigned ReTransIntrvl;		/* Retransmit interval */
   unsigned char ReTransBckoff;		/* Retransmit backoff rate */
   unsigned RestartIntrvl;		/* Restart sender interval */
   unsigned char RestartBckoff;		/* Restart sender backoff rate */
   unsigned SndQBufSize;		/* Send queue buffer size */
   unsigned short RcvQMaxSize;		/* Maximum size of the receive queue */
   unsigned short RcvQCpyThresh;	/* Receive queue copy threshold */
   unsigned FwdRstIntrvl;		/* Forward reset interval */
   unsigned char FwdRstBckoff;		/* Forward reset backoff rate */
   unsigned AttnIntrvl;			/* Retransmit attn msg interval */
   unsigned char AttnBckoff;		/* Retransmit attn msg backoff rate */
   unsigned OpenIntrvl;			/* Retransmit open request interval */
   unsigned char OpenMaxRetry;		/* Open request maximum retrys */
   unsigned char RetransThresh;		/* Retransmit advice threshold */
   unsigned ProbeRetryMax;		/* Maximum number of probes */
   unsigned SndByteCntMax;		/* Maximum number bytes in send queue */
};


/* Tuning Parameter Tags */

#define	ADSP_TRANSTHRESH	 1	/* Transmit threshold */
#define	ADSP_TRANSTIMERINTRVL	 2	/* Transmit timer interval */
#define	ADSP_SNDWDWCLOTHRESH	 3	/* Send window closing threshold */
#define	ADSP_SNDWDWCLOINTRVL	 4	/* Send window closed interval */
#define	ADSP_SNDWDWCLOBCKOFF	 5	/* Send window closed backoff rate */
#define	ADSP_RETRANSINTRVL	 6	/* Retransmit interval */
#define	ADSP_RETRANSBCKOFF	 7	/* Retransmit backoff rate */
#define	ADSP_RESTARTINTRVL	 8	/* Restart sender interval */
#define	ADSP_RESTARTBCKOFF	 9	/* Restart sender backoff rate */
#define	ADSP_SNDQBUFSIZE	 10	/* Send queue buffer size */
#define	ADSP_RCVQMAXSIZE	 11	/* Receive queue maximum size */
#define	ADSP_RCVQCPYTHRESH	 12	/* Receive queue copy threshold */
#define	ADSP_FWDRSTINTRVL	 13	/* Forward reset retransmit interval */
#define	ADSP_FWDRSTBCKOFF	 14	/* Forward reset backoff rate */
#define	ADSP_ATTNINTRVL		 15	/* Rexmit attention message interval */
#define	ADSP_ATTNBCKOFF		 16	/* Attention message backoff rate */
#define	ADSP_OPENINTRVL		 17	/* Retransmit open request interval */
#define	ADSP_OPENMAXRETRY	 18	/* Open request max retrys */
#define	ADSP_RETRANSTHRESH	 19	/* Retransmit advice threshold */
#define	ADSP_PROBERETRYMAX	 20
#define	ADSP_SNDBYTECNTMAX	 21

#define TuneParamCnt 21			/* The number of tuning parameters */

/* Connection Status Tags */

#define	ADSP_STATE		 1	/* The connection state */
#define	ADSP_SNDSEQ		 2	/* Send sequence number */
#define	ADSP_FIRSTRTMTSEQ	 3	/* First retransmit sequence number */
#define	ADSP_SNDWDWSEQ	  	 4	/* Send window sequence number */
#define	ADSP_RCVSEQ		 5	/* Receive sequence number */
#define	ADSP_ATTNSNDSEQ	 	 6	/* Attn msg send sequence number */
#define	ADSP_ATTNRCVSEQ	 	 7	/* Attn msg receive sequence number */
#define	ADSP_RCVWDW		 8	/* Receive window size */
#define	ADSP_ATTNMSGWAIT	 9	/* Attn msg is in the receive queue */

#define ConStatTagCnt 9			/* Number of connection status tags */

#define	ADSP_INVALID	 	0       /* Invalid connection control block */
#define	ADSP_LISTEN	 	1       /* Waiting for an open con req */
#define	ADSP_OPENING	 	2     	/* No state info, sending open req */
#define	ADSP_MYHALFOPEN		4   	/* His state info, sending open req */
#define	ADSP_HISHALFOPEN	8  	/* He has my state info, sndng op req */
#define	ADSP_OPEN	 	16     	/* Connection is operational */
#define	ADSP_TORNDOWN	 	32     	/* Probe timer has expired 4 times */
#define	ADSP_CLOSING	 	64	/* Client close, emptying send Queues */
#define	ADSP_CLOSED	 	128	/* Close adv rcvd, emptying rcv Queues */

/* Management Counters */

#define	ADSP_ATTNACKRCVD	 1	/* Attn msg ack received */
#define	ADSP_ATTNACKACPTD	 2	/* Attn msg ack accepted */
#define	ADSP_PROBERCVD	 	 3	/* Probe received */
#define	ADSP_ACKRCVD		 4	/* Explicit ack msg received */
#define	ADSP_FWDRSTRCVD	 	 5	/* Forward reset received */
#define	ADSP_FWDRSTACPTD	 6	/* Forward reset accepted */
#define	ADSP_FWDRSTACKRCVD	 7	/* Forward reset ack received */
#define	ADSP_FWDRSTACKACPTD	 8	/* Forward reset ack accepted */
#define	ADSP_ATTNRCVD		 9	/* Attn msg received */
#define	ADSP_ATTNACPTD	   	 10	/* Attn msg accepted */
#define	ADSP_DATARCVD		 11	/* Data msg received */
#define	ADSP_DATAACPTD	  	 12	/* Data msg Accepted */
#define	ADSP_ACKFIELDCHKD	 13	/* Ack field checked */
#define	ADSP_ACKNRSFIELDACPTD	 14	/* Next receive seq field accepted */
#define	ADSP_ACKSWSFIELDACPTD	 15	/* Send window seq field accepted */
#define	ADSP_ACKREQSTD	 	 16	/* Ack requested */
#define	ADSP_LOWMEM		 17	/* Low memory */
#define	ADSP_OPNREQEXP	 	 18	/* Open request timer expired */
#define	ADSP_PROBEEXP	  	 19	/* Probe timer expired */
#define	ADSP_FWDRSTEXP	 	 20	/* Forward reset timer expired */
#define	ADSP_ATTNEXP	 	 21	/* Attention timer expired */
#define	ADSP_TRANSEXP	         22	/* Transmit timer expired */
#define	ADSP_RETRANSEXP	 	 23	/* Retransmit timer expired */
#define	ADSP_SNDWDWCLOEXP	 24	/* Send window closed timer expired */
#define	ADSP_RESTARTEXP	 	 25	/* Restart sender timer expired */
#define	ADSP_RESLOWEXP	 	 26	/* Resources are low timer expired */
#define	ADSP_RETRANSRCVD	 27	/* Retransmit advice received */

#define	InfoTagCnt		 27

/* Length of the parameter and status lists */

#define	ADSP_DEFLEN	 (TuneParamCnt * 6 + 1)
#define	ADSP_STALEN	 (ConStatTagCnt * 6 + 1)
#define	ADSP_INFOLEN	 (InfoTagCnt * 6 + 1)

#endif /* __ADSP__ */
