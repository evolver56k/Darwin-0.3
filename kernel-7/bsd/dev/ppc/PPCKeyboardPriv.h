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

#ifdef	DRIVER_PRIVATE

/* ADB Keyboard Status - ADB Register 2 */

#define ADBKS_LED_NUMLOCK       0x0001
#define ADBKS_LED_CAPSLOCK      0x0002
#define ADBKS_LED_SCROLLLOCK        0x0004
#define ADBKS_SCROLL_LOCK       0x0040
#define ADBKS_NUMLOCK           0x0080
/* Bits 3 to 5 are reserved */
#define ADBKS_APPLE_CMD         0x0100
#define ADBKS_OPTION            0x0200
#define ADBKS_SHIFT         0x0400
#define ADBKS_CONTROL           0x0800
#define ADBKS_CAPSLOCK          0x2000
#define ADBKS_CAPSLOCK_short    0x20
#define ADBKS_RESET         0x1000
#define ADBKS_DELETE            0x4000
/* bit 16 is reserved */

/* 
 * Special key values
 */

#define ADBK_LEFT   0x3B
#define ADBK_RIGHT  0x3C
#define ADBK_UP     0x3E
#define ADBK_DOWN   0x3D
#define ADBK_PGUP   0x74
#define ADBK_PGDN   0x79
#define ADBK_HOME   0x73
#define ADBK_END    0x77
#define ADBK_CONTROL    0x36
#define ADBK_CONTROL_R  0x7D
#define ADBK_FLOWER 0x37
#define ADBK_SHIFT  0x38
#define ADBK_SHIFT_R    0x7B
#define ADBK_CAPSLOCK   0x39
#define ADBK_OPTION 0x3A
#define ADBK_OPTION_R   0x7C
#define ADBK_NUMLOCK    0x47
#define ADBK_SPACE  0x31
#define ADBK_F      0x03
#define ADBK_O      0x1F
#define ADBK_P      0x23
#define ADBK_Q      0x0C
#define ADBK_V      0x09
#define ADBK_1      0x12
#define ADBK_2      0x13
#define ADBK_3      0x14
#define ADBK_4      0x15
#define ADBK_5      0x17
#define ADBK_6      0x16
#define ADBK_7      0x1A
#define ADBK_8      0x1C
#define ADBK_9      0x19
#define ADBK_0      0x1D
#define ADBK_POWER2 0x7E
#define ADBK_POWER  0x7f    /* actual 0x7f 0x7f */

#define ADBK_KEYVAL(key)    ((key) & 0x7f)
#define ADBK_PRESS(key)     (((key) & 0x80) == 0)
#define ADBK_KEYDOWN(key)   (key)
#define ADBK_KEYUP(key)     ((key) | 0x80)
#define ADBK_MODIFIER(key)  ((((key) & 0x7f) == ADBK_SHIFT) || \
                 (((key) & 0x7f) == ADBK_SHIFT_R) || \
                 (((key) & 0x7f) == ADBK_CONTROL) || \
                 (((key) & 0x7f) == ADBK_CONTROL_R) || \
                 (((key) & 0x7f) == ADBK_FLOWER) || \
                 (((key) & 0x7f) == ADBK_OPTION) || \
                 (((key) & 0x7f) == ADBK_OPTION_R) || \
                 (((key) & 0x7f) == ADBK_NUMLOCK) || \
                 (((key) & 0x7f) == ADBK_CAPSLOCK))


#endif	/* DRIVER_PRIVATE */
