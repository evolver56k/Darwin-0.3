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

// Ports used to control the PS/2 keyboard/mouse and read data from it
#define K_RDWR 		0x60		// keyboard data & cmds (read/write)
#define K_STATUS 	0x64		// keybd status (read-only)
#define K_CMD	 	0x64		// keybd ctlr command (write-only)

//
// Bit definitions for K_STATUS port.
//
#define K_OBUF_FUL 	0x01		// output (from keybd) buffer full
#define K_IBUF_FUL 	0x02		// input (to keybd) buffer full
#define K_SYSFLAG	0x04		// "System Flag"
#define K_CMD_DATA	0x08		// 1 = input buf has cmd, 0 = data
#define K_KBD_INHBT	0x10		// 0 if keyboard inhibited
#define M_OBUF_FUL	0x20		// mouse data available

// 
// Keyboard controller commands (sent to K_CMD port).
//
#define KC_CMD_READ	0x20		// read controller command byte
#define KC_CMD_WRITE	0x60		// write controller command byte
#define KC_CMD_TEST	0xab		// test interface
#define KC_CMD_DUMP	0xac		// diagnostic dump
#define KC_CMD_DISBLE	0xad		// disable keyboard
#define KC_CMD_ENBLE	0xae		// enable keyboard
#define KC_CMD_RDKBD	0xc4		// read keyboard ID
#define KC_CMD_MOUSE	0xd4		// send next data to mouse
#define KC_CMD_ECHO	0xee		// used for diagnostic testing
#define	KC_REBOOT	0xfe		// cause a reboot to occur
// 
// Keyboard/mouse commands (send to K_RDWR).
//
#define K_CMD_LEDS	0xed		// set status LEDs (caps lock, etc.)
#define M_CMD_SETRES	0xe8		// set mouse resolution to 2**N/mm
#define M_CMD_SAMPLING	0xf3		// set mouse sampling rate
#define M_CMD_POLL	0xf4		// enable mouse polling

// 
// Bit definitions for controller command byte (sent following 
// K_CMD_WRITE command).
//
#define K_CB_ENBLIRQ	0x01		// enable data-ready intrpt
#define M_CB_ENBLIRQ	0x02		// enable data-ready intrpt
#define K_CB_SETSYSF	0x04		// Set System Flag
#define K_CB_INHBOVR	0x08		// Inhibit Override
#define K_CB_DISBLE	0x10		// disable keyboard
#define M_CB_DISBLE	0x20		// disable mouse
#define	K_CB_TRANSLATE	0x40		// keyboard translate mode

// 
// Bit definitions for "Indicator Status Byte" (sent after a 
// K_CMD_LEDS command).  If the bit is on, the LED is on.  Undefined 
// bit positions must be 0.
//
#define K_LED_SCRLLK	0x1		// scroll lock
#define K_LED_NUMLK	0x2		// num lock
#define K_LED_CAPSLK	0x4		// caps lock

//
// Special Codes: These codes do not represent actual keys. theyare generated
// under special circumstances to indicate some sort of status.
//
#define K_UP		0x80		// OR'd in if key below is released
#define K_EXTEND	0xe0		// marker for "extended" sequence
#define	K_PAUSE		0xe1		// marker for pause key sequence
#define K_ACKSC		0xfa		// ack for keyboard command
#define K_RESEND	0xfe		// request to resend keybd cmd

#define K_DATA_DELAY	7		// usec to delay before data is valid

#endif	/* DRIVER_PRIVATE */
