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

/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * ddm.c - driverkit debugging module, NRW kernel version.
 *
 * HISTORY
 * 08-Feb-92	Doug Mitchell at NeXT
 *	Converted to kernel version (no Objective C).
 * 22-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import "xpr_debug.h"
#import <bsd/sys/types.h>
#import <driverkit/debugging.h>
#import <driverkit/ddmPrivate.h>
#import <driverkit/generalFuncs.h>
#import <kernserv/lock.h>
#import <kernserv/prototypes.h>
#import <bsd/machine/spl.h>
#import <mach/machine/simple_lock.h>

#if	__nrw__
#import <architecture/nrw/clock.h>
#import <kernserv/machine/cpu_number.h>
#define _MON_TYPES_BOOLEAN_
#define _MON_TYPES_CPUTYPES_
#define _MON_TYPES_SIZE_
#define	_MON_BIT_MACROS_
#import <mon/mon_global.h>
#endif	__nrw__

#if	i386
#import <machdep/i386/timer.h>
#import <machdep/i386/timer_inline.h>
#endif	i386

#define	CLOCK_CALIBRATE	0

/*
 * xprbuf_t's are kept in a circular buffer, xprArray. New entries are added
 * when the client calls xprAdd. The buffer is examined from an external
 * task which communicates with xprServerThread via simple Mach messages.
 */
 
/*
 * Globals.
 */
u_int 			IODDMMasks[IO_NUM_DDM_MASKS];	
						// an array of task-specific
						//    bits. Typically tweaked
						//    at run time.
uxprGlobal_t		uxprGlobal;
simple_lock_data_t	xpr_lock;		// protects xprLocked 
int 			xprLocked = 0;		// Simple flag used to prevent
						//    adding xpr entries while
						//    server is accessing 
						//    xprArray. Only written by
					 	//    server.

static xprbuf_t 	*xprEnd;		// Pointer to entry at end of 
						//    xprArray
static int		xprInitialized;

/*
 * DEBUG kernels have a static array of xprBufs so XPRs work before
 * kalloc() is usable.
 */
#if	XPR_DEBUG
#define XPR_STATIC_ARRAY_SIZE	2048
xprbuf_t 			xprbuf_static[XPR_STATIC_ARRAY_SIZE];
#endif	XPR_DEBUG

/*
 * Initialize. Client must call this once, before using any services.
 */
void IOInitDDM(int numBufs)
{
	if(xprInitialized) {
		return;
	}
#if	XPR_DEBUG
	uxprGlobal.xprArray = xprbuf_static;
	uxprGlobal.numXprBufs = XPR_STATIC_ARRAY_SIZE;
#else	XPR_DEBUG
	uxprGlobal.xprArray = IOMalloc(sizeof(xprbuf_t) * numBufs);
	uxprGlobal.numXprBufs = numBufs;
#endif	XPR_DEBUG
	xprEnd = uxprGlobal.xprArray + uxprGlobal.numXprBufs - 1;
	IOClearDDM();
	simple_lock_init(&xpr_lock);
#if	__nrw__
	/*
	 * This lets the boot program and the ROM get to the XPR buffer.
	 */
	mon_global->machdep_global.uxpr_global = &uxprGlobal;
#endif	__nrw__
	xprInitialized = 1;

#if	CLOCK_CALIBRATE
	{
		int i;
		timer_cnt_val_t time;
		
		for(i=0; i<100; i++) {
			timer_latch(TIMER_CNT0_SEL); 
			time = timer_read(TIMER_CNT0_SEL);
			IOAddDDMEntry("test entry %d time %d\n", 
				i,time,3,4,5);
		}
	}
#endif	CLOCK_CALIBRATE
}

/*
 * add one entry to xprArray.
 */
