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
#define GC_CONTROL_SHRINK_CACHES 0x1
#define GC_CONTROL_COLLECT_ZONES 0x2
#define GC_CONTROL_RECLAIM_SPACE 0x4

#define SYS_gc_control	175

#import <libc.h>

void
do_gc()
{
//	gc_control(GC_CONTROL_SHRINK_CACHES);

#if defined(ppc)
	__asm__ volatile (" addi r3, 0, 1");
	__asm__ volatile (" addi r0, 0, 175");
	__asm__ volatile (" sc");
	__asm__ volatile (" nop");
#elif defined(i386)
	__asm__ volatile (" pushl    $0x01");
	__asm__ volatile (" movl    $175, %eax");
	__asm__ volatile (" lcall   $0x2b, $0");
	__asm__ volatile (" nop");
#else
#error architecture __ARCHITECTURE__ not supported.
#endif
}

void main(void) {
	daemon(0,0);
	while (1) {
		do_gc();
		sleep(30);
	}
}
