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

/* if_cnt.h
 *
 * defines for if_stat struct. 
 * note: set IF_TYPE_CNT to number of types supported and make sure 
 * 	that defines for those type  are LESS than this value
 */
#define IF_TYPENO_ET	0	/* ethernet */
#define IF_TYPENO_ATM	1	/* ATM */
#define IF_TYPENO_TR	2	/* token ring */
#define IF_TYPENO_FD	3	/* FDDI */
#define IF_TYPENO_NT	4	/* NULLTalk */
#define IF_TYPENO_LT	5	/* Localtalk */	/* *** not used *** */
#define IF_TYPENO_CNT	5	/* number of different types we support */
	
/* these defines must correspond to above defines and are used in
 * the global array if_types in at_elap.c 
 */
#define IF_TYPE_1 	"en"	/* ethernet */
#define IF_TYPE_2 	"at"	/* ATM */
#define IF_TYPE_3 	"tr"	/* token ring */
#define IF_TYPE_4	"fi"	/* FDDI */
#define IF_TYPE_5 	"nt"	/* NULLTalk */
#define IF_TYPE_6 	"lt"	/* Localtalk */

/* maximum number of I/F's allowed of each type */
/* *** "17" corresponds to Shiner *** */
#define IF_TYPE_ET_MAX	17
#define IF_TYPE_AT_MAX	17
#define IF_TYPE_TR_MAX	17
#define IF_TYPE_FD_MAX	17
#define IF_TYPE_NT_MAX	17
#define IF_TYPE_LT_MAX	0
#define IF_ANY_MAX	17	/* max count of ANY one type */
#define IF_TOTAL_MAX	17	/* max count of any combination of I/F's */

#define VERSION_LENGTH		80	/* length of version string */


