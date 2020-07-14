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

/******************************************************************************

    ev_private.h
    Internal defs for the events driver.  The contents of this module
    may need to be tweaked slightly from one architecture to the next.
    22 May 1992	Mike Paquette at NeXT Computers, Inc.
    
    Copyright 1992 NeXT, Inc.
    
    Modified:
    

******************************************************************************/

#ifdef	DRIVER_PRIVATE

#ifndef _PPC_DEV_EV_PRIVATE_
#define _PPC_DEV_EV_PRIVATE_

#import <bsd/dev/machine/event.h>
/*
 * Prototypes for functions implementing machine dependent code.
 */
 IOReturn
createEventShmem(	port_t task,			// in
			vm_size_t size,			// in
			struct vm_map **owner,		// out
			vm_offset_t *owner_addr,	// out
			vm_offset_t *shmem_addr	);	// out
 IOReturn
destroyEventShmem(	port_t task,
			struct vm_map *owner,
			vm_size_t size,
			vm_offset_t owner_addr,
			vm_offset_t shmem_addr );

const char **defaultEventSources(void);

/* Initial cursor position */
#define INIT_CURSOR_X		100
#define INIT_CURSOR_Y		100

/* Default mouse click time and motion constants */
#define	EV_DCLICKTIME	30	/* Default ticks for a double-click */
#define	EV_DCLICKSPACE	3	/* Default pixel threshold for double-clicks */

/* Default Wait Cursor Timing Constants (in nanoseconds) */
#define DefaultWCSustain	300000000ULL	/* 0.3 seconds */	
#define DefaultWCFrameRate	75000000ULL	/* 13.3 frames/second */
#define DefaultWCThreshold	1200000000ULL	/* 1.2 seconds */

#define EV_STD_CURSOR	0
#define EV_WAITCURSOR	1
#define EV_WAITCURSOR_1	2
#define EV_WAITCURSOR_2	3
#define EV_MAXCURSOR	(EV_WAITCURSOR_2)

/* Default dim time is 5 minutes */
#define DAUTODIMPERIOD	(EV_TICKS_PER_SEC*60*5)
/* Default dim level is one-fourth */
#define DDIMBRIGHTNESS	(EV_SCREEN_MAX_BRIGHTNESS/4)


/* Where event numbers start */
#define INITEVENTNUM	13
#define	NULLEVENTNUM 0		/* The event number that never was */

#define MOVEDEVENTMASK \
	(NX_MOUSEMOVEDMASK | NX_LMOUSEDRAGGEDMASK | NX_RMOUSEDRAGGEDMASK )
#define COALESCEEVENTMASK \
	(MOVEDEVENTMASK | NX_MOUSEEXITEDMASK)
#define MOUSEEVENTMASK \
	(NX_LMOUSEDOWNMASK|NX_RMOUSEDOWNMASK|NX_LMOUSEUPMASK|NX_RMOUSEUPMASK)
#define PRESSUREEVENTMASK \
	(NX_LMOUSEDOWNMASK|NX_LMOUSEUPMASK|NX_MOUSEMOVEDMASK|NX_LMOUSEDRAGGEDMASK)

#define HARD_POWEROFF_MASK 		(NX_COMMANDMASK|NX_ALTERNATEMASK)
/* Flags used to trigger debug operations in a DEBUG kernel */
#define PWR_BREAK_TO_DEBUGGER_MASK	(NX_CONTROLMASK)
#define BRIGHT_BREAK_TO_DEBUGGER_MASK	(NX_ALTERNATEMASK|NX_SHIFTMASK)

/* Flags which can modify meaning of special volume/brightness keys */
#define SPECIALKEYS_MODIFIER_MASK \
			(NX_COMMANDMASK|NX_ALTERNATEMASK|NX_CONTROLMASK)

/* Bits in evg->eventFlags owned by keyboard devices */
#define KEYBOARD_FLAGSMASK \
	(NX_ALPHASHIFTMASK | NX_SHIFTMASK | NX_CONTROLMASK | NX_ALTERNATEMASK \
	| NX_COMMANDMASK | NX_NUMERICPADMASK | NX_HELPMASK | NX_NEXTCTLKEYMASK\
	| NX_NEXTLSHIFTKEYMASK | NX_NEXTRSHIFTKEYMASK | NX_NEXTLCMDKEYMASK \
	| NX_NEXTRCMDKEYMASK | NX_NEXTLALTKEYMASK | NX_NEXTRALTKEYMASK)

