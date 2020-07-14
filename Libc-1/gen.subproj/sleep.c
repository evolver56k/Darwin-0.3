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
 * Copyright (c) 1989 NeXT, Inc.
 *
 * Thread-safe sleep()
 */

/**************************************************************************
 *  POSIX Requirements: 
 *
 *	The POSIX sleep() interface requires that if sleep() returns due 
 *	to delivery of a signal, the amount of unslept time in seconds is 
 *	returned (the requested time minus the time actually slept).  
 *	Therefore, the POSIX version of sleep returns after receiving
 *	a signal with a signal-handler. The NeXT version will return
 *	to the sleep after the signal-handler has been executed.
 *
 *  Strategy
 *
 *	Where the original version was broken up into two functions
 *	the POSIX version is in one.  This is to better handle the
 *	case when a signal interrupts the sleep.
 *	
 **************************************************************************/

#include <mach/mach.h>
#include <mach/message.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_SECONDS	(1 << 22)	/* Maximum time to hand to msleep. */

unsigned int
sleep(unsigned int seconds)
{
	msg_header_t	null_msg;
	port_t		port;
	struct timeval	before,
			after;
	msg_return_t	mr;
	int		ret = 0;

	/*
	 *	Because the msg_receive time-out works with milliseconds,
	 *	we have to be careful of overflow when converting the
	 *	seconds value into milliseconds.
	 */
	if (port_allocate(task_self(), &port) != KERN_SUCCESS) {
		errno = EAGAIN;
		return seconds;
	}
	null_msg.msg_local_port = port;
	null_msg.msg_size = sizeof(null_msg);

	while (seconds > 0) {
		unsigned int msecs;

		if (seconds > MAX_SECONDS) {
			msecs = MAX_SECONDS * 1000;
			seconds -= MAX_SECONDS;
		} else {
			msecs = seconds * 1000;
			seconds = 0;
		}

		(void) gettimeofday(&before, (struct timezone *) 0);

		/*
		 *	At this point, we have saved the current time
		 *	in "before".  If the msg_receive is interrupted,
		 *	then we will use this saved value to calculate
		 *	a new time-out for the next msg_receive.
		 */

		mr = msg_receive(&null_msg, RCV_TIMEOUT|RCV_INTERRUPT, msecs);
		if (mr != RCV_INTERRUPTED) {
			if (seconds)
				continue;
			break;
		}
			
		/*
		 *	Adjust the saved "before" time to be the time
		 *	we were hoping to sleep until.  If the current
		 *	time (in "after") is greater than this, then
		 *	we have slept sufficiently despite being interrupted.
		 */
		before.tv_sec += msecs / 1000;
		before.tv_usec += (msecs % 1000) * 1000;
		if (before.tv_usec > 1000000) {
			before.tv_usec -= 1000000;
			before.tv_sec += 1;
		}

		(void) gettimeofday(&after, (struct timezone *) 0);

		if (timercmp(&before, &after, <))
			break;

		/*
		 *	Calculate a new time-out value that should
		 *	get us to the "before" value.  If "before"
		 *	and "after" are close, this might be zero.
		 */

		msecs = ((before.tv_sec - after.tv_sec) * 1000 +
			 (before.tv_usec - after.tv_usec) / 1000);
		if (msecs == 0)
			break;

		ret = msecs / 1000;
		break;
	}

	(void) port_deallocate(task_self(), port);
	return ret;
}
