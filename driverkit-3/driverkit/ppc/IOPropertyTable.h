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

#import <objc/Object.h>
#import <driverkit/driverTypes.h>
#import <driverkit/ppc/IOMacOSTypes.h>

#ifdef	KERNEL

/* NameRegistry error codes */
enum {
    nrLockedErr                    = -2536,
    nrNotEnoughMemoryErr        = -2537,
    nrInvalidNodeErr            = -2538,
    nrNotFoundErr                = -2539,
    nrNotCreatedErr                = -2540,
    nrNameErr                     = -2541,
    nrNotSlotDeviceErr            = -2542,
    nrDataTruncatedErr            = -2543,
    nrPowerErr                    = -2544,
    nrPowerSwitchAbortErr        = -2545,
    nrTypeMismatchErr            = -2546,
    nrNotModifiedErr            = -2547,
    nrOverrunErr                = -2548,
    nrResultCodeBase             = -2549,
    nrPathNotFound                 = -2550,    /* a path component lookup failed */
    nrPathBufferTooSmall         = -2551,    /* buffer for path is too small */    
    nrInvalidEntryIterationOp     = -2552,    /* invalid entry iteration operation */
    nrPropertyAlreadyExists     = -2553,    /* property already exists */
    nrIterationDone                = -2554,    /* iteration operation is done */
    nrExitedIteratorScope        = -2555,    /* outer scope of iterator was exited */
    nrTransactionAborted        = -2556        /* transaction was aborted */
};


enum
{
    kReferenceProperty    = 0x00010000L             // data not allocated
};

@interface IOPropertyTable : Object
{
@private
	void	*_ptprivate;
}

-(IOReturn) createProperty:(const char *)name flags:(OptionBits)flags value:(void *)value length:(ByteCount)length;
-(IOReturn) deleteProperty:(const char *)name;
-(IOReturn) getProperty:(const char *)name flags:(OptionBits)flags value:(void **)value length:(ByteCount *)length;
-(IOReturn) setProperty:(const char *)name flags:(OptionBits)flags value:(void *)value length:(ByteCount)length;
-(IOReturn) getPropertyWithIndex:(UInt32)index name:(char *)name;

////////  IOConfigTable methods  ////////
- (const char *)valueForStringKey:(const char *)key;
+ (void)freeString : (const char *)string;
- (void)freeString : (const char *)string;
- addConfigData:(const char *)configData;

@end

#endif
