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
 * kbd_entries.c
 *      register necessary functions from the keyboard driver
 * History
 *	June 2, 1994	Created		Erik Kay at NeXT
 */

#import "kbd_entries.h"

static struct keyboard_entries function_list = {
    NULL,
    NULL
};

/*
 * Brute force reboot.
 */
 
#define K_STATUS 	0x64		// keybd status (read-only)
#define K_IBUF_FUL 	0x02		// input (to keybd) buffer full
#define K_CMD	 	0x64		// keybd ctlr command (write-only)
#define	KC_REBOOT	0xfe		// cause a reboot to occur

static void kdreboot(void)
// Description:	Sends a "magic" sequence to the keyboard controller
//		which causes it to send a signal back to the system which
//		causes the system to reboot.
{
    // Wait for room in the buffer
    while (inb(K_STATUS) & K_IBUF_FUL)
	continue;
    outb(K_CMD, KC_REBOOT);	// Send the command
    return;
}

void keyboard_reboot()
{
    if (function_list.keyboard_reboot)
	function_list.keyboard_reboot();
    kdreboot();
    return;
}

PCKeyboardEvent *StealKeyEvent()
{
    /*
     * Fill up a little code space here so that
     * the old keyboard driver has enough room to patch this function.
     */
    volatile int i;
    i = 1; i = 2; i = 3; i = 4; i = 5;
    
    return NULL;
}

PCKeyboardEvent *steal_keyboard_event()
{
    PCKeyboardEvent *event = NULL;
    if ((event = StealKeyEvent()) == NULL) {
	if (function_list.steal_keyboard_event)
	    event = (PCKeyboardEvent *)function_list.steal_keyboard_event();
    }
    return event;
}

void register_keyboard_entries(struct keyboard_entries *list)
{
    function_list = *list;
}

