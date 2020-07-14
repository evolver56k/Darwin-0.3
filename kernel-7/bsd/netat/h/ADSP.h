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

/* ADSP.h
 * 
 * From Mike Shoemaker (06/23/90)
 *
 * C Interfaces for the AppleTalk Data Stream Protocol (ADSP)
 *
*/

#ifndef __ADSP__
#define __ADSP__

#ifndef __APPLETALK__
#include <at/appletalk.h>
#endif


/* Solaris defines u as (curproc->p_user) */
#if defined(u)
# undef u
#endif

typedef short OSErr;
typedef long (*ProcPtr)();
typedef ProcPtr *ProcHandle;
typedef char *Ptr;
typedef Ptr *Handle;

/* result codes */

#define errENOBUFS	-1281
#define	errRefNum	-1280	/* bad connection refNum */
#define	errAborted	-1279	/* control call was aborted */
#define	errState	-1278	/* bad connection state for this operation */
#define	errOpening	-1277	/* open connection request failed */
#define	errAttention	-1276	/* attention message too long */
#define	errFwdReset	-1275	/* read terminated by forward reset */
#define errDSPQueueSize	-1274	/* DSP Read/Write Queue Too small */
#define errOpenDenied	-1273	/* open connection request was denied */

/* control codes */

#define	dspInit		255	/* create a new connection end */
#define	dspRemove	254	/* remove a connection end */
#define	dspOpen		253	/* open a connection */
#define	dspClose	252	/* close a connection */
#define	dspCLInit	251	/* create a connection listener */
#define	dspCLRemove	250	/* remove a connection listener */
#define	dspCLListen	249	/* post a listener request */
#define	dspCLDeny	248	/* deny an open connection request */
#define	dspStatus	247	/* get status of connection end */
#define	dspRead		246	/* read data from the connection */
#define	dspWrite	245	/* write data on the connection */
#define	dspAttention	244	/* send an attention message */
#define	dspOptions	243	/* set connection end options */
#define	dspReset	242	/* forward reset the connection */
#define	dspNewCID	241	/* generate a cid for a connection end */


/* connection opening modes */

#define	ocRequest	1	/* request a connection with remote */
#define	ocPassive	2	/* wait for a connection request from remote */
#define	ocAccept	3	/* accept request as delivered by listener */
#define	ocEstablish	4	/* consider connection to be open */


/* connection end states */

#define	sListening	1	/* for connection listeners */
#define	sPassive	2	/* waiting for a connection request from remote */
#define	sOpening	3	/* requesting a connection with remote */
#define	sOpen		4	/* connection is open */
#define	sClosing	5	/* connection is being torn down */
#define	sClosed		6	/* connection end state is closed */



/* client event flags */

#define	eClosed		0x80	/* received connection closed advice */
#define	eTearDown	0x40	/* connection closed due to broken connection */
#define	eAttention	0x20	/* received attention message */
#define	eFwdReset	0x10	/* received forward reset advice */

/* miscellaneous constants  */

#define	attnBufSize	570	/* size of client attention buffer */
#define	minDSPQueueSize	100	/* Minimum size of receive or send Queue */
#define defaultDSPQS	16384	/* random guess */
#define RecvQSize	defaultDSPQS
#define SendQSize	defaultDSPQS

/* connection control block */

struct TRCCB {
    u_char *ccbLink;	/* link to next ccb */
    u_short refNum;	/* user reference number */
    u_short state;	/* state of the connection end */
    u_char userFlags;	/* flags for unsolicited connection events */
    u_char localSocket;	/* socket number of this connection end */
    at_inet_t remoteAddress;	/* internet address of remote end */
    u_short attnCode;	/* attention code received */
    u_short attnSize;	/* size of received attention data */
    u_char *attnPtr;	/* ptr to received attention data */
    u_char reserved[220]; /* for adsp internal use */
};
	
typedef struct TRCCB TRCCB;
typedef TRCCB *TPCCB;

/* init connection end parameters */

struct TRinitParams {
    TPCCB ccbPtr;		/* pointer to connection control block */
    ProcPtr userRoutine;	/* client routine to call on event */
    u_char *sendQueue;		/* client passed send queue buffer */
    u_char *recvQueue;		/* client passed receive queue buffer */
    u_char *attnPtr;		/* client passed receive attention buffer */
    u_short sendQSize;		/* size of send queue (0..64K bytes) */
    u_short recvQSize;		/* size of receive queue (0..64K bytes) */
    u_char localSocket;		/* local socket number */
};

