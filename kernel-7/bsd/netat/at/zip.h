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
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

#ifndef __ZIP__
#define	__ZIP__

/* Definitions for ZIP, per AppleTalk Zone Information Protocol
 * documentation from `Inside AppleTalk', July 14, 1986.
 */

# include <at/nbp.h>

/* ZIP DDP socket type */

#define ZIP_SOCKET		6  	/* ZIP socket number */

/* ZIP packet types */

#define ZIP_DDP_TYPE		6  	/* ZIP packet */

#define ZIP_QUERY         	1  	/* ZIP zone query packet */
#define ZIP_REPLY           	2  	/* ZIP query reply packet */
#define ZIP_TAKEDOWN        	3  	/* ZIP takedown packet */
#define ZIP_BRINGUP        	4  	/* ZIP bringup packet */
#define ZIP_GETNETINFO		5	/* ZIP DDP get net info packet */
#define	ZIP_NETINFO_REPLY	6	/* ZIP GetNetInfo Reply */
#define ZIP_NOTIFY		7	/* Notification of zone name change */
#define ZIP_EXTENDED_REPLY	8	/* ZIP extended query reply packet */ 

#define ZIP_GETMYZONE    	7  	/* ZIP ATP get my zone packet */
#define ZIP_GETZONELIST    	8  	/* ZIP ATP get zone list packet */
#define	ZIP_GETLOCALZONES	9	/* ZIP ATP get cable list packet*/

#define ZIP_HDR_SIZE		2
#define	ZIP_DATA_SIZE		584


#define ZIP_MAX_ZONE_LENGTH	32	 		/* Max length for a Zone Name */

typedef	struct at_zip {
	u_char	command;
	u_char	flags;
	char	data[ZIP_DATA_SIZE];
} at_zip_t;

#define	 ZIP_ZIP(c)	((at_zip_t *)(&((at_ddp_t *)(c))->data[0]))

/* ioctl codes */
#define	ZIP_IOC_MYIOCTL(i)	((i>>8) == AT_MID_ZIP)
#define	ZIP_IOC_GET_CFG		((AT_MID_ZIP<<8) | 1)

typedef struct {
	at_nvestr_t	zonename;
} at_zip_cfg_t;

#endif /* __ZIP__ */
