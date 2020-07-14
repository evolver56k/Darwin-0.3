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
 * Copyright (c) 1991 NeXT Computer, Inc.
 *
 * adb.h -- ADB driver header.
 *
 * Author: ?
 */

#ifdef	KERNEL_PRIVATE

#ifndef		_NEXTDEV_ADB_H_
#define		_NEXTDEV_ADB_H_

#import <bsd/dev/ev_types.h>

typedef union {
    struct {
	unsigned char	cmd;
    } cmd;
    struct {
	unsigned char	addr:4,
			 cmd:4;
#define ADB_reset	0
#define ADB_flush	1
    } gen;
    struct {
	unsigned char	addr:4,
			 cmd:2,
#define ADB_listen	2
#define ADB_talk	3
			 reg:2;
    } reg;
} adb_cmd_t;

typedef union {
    struct {
	unsigned	data0;
	unsigned	data1;
    } longword;
    struct {
	unsigned char	data[8];
    } byte;
    struct {
	unsigned short	:1,
			exceptionalEvent:1,
			serviceReqEnb:1,
			:1,
			addr:4,
			devHandlerID:8;
    } reg3;
    struct {
	unsigned short	left_up:1,
			Y_delta:7,
			right_up:1,	/* Reserved in Apple mouse */
			X_delta:7;
    } m_reg0;
    struct {
	unsigned int	left_up:1,
			Y_delta:7,
			:1,
			X_delta:7,
			:5,
			left_down:1,
			middle_down:1,
			right_down:1,
			:8;
    } logitech_s13_m_reg0;
    struct {
	unsigned short	key1_up:1,
			key1_code:7,
			key2_up:1,
			key2_code:7;
    } k_reg0;
    struct {
	unsigned short	:1,
			key_delete:1,
			key_cap_lock:1,
			key_reset:1,
			key_control:1,
			key_shift:1,
			key_alt:1,
			key_command:1,
			key_num_lock:1,
			key_scroll_lock:1,
			:3,
			led_scroll_lock:1,
			led_cap_lock:1,
			led_num_lock:1;
    } k_reg2;
    struct {
	unsigned char	byteaddr;	/* Logitech Reg 1 byte mapping. */
	unsigned char	byteval;
	unsigned char	_unknown1_;
	unsigned char	_unknown2_;
	unsigned char	right_button;
	unsigned char	middle_button;
	unsigned char	left_button;
	unsigned char	_unknown3_;
    } logitech_s13_m_reg1;
} adb_data_t;

#define ADB_ADDR_INVALID	0
#define ADB_ADDR_LOW		1
#define ADB_ADDR_KEYBOARD	2
#define ADB_ADDR_MOUSE		3
#define ADB_ADDR_TABLET		4

#define ADB_ADDR_SOFT		8
#define ADB_ADDR_HIGH		15

extern void		adb_initialize(void);
extern void		adb_talk(int, int, adb_data_t *, int *);
extern void		adb_listen(int, int, adb_data_t *, int);
extern boolean_t	adb_poll_keyboard(adb_data_t *);
extern void		adb_force_NMI(void);
extern void		adb_watchdog(boolean_t);
extern int		adb_system_info(NXEventSystemDevice *, int);

#endif		/* _NEXTDEV_ADB_H_ */

#endif
