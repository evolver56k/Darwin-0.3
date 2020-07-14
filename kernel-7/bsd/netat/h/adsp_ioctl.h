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
 * Defines that correspond to atlog.h in the N & C Appletalk
 * sources.
 */

#define AT_MID(n)	(200+n)
#define AT_MID_ADSP	AT_MID(12)

/* Streams ioctl definitions */

#define ADSP_IOCTL(i)     ((i>>8) == AT_MID_ADSP)
#define ADSPATTNREAD	((AT_MID_ADSP<<8) | 254) /* read attention data */
#define	ADSPOPEN 	((AT_MID_ADSP<<8) | 253) /* open a connection */
#define	ADSPCLOSE 	((AT_MID_ADSP<<8) | 252) /* close a connection */
#define	ADSPCLINIT 	((AT_MID_ADSP<<8) | 251) /* create a conn listener */
#define	ADSPCLREMOVE 	((AT_MID_ADSP<<8) | 250) /* remove a conn listener */
#define	ADSPCLLISTEN 	((AT_MID_ADSP<<8) | 249) /* post a listener request */
#define	ADSPCLDENY 	((AT_MID_ADSP<<8) | 248) /* deny an open connection request */
#define	ADSPSTATUS 	((AT_MID_ADSP<<8) | 247) /* get status of conn end */
#define	ADSPREAD 	((AT_MID_ADSP<<8) | 246) /* read data from conn */
#define	ADSPWRITE 	((AT_MID_ADSP<<8) | 245) /* write data on the conn */
#define	ADSPATTENTION 	((AT_MID_ADSP<<8) | 244) /* send attention message */
#define	ADSPOPTIONS 	((AT_MID_ADSP<<8) | 243) /* set conn end options */
#define	ADSPRESET 	((AT_MID_ADSP<<8) | 242) /* forward reset connection */
#define	ADSPNEWCID 	((AT_MID_ADSP<<8) | 241) /* generate a cid conn end */
#define ADSPBINDREQ	((AT_MID_ADSP<<8) | 240)
#define ADSPGETSOCK	((AT_MID_ADSP<<8) | 239)
#define ADSPGETPEER	((AT_MID_ADSP<<8) | 238)