typedef struct TRinitParams TRinitParams;

/* open connection parameters */

struct TRopenParams {
    u_short localCID;		/* local connection id */
    u_short remoteCID;		/* remote connection id */
    at_inet_t remoteAddress;	/* address of remote end */
    at_inet_t filterAddress;	/* address filter */
    unsigned long sendSeq;	/* local send sequence number */
    u_long recvSeq;		/* receive sequence number */
    u_long attnSendSeq;		/* attention send sequence number */
    u_long attnRecvSeq;		/* attention receive sequence number */
    u_short sendWindow;		/* send window size */
    u_char ocMode;		/* open connection mode */
    u_char ocInterval;		/* open connection request retry interval */
    u_char ocMaximum;		/* open connection request retry maximum */
};

typedef struct TRopenParams TRopenParams;

/* close connection parameters */

struct TRcloseParams 	{
    u_char abort;		/* abort connection immediately if non-zero */
};

typedef struct TRcloseParams TRcloseParams;

/* client status parameter block */

struct TRstatusParams {
    TPCCB ccbPtr;		/* pointer to ccb */
    u_short sendQPending;	/* pending bytes in send queue */
    u_short sendQFree;		/* available buffer space in send queue */
    u_short recvQPending;	/* pending bytes in receive queue */
    u_short recvQFree;		/* available buffer space in receive queue */
};
	
typedef struct TRstatusParams TRstatusParams;

/* read/write parameter block */

struct TRioParams {
    u_short reqCount;		/* requested number of bytes */
    u_short actCount;		/* actual number of bytes */
    u_char *dataPtr;		/* pointer to data buffer */
    u_char eom;			/* indicates logical end of message */
    u_char flush;		/* send data now */
    u_char dummy[2];            /*### LD */
};

typedef struct TRioParams TRioParams;

/* attention parameter block */

struct TRattnParams {
    u_short attnCode;		/* client attention code */
    u_short attnSize;		/* size of attention data */
    u_char *attnData;		/* pointer to attention data */
    u_char attnInterval;	/* retransmit timer in 10-tick intervals */
    u_char dummy[3];		/* ### LD */
};

typedef struct TRattnParams TRattnParams;

/* client send option parameter block */

struct TRoptionParams {
    u_short sendBlocking;	/* quantum for data packets */
    u_char sendTimer;		/* send timer in 10-tick intervals */
    u_char rtmtTimer;		/* retransmit timer in 10-tick intervals */
    u_char badSeqMax;		/* threshold for sending retransmit advice */
    u_char useCheckSum;		/* use ddp packet checksum */
    u_short filler;		/* ### LD */
    int newPID;			/* ### Temp for backward compatibility 02/11/94 */
};

typedef struct TRoptionParams TRoptionParams;

/* new cid parameters */

struct TRnewcidParams {
    u_short newcid;		/* new connection id returned */
};

typedef struct TRnewcidParams TRnewcidParams;

union adsp_command {
	TRinitParams initParams; /* dspInit, dspCLInit */
	TRopenParams openParams; /* dspOpen, dspCLListen, dspCLDeny */
	TRcloseParams closeParams; /* dspClose, dspRemove */
	TRioParams ioParams;	/* dspRead, dspWrite, dspAttnRead */
	TRattnParams attnParams; /* dspAttention */
	TRstatusParams statusParams; /* dspStatus */
	TRoptionParams optionParams; /* dspOptions */
	TRnewcidParams newCIDParams; /* dspNewCID */
};

/* ADSP CntrlParam ioQElement */

struct DSPParamBlock {
    struct QElem *qLink;
    short qType;
    short ioTrap;
    Ptr ioCmdAddr;
    ProcPtr ioCompletion;
    OSErr ioResult;
    char *ioNamePtr;
    short ioVRefNum;
    short ioCRefNum;		/* adsp driver refNum */
    short csCode;		/* adsp driver control code */
    long qStatus;		/* adsp internal use */
    u_short ccbRefNum;		/* connection end refNum */
    union adsp_command u;
};

typedef struct DSPParamBlock DSPParamBlock;
typedef DSPParamBlock *DSPPBPtr;

struct adspcmd {
    struct adspcmd *qLink;
    u_int ccbRefNum;
    caddr_t ioc;
#ifdef _KERNEL
    gref_t *gref;
    gbuf_t *mp;
#else
    void *gref;
    void *mp;
#endif
    short ioResult;
    u_short ioDirection;
    short csCode;
    u_short socket;
    union adsp_command u;
};
#endif
