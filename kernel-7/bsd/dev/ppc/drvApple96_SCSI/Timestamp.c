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
	File:		Timestamp.c

    Contains:	Low-impact event timing function library

    Copyright:	1995-1997 by Apple Computer, Inc., all rights reserved.

    Version:	1.1

    Written by: Martin Minow

    Change History (most recent first):

	 <1>	04/14/97    MM	    Conversion for Rhapsody SCSI from Copland Timestamp.c;8.
	 <2>	97.07.17    MM	    Radar 1669061 Convert PPC tick to nanoseconds for vector return, too.

*/

#define USE_RAW_PPC_CLOCK   1

#include "Timestamp.h"
#import <kernserv/prototypes.h>
#if USE_RAW_PPC_CLOCK
#include <machdep/ppc/powermac.h>
extern long long read_processor_clock(void);
#endif /* USE_RAW_PPC_CLOCK */
#ifndef FALSE
#define FALSE	0
#define TRUE	1
#endif
#ifndef NULL
#define NULL	0
#endif

/*
 * Values for the flags variable in the LogDataRecord. These are private to the
 * LogData library and dcmd display routine.
 */
enum {
    kTimestampEnabledMask	= (1L <<  0),	/* Enable logging if set	    */
    kTimestampPreserveFirstMask = (1L <<  1),	/* Preserve first entry if set	    */
    kTimestampWrapAroundMask	= (1L <<  2)	/* Record has wrapped around once   */
};

struct TimestampRecord {
    volatile UInt32	semaphore;	    /* In critical section if non-zero	    */
    volatile UInt32	lostLockCounter;    /* Can't enter critical section	    */
    volatile UInt32	flags;		    /* Logging & lost data flags	    */
    volatile UInt32	entryPutIndex;	    /* Where to store the next record	    */
    volatile UInt32	entryGetIndex;	    /* Where to retrieve the next record    */
    volatile UInt32	entryMaxIndex;	    /* Actual number of log entries	    */
    TimestampDataRecord entries[1];	    /* Ring buffer of log entries	    */
};
typedef struct TimestampRecord TimestampRecord, *TimestampRecordPtr;



#if 1	/* TEMP TEMP TEMP */
/*
 * Temporary atomic instruction implementations
 */
static inline SInt32 IncrementAtomicAligned(
		volatile SInt32		*theValue
    )
{
		SInt32				result = *theValue;
		++(*theValue);
		return (result);
}
static inline UInt32 BitAndAtomicAligned(
		UInt32				theMask,
		volatile UInt32		*theValue
    )
{
		SInt32				result = *theValue;
		*theValue &= theMask;
		return (result);
}
static inline Boolean CompareAndSwapAligned(
		UInt32				oldValue,
		UInt32				newValue,
		volatile UInt32		*theValue
    )
{
		if (oldValue != (*theValue)) {
			return (FALSE);			/* False */
		}
		else {
			*theValue = newValue;
			return (TRUE);
		}
}

#endif

typedef union TimestampTime {
    long		d[2];
    long long		ppcClock;
    ns_time_t		nsecTime;
} TimestampTime;

static void					StoreRawTimestamp(
	UInt32			timestampTag,
		UInt32					timestampValue,
		const TimestampTime		*timestampTime
    );

#if TIMESTAMP
static TimestampRecordPtr			gTimestampRecordPtr;
#define LOG (*gTimestampRecordPtr)

void
MakeTimestampRecord(
		UInt32					nEntries
    )
{
		UInt32					areaSize;
		UInt32					pageSize;

		if (gTimestampRecordPtr == NULL) {
			areaSize = (nEntries * sizeof (TimestampDataRecord))
					 + sizeof (TimestampRecord)
					 - sizeof (TimestampDataRecord)
				;
			/*
			 * Round up areaSize to a page size.
			 */
			pageSize = 4096;
			areaSize = (areaSize + pageSize - 1) & ~(pageSize - 1);
			/*
			 * Recompute nEntries.
			 */
			nEntries = (areaSize - sizeof (TimestampRecord))
					 / sizeof (TimestampDataRecord);
			nEntries += 1;
	    gTimestampRecordPtr = (TimestampRecordPtr)	kalloc(areaSize);
			if (gTimestampRecordPtr != NULL) {
				LOG.entryMaxIndex = nEntries;
				LOG.flags = ( (1 * kTimestampEnabledMask)		/* Enabled			*/
			    | (0 * kTimestampPreserveFirstMask) /* Save last	    */
							| (0 * kTimestampWrapAroundMask)	/* Always zero		*/
							);
				LOG.entryPutIndex = 0;
				LOG.entryGetIndex = 0;
			} /* If we created the area */
#if 0	/* Temp for initial debugging */
			{
				int				i;
				TimestampTime	t;
				TimestampTime	x;
				for (i = 0; i < 10; i++) {
					t.ppcClock = read_processor_clock();
					x.ppcClock = t.ppcClock;
					x.ppcClock *= powermac_info.proc_clock_to_nsec_numerator;
					x.ppcClock /= powermac_info.proc_clock_to_nsec_denominator;
					IOLog("clock numerator %u, clock denominator %u, value %u %u -> %u %u\n", 
						powermac_info.proc_clock_to_nsec_numerator,
						powermac_info.proc_clock_to_nsec_denominator,
						t.d[0],
						t.d[1],
						x.d[0],
						x.d[1]
					);
				}
			}
#endif
		} /* gTimestampRecordPtr == NULL */
}


