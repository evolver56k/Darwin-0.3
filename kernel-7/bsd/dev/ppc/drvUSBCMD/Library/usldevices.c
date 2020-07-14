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
	File:		USLDevices.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(CJK)	Craig Keithley

	Change History (most recent first):

		 <2>	 1/15/98	CJK		Change include of USL.h to USBServicesLib.h
*/

//#include <Types.h>

/* If I don't include Types, what do I need? */
#ifndef __TYPES__

#define nil ((void *) 0)
//naga typedef int Boolean;

#endif


#include "usbserviceslib.h"
#include "../USBpriv.h"
#include "../uimpriv.h"
#include "uslpriv.h"

#ifdef 0
static unsigned makeCtlIdx(uslDeviceRef device)
{
	return(device & kUSBDeviceIDMask);
}

/* ************* Call Backs ************* */

static void internalDeviceHandler(
		OSStatus status, 			/* normal or error condition */
		uslTransRef transaction,/* The transaction completing */
		uslCount length) 	/* actual length of data transferred */
{
	status = 0;
	transaction = 0;
	length = 0;
}
#endif