void IOAddDDMEntry(char *str, int arg1, int arg2, int arg3, int arg4, int arg5)
{
	xprbuf_t *last;
	boolean_t rtn;
	unsigned s;
	
	if(uxprGlobal.xprArray == NULL)
		return;				/* not initialized */
	s = splhigh();
	rtn = simple_lock_try(&xpr_lock);	
	if(!rtn) {
		splx(s);
		return;				/* in case of recursion (e.g., 
						 *    in IOTimeStamp()) */
	}
	if(xprLocked) {
		goto out;			/* server is messing with
						 *    xprArray[] */
	}
	if(++uxprGlobal.xprLast > xprEnd)	/* wrap around */
		uxprGlobal.xprLast = uxprGlobal.xprArray;	
	last = uxprGlobal.xprLast;
	last->msg = str;
	last->arg1 = arg1;
	last->arg2 = arg2;
	last->arg3 = arg3;
	last->arg4 = arg4;
	last->arg5 = arg5;
	
#if	__nrw__
	/*
	 * Note that for nrw kernel, timestamp is 32 bits of 
	 * half-microseconds.
	 */
	last->timestamp = (unsigned long long)SYSCLK;
	last->cpu_num = cpu_number();
#else	__nrw__
	last->cpu_num = 0;
#if	i386 || hppa || ppc
	IOGetTimestamp(&last->timestamp);
#else	i386 || hppa || ppc
#error	machine dependent time stamp needed in ddm.c
#endif	i386
#endif	__nrw__
	if(uxprGlobal.numValidEntries < uxprGlobal.numXprBufs)
		uxprGlobal.numValidEntries++;
out:
	simple_unlock(&xpr_lock);
	splx(s);
}

/*
 * Clear the array.
 */
void IOClearDDM()
{
	uxprGlobal.xprLast = xprEnd;		/* last entry */
	uxprGlobal.numValidEntries = 0;
}

/*
 * Set bit mask of IODDMMasks[index]. Typically used by both client and 
 * the server thread.
 */
void IOSetDDMMask(int index, unsigned int bitmask)
{
	if(index > IO_NUM_DDM_MASKS) {
		IOLog("xprSetBitmask: illegal index (%d)\n", index);
		return;
	}
	IODDMMasks[index] = bitmask;
}

unsigned IOGetDDMMask(int index)
{
	if(index > IO_NUM_DDM_MASKS) {
		IOLog("xprGetBitmask: illegal index (%d)\n", index);
		return(0);
	}
	return IODDMMasks[index];
}

/*
 * Obtain one sprintf'd entry from the xprArray. index indicates which entry 
 * to return, counting backwards from the last (latest) entry. Returns 
 * nonzero if specified entry does not exist.
 */
int IOGetDDMEntry(
	int 		index, 		// desired entry
	int 		outStringSize,	// memory available in outString
	char 		*outString, 	// result goes here
	unsigned long long *timestamp,	// returned
	int 		*cpu_num)	// returned
{
	xprbuf_t *xprbuf;
	char xprstring[300];

	if(index >= uxprGlobal.numValidEntries) {
		/*
		 * Ran off the end of the buffer.
		 */
		return -1;
	}
	xprbuf = uxprGlobal.xprLast - index;
	if(xprbuf < uxprGlobal.xprArray)
		xprbuf += uxprGlobal.numXprBufs;		
	
	sprintf(xprstring, xprbuf->msg, xprbuf->arg1, 
		xprbuf->arg2, 
		xprbuf->arg3, 
		xprbuf->arg4, 
		xprbuf->arg5); 
	if(strlen(xprstring) > outStringSize)
		xprstring[outStringSize - 1] = '\0';
	strcpy(outString, xprstring);
	*timestamp = xprbuf->timestamp;
	*cpu_num   = xprbuf->cpu_num;
	return 0;
}

/*
 * return a malloc'd copy of instring. To be used when a string to 
 * be XPR'd is volatile; the memory malloc'd here is never
 * freed!
 */
const char *IOCopyString(const char *instring)
{
	char *outstring = IOMalloc(strlen((char *)instring) + 1);
	
	strcpy(outstring, instring);
	return((const char *)outstring);
}