void
StoreNSecTimestamp(
	UInt32			timestampTag,
		UInt32					timestampValue,
		ns_time_t				timestampEvent
    )
{
		TimestampTime			timestampTime;

		timestampTime.nsecTime = timestampEvent;
#if USE_RAW_PPC_CLOCK
		timestampTime.ppcClock *= powermac_info.proc_clock_to_nsec_denominator;
		timestampTime.ppcClock /= powermac_info.proc_clock_to_nsec_numerator;
#endif
		StoreRawTimestamp(
			timestampTag,
			timestampValue,
			&timestampTime
		);
}

void
StoreTimestamp(
	UInt32			timestampTag,
		UInt32					timestampValue
    )
{
		TimestampTime			timestampTime;
#if USE_RAW_PPC_CLOCK
		timestampTime.ppcClock = read_processor_clock();
#else
		IOGetTimestamp(&timestampTime.nsecTime);
#endif
		StoreRawTimestamp(
			timestampTag,
			timestampValue,
			&timestampTime
		);
}

static void
StoreRawTimestamp(
	UInt32			timestampTag,
		UInt32					timestampValue,
		const TimestampTime		*timestampTime
    )
{
		UInt32					putIndex;
		UInt32					getIndex;
		TimestampDataPtr		entryPtr;

		if (gTimestampRecordPtr != NULL
		 && (LOG.flags & kTimestampEnabledMask) != 0) {
			if (CompareAndSwapAligned(0, 1, (UInt32 *) &LOG.semaphore) == FALSE) {
				IncrementAtomicAligned((volatile SInt32 *) &LOG.lostLockCounter);
			}
	    else {						/* Nope, we got it  */
				/*
				 * The ring buffer is designed so that put == get implies empty
				 * and pointers are always incremented before use.
				 */
				putIndex = LOG.entryPutIndex + 1;
				if (putIndex >= LOG.entryMaxIndex) {
					putIndex = 0;
					LOG.flags |= kTimestampWrapAroundMask;
				}
				if (putIndex == LOG.entryGetIndex) {		/* Did it fill?		*/
					if ((LOG.flags & kTimestampPreserveFirstMask) != 0) {
						;									/* Keeping first	*/
					}
					else {
						/*
						 * We want to retain the latest entry. Do this by
						 * advancing the "get" pointer as if the earliest entry
						 * has been read. Then jump around the if bracket to store
						 * this datum.
						 */
						getIndex = LOG.entryGetIndex + 1;
						if (getIndex >= LOG.entryMaxIndex)
							getIndex = 0;
						LOG.entryGetIndex = getIndex;
						goto storeDatum;
					}
				}
				else {
storeDatum:			entryPtr = &LOG.entries[putIndex];
					LOG.entryPutIndex = putIndex;
					entryPtr->eventTime = timestampTime->nsecTime;
					entryPtr->timestampTag = timestampTag;
					entryPtr->timestampValue = timestampValue;
				}
		LOG.semaphore = 0;				/* Free semaphore   */
			}
		}
}

/**
 * Returns the next timestamp, if any, in resultData.
 * @param   resultData	    Where to store the data
 * @return  TRUE	    Valid data returned
 *			FALSE			No data is available.
 */
