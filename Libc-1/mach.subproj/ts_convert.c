/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 *	ts_convert.c -- convert machine dependent timestamps to
 *		machine independent timevals.
 *
 * HISTORY
 * 13-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Timestamp format uses both high and low values for microseconds.  Conversion
 *	to secs/usecs uses GNU long long support.
 */

#ifdef SHLIB
#include <sys/time.h>
#include <kern/time_stamp.h>

convert_ts_to_tv(ts_format,tsp,tvp)
int	ts_format;
struct tsval *tsp;
struct timeval *tvp;
{
	switch(ts_format) {
		case TS_FORMAT_DEFAULT:
			/*
			 *	High value is tick count at 100 Hz
			 */
			tvp->tv_sec = tsp->high_val/100;
			tvp->tv_usec = (tsp->high_val % 100) * 10000;
			break;
		case TS_FORMAT_MMAX:
			/*
			 *	Low value is usec.
			 */
			tvp->tv_sec = tsp->low_val/1000000;
			tvp->tv_usec = tsp->low_val % 1000000;
			break;
		case TS_FORMAT_NeXT: {
			/*
			 * Timestamp is in a 64 bit value.  Get the number
			 * of seconds by dividing this value by 1000000.
			 */
			
			unsigned long long usec = *(unsigned long long *)tsp;
			tvp->tv_usec = (long)usec % 1000000;
			tvp->tv_sec = (long)usec / 1000000;
			break;
		}
		default:
			/*
			 *	Zero output timeval to indicate that
			 *	we can't decode this timestamp.
			 */
			tvp->tv_sec = 0;
			tvp->tv_usec = 0;
			break;
	 }
}
#endif SHLIB
