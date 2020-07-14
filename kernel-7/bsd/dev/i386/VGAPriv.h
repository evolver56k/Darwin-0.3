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
 * VGAPriv.h - Definitions useful to implementors of VGA code
 *
 *
 * HISTORY
 * 15 Sep 92	Joe Pasqua
 *      Created. 
 */

#ifdef	DRIVER_PRIVATE

#include <mach/boolean.h>
#include <sys/types.h>
#include <sys/time.h>

//
// CMOS RAM definitions
//

// I/O ports
#define CMOS_ADDR	0x70		// port for CMOS ram address
#define CMOS_DATA	0x71		// port for CMOS ram data

// Addresses, related masks, and potential results
#define CMOS_EB		0x14		// read Equipment Byte
#define CM_SCRMSK	0x30		// mask for EB query to get screen
#define CM_EGA_VGA	0x00		// "not CGA or MONO"
#define CM_CGA_40	0x10
#define CM_CGA_80	0x20
#define CM_MONO_80	0x30

//
// Where memory for various graphics adapters starts.
//
#define EGA_START	0x0b8000
#define CGA_START	0x0b8000
#define MONO_START	0x0b0000

//
// Common I/O ports.
//
#define K_TMR2		0x42
#define K_TMRCTL	0x43		/* timer control (write-only) */
#define K_PORTB		0x61		/* r/w. speaker & status lines */

//
// I/O ports for various graphics adapters.
//
#define EGA_IDX_REG	0x3d4
#define EGA_IO_REG	0x3d5
#define CGA_IDX_REG	0x3d4
#define CGA_IO_REG	0x3d5
#define MONO_IDX_REG	0x3b4
#define MONO_IO_REG	0x3b5

//
// Commands sent to graphics adapter.
//
#define C_LOW 		0x0f		/* return low byte of cursor addr */
#define C_HIGH 		0x0e		/* high byte */


/* 
 * Bit definitions for "Miscellaneous port B" (K_PORTB).
 */
/* read/write */
#define K_ENABLETMR2	0x01		/* enable output from timer 2 */
#define K_SPKRDATA	0x02		/* direct input to speaker */

// 
// Bit definitions for timer control port (K_TMRCTL).
//
/* select timer 0, 1, or 2. Do not mess with 0 or 1. */
#define K_SELTMRMASK	0xc0
#define K_SELTMR0	0x00
#define K_SELTMR1	0x40
#define K_SELTMR2	0x80

// read/load control
#define K_RDLDTMRMASK	0x30
#define K_HOLDTMR	0x00		/* freeze timer until read */
#define K_RDLDTLSB	0x10		/* read/load LSB */
#define K_RDLDTMSB	0x20		/* read/load MSB */
#define K_RDLDTWORD	0x30		/* read/load LSB then MSB */

// mode control
#define K_TMDCTLMASK	0x0e
#define K_TCOUNTINTR	0x00		/* "Term Count Intr" */
#define K_TONESHOT	0x02		/* "Progr One-Shot" */
#define K_TRATEGEN	0x04		/* "Rate Gen (/n)" */
#define K_TSQRWAVE	0x06		/* "Sqr Wave Gen" */
#define K_TSOFTSTRB	0x08		/* "Softw Trig Strob" */
#define K_THARDSTRB	0x0a		/* "Hardw Trig Strob" */

// count mode
#define K_TCNTMDMASK	0x01
#define K_TBINARY	0x00		/* 16-bit binary counter */
#define K_TBCD		0x01		/* 4-decade BCD counter */



/*
 Definitions relating to the display of characters on an EGA-like display


 For an EGA-like display, each character takes two bytes, one for the 
 actual character, followed by one for its attributes. Note that we
 decrease the amount of bytes per page by one line's worth because
 we reserve the top line for a title.
*/

#define ONE_SPACE	2		/* bytes in 1 char, EGA-like display */
#define ONE_LINE 	160		/* number of bytes in line */
#define	REAL_PAGE	4000		// Actual number of bytes per page
#define ONE_PAGE 	(REAL_PAGE - ONE_LINE)	// Bytes per page
#define BOTTOM_LINE 	(ONE_PAGE-ONE_LINE)	// 1st byte on last line

#define BEG_OF_LINE(pos)	((pos) - (pos)%ONE_LINE)
#define CURRENT_COLUMN(pos)	(((pos) % ONE_LINE) / ONE_SPACE)


// Some useful ASCII characters
#define K_SPACE		0x20		/* space character	*/
#define K_HT		0x09
#define K_LF		0x0a		/* line feed		*/
#define K_CR		0x0d		/* carriage return	*/
#define K_BS		0x08		/* back space		*/
#define K_BEL		0x07		/* bell character	*/

#define vga_regi_out_ok(reg,indx,data)				\
	{							\
	OUTB(WRIT_##reg##_ADDR, indx)				\
	OUTB(WRIT_##reg##_DATA, data)				\
	}

#define vga_reg_in_ok(reg,indx,data)				\
	{							\
	OUTB(WRIT_##reg##_ADDR, indx)				\
	INDB(READ_##reg##_DATA, data)				\
	}

#endif	/* DRIVER_PRIVATE */


