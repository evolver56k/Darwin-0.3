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
 * libIO.m - IO Library, user version.
 *
 * HISTORY
 * 17-Apr-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <objc/objc.h>
#import <driverkit/return.h>
#import <driverkit/memcpy.h>
#import <driverkit/generalFuncs.h>
#import <mach/cthreads.h>
#import <mach/mach.h>
#import <bsd/syslog.h>
#import	<stdarg.h>
#import <machkit/NXLock.h>
#import <kernserv/queue.h>
#import <driverkit/Device_ddm.h>
#import <kern/time_stamp.h>

#if	0
/*
 * For now...
 */
#define xpr_libio(x,a,b,c,d,e) printf(x,a,b,c,d,e)
#endif	0

/*
 * Struct for queueing IOScheduleFunc() requests.
 */
typedef struct {
	IOThreadFunc	fcn;
	void		*arg;
	ns_time_t	calloutTime;
	queue_chain_t	link;
} ioCallout_t;

/*
 * Static variables for this module. 
 */
static queue_head_t 	calloutChain;		// queue of ioCallout_t's
static id		calloutLock;		// NXLock
static port_t		sleepPort;

/*
 * Local prototypes.
 */
static void calloutThread(void *foo);

/*
 * User-level libIO implementation.
 */
 
void *IOMalloc(int size)
{
	return(malloc(size));
}

void IOFree(void *p, int size)
{
	free(p);
}

void IOCopyMemory(void *from, void *to, unsigned int numBytes, 
	unsigned int bytesPerTransfer)
{
 	_IOCopyMemory(from, to, numBytes, bytesPerTransfer);	
}

/*
 * Note in user space:
 *	 IOThread == cthread_t 
 *	 IOThreadFunc = cthread_fn_t
 * for now.
 */
IOThread IOForkThread(IOThreadFunc fcn, void *arg)
{
	return((IOThread)cthread_fork((cthread_fn_t)fcn, (any_t *)arg));
}

void IOSuspendThread(IOThread thread)
{
	cthread_t cth = (cthread_t)thread;
	
	/*
	 * Note no cthread_suspend...what a kludge.
	 */
	thread_suspend(cthread_thread(cth));
}

void IOResumeThread(IOThread thread)
{
	cthread_t cth = (cthread_t)thread;

	thread_resume(cthread_thread(cth));
}

/*
 * 30-Jul-91 - removed IOThreadAbort(); there's no way for this to work 
 * 	       cleanly (dmitch)
 */
#ifdef	notdef
void IOThreadAbort(IOThread thread)
{
	cthread_abort(thread);
}
#endif	notdef

volatile void IOExitThread()
{
	(volatile void)cthread_exit((any_t)-1);
}

/*
 * Sleep for indicated number of milliseconds.
 *
 * Current implementation is like libc's msleep, which uses 
 * the millisecond-resolution timeout on a msg_receive() on a local
 * port (to which no messages will ever be sent). Unlike the libc version,
 * this will return if the current thread is interrupted before the desired
 * delay expires.
 */
void IOSleep(unsigned milliseconds)
{
	msg_header_t null_msg;
	
	null_msg.msg_local_port = sleepPort;
	null_msg.msg_size = sizeof(null_msg);
	msg_receive(&null_msg, RCV_TIMEOUT|RCV_INTERRUPT, milliseconds);
}

/*
 * Spin for indicated number of microseconds.
 */
void IODelay(unsigned microseconds)
{
	ns_time_t currentTime;
	ns_time_t endTime;
	unsigned diff;
	
	IOGetTimestamp(&endTime);
	endTime += ((ns_time_t)microseconds * 1000);
	while(1) {
		IOGetTimestamp(&currentTime);
		diff = (unsigned)(endTime - currentTime);
		if((int)(diff) < 0) {
			return;
		}
	}
}

/*
 * Call function fcn with argument arg in specified number of seconds.
 */
void IOScheduleFunc(IOThreadFunc fcn, void *arg, int seconds)
{
	ioCallout_t *callout;
	ns_time_t ns = 0;
	
	/* 
	 * Enqueue this request. The calloutThread will eventually process
	 * it.
	 */
	if(seconds == 0) {
		(*fcn)(arg);
		return;
	}
	callout = IOMalloc(sizeof(*callout));
	callout->fcn = fcn;
	callout->arg = arg;
	[calloutLock lock];
	IOGetTimestamp(&callout->calloutTime);
	ns = seconds;
	ns *= (1000 * 1000 * 1000);
	callout->calloutTime += ns;
	xpr_libio("IOScheduleFunc(%d): making request\n", 
		seconds, 2,3,4,5);
	queue_enter(&calloutChain,
		callout,
		ioCallout_t *,
		link);
	[calloutLock unlock];
	return;
}

