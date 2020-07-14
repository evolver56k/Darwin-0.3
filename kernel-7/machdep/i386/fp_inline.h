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
 * Intel386 Family:	Floating point maintenance support.
 *
 * HISTORY
 *
 * 7 April 1992 ? at NeXT
 *	Created.
 */

#import <architecture/i386/fpu.h>

/*
 * Reinitialize the floating
 * point unit.
 */

static inline
void
fninit(
    void
)
{
    asm volatile("
	fninit");
}

/*
 * Save the entire state of
 * the floating point unit.
 * The task switched flag
 * is cleared before the save
 * so that we are guaranteed
 * not to get an INT 7.  Also
 * wait until the state save is
 * completed before returning.
 */

static inline
void
fnsave(
    fp_state_t *	state
)
{
    asm volatile("
	clts;
	fnsave %0;
	fwait"
	    :
	    : "m" (*state));
}

/*
 * Restore the entire state of
 * the floating point unit.  The
 * task switched flags is cleared
 * before the restore so that we
 * are guaranteed not to get an
 * INT 7.
 */

static inline
void
frstor(
    fp_state_t *	state
)
{
    asm volatile("
	clts;
	frstor %0"
	    :
	    : "m" (*state));
}
