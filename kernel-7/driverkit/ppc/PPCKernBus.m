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
 * Kernel PPC Bus Resource Object(s).
 *
 * HISTORY
 *
 * 28 June 1994 Curtis Galloway at NeXT
 *	Derived from EISA files.
 */

#import <mach/mach_types.h>

#import <driverkit/KernLock.h>
#import <driverkit/ppc/PPCKernBus.h>
#import <driverkit/ppc/PPCKernBusPrivate.h>
#import <driverkit/KernDevice.h>
#import <driverkit/ppc/driverTypes.h>
#import <driverkit/ppc/driverTypesPrivate.h>
#import <kernserv/ppc/spl.h>
#import <machdep/ppc/interrupts.h>



static void
PPCKernBusInterruptDispatch(int deviceIntr, void * ssp, void *_interrupt)
{
    BOOL			leave_enabled;
    PPCKernBusInterrupt_ *	interrupt = (PPCKernBusInterrupt_ *)_interrupt;

    leave_enabled = KernBusInterruptDispatch(_interrupt, ssp);
    if (!leave_enabled) {
        KernLockAcquire(interrupt->_PPCLock);
        pmac_disable_irq(interrupt->_irq);
        interrupt->_irqEnabled = NO;
        KernLockRelease(interrupt->_PPCLock);
    }
}

@implementation PPCKernBusInterrupt

- initForResource:	resource
	item:		(unsigned int)item
	shareable:	(BOOL)shareable
{
    [super initForResource:resource item:item shareable:shareable];

    _irq = item;
    _irqEnabled = NO;
    _PPCLock = [[KernLock alloc] initWithLevel:7];
    _priorityLevel = IPLDEVICE;

    return self;
}

- dealloc
{
    [_PPCLock free];
    return [super dealloc];
}

- attachDeviceInterrupt:	interrupt
{
    unsigned int pmac_device = pmac_int_to_number(_irq);

    if (!interrupt)
    	return nil;

    if (pmac_device == (-1))
	return nil;

    [_PPCLock acquire];

    if( NO == _irqAttached) {
#warning all PPC driverkit drivers get SPLBIO
        pmac_register_int(pmac_device, SPLBIO,
                        (void (*)(int, void *, void *))PPCKernBusInterruptDispatch,
                        (void *)self);
	_irqAttached = YES;
    }
    /*
     * -attachDeviceInterrupt will return nil
     * if the interrupt is suspended.
     */
    if ([super attachDeviceInterrupt:interrupt]) {
        _irqEnabled = YES;
        pmac_enable_irq(_irq);
    } else {
        pmac_disable_irq(_irq);
        _irqEnabled = NO;
    }

//  _irqAttached = YES;
    
    [_PPCLock release];
	
    return self;
}

- attachDeviceInterrupt:	interrupt
		atLevel: 	(int)level
{
    unsigned int pmac_device = pmac_int_to_number(_irq);

    if (!interrupt)
	return nil;
	
    if (pmac_device == (-1))
	return nil;

    [_PPCLock acquire];

    if (level < _priorityLevel || level >  IPLSCHED) {
	[_PPCLock release];
    	return nil;
    }
	
//    if (level > _priorityLevel)
//    	intr_change_ipl(irq, level);
	
    _priorityLevel = level;

    if( NO == _irqAttached) {
        pmac_register_int(pmac_device, SPLBIO,
                        (void (*)(int, void *, void *))PPCKernBusInterruptDispatch,
                        (void *)self);
	_irqAttached = YES;
    }
    /*
     * -attachDeviceInterrupt will return nil
     * if the interrupt is suspended.
     */
    if ([super attachDeviceInterrupt:interrupt]) {
        _irqEnabled = YES;
        pmac_enable_irq(_irq);
    } else {
        pmac_disable_irq(_irq);
        _irqEnabled = NO;
    }

//  _irqAttached = YES;

    [_PPCLock release];
    return self;
}

- detachDeviceInterrupt:	interrupt
{
    int			irq = [self item];
    
    [_PPCLock acquire];

    if ( ![super detachDeviceInterrupt:interrupt]) {
      pmac_disable_irq(_irq);
      _irqEnabled = NO;
    }

//  _irqAttached = NO;
	
    [_PPCLock release];
    return self;
}

- suspend
{
    [_PPCLock acquire];

    [super suspend];


    if (_irqEnabled) {
      pmac_disable_irq(_irq);
      _irqEnabled = NO;
    }
	
    [_PPCLock release];
    
    return self;
}

- resume
{
    [_PPCLock acquire];

    if ([super resume] && !_irqEnabled) {
        _irqEnabled = YES;
        pmac_enable_irq(_irq);
    }

    [_PPCLock release];
    
    return self;
}

@end



@implementation PPCKernBus

static const char *resourceNameStrings[] = {
    IRQ_LEVELS_KEY,
    DMA_CHANNELS_KEY,
    MEM_MAPS_KEY,
    IO_PORTS_KEY,
    NULL
};

+ initialize
{
    [self registerBusClass:self name:"PPC"];
    return self;
}

- init
{
    [super init];
    
    [self _insertResource:[[KernBusItemResource alloc]
				initWithItemCount:IO_NUM_PPC_INTERRUPTS
				itemKind:[PPCKernBusInterrupt class]
				owner:self]
		    withKey:IRQ_LEVELS_KEY];

    [self _insertResource:[[KernBusRangeResource alloc]
    					initWithExtent:RangeMAX
					kind:[KernBusMemoryRange class]
					owner:self]
		    withKey:MEM_MAPS_KEY];

    [[self class] registerBusInstance:self name:"PPC" busId:[self busId]];

    printf("PPC bus support enabled\n");
    return self;
}

- (const char **)resourceNames
{
    return resourceNameStrings;
}

- free
{

    if ([self areResourcesActive])
    	return self;

    [[self _deleteResourceWithKey:IRQ_LEVELS_KEY] free];
    [[self _deleteResourceWithKey:MEM_MAPS_KEY] free];
    
    return [super free];
}

@end
