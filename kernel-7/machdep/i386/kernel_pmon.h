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
 * Copyright (c) 1990 NeXT, Inc.
 *
 * This file is the single kernel kernel include file for the pmon interface.
 *
 * HISTORY
 *
 * 1-Mar-90	Brian Pinkerton at NeXT
 *	Created
 *
 */

#ifdef DEBUG

#define PMON	1			/* make PMON conditional on DEBUG */

#import <sys/types.h>
#import <sys/proc.h>
#import <sys/time_stamp.h>
#import <machine/eventc.h>
#import <machine/pmon.h>		/* generic pmon interface */
#import <machine/pmon_targets.h>	/* list of kernel pmon targets */


extern int pmon_flags[];		/* currently enabled event masks */
extern void(*pmon_event_log_p)();	/* pointer to pmon_event_log() 
					   (set by the kern_loader)  */


/*
 *  This is the usual interface to pmon in the kernel.  It just checks to
 *  see if the source,event_type pair is enabled, and if so, grabs the 
 *  current clock and logs the event.
 */

static inline void pmon_log_event(source, event_type, data1, data2, data3)
	int source, event_type;
	int data1, data2, data3;
{
	if (pmon_flags[source] & event_type) {
	    struct tsval event_timestamp;
    
	    event_set_ts(&event_timestamp);
	    (*pmon_event_log_p)(source,
		    event_type,
		    &event_timestamp,
		    data1,
		    data2,
		    data3);
	}
}

#else /* DEBUG */


/*
 *  If we don't have a debug kernel, make sure to turn off pmon event
 *  logging.  We import the definitions anyway, so nothing will be undefined.
 */

#import <machine/pmon.h>		/* generic pmon interface */
#import <machine/pmon_targets.h>	/* list of kernel pmon targets */

#define pmon_log_event(source, event_type, data1, data2, data3)

#endif /* DEBUG */



