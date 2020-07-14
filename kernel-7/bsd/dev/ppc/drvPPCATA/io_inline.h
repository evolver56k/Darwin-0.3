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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Inlines for io space access.
 *
 * HISTORY
 *
 * 20 May 1992 ? at NeXT
 *	Created.
 */
 
#import <machine/types.h>

#ifndef eieio
#define eieio() \
        __asm__ volatile("eieio")
#endif

typedef u_int32_t io_addr_t; 

#if	!defined(DEFINE_INLINE_FUNCTIONS)
static
#endif
inline
unsigned char
inb(
    io_addr_t		port
)
{
    unsigned char	data;
    data = *(volatile u_int8_t *)port;	
    return (data);
}
 
#if	!defined(DEFINE_INLINE_FUNCTIONS)
static
#endif
inline
unsigned short
inw(
    io_addr_t		port
)
{
    unsigned short	data;
    
    data = *(volatile u_int16_t *)port;	
    return (data);
}

#if	!defined(DEFINE_INLINE_FUNCTIONS)
static
#endif
inline
unsigned short
inl(
    io_addr_t		port
)
{
    unsigned long	data;
    
    data = *(volatile u_int32_t *)port;	
    return (data);
}

#if	!defined(DEFINE_INLINE_FUNCTIONS)
static
#endif
inline
void
outb(
    io_addr_t		port,
    unsigned char	data
)
{
    *(volatile u_int8_t *)port = data;
    eieio();
}

#if	!defined(DEFINE_INLINE_FUNCTIONS)
static
#endif
inline
void
outw(
    io_addr_t		port,
    unsigned short	data
)
{
    *(volatile u_int16_t *)port = data;
    eieio();
}

#if	!defined(DEFINE_INLINE_FUNCTIONS)
static
#endif
inline
void
outl(
    io_addr_t		port,
    unsigned long 	data
)
{
    *(volatile u_int32_t *)port = data;
    eieio();
}

