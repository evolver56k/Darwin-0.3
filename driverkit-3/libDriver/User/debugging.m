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
 * debugging.m - driverkit debugging module.
 *
 * HISTORY
 * 22-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <bsd/sys/types.h>
#import <driverkit/debugging.h>
#import <driverkit/ddmPrivate.h>
#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <driverkit/return.h>
#ifdef	KERNEL
#import <kernserv/prototypes.h>
#import <kernserv/machine/cpu_number.h>
#else	KERNEL
#import <bsd/libc.h>
#endif	KERNEL

#if	KERNEL && __nrw__
#define _MON_TYPES_BOOLEAN_
#define _MON_TYPES_CPUTYPES_
#define _MON_TYPES_SIZE_
#define	_MON_BIT_MACROS_
#import <mon/mon_global.h>
#endif	KERNEL && __nrw__

#if	TIMESTAMP_HALF_USEC
static unsigned xprTimeStamp();
#endif	TIMESTAMP_HALF_USEC

/*
 * xprbuf_t's are kept in a circular buffer, uxprGlobal.xprArray. New 
 * entries are added when the client calls xprAdd. The buffer is examined 
 * from an external task which communicates with xprServerThread via 
 * simple Mach messages.
 */
 
/*
 * Globals.
 */
unsigned	IODDMMasks[IO_NUM_DDM_MASKS];	
					/* an array of task-specific bits. 
					 * Typically tweaked at run time. */
uxprGlobal_t	uxprGlobal;
id		xprLock;		/* NXSpinLock. protects xprLocked */
int 		xprLocked = 0;		/* Simple flag used to prevent adding 
					 * xpr entries while server is 
					 * accessing xprArray. Only written by
					 * server. */

static xprbuf_t *xprEnd;		/* Pointer to entry at end of 
					 * xprArray */
static int	xprInitialized;

/*
 * Initialize. Client must call this once, before using any services.
 */
#ifdef	KERNEL
void IOInitDDM(int numBufs)
#else	KERNEL
void IOInitDDM(int numBufs, char *serverPortName)
#endif	KERNEL
{
	if(xprInitialized) {
		return;
	}
	uxprGlobal.xprArray = IOMalloc(sizeof(xprbuf_t) * numBufs);
	uxprGlobal.numXprBufs = numBufs;
	xprEnd = uxprGlobal.xprArray + uxprGlobal.numXprBufs - 1;
	IOClearDDM();
	xprLock = [NXSpinLock new];
	
#ifndef	KERNEL
	/*
	 * Start up the server thread for user version. Kernel server 
	 * is implemented via kernserv.
	 */
	IOForkThread((IOThreadFunc)xprServerThread, serverPortName);
#else	KERNEL
#if	__nrw__
	mon_global->machdep_global.uxpr_global = &uxprGlobal;
#endif	__nrw__
#endif	KERNEL
	xprInitialized = 1;
}

/*
 * add one entry to xprArray.
 */
void IOAddDDMEntry(char *str, int arg1, int arg2, 
				    int arg3, int arg4, int arg5)
{
	xprbuf_t *last;
	
	if(uxprGlobal.xprArray == NULL)
		return;				/* not initialized */
	[xprLock lock];
	if(xprLocked) {
		[xprLock unlock];
		return;
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
	
	/*
	 * As of 17-Aug-92, timestamp is 32 bits of 
	 * half-microseconds.
	 */
#if	TIMESTAMP_HALF_USEC
	last->timestamp = (unsigned long long)xprTimeStamp();
#else	TIMESTAMP_HALF_USEC
	IOGetTimestamp(&last->timestamp);
#endif	TIMESTAMP_HALF_USEC

#if	KERNEL && __nrw__
	last->cpu_num = cpu_number();
#else	KERNEL && __nrw__
	last->cpu_num = 0;
#endif	KERNEL && __nrw__
	if(uxprGlobal.numValidEntries < uxprGlobal.numXprBufs)
		uxprGlobal.numValidEntries++;
	[xprLock unlock];
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
	char *outstring = IOMalloc(strlen((char *)instring));
	
	strcpy(outstring, instring);
	return((const char *)outstring);
}

#if	TIMESTAMP_HALF_USEC
/*
 * Obtain timestamp - 32 bits of half-microseconds.
 */
static unsigned xprTimeStamp()
{
	struct tsval ts;
	
	kern_timestamp(&ts);
	return (ts.low_val * 2);
}

#endif	TIMESTAMP_HALF_USEC
