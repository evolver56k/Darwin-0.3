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
 * Copyright (c) 1995-1996 NeXT Software, Inc.
 *
 * HISTORY
 *
 * 11-Dec-95	Dieter Siegmund at NeXT (dieter@next.com)
 *		Split out 21040 and 21041 into separate personalities
 */

#import "DECchip21040.h"

//#define DEBUG

#import "DECchip2104xInline.h"

@implementation DECchip21040

/*
 * Public Instance Methods
 */

- initFromDeviceDescription:(IODeviceDescription *)devDesc
{
    IOPCIDevice *deviceDescription = (IOPCIDevice *)devDesc;
    const char  *interface;

    if ([super initFromDeviceDescription:devDesc] == nil) {
    	return nil;
    }

#ifdef PERF
    perfQ = perfGetQueue(OUR_PRODUCER_NAME, &ourProducerId, 0);
    if (!perfQ)
	IOLog("%s: perfGetQueue failed\n", OUR_PRODUCER_NAME);
    else {
	IOLog("%s: perf logging enabled\n", OUR_PRODUCER_NAME);
	ev_p.len = sizeof(ev_p);
	ev_p.producerId = ourProducerId;
    }
#endif PERF

    [self mapMemoryRange:2 to:(vm_address_t *)&ioBase
	  findSpace:YES cache:IO_CacheOff];
    
    irq = [deviceDescription interrupt];

    /* Select network interface */
    connector = connectorAUTO_e; /* default is auto select */
    interface = [[deviceDescription configTable] 
    			valueForStringKey: NETWORK_INTERFACE];
    if (interface != NULL) {
	int i;

	for (i = 0; i < NUM_CONNECTOR_TYPES; i++) {
	    if (strcmp(interface, connectorType(i)) == 0) {
		connector = i;
		break;
	    }
	}
    }

    [self getStationAddress:&myAddress];
    
    if ([self _allocateMemory] == NO) {
        [self free];
	return nil;
    }
    
    isPromiscuous = NO;
    multicastEnabled = NO;

    /* Inform user what we are upto here */
    IOLog("DECchip21040 based adapter at port 0x%0x irq %d", 
	 ioBase, irq);
    
    if (connector != connectorAUTO_e) {
    	IOLog(" interface %s", interface);
    }
    IOLog("\n");
    
    if (![self resetAndEnable:NO]) {
    	[self free];
	return nil;
    }
    
    networkInterface = [super attachToNetworkWithAddress:myAddress];
    
    return self;
}

- (void)getStationAddress:(enet_addr_t *)ea
{
    getStationAddress(ioBase, ea);
}

/*
 * These are the magic values that need to be written to CSR 12, 14 and 15 to
 * enable appropriate network interfaces. These values are from the
 * respective hardware docs. 
 */
#define DEC_21040_RESET_SIA		0x0

#define DEC_21040_SIA0_10BT         	0x00008f01
#define DEC_21040_SIA1_10BT         	0x0000ffff
#define DEC_21040_SIA2_10BT         	0x00000000

#define DEC_21040_SIA0_AUI         	0x0000ef09
#define DEC_21040_SIA1_AUI         	0x00000705
#define DEC_21040_SIA2_AUI         	0x00000006

#define DEC_21040_SIA0_EXT         	0x00008f09
#define DEC_21040_SIA1_EXT         	0x00000705
#define DEC_21040_SIA2_EXT         	0x00000006

- (void)_setInterface:(connector_t)netInterface
{
    writeCsr(ioBase, DEC_21X40_CSR13, DEC_21040_RESET_SIA);
    IOSleep(1);
    switch (netInterface) {
      case connectorTP_e:
	writeCsr(ioBase, DEC_21X40_CSR15, DEC_21040_SIA2_10BT);
	writeCsr(ioBase, DEC_21X40_CSR14, DEC_21040_SIA1_10BT);
	writeCsr(ioBase, DEC_21X40_CSR13, DEC_21040_SIA0_10BT);
	break;
      case connectorAUI_e:
	writeCsr(ioBase, DEC_21X40_CSR15, DEC_21040_SIA2_AUI);
	writeCsr(ioBase, DEC_21X40_CSR14, DEC_21040_SIA1_AUI);
	writeCsr(ioBase, DEC_21X40_CSR13, DEC_21040_SIA0_AUI);
	break;
      case connectorBNC_e:
	writeCsr(ioBase, DEC_21X40_CSR15, DEC_21040_SIA2_EXT);
	writeCsr(ioBase, DEC_21X40_CSR14, DEC_21040_SIA1_EXT);
	writeCsr(ioBase, DEC_21X40_CSR13, DEC_21040_SIA0_EXT);
	break;
      default:
	break;
    }
    IODelay(100);
    connector = netInterface;
}

- (void)selectInterface
{
    csrRegUnion reg;
    unsigned int time;
    
    /*
     * Auto-selection is done only once. 
     */
    if (connector != connectorAUTO_e) {
	[self _setInterface:connector];
	return;
    }
    
    /*
     * Try 10Base-T
     */
    [self _setInterface:connectorTP_e];
    
    /*
     * Check the SIA TP Link status
     */
    time = 1000;
    while (--time) {
	IODelay(1000);
	reg.data = readCsr(ioBase, DEC_21X40_CSR12);
	if (reg.csr12.lkf == 0) {
#ifdef DEBUG
	    IOLog("%s: Using RJ-45 connector\n", [self name]);
#endif DEBUG
	    return;
	}
    }
    
    /*
     * Try AUI now
     */
    [self _setInterface:connectorAUI_e];
    
    time = 1000;
    while (--time) {
	IODelay(1000);
	reg.data = readCsr(ioBase, DEC_21X40_CSR12);
	if (reg.csr12.ncr == 1) {
#ifdef DEBUG
	    IOLog("%s: Using BNC connector\n", [self name]);
#endif DEBUG	
	    [self _setInterface:connectorBNC_e];
	    return;
	}
    }
    
#ifdef DEBUG
    IOLog("%s: Using AUI connector\n", [self name]);
#endif DEBUG	
}

@end

