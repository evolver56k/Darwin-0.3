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
 *	Copyright (c) 1988, 1989 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* @(#)atlog.h: 2.0, 1.3; 7/14/89; Copyright 1988-89, Apple Computer, Inc. */

/* These pointers are non-NULL if logging or tracing are activated. */
#ifndef LOG_DRIVER
extern char *log_errp;	
extern char *log_trcp;
#endif  /* LOG_DRIVER */

/* ATTRACE() macro.  Use this routine for calling 
 * streams tracing and logging.  If `log' is TRUE, then
 * this event will also be logged if logging is on.
 */
#if !defined(lint) && defined(AT_DEBUG)
#define	ATTRACE(mid,sid,level,log,fmt,arg1,arg2,arg3)		\
	if (log_trcp || (log && log_errp)) {			\
		strlog(mid,sid,level,SL_TRACE |			\
			(log ? SL_ERROR : 0)  |			\
			(level <= AT_LV_FATAL ? SL_FATAL : 0),	\
			fmt,arg1,arg2,arg3);			\
	}
#else
#define	ATTRACE(mid,sid,level,log,fmt,arg1,arg2,arg3)		\
/*	printf(fmt, arg1, arg2, arg3); */

#endif


/* Levels for AppleTalk tracing */

#define	AT_LV_FATAL	1
#define	AT_LV_ERROR	3
#define	AT_LV_WARNING	5
#define	AT_LV_INFO	7
#define	AT_LV_VERBOSE	9


/* Sub-ids for AppleTalk tracing, add more if you can't figure
 * out where your event belongs.
 */

#define	AT_SID_INPUT	1	/* Network incoming packets */
#define	AT_SID_OUTPUT	2	/* Network outgoing packets */
#define	AT_SID_TIMERS	3	/* Protocol timers */
#define	AT_SID_FLOWCTRL	4	/* Protocol flow control */
#define	AT_SID_USERREQ	5	/* User requests */
#define	AT_SID_RESOURCE	6	/* Resource limitations */



/* Module ID's for AppleTalk subsystems */

#define	AT_MID(n)	(200+n)

#define	AT_MID_MISC	AT_MID(0)
#define	AT_MID_LLAP	AT_MID(1)
#define	AT_MID_ELAP	AT_MID(2)
#define	AT_MID_DDP	AT_MID(3)
#define	AT_MID_RTMP	AT_MID(4)
#define	AT_MID_NBP	AT_MID(5)
#define	AT_MID_EP	AT_MID(6)
#define	AT_MID_ATP	AT_MID(7)
#define	AT_MID_ZIP	AT_MID(8)
#define	AT_MID_PAP	AT_MID(9)
#define	AT_MID_ASP	AT_MID(10)
#define	AT_MID_AFP	AT_MID(11)
#define	AT_MID_ADSP	AT_MID(12)
#define	AT_MID_NBPD	AT_MID(13)
#define	AT_MID_LAP	AT_MID(14)

#define	AT_MID_LAST	AT_MID_LAP

#ifdef	AT_MID_STRINGS
static char *at_mid_strings[] = {
	"misc",
	"LLAP",
	"ELAP",
	"DDP",
	"RTMP",
	"NBP",
	"EP",
	"ATP",
	"ZIP",
	"PAP",
	"ASP",
	"AFP",
	"ADSP",
	"NBPD",
	"LAP"
};
#endif


#ifndef SL_FATAL
/* Don't define these if they're already defined */

/* Flags for log messages */

#define SL_FATAL	01	/* indicates fatal error */
#define SL_NOTIFY	02	/* logger must notify administrator */
#define SL_ERROR	04	/* include on the error log */
#define SL_TRACE	010	/* include on the trace log */

#endif

/* Driver and ioctl definitions */


/* DDP streams module ioctls */

#define DDP_IOC_MYIOCTL(i)      ((i>>8) == AT_MID_DDP)
#define DDP_IOC_GET_CFG        	((AT_MID_DDP<<8) | 1)
#define DDP_IOC_BIND_SOCK	((AT_MID_DDP<<8) | 2)
#define	DDP_IOC_GET_STATS	((AT_MID_DDP<<8) | 3)
#define DDP_IOC_LSTATUS_TABLE	((AT_MID_DDP<<8) | 4)
#define DDP_IOC_ULSTATUS_TABLE	((AT_MID_DDP<<8) | 5)
#define DDP_IOC_RSTATUS_TABLE	((AT_MID_DDP<<8) | 6)
#define DDP_IOC_SET_WROFF	((AT_MID_DDP<<8) | 7 )
#define DDP_IOC_SET_OPTS	((AT_MID_DDP<<8) | 8 )
#define DDP_IOC_GET_OPTS	((AT_MID_DDP<<8) | 9 )
#define DDP_IOC_GET_SOCK	((AT_MID_DDP<<8) | 10)
#define DDP_IOC_GET_PEER	((AT_MID_DDP<<8) | 11)
#define DDP_IOC_SET_PEER	((AT_MID_DDP<<8) | 12)
#define DDP_IOC_SET_PROTO	((AT_MID_DDP<<8) | 13)



