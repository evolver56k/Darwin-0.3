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
 *	File:	spl.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Define inline macros for spl routines.
 *	
 * HISTORY
 * 21-May-91  Gregg Kellogg (gk) at NeXT
 *	Moved public portion to exported directory.
 *
 * 14-May-90  Gregg Kellogg (gk) at NeXT
 *	Changed SPLCLOCK from 6 to 3, as much scheduling code expects
 *	splclock() == splsched().  Added splusclock().
 *
 * 19-Jun-89  Mike DeMoney (mike) at NeXT
 *	Modified to allow spl assertions in spl_measured.h
 */

#ifndef	_KERNSERV_I386_SPL_H_
#define	_KERNSERV_I386_SPL_H_

extern int spl7(void);
extern int splhigh(void);

extern int spl6(void);
extern int spldma(void);
extern int splusclock(void);
extern int splsched(void);
extern int splclock(void);

extern int spl5(void);

extern int spl4(void);

extern int spl3(void);
extern int splbio(void);
extern int splimp(void);
extern int splvm(void);
extern int splnet(void);
extern int spldevice(void);

extern int spl2(void);

extern int spl1(void);
extern int spltty(void);

extern int spl0(void);
		    
extern int splx(int	ipl);
extern int spln(int	ipl);

extern int ipltospl(int	ipl);

extern int curipl(void);

#define splsoftclock()	spl0()
#define splstatclock()	splhigh()

#define IPLHIGH         7
#define IPLDMA          6
#define IPLUSCLOCK      6
#define IPLSCHED        6
#define IPLCLOCK        6
#define IPLBIO          3
#define IPLIMP          3
#define IPLVM           3
#define IPLNET          3
#define IPLDEVICE	3
#define IPLTTY          1

#endif	/* _KERNSERV_I386_SPL_H_ */
