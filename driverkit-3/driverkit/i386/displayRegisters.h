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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * displayRegisters.h - i386-specific register helpers
 *
 *
 * HISTORY
 * 03 Aug 93	Scott Forstall
 *      Created. 
 */

#import <driverkit/i386/ioPorts.h>

static inline unsigned char IOReadRegister(
	IOEISAPortAddress port,
	unsigned char index)
// Description:	Read register at port and index.  Return the
//		current value of the register.
{
    outb(port, index);
    return(inb(port + 1));
}

static inline void IOWriteRegister(
	IOEISAPortAddress port,
	unsigned char index,
	unsigned char value)
// Description:	Write value to register at port and index.
{
    outb(port, index);
    outb(port + 1, value);
}

static inline void IOReadModifyWriteRegister(
	IOEISAPortAddress port,
	unsigned char index,
	unsigned char protect,
	unsigned char value)
// Description:	Read-modify-write.
//		Read register at port and index.  Change value while
//		retaining protect bits.
{
    unsigned char val = IOReadRegister(port, index);
    IOWriteRegister(port, index, (val & protect) | value);
}

