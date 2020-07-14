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
 * libIO.m - IO Library, kernel version.
 *
 * HISTORY
 * 17-Apr-91    Doug Mitchell at NeXT
 *      Created.
 *
 * This file contains most of the kernel libIO implementation. Some portions
 * deal with kernel task_t and thread_t structs directly; that code is
 * in libIO_Private.m.
 */

#import <bsd/sys/types.h>
#import <objc/objc.h> 
#import <driverkit/return.h>
#import <driverkit/memcpy.h>
#import <driverkit/generalFuncs.h> 
#import <mach/mach_user_internal.h>
#import <mach/mach_interface.h>
#import <stdarg.h>
#import <machkit/NXLock.h>
#import <machine/mach_param.h>
#import <bsd/sys/syslog.h>
#import <bsd/sys/callout.h>
#import <bsd/dev/ldd.h>
#import <kernserv/ns_timer.h>
#import <kern/clock.h>

/*
 * Misc. kernel prototypes.
 */

extern port_name_t IOTask;			// IOTask's task port

/*
 * Take this out when we have a kernel debugger that works.
 */
extern int libIO_dbg;
#define cdprint(x) { 		\
	if(libIO_dbg) { 	\
		printf x; 	\
	}			\
}

/*
 * Kernel level libIO implementation.
 */
 
void *IOMalloc(int size)
{
	return(kalloc(size));
}

void IOFree(void *p, int size)
{
	kfree(p, size);
}

void IOCopyMemory(void *from, void *to, unsigned int numBytes, 
	unsigned int bytesPerTransfer)
{
 	_IOCopyMemory(from, to, numBytes, bytesPerTransfer);	
}
 

void IOSleep(unsigned milliseconds)
{
	ns_sleep((ns_time_t)(milliseconds) * 1000000LL);
}

/*
 * Spin for indicated number of microseconds.
 */
void IODelay(unsigned microseconds)
{
	DELAY(microseconds);
}

/*
 * Call function fcn with argument arg in specified number of seconds.
 */
void IOScheduleFunc(IOThreadFunc fcn, void *arg, int seconds)
{
	ns_time_t ns_time = (ns_time_t) seconds * 1000000000;
	
	ns_timeout((func)fcn, arg, ns_time, CALLOUT_PRI_THREAD);
}

/*
 * Cancel callout requested in IOScheduleFunc().
 */
void IOUnscheduleFunc(IOThreadFunc fcn, void *arg)
{
	ns_untimeout((func)fcn, arg);
}

/*
 * Obtain current time in nanoseconds.
 */
void IOGetTimestamp(ns_time_t *nsp)
{
	tvalspec_t		now = clock_get_counter(System);

	*nsp = ((ns_time_t)now.tv_sec * NSEC_PER_SEC) + now.tv_nsec;
}

void IOLog(const char *format, ...)
{
        va_list ap;
        
        va_start(ap, format);
        vlog(LOG_INFO, format, ap);
        va_end(ap);
}

/*
 * Panic.
 */
void IOPanic(const char *reason)
{
	panic(reason);
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

IOAlignment IOSizeToAlignment(unsigned int size)
{
    register int shift;
    const intsize = sizeof(unsigned int) * 8;
    
    for (shift = 1; shift < intsize; shift++) {
	if (size & 0x80000000)
	    return (IOAlignment)(intsize - shift);
	size <<= 1;
    }
    return 0;
}

unsigned int IOAlignmentToSize(IOAlignment align)
{
    unsigned int size;
    
    for (size = 1; align; align--) {
	size <<= 1;
    }
    return size;
}

/*
 * Temporary test of IOTask context.
 */
#ifdef	DEBUG
void iotaskTest()
{
	kern_return_t krtn;
	port_t port1;
	port_t port2;
	msg_header_t *msg1, *msg2;
	
	/*
	 * Allocate 2 ports.
	 */
	krtn = port_allocate(task_self(), &port1);
	if(krtn) {
		printf("iotaskTest: port_allocate() returned %d\n", krtn);
		return;	
	}
	else {
		cdprint(("...iotaskTest port1 %d\n", port1));
	}
	krtn = port_allocate(task_self(), &port2);
	if(krtn) {
		printf("iotaskTest: port_allocate() returned %d\n", krtn);
		return;	
	}
	else {
		cdprint(("...iotaskTest port2 %d\n", port2));
	}
	
	/*
	 * Send a dummy message from one port to another.
	 */
	msg1 = IOMalloc(sizeof(*msg1));
	msg1->msg_simple = 1;
	msg1->msg_size = sizeof(*msg1);
	msg1->msg_type = MSG_TYPE_NORMAL;
	msg1->msg_local_port = port1;
	msg1->msg_remote_port = port2;
	msg1->msg_id = 123;
	krtn = msg_send(msg1, SEND_TIMEOUT, 1);
	if(krtn) {
		printf("iotaskTest: msg_send() returned %d\n", krtn);
		return;	
	}
	
	/*
	 * Receive the message.
	 */
	msg2 = IOMalloc(sizeof(*msg2));
	msg2->msg_size = sizeof(*msg2);
	msg2->msg_local_port = port2;
	krtn = msg_receive(msg2, RCV_TIMEOUT, 0);
	if(krtn) {
		printf("iotaskTest: msg_receive() returned %d\n", krtn);
		return;	
	}
	cdprint(("iotaskTest: SUCCESS\n"));
	port_deallocate(task_self(), port1);
	port_deallocate(task_self(), port2);
	IOFree(msg1, sizeof(*msg1));
	IOFree(msg2, sizeof(*msg2));
	IOExitThread();
}
#endif	DEBUG
/* end of libIO.m */
