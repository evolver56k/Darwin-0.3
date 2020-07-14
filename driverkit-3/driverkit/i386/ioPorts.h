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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * Primatives for IO port access.
 *
 * HISTORY
 *
 * 16Feb93 David E. Bohman at NeXT
 *	Created.
 */

#ifndef _DRIVERKIT_I386_IOPORTS_
#define _DRIVERKIT_I386_IOPORTS_

#import <driverkit/i386/driverTypes.h>

static __inline__
unsigned char
inb(
    IOEISAPortAddress	port
)
{
    unsigned char	data;
    
    asm volatile(
    	"inb %1,%0"
	
	: "=a" (data)
	: "d" (port));
	
    return (data);
}
 
static __inline__
unsigned short
inw(
    IOEISAPortAddress	port
)
{
    unsigned short	data;
    
    asm volatile(
    	"inw %1,%0"
	
	: "=a" (data)
	: "d" (port));
	
    return (data);
}

static __inline__
unsigned long
inl(
    IOEISAPortAddress	port
)
{
    unsigned long	data;
    
    asm volatile(
    	"inl %1,%0"
	
	: "=a" (data)
	: "d" (port));
	
    return (data);
}

static __inline__
void
outb(
    IOEISAPortAddress	port,
    unsigned char	data
)
{
    static int		xxx;

    asm volatile(
    	"outb %2,%1; lock; incl %0"
	
	: "=m" (xxx)
	: "d" (port), "a" (data), "0" (xxx)
	: "cc");
}

static __inline__
void
outw(
    IOEISAPortAddress	port,
    unsigned short	data
)
{
    static int		xxx;

    asm volatile(
    	"outw %2,%1; lock; incl %0"
	
	: "=m" (xxx)
	: "d" (port), "a" (data), "0" (xxx)
	: "cc");
}

static __inline__
void
outl(
    IOEISAPortAddress	port,
    unsigned long	data
)
{
    static int		xxx;

    asm volatile(
    	"outl %2,%1; lock; incl %0"
	
	: "=m" (xxx)
	: "d" (port), "a" (data), "0" (xxx)
	: "cc");
}
#endif	_DRIVERKIT_I386_IOPORTS_
