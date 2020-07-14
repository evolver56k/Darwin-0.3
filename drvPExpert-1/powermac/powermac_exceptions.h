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
/* Trap handling function prototypes */

extern void thandler(void);	/* trap handler */
extern void ihandler(void);	/* interrupt handler */
extern void shandler(void);	/* syscall handler */
extern void gdbhandler(void);	/* debugger handler */
extern void fpu_switch(void);	/* fp handler */

void (*exception_handlers[])(void) = {
	thandler,	/* 0x0000  INVALID EXCEPTION */
	thandler,	/* 0x0100  System reset */
	thandler,	/* 0x0200  Machine check */
	thandler,	/* 0x0300  Data access */
	thandler,	/* 0x0400  Instruction access */
	ihandler,	/* 0x0500  External interrupt */
	thandler,	/* 0x0600  Alignment */
	thandler,	/* 0x0700  Program - fp exc, ill/priv instr, trap */
	fpu_switch,	/* 0x0800  Floating point disabled */
	ihandler,	/* 0x0900  Decrementer */
	thandler,	/* 0x0A00  I/O controller interface */
	thandler,	/* 0x0B00  INVALID EXCEPTION */
	shandler,	/* 0x0C00  System call exception */
	thandler,	/* 0x0D00  Trace */
	thandler,	/* 0x0E00  FP assist */
	thandler,	/* 0x0F00  Performance monitoring */
	thandler,	/* 0x1000  Instruction PTE miss */
	thandler,	/* 0x1100  Data load PTE miss */
	thandler,	/* 0x1200  Data store PTE miss */
	thandler,	/* 0x1300  Instruction breakpoint */
	thandler,	/* 0x1400  System management */
	thandler,	/* 0x1500  INVALID EXCEPTION */
	thandler,	/* 0x1600  INVALID EXCEPTION */
	thandler,	/* 0x1700  INVALID EXCEPTION */
	thandler,	/* 0x1800  INVALID EXCEPTION */
	thandler,	/* 0x1900  INVALID EXCEPTION */
	thandler,	/* 0x1A00  INVALID EXCEPTION */
	thandler,	/* 0x1B00  INVALID EXCEPTION */
	thandler,	/* 0x1C00  INVALID EXCEPTION */
	thandler,	/* 0x1D00  INVALID EXCEPTION */
	thandler,	/* 0x1E00  INVALID EXCEPTION */
	thandler,	/* 0x1F00  INVALID EXCEPTION */
	thandler,	/* 0x2000  Run Mode/Trace */
	thandler,	/* 0x2100  INVALID EXCEPTION */
	thandler,	/* 0x2200  INVALID EXCEPTION */
	thandler,	/* 0x2300  INVALID EXCEPTION */
	thandler,	/* 0x2400  INVALID EXCEPTION */
	thandler,	/* 0x2500  INVALID EXCEPTION */
	thandler,	/* 0x2600  INVALID EXCEPTION */
	thandler,	/* 0x2700  INVALID EXCEPTION */
	thandler,	/* 0x2800  INVALID EXCEPTION */
	thandler,	/* 0x2900  INVALID EXCEPTION */
	thandler,	/* 0x2A00  INVALID EXCEPTION */
	thandler,	/* 0x2B00  INVALID EXCEPTION */
	thandler,	/* 0x2C00  INVALID EXCEPTION */
	thandler,	/* 0x2D00  INVALID EXCEPTION */
	thandler,	/* 0x2E00  INVALID EXCEPTION */
	thandler	/* 0x2F00  INVALID EXCEPTION */
};

