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
 * kbd_entries.h
 *      register necessary functions from the keyboard driver
 * History
 *	June 2, 1994	Created		Erik Kay at NeXT
 */

#ifdef	KERNEL_PRIVATE

#import	<bsd/dev/evsio.h>
#import <driverkit/IOEventSource.h>
#import	<bsd/dev/i386/PCKeyboardDefs.h>

struct keyboard_entries {
    int (*keyboard_reboot)();
    int (*steal_keyboard_event)();
};

void keyboard_reboot(void);
PCKeyboardEvent *steal_keyboard_event(void);
void register_keyboard_entries(struct keyboard_entries *list);

#endif	/* KERNEL_PRIVATE */
