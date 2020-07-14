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

/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *	Serial port ioctl definitions.
 */

#import	<sys/ioctl.h>


/*
 * Modem signals: for use with TIOCMGET, TIOCMBIS, TIOCMBIC, and TIOCMSET
 */
#define	DML_DSR		0000400		/* data set ready, not a real DM bit */
#define	DML_RNG		0000200		/* ring */
#define	DML_CAR		0000100		/* carrier detect */
#define	DML_CTS		0000040		/* clear to send */
#define	DML_SR		0000020		/* secondary receive */
#define	DML_ST		0000010		/* secondary transmit */
#define	DML_RTS		0000004		/* request to send */
#define	DML_DTR		0000002		/* data terminal ready */
#define	DML_LE		0000001		/* line enable */

#ifdef KERNEL_PRIVATE

/*
 * Macros for breaking subfields out of device minor number
 */
#define	ZSUNIT(m)	((m) & 0x1f)	/* unit number */
#define	ZSFLOWCTL(m)	((m) & 0x20)	/* hw flow control */
#define	ZSHARDCAR(m)	((m) & 0x40)	/* hard carrier */
#define	ZSOUTWARD(m)	((m) & 0x80)	/* outward line */

#endif	/* KERNEL_PRIVATE */

/*
 * ioctl to change receiver silo delay
 */
#define	ZIOCTGET	_IOR('z', 0, int)	/* get silo delay */
#define	ZIOCTSET	_IOW('z', 1, int)	/* set silo delay */

/*
 * ioctl to get serial line features
 * (OR of supported features is returned)
 */
#define	ZIOCFGET	_IOR('z', 2, int)	/* get features */

#define	ZSFEATURE_HWFLOWCTRL	0x0001		/* CTS/RTS flow control */