/* bits in evg->eventFlags owned by pointer devices */
#define POINTER_FLAGSMASK \
	(NX_STYLUSPROXIMITYMASK | NX_NONCOALSESCEDMASK)
#define SCREENTOKEN 256	/* Some non-zero token to or with screen number */

/* A macro to report if the event queue is not empty */
#define EventsInQueue() \
    (eventsOpen && (((EvGlobals*)evg)->LLEHead != ((EvGlobals*)evg)->LLETail))

/*
 * Internal types and definitions used to drive I/O thread operations.
 */
#define EV_IO_OP_MSG_ID	1
/* Private control message IDs used in talking to the EventListener thread */
typedef enum {
	EVENT_LISTENER_EXIT,
	EVENT_LISTENER_PING,
	EVENT_LISTENER_CALLBACK
} evIoOp;

// Parameters to use for Obj-C callback from I/O thread
typedef struct
{
	id		instance;	// Sender and reciever MUST be
	SEL		selector;	// in the same address space
	id		data;		// for this to work.
} evCallback;

// Union of all possible param structs we may want to pass
typedef union {
	    evCallback	callback;	// EVENT_LISTENER_CALLBACK
} evIoOpParams;

typedef struct {
	evIoOp			op;		// EVENT_LISTENER_EXIT, etc.
	id			cmdLock;	// NXConditionLock. Exported
						//   methods sleep on this.
	IOReturn		*status;	// Returned status if any
	evIoOpParams		params;
} evIoOpBuf;

/*
 * Condition variable states for evIoOpBuf.cmdLock.
 */
#define CMD_INPROGRESS		1
#define CMD_DONE		2

/*
 * Mach message wrapper for threadOpBuf.
 */
typedef struct {
	msg_header_t	header;
	msg_type_t	type;
	evIoOpBuf	opBuf;
} evIoOpMsg;

/*
 * Low level communications buffers between the Event IO thread and
 * the Event Driver and clients.
 */
#define EV_INBUFLEN	512	// sizes for message buffers
#define EV_OUTBUFLEN	5120	// Tweak to match MiG max sizes

typedef struct {
	msg_header_t	hdr;
	char		data[EV_INBUFLEN - sizeof (msg_header_t)];
} EvInMsg;

typedef struct {
	msg_header_t	hdr;
	char		data[EV_OUTBUFLEN - sizeof (msg_header_t)];
} EvOutMsg;

struct _eventMsg {
    msg_header_t h;
    msg_type_t   t;
};

/******************************************************************************
    EvScreen
    This structure is used by the ev driver.
    It holds information about a single screen: how much private shmem it owns,
    where its private shmem region starts, its global bounds and four procedure
    vectors. This structure is allocated by the ev driver and is filled in
    when a driver calls ev_register_screen().
******************************************************************************/

typedef volatile struct _evScreen {
    id instance;	/* Driver instance owning this screen. */
    void *shmemPtr;	/* Ptr to private shmem (if non-zero size) */
    int shmemSize;	/* Size of private shmem */
    Bounds bounds;	/* Screen's bounds in device coordinates */
} EvScreen;

/*
 *	We maintain a queue of EventSrc instances attached to the Event
 *	Driver.  These sources are dynamically checked in with the Event
 *	Driver.  When the driver is closed (as on Window Server exit) we
 *	post a relinquishOwnership:client message to the drivers.
 */
typedef struct {
	id			eventSrc;
} eventSrcInfo;

typedef struct {
	eventSrcInfo		info;
	queue_chain_t 		link;
} attachedEventSrc;

// No-op XPR stuff
#define xpr_ev_oc(x, a, b, c, d, e)
#define xpr_ev_shmemalloc(x, a, b, c, d, e)
#define xpr_ev_shmemlock(x, a, b, c, d, e)
#define xpr_ev_regscr(x, a, b, c, d, e)
#define xpr_ev_cursor(x, a, b, c, d, e)
#define xpr_ev_keybd(x, a, b, c, d, e)
#define xpr_evsrc(x, a, b, c, d, e)
#define xpr_ev_post(x, a, b, c, d, e)
#define xpr_ev_dspy(x, a, b, c, d, e)

#endif /* _PPC_DEV_EV_PRIVATE_ */

#endif /* DRIVER_PRIVATE */
