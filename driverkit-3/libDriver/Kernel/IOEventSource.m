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
 * IOEventSource.m - Common Event Source object class.  This is an abstract
 *		class which implements a device ownership protocol, and 
 *		provides a core of common services for working Event Source 
 *		objects to take advantage of.
 *
 * HISTORY
 * 22 May 1992    Mike Paquette at NeXT
 *      Created. 
 * 4  Aug 1993	  Erik Kay at NeXT
 *	API cleanup
 */

#define KERNEL_PRIVATE 1

#import <driverkit/IOEventSource.h> 
#import <driverkit/EventInput.h>
#import <driverkit/eventProtocols.h> 
#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <bsd/dev/ev_types.h>

#define EVSRC_PRINT	0
#if	EVSRC_PRINT
#undef	xpr_evsrc
#define xpr_evsrc(x,a,b,c,d,e) printf(x,a,b,c,d,e)
#endif	EVSRC_PRINT

#ifndef	xpr_evsrc
#define xpr_evsrc(x,a,b,c,d,e)
#endif

@implementation IOEventSource: IODevice

+ registerEventSource:source
{
    return [[EventDriver instance] registerEventSource:source];
}

/*
 * Common EventSrc initialization
 */
- init
{
	[super init];

	_owner = nil;
	_desiredOwner = nil;
	if ( _ownerLock != nil )
		[_ownerLock free];
	_ownerLock = [NXLock new];
	return self;
}

- free
{
	if ( _ownerLock != nil )
		[_ownerLock free];
	return [super free];
}

/*
 * Methods used only by subclass.
 */
- owner
{
	return _owner;
}

- (NXLock *)ownerLock
{
	return _ownerLock;
}

/*
 * Exported methods (EventSrcExported protocol).
 */

/*
 * Register for event dispatch via EventTarget protocol. Returns IO_R_SUCCESS
 * if successful, else IO_R_BUSY. The relinquishOwnership: method
 * may be called on another client during the execution of this method.
 */
- (IOReturn)becomeOwner	: client
{
	IOReturn rtn;
	
	[_ownerLock lock];
	if(_owner != nil) {
		xpr_evsrc("%s becomeOwner: NEGOTIATING\n", 
			[self name], 2,3,4,5);
		if([_owner respondsTo:@selector(relinquishOwnership:)]) {
			rtn = [_owner relinquishOwnership:self];
		}
		else {
			IOLog("%s: owner does not respond to "
				"relinquishOwnership:\n", [self name]);
			rtn = IO_R_BUSY;
		}
		if(rtn == IO_R_SUCCESS) {
		    	xpr_evsrc("becomeOwner: Transfer ownership\n",
				1,2,3,4,5);
			_owner = client;
		}
		else {
			xpr_evsrc("becomeOwner: NEGOTIATION FAILED\n",
				1,2,3,4,5);
		}
	}
	else {
		_owner = client;
		rtn = IO_R_SUCCESS;
		xpr_evsrc("%s become owner: SIMPLE CASE\n", 
			[self name], 2,3,4,5);
	}
	[_ownerLock unlock];
	return rtn;
}

/*
 * Relinquish ownership. Returns IO_R_BUSY if caller is not current owner.
 */
- (IOReturn)relinquishOwnership	: client
{
	IOReturn rtn;
	
	[_ownerLock lock];
	if(_owner == client) {
		rtn = IO_R_SUCCESS;
		_owner = nil;
	}
	else {
		rtn = IO_R_BUSY;
	}
	[_ownerLock unlock];
	if((rtn == IO_R_SUCCESS) && 
	    _desiredOwner &&
	    (_desiredOwner != client)) {
		/*
		 * Notify this potential client that it can take over.
		 * We'll most likely be called back during this method,
		 * which is why we released _ownerLock.
		 */
		if([_desiredOwner respondsTo:@selector(canBecomeOwner:)]) {
			xpr_evsrc("%s: notifying %s via canBecomeOwner\n",
				[self name], [_desiredOwner name],
				3,4,5);
		     	[_desiredOwner canBecomeOwner:self];
		}
		else {
		    	IOLog("%s: desiredOwner does not respond to "
				"canBecomeOwner:\n", [self name]);
		}
	}
	xpr_evsrc("%s relinquishOwnership: returning %s\n",
		[self name], [self stringFromReturn:rtn], 3,4,5);
	return rtn;
}

/*
 * Register for intent to own upon another owner's relinquishOwnership:.
 */
- (IOReturn)desireOwnership	: client
{
	IOReturn rtn;
	
	[_ownerLock lock];
	if(_desiredOwner && (_desiredOwner != client)) {
		rtn = IO_R_BUSY;
	}
	else {
		_desiredOwner = client;
		rtn = IO_R_SUCCESS;
	}
	[_ownerLock unlock];
	xpr_evsrc("%s desireOwnership: returning %s\n",
		[self name], [self stringFromReturn:rtn], 3,4,5);
	return rtn;
		
}

@end

