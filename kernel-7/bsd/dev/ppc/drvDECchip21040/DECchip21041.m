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
 * 11-Dec-95	Dieter Siegmund at NeXT (dieter@next.com)
 *		Split out 21040 and 21041 into separate personalities
 */

#import <machdep/ppc/powermac.h>

#import "DECchip21041.h"

//#define DEBUG

#import "DECchip2104xInline.h"

#define SROM_ADDRESS_BITS 		"SROM Address Bits"
#define ENET_ADDRESS_OFFSET 		"Address Offset"

static u_int8_t reverseBitOrder(u_int8_t data);

 
@implementation DECchip21041
/*
 * Public Instance Methods
 */

- initFromDeviceDescription:(IODeviceDescription *)devDesc
{
    IOPCIDevice *deviceDescription = (IOPCIDevice *)devDesc;
    const char  *params, *interface;
    unsigned long		PCICommandReg;

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

    // Set the PCI Command Register for Bus Master, Memory Access
    // and not I/O Access.
    [deviceDescription configReadLong:0x04 value:&PCICommandReg];
    PCICommandReg |= (4 | 1);  // Master & Memory
    PCICommandReg &= ~2;       // Not I/O
    [deviceDescription configWriteLong:0x04 value:PCICommandReg];

    [self mapMemoryRange:0 to:(vm_address_t *)&ioBase
	  findSpace:YES cache:IO_CacheOff];

//kprintf("ioBase = %8x\n", ioBase);

    irq = [deviceDescription interrupt];

    /* Address bits needed depends upon SROM size. */
    params = [[deviceDescription configTable] 
    			valueForStringKey: SROM_ADDRESS_BITS];
    if ((params == NULL) || (strcmp(params, "8") != 0))
	sromAddressBits = 6;
    else
	sromAddressBits = 8;
			
    params = [[deviceDescription configTable] 
    			valueForStringKey: ENET_ADDRESS_OFFSET];
    if (params == NULL)
	enetAddressOffset = 20;			/* DEC standard location */
    else
	enetAddressOffset = _atoi(params);
	
    /* Select network interface */
    connector = connectorAUTO_e;		/* default is auto select */
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

    IOLog("DECchip21041 based adapter at port 0x%0x irq %d", ioBase, irq);
    
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
    int i;
    unsigned short data;
    
    //_dump_srom(ioBase, sromAddressBits);
    for (i = 0; i < sizeof(*ea)/2; i++)	{
	reset_and_select_srom(ioBase);
	data = read_srom(ioBase, i + enetAddressOffset/2, sromAddressBits);

	// See if this is a Powerbook with combo ethernet/modem.
	if (IsPowerStar() && HasPMU() && (powermac_io_info.io_base2)) {
	  // Hooper/Kanga Enet/modem Addr is not bit reversed
	  ea->ea_byte[2*i] = data & 0x0ff;
	  ea->ea_byte[2*i+1] = (data >> 8) & 0x0ff;
	} else {
	  ea->ea_byte[2*i] = reverseBitOrder(data & 0x0ff);
	  ea->ea_byte[2*i+1] = reverseBitOrder((data >> 8) & 0x0ff);
	}
    }
}

/*
 * These are the magic values that need to be written to CSR 12, 14 and 15 to
 * enable appropriate network interfaces. These values are from the
 * respective hardware docs. 
 */
#define DEC_21041_RESET_SIA		0x0

#if 1
#define DEC_21041_SIA0_10BT         	0x0000ef01
#define DEC_21041_SIA1_10BT         	0x00007f3f
#define DEC_21041_SIA2_10BT         	0x00000048
#else
#define DEC_21041_SIA0_10BT         	0x0000ef01
#define DEC_21041_SIA1_10BT         	0x0000ff3f
#define DEC_21041_SIA2_10BT         	0x00000008
#endif

