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
 * Intel386 Family:	User registers for U**X.
 *
 * HISTORY
 *
 * 20 April 1992 ? at NeXT
 *	Created.
 */
 
#ifdef	KERNEL_PRIVATE

#ifndef _BSD_I386_REG_H_
#define _BSD_I386_REG_H_

#import <machdep/i386/thread.h>

extern thread_saved_state_t	*___xxx_state;

#define ECX	(int)((int *)&___xxx_state->regs.ecx - (int *)___xxx_state)
#define EDX	(int)((int *)&___xxx_state->regs.edx - (int *)___xxx_state)
#define ESP	(int)((int *)&___xxx_state->frame.esp - (int *)___xxx_state)
#define	SP	ESP
#define EFL	\
	(int)((int *)&___xxx_state->frame.eflags - (int *)___xxx_state)
#define PS	EFL
#define EIP	(int)((int *)&___xxx_state->frame.eip - (int *)___xxx_state)
#define PC	EIP

#endif	/* _BSD_I386_REG_H_ */

#endif	/* KERNEL_PRIVATE */
