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

typedef struct {
	void *ccbList;		/* Ptr to list of connection control blocks */

	TimerElemPtr slowTimers; /* The probe timer list */
	TimerElemPtr fastTimers; /* The fast timer list */

	unsigned short lastCID;		/* Last connection ID assigned */
	char inTimer;		/* We're inside timer routine */
} GLOBAL;

extern GLOBAL adspGlobal;

/* Address of ptr to list of ccb's */
#define AT_ADSP_STREAMS ((CCB **)&(adspGlobal.ccbList))


