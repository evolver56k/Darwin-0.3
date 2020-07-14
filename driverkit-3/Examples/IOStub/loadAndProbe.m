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
 * Kludge "load and probe" function. Each executable needs one of these;
 * it does a probe: on each class we want to "load" into the running image.
 *
 * For now this is just a hard-coded set of callouts...
 */
 
#import "IOStub.h"
#import <driverkit/debugging.h>
#import <driverkit/generalFuncs.h>

int loadAndProbe(id serverId)
{
	IOInitGeneralFuncs();
#ifdef	DDM_DEBUG
	IOInitDDM(500, "IOStubXpr");
#endif	DDM_DEBUG
	[IOStub IOProbe:serverId];
	return(0);
}
