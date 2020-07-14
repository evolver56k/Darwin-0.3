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



#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/powermac.h>

#import "ohare.h"


@implementation AppleOHare

+ (BOOL)probe:(IOPCIDevice *)deviceDescription
{
  AppleOHare  *OHareInstance;

  OHareInstance = [self alloc];
  
  return [OHareInstance initFromDeviceDescription:deviceDescription] != nil;
}


- initFromDeviceDescription:(IOPCIDevice *)deviceDescription
{
  unsigned long PCICommandReg;

  if ([super initFromDeviceDescription:deviceDescription] == nil) {
    IOLog("AppleOHare: [super initFromDeviceDescription] failed\n");
    return nil;
  }
  
  // Set the PCI Command Register for Bus Master, Memory Access
  // and not I/O Access.
  [deviceDescription configReadLong:0x04 value:&PCICommandReg];
  PCICommandReg |= (2 | 1);  // Master & Memory
  PCICommandReg &= ~4;       // Not I/O
  [deviceDescription configWriteLong:0x04 value:PCICommandReg];

  [self mapMemoryRange:0 to:(vm_address_t *)&ioBaseOHare
	findSpace:YES cache:IO_CacheOff];

  powermac_io_info.io_base2 = ioBaseOHare;

  *(long *)(ioBaseOHare + 0x00024) = 0;    /* Disable all interrupts */
  eieio();
  *(long *)(ioBaseOHare + 0x00028) = 0xffffffff; /* Clear pending interrupts */
  eieio();
  *(long *)(ioBaseOHare + 0x00024)  = 0;    /* Disable all interrupts */
  eieio();
  
  [deviceDescription setInterruptList:NULL num:0];

  [self setName:"AppleOHare"];
  [self registerDevice];

  return self;
}

@end
