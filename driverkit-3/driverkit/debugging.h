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
 * debugging.h - public interface for driver debugging module (DDM) This 
 *	    interface is used by modules which generate DDM data (as 
 *	    opposed to Apps which collect or analyze debugging data).
 *
 * HISTORY
 * 22-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

/*
 * The DDM provides fast, cheap text logging functions. These
 * functions are typically invoked via macros which are defined to be
 * null unless the DDM_DEBUG cpp flag is defined true. Thus, a debug
 * configuration would typically define DDM_DEBUG true and a release
 * configuration would not define DDM_DEBUG. 
 */

#import <kernserv/clock_timer.h>	/* for ns_time_t */

#define IO_NUM_DDM_MASKS	4

/*
 * Bitmask used to filter storing of events.
 */
extern unsigned int IODDMMasks[IO_NUM_DDM_MASKS];

/*
 * Initialize. Client must call this once, before using any services.
 */
#ifdef	KERNEL
void IOInitDDM(int numBufs);
#else	KERNEL
void IOInitDDM(int numBufs, char *serverPortName);
#endif	KERNEL

/*
 * add one entry to debugging log.
 * This function is actually the same in User and Kernel space; the Kernel
 * prototype has no argument for backwards compatibility with callers who
 * did not specify all 6 arguments.
 */
#ifdef	KERNEL
extern void IOAddDDMEntry();
#else	KERNEL
void IOAddDDMEntry(char *str, int arg1, int arg2, int arg3, 
	int arg4, int arg5);
#endif	KERNEL

/*
 * Clear the debugging log.
 */
void IOClearDDM();

/*
 * Get/Set bit mask of IODebuggingMasks[index].
 */
void IOSetDDMMask(int index, unsigned int bitmask);
unsigned IOGetDDMMask(int index);

/*
 * Obtain one sprintf'd entry from the debugging log. 'index' indicates 
 * which entry to return, counting backwards from the last (latest) entry.
 * Returns nonzero if specified entry does not exist.
 */
int IOGetDDMEntry(
	int 		entry, 		// desired entry
	int 		outStringSize,	// memory available in outString
	char 		*outString, 	// result goes here
	ns_time_t 	*timestamp,	// returned
	int 		*cpuNum);	// returned

/*
 * return a malloc'd copy of instring.
 */
const char *IOCopyString(const char *instring);

#if	DDM_DEBUG

#define IODEBUG(index, mask, x, a, b, c, d, e) { 			\
	if(IODDMMasks[index] & mask) {					\
		IOAddDDMEntry(x, (int)a, (int)b, (int)c, 		\
				       (int)d, (int)e); 		\
	}								\
}

#else	DDM_DEBUG

#define IODEBUG(index, mask, x, a, b, c, d, e)

#endif	DDM_DEBUG