#define DEC_21041_SIA0_AUI         	0x0000ef09
#define DEC_21041_SIA1_AUI         	0x0000f73d
#define DEC_21041_SIA2_AUI         	0x0000000e

#define DEC_21041_SIA0_BNC         	0x0000ef09
#define DEC_21041_SIA1_BNC         	0x0000f73d
#define DEC_21041_SIA2_BNC         	0x00000006

- (void)_setInterface:(connector_t)netInterface
{
    writeCsr(ioBase, DEC_21X40_CSR13, DEC_21041_RESET_SIA);
    IOSleep(1);
    switch (netInterface) {
      case connectorTP_e:
	writeCsr(ioBase, DEC_21X40_CSR15, DEC_21041_SIA2_10BT);
	writeCsr(ioBase, DEC_21X40_CSR14, DEC_21041_SIA1_10BT);
	writeCsr(ioBase, DEC_21X40_CSR13, DEC_21041_SIA0_10BT);
	break;
      case connectorAUI_e:
	writeCsr(ioBase, DEC_21X40_CSR15, DEC_21041_SIA2_AUI);
	writeCsr(ioBase, DEC_21X40_CSR14, DEC_21041_SIA1_AUI);
	writeCsr(ioBase, DEC_21X40_CSR13, DEC_21041_SIA0_AUI);
	break;
      case connectorBNC_e:
	writeCsr(ioBase, DEC_21X40_CSR15, DEC_21041_SIA2_BNC);
	writeCsr(ioBase, DEC_21X40_CSR14, DEC_21041_SIA1_BNC);
	writeCsr(ioBase, DEC_21X40_CSR13, DEC_21041_SIA0_BNC);
	break;
      default:
	break;
    }
    IODelay(100);
    connector = netInterface;

//kprintf("CSR13 = %x  CSR14 = %x  CSR15 = %x\n",
//	readCsr(ioBase, DEC_21X40_CSR13),
//	readCsr(ioBase, DEC_21X40_CSR14),
//	readCsr(ioBase, DEC_21X40_CSR15));
}

- (void)selectInterface
{
    csrRegUnion reg;
    unsigned int time;

//kprintf("[DECchip21041 selectInterface]\n");

    /*
     * Auto-selection is done only once. 
     */
    if (connector != connectorAUTO_e) {
	[self _setInterface:connector];
	return;
    }
    
    
    /* check for link pass interrupt using RJ-45 */
    [self _setInterface:connectorTP_e];
    time = 50;
    while (--time) {
	IOSleep(1);
	reg.data = readCsr(ioBase, DEC_21X40_CSR5);
	if (reg.csr5_41.lnp) {
	    IOLog("%s: detected TP port\n", [self name]);
	    return;
	}
	if (reg.csr5_41.lnf == 1)
	    break;
    }

    /* check for activity on the AUI port */
    [self _setInterface:connectorAUI_e];
    time = 50;
    while (--time) {
	IOSleep(1);
	reg.data = readCsr(ioBase, DEC_21X40_CSR5);
	if (reg.csr5_41.lnp) {
	    IOLog("%s: detected TP port\n", [self name]);
	    [self _setInterface:connectorTP_e];
	    return;
	}
	reg.data = readCsr(ioBase, DEC_21X40_CSR12);
	if (reg.csr12_41.sra)
	    break;
    }
    reg.data = readCsr(ioBase, DEC_21X40_CSR12);
    if (reg.csr12_41.sra) {
	IOLog("%s: detected AUI port\n", [self name]);
	return;
    }
    /* use BNC if nothing else was sensed */
    [self _setInterface:connectorBNC_e];
    IOLog("%s: detected BNC port\n", [self name]);
    return;
}

@end

static u_int8_t reverseBitOrder(u_int8_t data)
{
  u_int8_t            val = 0;
  int                 i;
  
  for (i = 0; i < 8; i++) {
    val <<= 1;
    if (data & 1) val |= 1;
    data >>= 1;
  }
  return val;
}

