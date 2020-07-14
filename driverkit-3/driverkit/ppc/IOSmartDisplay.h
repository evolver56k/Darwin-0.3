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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 * HISTORY
 *
 * Simon Douglas  22 Oct 97
 * - first checked in.
 */


#ifndef __IOSMARTDISPLAY_H__
#define __IOSMARTDISPLAY_H__

#import <objc/Object.h>
#import <driverkit/ppc/IOMacOSTypes.h>
#import <driverkit/ppc/IOFramebuffer.h>

@protocol IOSmartDisplayExported

+ findForConnection:framebuffer refCon:(UInt32)refCon;
- attach:framebuffer refCon:(UInt32)refCon;
- detach;
- (BOOL) attached;

- (IOReturn)
    getDisplayInfoForMode:(IOFBTimingInformation *)mode flags:(UInt32 *)flags;

- (IOReturn)
    getGammaTableByIndex:
	(UInt32 *)channelCount dataCount:(UInt32 *)dataCount
    	dataWidth:(UInt32 *)dataWidth data:(void **)data;		

@end

#endif	/* __IOSMARTDISPLAY_H__ */
