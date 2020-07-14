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
 *	objc-errors.m
 * 	Copyright 1988, NeXT, Inc.
 */

/*
	NXLogObjcError was snarfed from "logErrorInc.c" in the kit.
  
	Contains code for writing error messages to stderr or syslog.
  
	This code is included in errors.m in the kit, and in pbs.c
	so pbs can use it also.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#include <stdarg.h>
#import <syslog.h>

#import "objc-private.h"

/*	
 *	this routine handles errors that involve an object (or class).
 */
volatile void __objc_error(id rcv, const char *fmt, ...) 
{ 
	va_list vp; 

	va_start(vp,fmt); 
	(*_error)(rcv, fmt, vp); 
	va_end(vp);
	_objc_error (rcv, fmt, vp);	/* In case (*_error)() returns. */
}

#ifndef KERNEL

static int hasTerminal()
{
    static char hasTerm = -1;

    if (hasTerm == -1) {
	int fd = open("/dev/tty", O_RDWR, 0);
	if (fd >= 0) {
	    (void)close(fd);
	    hasTerm = 1;
	} else
	    hasTerm = 0;
    }
    return hasTerm;
}

void _NXLogError(const char *format, ...)
{
    va_list ap;
    char bigBuffer[4*1024];

    va_start(ap, format);
    vsprintf(bigBuffer, format, ap);
    va_end(ap);
    if (hasTerminal()) {
	fwrite(bigBuffer, sizeof(char), strlen(bigBuffer), stderr);
	if (bigBuffer[strlen(bigBuffer)-1] != '\n')
	    fputc('\n', stderr);
    } else
	syslog(LOG_ERR, "%s", bigBuffer);
}

/*
 * 	this routine is never called directly...it is only called indirectly
 * 	through "_error", which can be overriden by an application. It is
 *	not declared static because it needs to be referenced in 
 *	"objc-globaldata.m" (this file organization simplifies the shlib
 *	maintenance problem...oh well). It is, however, a "private extern".
 */
volatile void _objc_error(id self, const char *fmt, va_list ap) 
{ 
    char bigBuffer[4*1024];

    vsprintf (bigBuffer, fmt, ap);
    _NXLogError ("objc: %s: %s", object_getClassName (self), bigBuffer);

    abort();		/* generates a core file */
}

/*	
 *	this routine handles severe runtime errors...like not being able
 * 	to read the mach headers, allocate space, etc...very uncommon.
 */
volatile void _objc_fatal(const char *msg)
{
    _NXLogError("objc: %s\n", msg);

    exit(1);
}

/*
 *	this routine handles soft runtime errors...like not being able
 *      add a category to a class (because it wasn't linked in).
 */
void _objc_inform(const char *fmt, ...)
{
    va_list ap; 
    char bigBuffer[4*1024];

    va_start (ap,fmt); 
    vsprintf (bigBuffer, fmt, ap);
    _NXLogError ("objc: %s", bigBuffer);
    va_end (ap);
}

#else /* not KERNEL */

extern int vlog(int level, const char *format, va_list ap);
extern volatile void panic(const char *reason);

/* special panic versions of the objc error routines */

void _NXLogError(const char *format, ...)
{
        va_list ap;
        
        va_start(ap, format);
        vlog(LOG_ERR, format, ap);
        va_end(ap);
	if(format[strlen(format)-1] != '\n') {
		log(LOG_ERR, "\n");
	}
}

volatile void _objc_error(id self, const char *fmt, va_list ap) 
{ 
	log(LOG_ERR, "objc error: %s ", object_getClassName(self));
	vlog(LOG_ERR, fmt, ap);
	if(fmt[strlen(fmt)-1] != '\n') {
		log(LOG_ERR, "\n");
	}
	abort();
}

volatile void _objc_fatal(const char *msg)
{
	printf("objc fatal: %s\n", msg);
	panic("Objective-C fatal");
}

void _objc_inform(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
	vlog(LOG_ERR, fmt, ap);
	va_end(ap);
	if(fmt[strlen(fmt)-1] != '\n') {
		log(LOG_ERR, "\n");
	}
}

#endif /* not KERNEL */
