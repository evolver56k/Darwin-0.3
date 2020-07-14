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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 */

#include <diagnostic.h>
#import <mach/mach_types.h>
#include <machine/setjmp.h>
#import <machine/label_t.h>

/* Call the TVector with thread exception recovery */

extern int
CallTVector_NoRecover(
    void * p1, void * p2, void * p3, void * p4, void * p5, void * p6,
    void * entry );

int
CallTVector(
    void * p1, void * p2, void * p3, void * p4, void * p5, void * p6,
    void * entry )
{
    label_t	jmpbuf;
    int		err;
	vm_offset_t tmpbuf = current_thread()->recover;

#if DIAGNOSTIC
	if(tmpbuf)
		kprintf("CallTVector() ****** NESTED SETJUMP ******\n");
#endif /* DIAGNOSTIC */

    if (setjmp(&jmpbuf)) {
        err = -999;
    } else {

        current_thread()->recover = (vm_offset_t)&jmpbuf;
        err = CallTVector_NoRecover( p1, p2, p3, p4, p5, p6, entry);
    }
    current_thread()->recover = (vm_offset_t)tmpbuf;
    return( err);
}

