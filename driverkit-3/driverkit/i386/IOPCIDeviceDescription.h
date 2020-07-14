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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * PCI device description class.
 *
 * HISTORY
 *
 * 19 Aug 1994 Dean Reece at NeXT
 *	Complete re-write.
 *
 * 19 Jul 1994 Curtis Galloway at NeXT
 *	Created.
 *
 */
#import <driverkit/i386/IOEISADeviceDescription.h>
#import <driverkit/driverTypes.h>


@interface IOPCIDeviceDescription : IOEISADeviceDescription
{
    void	*_pci_private;
}

/*
 * getPCIdevice is the whole purpose for this object.  This method allows
 * callers to get the PCI config address of the PCI device associated
 * with this device description.  If all goes well, the three params are
 * filled in and IO_R_SUCCESS is returned.  There are a variety of reasons
 * that the address couldn't be known, in which case an appropriate code is
 * returned and the parameters are left untouched.  It is acceptable for any
 * of the parameter pointers to be NULL.
 */
- (IOReturn) getPCIdevice: (unsigned char *) devNum
		 function: (unsigned char *) funNum
		      bus: (unsigned char *) busNum;

@end
