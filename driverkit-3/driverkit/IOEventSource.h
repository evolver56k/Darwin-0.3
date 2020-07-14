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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOEventSource.h - Common Event Source object class.
 *
 * HISTORY
 * 22 May 1992    Mike Paquette at NeXT
 *      Created. 
 * 4  Aug 1993	  Erik Kay at NeXT
 *	API cleanup
 * 5  Aug 1993	  Erik Kay at NeXT
 *	added ivar space for future expansion
 */

#import <driverkit/IODevice.h>
#import <driverkit/eventProtocols.h>
#import <machkit/NXLock.h>

@interface IOEventSource : IODevice <IOEventSourceExported>
{
@private
	id		_owner;
	id		_desiredOwner;
	NXLock *	_ownerLock;		// NXLock; protects _owner and
						//   desiredOwner
	int		_reserved[4];		// reserved for future expansion
}

+ registerEventSource:source; // register as an event source

- init;			// Called by subclass's init to set up ownership glue
- free;
/*
 * Methods used only by subclass.
 */
- owner;
- (NXLock *)ownerLock;

@end