Boolean
ReadTimestamp(
		TimestampDataPtr		resultData				/* Result stored here		*/
    )
{
		UInt32					getIndex;
		Boolean					result = FALSE;

		if (resultData != NULL && gTimestampRecordPtr != NULL) {
			/*
			 * Try to grab the semaphore.
			 */
			if (CompareAndSwapAligned(0, 1, (UInt32 *) &LOG.semaphore) == FALSE) {
				IncrementAtomicAligned((volatile SInt32 *) &LOG.lostLockCounter);
			}
			else {												/* Nope, we got it		*/
				getIndex = LOG.entryGetIndex;
				if (getIndex != LOG.entryPutIndex) {			/* Empty?				*/
					result = TRUE;								/* No: get some data	*/
					if (++getIndex >= LOG.entryMaxIndex)
						getIndex = 0;
					*resultData = LOG.entries[getIndex];
					LOG.entryGetIndex = getIndex;
				}
		LOG.semaphore = 0;				/* Free semaphore   */
			}
		}
#if USE_RAW_PPC_CLOCK
		if (result) {
			TimestampTime			timestampTime;

			timestampTime.nsecTime = resultData->eventTime;
			timestampTime.ppcClock *= powermac_info.proc_clock_to_nsec_numerator;
			timestampTime.ppcClock /= powermac_info.proc_clock_to_nsec_denominator;
			resultData->eventTime = timestampTime.nsecTime;
		}
#endif
		return (result);
}

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
void
ReadTimestampVector(
		TimestampDataPtr		resultVector,			/* -> Result buffer			*/
	UInt32			*count			/* -> Max count, <-actual   */
    )
{
		UInt32					getIndex;
		UInt32					i;
		
		if (resultVector != NULL
		 && gTimestampRecordPtr != NULL
		 && count != NULL) {
		 	i = 0;
			if (CompareAndSwapAligned(0, 1, (UInt32 *) &LOG.semaphore) == FALSE) {
				IncrementAtomicAligned((volatile SInt32 *) &LOG.lostLockCounter);
			}
			else {
				getIndex = LOG.entryGetIndex;
				for (; i < *count && getIndex != LOG.entryPutIndex; i++) {
					if (++getIndex >= LOG.entryMaxIndex)
						getIndex = 0;
					*resultVector = LOG.entries[getIndex];
#if USE_RAW_PPC_CLOCK
					resultVector->eventTime =
						(resultVector->eventTime * powermac_info.proc_clock_to_nsec_numerator)
						/ powermac_info.proc_clock_to_nsec_denominator;
#endif
					resultVector++;
				}
				LOG.entryGetIndex = getIndex;
				LOG.semaphore = 0;						/* Free semaphore			*/
			}
			*count = i;									/* Return actual count		*/
		}
}

Boolean
EnableTimestamp(
		Boolean					enableTimestamp			/* True to enable timestamp */
    )
{
		UInt32					newFlags;
		Boolean					timestampsWereEnabled;

		if (gTimestampRecordPtr == NULL) {
			timestampsWereEnabled = FALSE;
		}
		else {
			do {
				timestampsWereEnabled = (LOG.flags & kTimestampEnabledMask) != 0;
				if (enableTimestamp)
					newFlags = LOG.flags | kTimestampEnabledMask;
				else {
					newFlags = LOG.flags & ~kTimestampEnabledMask;
				}
			} while (CompareAndSwapAligned(LOG.flags, newFlags, &LOG.flags) == FALSE);
		}
		return (timestampsWereEnabled);
}

Boolean
PreserveTimestamp(
	Boolean			preserveFirst		/* TRUE to preserve start   */
    )
{
		UInt32					newFlags;
		Boolean					wasFirst;

		if (gTimestampRecordPtr == NULL) {
			wasFirst = FALSE;
		}
		else {
			do {
				wasFirst = (LOG.flags & kTimestampPreserveFirstMask) != 0;
				if (preserveFirst)
					newFlags = LOG.flags | kTimestampPreserveFirstMask;
				else {
					newFlags = LOG.flags & kTimestampPreserveFirstMask;
				}
			} while (CompareAndSwapAligned(LOG.flags, newFlags, &LOG.flags) == FALSE);
		}
		return (wasFirst);
}

void
ResetTimestampIndex(void)
{
		if (gTimestampRecordPtr != NULL) {
			BitAndAtomicAligned(0, &LOG.entryPutIndex);
			BitAndAtomicAligned(0, &LOG.entryGetIndex);
		}
}

UInt32
GetTimestampSemaphoreLostCounter(void)
{
		UInt32					result;

		if (gTimestampRecordPtr == NULL) {
			result = 0;
		}
		else {
			do {
				result = LOG.lostLockCounter;
			} while (CompareAndSwapAligned(
					result, result, (UInt32 *) &LOG.lostLockCounter) == FALSE);
		}
		return (result);
}
#endif /* TIMESTAMP */
