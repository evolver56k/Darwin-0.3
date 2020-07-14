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

#ifndef __AT_CONFIG__
#define	__AT_CONFIG__
/*
 *
 ******************************************************************************
 *                                                                            *
 *        Copyright (c) 1988-1993 Apple Computer, Inc.                             *
 *                                                                            *
 *        The information contained herein is subject to change without       *
 *        notice and  should not be  construed as a commitment by Apple       *
 *        Computer, Inc. Apple Computer, Inc. assumes no responsibility       *
 *        for any errors that may appear.                                     *
 *                                                                            *
 *        Confidential and Proprietary to Apple Computer, Inc.                *
 *                                                                            *
 ******************************************************************************
 *
 *
 */

#include <sys/types.h>


/* old portability characteristic definitions have been removed (we're only
   running on AIX and haven't been supporting them up till now anyway
   -js
*/



#define AT_VERSION_MAJOR	1
#define AT_VERSION_MINOR	1	
	/* date & str no longer used in lap_init, but left here for the 
	   hell of it. */
#define AT_VERSION_DATE		970722
#define AT_VERSION_STR		"D0"


#define	UAS_ASSIGN(x,s)	*(unsigned short *) &(x[0]) = (unsigned short) (s)

#define	UAS_UAS(x,y)	*(unsigned short *) &(x[0]) = *(unsigned short *) &(y[0])

#define	UAL_ASSIGN(x,l)	*(unsigned long *) &(x[0]) = (unsigned long) (l)

#define	UAL_UAL(x,y)	*(unsigned long *) &(x[0]) = *(unsigned long *) &(y[0])

#define	UAS_VALUE(x)	(*(unsigned short *) &(x[0]))

#define	UAL_VALUE(x)	(*(unsigned long *) &(x[0]))


/* Macros to manipulate at_net variables */
#define	NET_ASSIGN(x,s)	UAS_ASSIGN(x, s)
#define	NET_NET(x, y)	UAS_UAS(x, y)
#define	NET_VALUE(x)	UAS_VALUE(x)

#define NET_EQUAL(a, b)	(NET_VALUE(a) == NET_VALUE(b))
#define NET_NOTEQ(a, b)	(NET_VALUE(a) != NET_VALUE(b))

#define NET_EQUAL0(a)	(NET_VALUE(a) == 0)
#define NET_NOTEQ0(a)	(NET_VALUE(a) != 0)

#define IF_NAME_LEN			5		/* char array size of if_name */

/*#define	AT_INF_RETRY	-1	*/ 	/* Retry forever */

	/* the kern_err struct was added to allow the kernel to return
	   information to the user upon startup while supporting NLS. 
	   The kernel error number (KE_xxx) is tied to a specific NLS
	   message via the error handling routine in router.c. The specific
	   error determines which of the elements of the kern_err struct
	   are used and therefore filled in by the kernel. For clarity 
	   each KE_xxx error corresponds to a M_xxx define except for the
	   M_ & KE_ prefixes. The message catalog is sbin/appletalk/atsbin.msg
	 */


typedef struct kern_err {
	int		errno;				/* kernel error # (KE_xxx) */
	int		port1;
	int		port2;
	char	name1[IF_NAME_LEN];
	char	name2[IF_NAME_LEN];
	u_short net;				
	u_char	node;
	u_short netr1b, netr1e;		/* net range 1 begin & end */
	u_short netr2b, netr2e;		/* net range 2 begin & end */
	u_char	rtmp_id;
} kern_err_t;

#define KE_CONF_RANGE 			1
#define KE_CONF_SEED_RNG 		2
#define KE_CONF_SEED1			3
#define KE_CONF_SEED_NODE		4
#define KE_NO_ZONES_FOUND		5
#define KE_NO_SEED 				6
#define KE_INVAL_RANGE			7
#define KE_SEED_STARTUP			8	
#define KE_BAD_VER				9
#define KE_RTMP_OVERFLOW		10
#define KE_ZIP_OVERFLOW			11


#endif /* __AT-CONFIG__ */
