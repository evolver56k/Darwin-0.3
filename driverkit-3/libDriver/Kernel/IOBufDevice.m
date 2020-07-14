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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOBufDevice.m - Buffered Device abstract superclass.
 *
 * HISTORY
 * 13-Jan-93	John Seamons (jks) at NeXT
 *	Ported to i386.
 *
 * 23-Jun-92	Mike DeMoney (mike@next.com)
 *      Major hacks... 
 *
 * 16-Jul-91    Doug Mitchell at NeXT
 *      Created. 
 *
 * FIXME:
 *	Check unit number.  Clean-up unit number usage between this
 *	class and subclasses.
 */

#import <driverkit/IOBufDevice.h>

#import	<sys/errno.h>

@interface IOBufDevice (ProxyForSubclass)
- (IOReturn)initializeUnit			: (unsigned) unitNumber;
- (IOReturn)shutdownUnit			: (unsigned) unitNumber;
@end

@implementation IOBufDevice

- init
{
	unsigned unit;
	UnitInfo *unitInfoP;

	for (unit = 0; unit < MAX_IOBUFDEVICE_UNITS; unit += 1) {
		unitInfoP = &unitInfo[unit];
		unitInfoP->callbackId = nil;
		unitInfoP->deviceTag = 0;
		unitInfoP->ownerName[0] = '\0';
	}
	return self;
}

- (IOReturn)acquireUnit				: (unsigned) unit
				for		: (OwnerName) newName
				callbackId	: (id) newId
				deviceTag	: (Tag) newTag;
{
	UnitInfo *unitInfoP = &unitInfo[unit];

	if (unitInfoP->callbackId != NULL) {
		return /* FIXME */ IO_R_BUSY;
	}
	strncpy(unitInfoP->ownerName, newName, sizeof(unitInfoP->ownerName));
	unitInfoP->callbackId = newId;
	unitInfoP->deviceTag = newTag;
	[self initializeUnit:unit];
	return IO_R_SUCCESS;
}


- (IOReturn)releaseUnit				: (unsigned) unit
{
	UnitInfo *unitInfoP = &unitInfo[unit];

	if (unitInfoP->callbackId == NULL) {
		return /* FIXME */ IO_R_NOT_OWNER;
	}
	[self shutdownUnit:unit];
	unitInfoP->callbackId = NULL;
	unitInfoP->ownerName[0] = '\0';
	return IO_R_SUCCESS;
}

- (IOReturn)ownerForUnit		: (unsigned) unit
			isNamed		: (OwnerName) currentName
{
	UnitInfo *unitInfoP = &unitInfo[unit];

	strncpy(currentName, unitInfoP->ownerName, sizeof(currentName));
	return IO_R_SUCCESS;
}

/*
 * Convert an IOReturn to text. Subclasses which add additional
 * IOReturn's should override this method and call [super ioReturnText] if
 * the desired value is not found.
 */
- (const char *)stringFromReturn	: (IOReturn) rtn
{
	switch (rtn) {
	case IO_R_FLUSHED:
		return "Buffer Flushed";
	case IO_R_NOT_OWNER:
		return "Not Owner";
	}
	return [super stringFromReturn:rtn];
}

/*
 * Convert an IOReturn to a Unix errno.
 */
- (int)errnoFromReturn 			: (IOReturn) rtn
{
	switch (rtn) {
	case IO_R_FLUSHED:
		return ESHUTDOWN;
	case IO_R_NOT_OWNER:
		return EPERM;
	}
	return [super errnoFromReturn:rtn];
}
@end

@implementation IOBufDevice (ProxyForSubclass)
- (IOReturn)initializeUnit			: (unsigned) unitNumber
{
}

- (IOReturn)shutdownUnit			: (unsigned) unitNumber
{
}
@end
