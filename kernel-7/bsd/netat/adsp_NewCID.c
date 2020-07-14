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


#include	<adsp_local.h>


/*
 * dspNewCID
 * 
 * INPUTS:
 * 	--> ccbRefNum		refnum of connection end
 *
 * OUTPUTS:
 *	<-- newCID		new connection identifier
 *
 * ERRORS:
 *	errRefNum		bad connection refnum
 *	errState		connection is not closed
 */
int adspNewCID(sp, pb)		/* (DSPPBPtr pb) */
    CCBPtr		sp;
    struct adspcmd *pb;
{
    OSErr err;
	
    if (sp == 0) {
	pb->ioResult = errRefNum;
	return EINVAL;
    }

    if (sp->state != sClosed) {	/* Can only assign to a closed connection */
	pb->ioResult = errState;
	return EINVAL;
    }

    /*
     * Assign a unique connection ID to this ccb
     */
    sp->locCID = pb->u.newCIDParams.newcid = NextCID();

    pb->ioResult = 0;
    adspioc_ack(0, pb->ioc, pb->gref);
    return 0;
}