/*
 * This thread's job is to wake up once a second, scan the calloutChain, and
 * do callouts for each entry whose time has come. This is pretty inefficient
 * (yet another thread waiting around, running every second...). Maybe we can
 * do better in the future.
 *
 * FIXME - this doesn't work. The libc long long support seems to be broken...
 */
static void calloutThread(void *foo)
{
	ioCallout_t *callout, *calloutNext;
	ns_time_t currentTime;

	while(1) {
		[calloutLock lock];
		callout = (ioCallout_t *)queue_first(&calloutChain);	
		while(!queue_end(&calloutChain, (queue_t)callout)) {
			calloutNext = (ioCallout_t *)callout->link.next;
			IOGetTimestamp(&currentTime);
			xpr_libio("calloutThread: currentTime %u:%u\n",
				(unsigned)(currentTime / 0x100000000ULL),
				(unsigned)(currentTime && 0xffffffff), 3,4,5);
			if(currentTime >= callout->calloutTime) {
				xpr_libio("calloutThread: doing callout\n",
					1,2,3,4,5);
				queue_remove(&calloutChain,
					callout,
					ioCallout_t *,
					link);
				[calloutLock unlock];
				(*callout->fcn)(callout->arg);
				IOFree(callout, sizeof(*callout));
				[calloutLock lock];
			}
			callout = calloutNext;
		}
		[calloutLock unlock];
		IOSleep(1000);
	}
	/* NOT REACHED */
}


/*
 * Cancel callout requested in IOScheduleFunc().
 */
void IOUnscheduleFunc(IOThreadFunc fcn, void *arg)
{
	ioCallout_t *callout;
	
	[calloutLock lock];
	callout = (ioCallout_t *)queue_first(&calloutChain);	
	while(!queue_end(&calloutChain, (queue_t)callout)) {
		if((callout->fcn == fcn) && (callout->arg == arg)) {
			queue_remove(&calloutChain,
				callout,
				ioCallout_t *,
				link);
			IOFree(callout, sizeof(*callout));
			goto done;
		}
		callout = (ioCallout_t *)callout->link.next;
	}
	xpr_libio("IOUnTimeout: callout not registered\n", 1,2,3,4,5);
done:
	[calloutLock unlock];
	return;
}

/*
 * Obtain current time in nanoseconds.
 */

static inline ns_time_t
tsval_to_ns_time(struct tsval *tsp)
{
	ns_time_t	a_time;
	
	a_time = ((ns_time_t)tsp->high_val) << 32;
	a_time += tsp->low_val;
	a_time *= 1000;
	
	return a_time;
}


void IOGetTimestamp(ns_time_t *nsp)
{
	struct tsval ts;
	ns_time_t ns;
	
	kern_timestamp(&ts);
#if	0
	ns = tsval_to_ns_time(&ts);
#else	0
	ns = ((ns_time_t)ts.high_val) * 0x100000000ULL;
	ns += ts.low_val;
	ns *= 1000;
#endif	0
	*nsp = ns;
}

void IOLog(const char *format, ...)
{
        va_list ap;
        char buf[300];
        
        va_start(ap, format);
        vsprintf(buf, format, ap);
        syslog(LOG_ERR, "%s", buf);
        va_end(ap);
}

void IOPanic(const char *reason)
{
	IOLog(reason);
	IOLog("waiting for debugger connection...");
	/* FIXME */
	while(1)
		;
}

/*
 * Convert a integer constant (typically a #define or enum) to a string.
 */
static char noValue[80];

const char *IOFindNameForValue(int value, const IONamedValue *regValueArray)
{
	for( ; regValueArray->name; regValueArray++) {
		if(regValueArray->value == value)
			return(regValueArray->name);
	}
	sprintf(noValue, "%d(d) (UNDEFINED)", value);
	return((const char *)noValue);
}

IOReturn IOFindValueForName(const char *string, 
	const IONamedValue *regValueArray,
	int *value)
{
	for( ; regValueArray->name; regValueArray++) {
		if(!strcmp(regValueArray->name, string)) {
			*value = regValueArray->value;
			return IO_R_SUCCESS;
		}
	}
	return IO_R_INVALID_ARG;
}

/*
 * One-time only init for this module.
 */
void IOInitGeneralFuncs()
{	
	queue_init(&calloutChain);
	calloutLock = [NXLock new];
	if(port_allocate(task_self(), &sleepPort) != KERN_SUCCESS) {
		IOLog("IOInitGeneralFunc: port_allocate error\n");
	}
	IOForkThread((IOThreadFunc)&calloutThread, NULL);
}

