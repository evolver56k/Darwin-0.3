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
	File:		OHCIDispatchTable.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(CJK)	Craig Keithley
		(DF)	David Ferguson
		(GG)	Guillermo Gallegos
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB13>	  9/3/98	GG		Add entry points for Isoch functions.
	 <USB12>	 8/12/98	BT		Move root hub into UIM again
	 <USB11>	 8/11/98	BT		Add actual version to UIM
	 <USB10>	 7/10/98	TC		Back out previous rev.
	  <USB9>	 6/30/98	BT		Move Root hub sim into UIM
	  <USB8>	  6/2/98	GG		Added GetFramecount.
	  <USB7>	 5/28/98	CJK		change file creater to 'MPS '
	  <USB6>	 4/23/98	BT		Add reset portsuspend change
	  <USB5>	 4/15/98	BT		Add over current change reset
	  <USB4>	 4/14/98	DF		Add back OHCIProcessDoneQueue
	  <USB3>	  4/9/98	BT		Use USB.h
		 <2>	  4/8/98	GG		Added Abort and delete apis.
		 <1>	 3/19/98	BT		first checked in
*/

#include "driverservices.h"
//naga#include "USBpriv.h"
#include "OHCIUIM.h"
#include "OHCIRootHub.h"

struct UIMPluginDispatchTable ThePluginDispatchTable={
	kUIMPluginTableVersion,	/* Version */
	OHCIUIMInitialize,
	OHCIUIMFinalize,
	
	OHCIUIMControlEDCreate,
	OHCIUIMControlEDDelete,
	OHCIUIMControlTransfer,
	
	0,		/* Abort Ctl */
	0,		/* Enable ctl */
	0,		/* Disable ctl */
	OHCIUIMBulkEDCreate,		/* Create Bulk */
	OHCIUIMBulkEDDelete,		/* Delete Bulk */
	OHCIUIMBulkTransfer,		/* Do bulk */
	0,		
	0,
	0,
	OHCIUIMInterruptEDCreate,
	0,
	OHCIUIMInterruptTransfer,
	0,
	0,
	0,
	OHCIUIMIsochEDCreate,
	0,
	OHCIUIMIsochTransfer,
	0,
	0,
	0,
	OHCIUIMAbortEndpoint,
	OHCIUIMEndpointDelete,
	
	OHCIUIMClearEndPointStall,
	
	OHCIPollRootHubSim,
	OHCIResetRootHub,
	OHCIProcessDoneQueue,
	OHCIUIMGetCurrentFrameNumber, 

	
	};
