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
	File:		Timestamp.h

    Contains:	Low-impact event timing function library

    Version:	1.1

    Written by: Martin Minow

    Copyright:	© 1995-1997 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

	 <1>	04/14/97    MM	    Conversion for Rhapsody SCSI from Copland Timestamp.h;7.
*/

/*										Timestamp.h								*/
/*
 * Timestamp.h
 * Copyright 1992-97 Apple Computer Inc. All Rights Reserved.
 * Programmed by Martin Minow,
 *  Internet:	minow@apple.com
 *  AppleLink:	MINOW
 * Version of May 25, 1995
 */
/*
 * Usage:
 *  1.	In the makefile (or elsewhere), define TIMESTAMP non-zero. If zero, this
 *		code will be stubbed out.
 *  2.	In your initialization routine, call MakeTimestampRecord() to create a
 *		timestamp record. This will be stored in a static, private, variable.
 *  3.	When you want to time something, call StoreTimestamp() as follows:
 *			{
 *				ns_time_t		eventTime;
 *				IOGetTimestamp(&eventTime);
 *				StoreTimestamp(timestampTag, timestampValue, eventTime);
 *			}
 *		Where timestampTag and timestampValue are 32-bit unsigned integers
 *		that are not otherwise interpreted by the Timestamp library. By
 *		convention, timestampTag contains a 4-byte character (Macintosh OSType)
 *		that distinguishes timing events. The OSType, OSTag, and OSString macros
 *		can be used to construct tag values. OSTag is useful for recording
 *		elapsed time:
 *			StoreTimestamp(OSTag('+', "foo"), 0, startTime);
 *			...
 *			StoreTimestamp(OSTag('-', "foo"), 0, endTime);
 */

#import <objc/objc.h>
#import <kernserv/ns_timer.h>
#import <driverkit/return.h>
#ifndef TIMESTAMP
#define TIMESTAMP   1	    /* TEMP TEMP TEMP */
#endif

#ifndef __APPLE_TYPES_DEFINED__
#define __APPLE_TYPES_DEFINED__ 1
/**
 * These typedef's reproduce (more or less) the Macintosh data types I'm familiar with.
 */
typedef void		*PhysicalAddress;   /* This is an address on the PCI bus	*/
typedef vm_address_t	LogicalAddress;		/* This address is "visible" to software	*/
typedef unsigned int	UInt32;				/* A 32-bit unsigned integer				*/
typedef unsigned char	UInt8;				/* A "byte-sized" integer					*/
typedef signed int		SInt32;				/* A 32-bit signed integer					*/
typedef BOOL			Boolean;			/* A TRUE/FALSE value (YES/NO in NeXT)		*/
#endif /* __APPLE_TYPES_DEFINED__ */

/**
 * Construct a UInt32 from four characters.
 */
#define OSType(c0, c1, c2, c3) ( \
	(   ((c0) << 24)	\
	 |  ((c1) << 16)	\
	 |  ((c2) <<  8)	\
	 |  ((c3) <<  0)	\
		))
/**
 * Construct an OSType from a single character and the first three characters from
 * a given string.
 */
#define OSTag(where, what)  (OSType((where), (what)[0], (what)[1], (what)[2]))
/**
 * Construct an OSType from the first four characters of a C-string.
 */
#define OSString(what)		(OSType((what)[0], (what)[1], (what)[2], (what)[3]))

/*  .___________________________________________________________________________________.
    | Each timestamp entry contains the following information:				|
    |	    timestampTag    A user-specified OSType that identifies this timestamp	|
    |	    timestampValue  A user-specified additional value				|
    |	    eventTime	    The system UpTime value at the time the data was collected. |
    .___________________________________________________________________________________.
*/
struct TimestampDataRecord {
    UInt32		timestampTag;	    /* Caller's tag parameter			*/
    UInt32		timestampValue;	    /* Caller's value parameter			*/
    ns_time_t		eventTime;	    /* Nanoseconds at Timestamp call		*/
};
typedef struct TimestampDataRecord TimestampDataRecord, *TimestampDataPtr;

#if TIMESTAMP
void						MakeTimestampRecord(
		UInt32					nEntries
    );
void						StoreNSecTimestamp(
	UInt32			timestampTag,
		UInt32					timestampValue,
		ns_time_t				timestampEventNSec
    );
void						StoreTimestamp(
	UInt32			timestampTag,
		UInt32					timestampValue
    );
/**
 * Returns the next timestamp, if any, in resultData.
 * @param   resultData	    Where to store the data
 * @return  TRUE	    Valid data returned
 *			FALSE			No data is available.
 */
Boolean						ReadTimestamp(
		TimestampDataPtr		resultData
    );
/**
 * Return a vector of timestamps.
 * @param   resultVector    Where to store the data
 * @param   count	    On entrance, this has the maximum number of elements
 *							to return. On exit, this will have the actual number
 *							of elements that were returned.
 * Note that, if the semaphore is blocked, ReadTimestampVector will not return any
 * data. Data cannot be collected while ReadTimestampVector is copying data
 * to the user's buffer. Note that, since the user's buffer will typically be
 * in pageable memory, pageing I/O that might otherwise be timestamped will
 * be lost.
 */
void						ReadTimestampVector(
		TimestampDataPtr		resultVector,			/* -> Result buffer			*/
	UInt32			*count			/* -> Max count, <-actual   */
    );

Boolean						EnableTimestamp(
	Boolean			enableTimestamp		/* TRUE to enable timing    */
    );
Boolean						PreserveTimestamp(
	Boolean			preserveFirst		/* TRUE to preserve start   */
    );
void						ResetTimestampIndex(void);
UInt32						GetTimestampSemaphoreLostCounter(void);
#else /* TIMESTAMP not compiled */
#define MakeTimestampRecord(nEntries)	/* Nothing */
#define StoreTimestamp(timestampTag, timestampValue, timestampEvent)	/* Nothing  */
#define ReadTimestamp(resultData)   (0)					/* Fails    */
#define ReadTimestampVector(resultVector, count) do {	\
		if ((count) != NULL) {							\
			*(count) = 0;								\
		}												\
    } while (0)
#define EnableTimestamp(enableTimestamp)    (enableTimestamp)
#define PreserveTimestamp(preserveFirst)    (preserveFirst)
#define ResetTimestampIndex()						/* Nothing  */
#define GetTimestampSemaphoreLostCount()    (0)
#endif /* TIMESTAMP */

