/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IdePIIX.m - PIIX/PCI specific ATA controller initialization module. 
 *
 * 23-Jan-1998 Joe Liu at Apple
 *  Added support for PIIX/PIIX3/PIIX4 PCI IDE controllers.
 *
 * 05-Apr-1995	Rakesh Dubey at NeXT
 *	Fixed some bugs in PCI support.
 * 03-Oct-1994 	Rakesh Dubey at NeXT
 *      Created. 
 */

#import "IdeCnt.h"
#import "IdePIIX.h"
#import "IdeCntCmds.h"
#import <driverkit/i386/IOPCIDeviceDescription.h>
#import <driverkit/i386/IOPCIDirectDevice.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <machdep/i386/io_inline.h>
#import <string.h>
#import <stdio.h>
#import "IdeDDM.h"
#import "PIIX.h"
#import "PIIXTiming.h"
#import "IdeShared.h"

// XXX get rid of this
#if (IO_DRIVERKIT_VERSION != 330)
#import <machdep/machine/pmap.h>
#endif

extern vm_offset_t pmap_resident_extract(pmap_t pmap, vm_offset_t va);

//#define DEBUG

#ifndef MIN
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif  MIN

#ifndef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#endif  MAX

/*
 * Function: IOMallocPage
 *
 * Purpose:
 *   Returns a pointer to a page-aligned memory block of size >= PAGE_SIZE.
 *   Note that on Intel, the hardware page size of 4K. However, MACH's
 *   notion of a page is 8K, which is comprised of two contiguous
 *   (physical/virtual) hardware pages.
 *
 * Return:
 *   Actual pointer and size of block returned in actual_ptr and actual_size.
 *   Use these as arguments to IOFree: IOFree(*actual_ptr, *actual_size);
 */
static void *
IOMallocPage(int request_size, void ** actual_ptr,
				 int * actual_size)
{
    void * mem_ptr;
    
	/*
	 * Minimize memory use by first trying to allocate the requested size
	 * without any padding.
	 */
	*actual_size = round_page(request_size);
	mem_ptr = IOMalloc(*actual_size);
	if (mem_ptr == NULL)
		return NULL;
	
	/*
	 * Check alignment of this page.
	 */
	if ((vm_offset_t)mem_ptr & (PAGE_SIZE - 1)) {	// NOT page aligned.
		IOFree(mem_ptr, *actual_size);
		*actual_size = round_page(request_size) + PAGE_SIZE;
		mem_ptr = IOMalloc(*actual_size);
		if (mem_ptr == NULL)
			return NULL;		
	}

	*actual_ptr = mem_ptr;
	return ((void *)round_page(mem_ptr));		
}

@implementation IdeController(PIIX)

/*
 * Method: probePCIController
 *
 * Purpose:
 * Probe the existence of a supported PCI chipset, then proceed to
 * record the PCI IDE controller found.
 *
 * Note:
 * This is called before [super init...], and so we use the class version
 * of getPCIdevice method calls.
 *
 */
- (BOOL) probePCIController:(IOPCIDeviceDescription *)devDesc
{
    unsigned char 	devNum, funcNum, busNum;
    const char 		*value;
	const char		*deviceName;
	IOConfigTable 	*configTable;
    IOReturn 		rtn;
    const id self_class = [self class];

	/*
	 * Initialize PCI ivars.
	 */
	_controllerID = PCI_ID_NONE;
	_ideChannel   = PCI_CHANNEL_OTHER;
	_busMaster    = NO;
	bzero((char *)&_prdTable, sizeof(_prdTable));
	
	/*
	 * Make sure we are dealing with a PCI config table by reading the
	 * BUS_TYPE key.
	 */
    configTable = [devDesc configTable];
    value = [configTable valueForStringKey:BUS_TYPE];
    if (!value || strcmp(value, "PCI") != 0) {
		// Not PCI, return YES to continue probing for non PCI controllers.
		return YES;
    }
    
	/*
	 * Read PCI config space for VendorID and DeviceID.
	 */
    rtn = [devDesc getPCIdevice:&devNum function:&funcNum bus:&busNum];
    if (rtn != IO_R_SUCCESS) {
    	IOLog("%s: Unsupported PCI hardware\n", [self name]);
		return NO;
    }
    rtn = [self_class getPCIConfigData:&_controllerID atRegister:0x00
		withDeviceDescription:devDesc];
    if (rtn != IO_R_SUCCESS)	{
    	IOLog("%s: PCI config space access error %d\n", [self name], rtn);
		return NO;
    }	  

	switch (_controllerID) {
		case PCI_ID_PIIX:
			deviceName = "PIIX";
			break;
		case PCI_ID_PIIX3:
			deviceName = "PIIX3";
			break;
		case PCI_ID_PIIX4:
			deviceName = "PIIX4";
			break;
		default:
			IOLog("%s: Unknown PCI IDE controller (0x%08lx)\n",
				[self name], _controllerID);
			_controllerID = PCI_ID_NONE;
			return NO;
	}

	/*
	 * Report the PCI controller found.
	 */
	IOLog("%s: %s PCI IDE Controller at Dev:%d Func:%d Bus:%d\n",
		[self name], deviceName, devNum, funcNum, busNum);

	/*
	 * At this point, we are certain that we are dealing with a
	 * Intel PIIX class controller.
	 */
	return ([self PIIXInitController:devDesc]);
}

/*
 * Method: initPIIXController
 *
 * Initializes the Intel PIIX IDE controller.
 */
- (BOOL) PIIXInitController:(IOPCIDeviceDescription *)devDesc
{
	piix_idetim_u	idetim;
	IOReturn 		rtn;
	unsigned long	configReg;
	const id self_class = [self class];

	/*
	 * Are we initializing the primary or the secondary channel?
	 * Set the ivar _ideChannel.
	 */
	switch ([devDesc portRangeList]->start) {
		case PIIX_P_CMD_ADDR:
			_ideChannel = PCI_CHANNEL_PRIMARY;
			break;
		case PIIX_S_CMD_ADDR:
			_ideChannel = PCI_CHANNEL_SECONDARY;
			break;
		default:
			_ideChannel = PCI_CHANNEL_OTHER;
	}
	
	/*
	 * PIIX configured on a weird location, cannot continue.
	 *
	 * NOTE:
	 * The I/O ranges does NOT show up as a I/O range in the PCI
	 * configuration space. However, the Bus-Mastering I/O range
	 * does show up at configuration space location 0x20.
	 */
	if ((_ideChannel == PCI_CHANNEL_OTHER) ||
		([devDesc portRangeList]->size != PIIX_CMD_SIZE)) {
		IOLog("%s: Invalid IDE Command Block set to 0x%x size %d\n",
			[self name],
			[devDesc portRangeList]->start,
			[devDesc portRangeList]->size);
		return NO;
	}
	
	/*
	 * Verify our IRQ assignment.
	 *
	 * PIIX hardcodes the following settings:
	 * IRQ 14 - primary channel
	 * IRQ 15 - secondary channel
	 */
	{
	unsigned int irq;
	
	irq = (_ideChannel == PCI_CHANNEL_PRIMARY) ? PIIX_P_IRQ : PIIX_S_IRQ;
	if ([devDesc interrupt] != irq) {
		IOLog("%s: Invalid IRQ: %d\n", [self name], [devDesc interrupt]);
		return NO;
	}
	}
	
	/*
	 * Check the I/O Space Enable bit in the PCI command register.
	 *
	 * This is the master enable bit for the PIIX controller.
	 */
    rtn = [self_class getPCIConfigData:&configReg atRegister:PIIX_PCICMD
		withDeviceDescription:devDesc];
    if (rtn != IO_R_SUCCESS)	{
    	IOLog("%s: PCI config space access error %d\n", [self name], rtn);
		return NO;
    }	
	if (!(configReg & 0x0001)) {
		IOLog("%s: PCI IDE controller is not enabled\n", [self name]);
		return NO;
	}
	if (configReg & 0x0004)
		_busMaster = YES;
	else
		_busMaster = NO;
	
	/*
	 * Fetch the corresponding primary/secondary IDETIM register and
	 * verify that the individual channels are enabled.
	 */
    rtn = [self_class getPCIConfigData:&configReg atRegister:PIIX_IDETIM
		withDeviceDescription:devDesc];
    if (rtn != IO_R_SUCCESS)	{
    	IOLog("%s: PCI config space access error %d\n", [self name], rtn);
		return NO;
    }
	if (_ideChannel == PCI_CHANNEL_SECONDARY)
		configReg >>= 16;	// PIIX_IDETIM + 2 for secondary channel
	idetim.word = (u_short)configReg;
	
	if (!idetim.bits.ide) {
		IOLog("%s: %s PCI IDE channel is not enabled\n",
			[self name],
			(_ideChannel == PCI_CHANNEL_PRIMARY) ? "Primary" : "Secondary");
		return NO;
	}

	/*
	 * Register and record the location of our Bus Master 
	 * interface registers.
	 */
	if (_busMaster && ([self PIIXRegisterBMRange:devDesc] == NO)) {
		IOLog("%s: Bus master I/O range registration failed\n",
			[self name]);
		_busMaster = NO;
	}
	
	/*
	 * Allocate a 4K-page (perhaps 8K) aligned page of memory for
	 * the PRD table.
	 */
	if (_busMaster && ([self PIIXInitPRDTable] == NO)) {
		IOLog("%s: cannot allocate memory for descriptor table\n",
			[self name]);
		_busMaster = NO;
	}

#if 0
	IOLog("%s: PCI bus master DMA: %s\n",
		[self name], busMaster ? "Enabled" : "Disabled");
#endif

	/*
	 * Revert to default timing.
	 */
	[self PIIXResetTimings:devDesc];
	
    return YES;
}

/*
 * Method: getPCIControllerCapabilities
 *
 * Return the capability of the PCI IDE controller in 'm'.
 *
 */
- (void) getPCIControllerCapabilities:(txferModes_t *)m
{
	m->mode.swdma = m->mode.mwdma = m->mode.udma = ATA_MODE_NONE;
	switch (_controllerID) {
		case PCI_ID_PIIX:
		case PCI_ID_PIIX3:
		case PCI_ID_PIIX4:
			m->mode.pio   = ata_mode_to_mask(ATA_MODE_4);
			if (_busMaster) {
				m->mode.mwdma = ata_mode_to_mask(ATA_MODE_2);
				if (_controllerID == PCI_ID_PIIX4)
					m->mode.udma = ata_mode_to_mask(ATA_MODE_2);
			}
			break;
	}
}

/*
 * Get the PIO port transfer width. This refers to the width of the
 * I/O transfer on the PIO port, the IDE bus width is always 16-bits.
 *
 * All PIIX controllers are capable of 32-bit transfers on the data
 * port.
 */
- (ideTransferWidth_t) getPIOTransferWidth
{
	return (IDE_TRANSFER_32_BIT);
}

/*
 * Method: resetPCIController
 *
 * Not a true RESET, simply return the PCI controller to a quiescent state
 * and return all IDE ports to the default timing.
 */
- (void) resetPCIController
{
	switch (_controllerID) {
		case PCI_ID_PIIX:
		case PCI_ID_PIIX3:
		case PCI_ID_PIIX4:
			[self PIIXInit];
			[self PIIXResetTimings:[self deviceDescription]];
			break;
	}
}

/*
 * Method: PIIXResetTimings
 *
 * Purpose:
 * Revert the timing register to the default value. The transfer timing
 * is set to the compatible mode. We need to be careful to initialize the
 * register only for our current IDE channel.
 */
- (void) PIIXResetTimings:(IOPCIDeviceDescription *)devDesc
{
	union {
		u_long dword;
		struct {
			piix_idetim_u pri;
			piix_idetim_u sec;
		} tim;
	} timings;

    IOReturn rtn;
	u_long udma;

	/*
	 * Read the PIIX_IDETIM register.
	 */	
	rtn = [[self class] getPCIConfigData:&timings.dword
		atRegister:PIIX_IDETIM
		withDeviceDescription:devDesc];
    if (rtn != IO_R_SUCCESS)
		return;

	/*
	 * Read both PIIX_UDMACTL and PIIX_UDMATIM register.
	 */
	rtn = [[self class] getPCIConfigData:&udma atRegister:PIIX_UDMACTL
		withDeviceDescription:devDesc];

	/*
	 * Set compatible timing.
	 * Disable UDDMA and set its timing registers to the slowest mode.
	 */	
	switch (_ideChannel) {
		case PCI_CHANNEL_PRIMARY:
			timings.tim.pri.word &= 0x8000;
			udma &= 0xffccfffc;
			break;
		case PCI_CHANNEL_SECONDARY:
			timings.tim.sec.word &= 0x8000;
			udma &= 0xccfffff3;
			break;
		default:
			return;
	}
	
	/*
	 * Write the modified PCI config space registers back.
	 */
	[[self class] setPCIConfigData:timings.dword atRegister:PIIX_IDETIM
		withDeviceDescription:devDesc];
	[[self class] setPCIConfigData:udma atRegister:PIIX_UDMACTL
		withDeviceDescription:devDesc];
}

/*
 * Method: PIIXRegisterBMRange:
 *
 * Purpose:
 * Add the 8-byte Bus-Master control registers to the portRangeList in
 * the deviceDescription. The base address for the registers resides in
 * PCI config space location 0x20. The first 8 bytes are for the primary
 * IDE channel, the next eight bytes are for the secondary IDE channel.
 *
 * Note:
 * This must be called before [super init...] because that's when the
 * resources are registered.
 */
- (BOOL) PIIXRegisterBMRange:(IOPCIDeviceDescription *)devDesc
{
    IOReturn 		rtn;
	unsigned long	bmiba;
	IORange 		io_range[2];
	
    rtn = [[self class] getPCIConfigData:&bmiba atRegister:PIIX_BMIBA
		withDeviceDescription:devDesc];
    if (rtn != IO_R_SUCCESS) {
    	IOLog("%s: PCI config space access error %d\n", [self name], rtn);
		return NO;
    }
	
	/*
	 * Sanity check. Make sure this is an I/O range.
	 */
	if ((bmiba & 0x01) == 0) {
		IOLog("%s: PCI memory range 0x%02x (0x%08lx) is not an I/O range\n",
			[self name], PIIX_BMIBA, bmiba);
		return NO;
	}
	
	_bmRegs = bmiba & PIIX_BM_MASK;

	if (_bmRegs == 0)	// uninitialized range
		return NO;

	if (_ideChannel == PCI_CHANNEL_SECONDARY)
		_bmRegs += PIIX_BM_OFFSET;
	
	/*
	 * Add this range to our device description's port range list.
	 */
	io_range[0] = [devDesc portRangeList][0];
	io_range[1].start = _bmRegs;
	io_range[1].size  = PIIX_BM_SIZE;
	if ([devDesc setPortRangeList:io_range num:2] != IO_R_SUCCESS) {
		IOLog("%s: setPortRangeList failed\n", [self name]);
		return NO;
	}
	
	return YES;
}

/*
 * Method: PIIXInitPRDTable
 *
 * Purpose:
 * Initialize a "page-aligned" page of memory for the PRD descriptors
 * used by the bus master IDE controller.
 *
 * FIXME: Need to free the _prdTable memory.
 */
- (BOOL) PIIXInitPRDTable
{
	_prdTable.size = PAGE_SIZE;
	_prdTable.ptr = (void *)IOMallocPage(
						_prdTable.size,
						&_prdTable.ptrReal,
						&_prdTable.sizeReal
						);

	/*
	 * _prdTable->ptr should now points to a physically contiguous block
	 * of PAGE_SIZE bytes.
	 */
    if (_prdTable.ptr == NULL)
		return NO;
	
	/*
	 * cache the physical address of the descriptor table to _tablePhyAddr.
	 */
	if (IOPhysicalFromVirtual(IOVmTaskSelf(), (vm_address_t)_prdTable.ptr,
		&_tablePhyAddr) != IO_R_SUCCESS) {
		IOFree(_prdTable.ptrReal, _prdTable.sizeReal);
		return NO;
	}

	bzero(_prdTable.ptr, _prdTable.size);
	return YES;
}

/*
 * Method: PIIXReportTimings:slaveTiming:isPrimary:
 *
 * Purpose:
 * Log the drive timings set in the two PIIX timing registers.
 * The units for the values are in PCI clocks.
 */
- (void) PIIXReportTimings:(piix_idetim_u)tim
              slaveTiming:(piix_sidetim_u)stim
                isPrimary:(BOOL)primary
{
	if (!_ide_debug)
		return;

	IOLog("%s: Drive 0: ISP:%d Clks RCT:%d Clks\n",
		[self name],
		PIIX_ISP_TO_CLK(tim.bits.isp),
		PIIX_RCT_TO_CLK(tim.bits.rct));
#if 0
	IOLog("%s: Drive 0 Fast timing DMA only: %s\n", [self name],
		tim.bits.dte0 ? "on" : "off");
	IOLog("%s: Drive 0 Prefetch and Posting: %s\n", [self name],
		tim.bits.ppe0 ? "on" : "off");
	IOLog("%s: Drive 0 IORDY sample enable : %s\n", [self name],
		tim.bits.ie0 ? "on" : "off");
	IOLog("%s: Drive 0 Fast timing enable  : %s\n", [self name],
		tim.bits.time0 ? "on" : "off");
#endif 0
	IOLog("%s: Drive 1: ISP:%d Clks RCT:%d Clks\n", [self name],
		tim.bits.sitre ?
			(primary ?
				PIIX_ISP_TO_CLK(stim.bits.pisp1) : 
				PIIX_ISP_TO_CLK(stim.bits.sisp1)) : 
			PIIX_ISP_TO_CLK(tim.bits.isp),
		tim.bits.sitre ?
			(primary ?
				PIIX_RCT_TO_CLK(stim.bits.prct1) :
				PIIX_RCT_TO_CLK(stim.bits.srct1)) :
			PIIX_RCT_TO_CLK(tim.bits.rct));
#if 0
	IOLog("%s: Drive 1 Fast timing DMA only: %s\n", [self name],
		tim.bits.dte1 ? "on" : "off");
	IOLog("%s: Drive 1 Prefetch and Posting: %s\n", [self name],
		tim.bits.ppe1 ? "on" : "off");
	IOLog("%s: Drive 1 IORDY sample enable : %s\n", [self name],
		tim.bits.ie1 ? "on" : "off");
	IOLog("%s: Drive 1 Fast timing enable  : %s\n", [self name],
		tim.bits.time1 ? "on" : "off");
#endif 0
}

/*
 * Method: setPCIControllerCapabilities
 *
 * Purpose:
 * Based on the transfer modes and types for both IDE drives, setup the
 * controller to support those modes.
 *
 */
- (BOOL) setPCIControllerCapabilitiesForDrives:(driveInfo_t *)drives
{
	IOPCIConfigSpace configSpace;
	
	if (_controllerID == PCI_ID_NONE)
		return NO;
	
	[self getPCIConfigSpace:&configSpace];
	
	switch (_controllerID) {
		case PCI_ID_PIIX:
		case PCI_ID_PIIX3:
		case PCI_ID_PIIX4:
			[self PIIXComputePCIConfigSpace:&configSpace forDrives:drives];
			break;
		default:
			return NO;
	}

	[self setPCIConfigSpace:&configSpace];
    return YES;
}

/*
 * Method: computePCIConfigSpaceForPIIX:modes:
 *
 * Purpose:
 * Set the IDETIM and the SIDETIM IDE timing registers based on the
 * PIO modes supported by the two drives on the IDE channel.
 */
- (void) PIIXComputePCIConfigSpace:(IOPCIConfigSpace *)configSpace
		forDrives:(driveInfo_t *)drv
{
	u_char *pci_space = (u_char *)configSpace;
    piix_idetim_u	*idetim;
	piix_sidetim_u  *sidetim;
	piix_udmactl_u	*udmactl;
	piix_udmatim_u  *udmatim;
	unsigned char	modeDrive0;
	unsigned char	modeDrive1;
	u_char 			isp, rct;

	if (MAX_IDE_DRIVES != 2)
		return;
	
	switch (_ideChannel) {
		case PCI_CHANNEL_PRIMARY:
			idetim = (piix_idetim_u	*)&pci_space[PIIX_IDETIM];
			break;
		case PCI_CHANNEL_SECONDARY:
			idetim = (piix_idetim_u	*)&pci_space[PIIX_IDETIM_S];
			break;
		default:
			IOLog("%s: PIIX: Unknown IDE channel\n", [self name]);
			return;
	}
	sidetim = (piix_sidetim_u *)&pci_space[PIIX_SIDETIM];
	udmactl = (piix_udmactl_u *)&pci_space[PIIX_UDMACTL];
	udmatim = (piix_udmatim_u *)&pci_space[PIIX_UDMATIM];

	modeDrive0 = ata_mode_to_num(drv[0].transferMode);
	modeDrive1 = ata_mode_to_num(drv[1].transferMode);
	
	/* Enable slave timing if timings are different and
	 * a slave device is present.
	 */
	idetim->bits.sitre = 0;
	isp = PIIXGetISPForMode(modeDrive0, drv[0].transferType);
	rct = PIIXGetRCTForMode(modeDrive0, drv[0].transferType);
	
	if ((PIIXGetCycleForMode(modeDrive0, drv[0].transferType) != 
		PIIXGetCycleForMode(modeDrive1, drv[1].transferType)) &&
		(drv[1].ideInfo.type != 0)) {
		if (_controllerID == PCI_ID_PIIX) {
			/* Do not have the luxury of separate timing register for
			 * drive 0 and drive 1. Use the minimum of the two PIO modes.
			 * Or, the max of the two timings.
			 */
			isp = MAX(PIIXGetISPForMode(modeDrive0, drv[0].transferType),
				      PIIXGetISPForMode(modeDrive1, drv[1].transferType));
			rct = MAX(PIIXGetRCTForMode(modeDrive0, drv[0].transferType),
				      PIIXGetRCTForMode(modeDrive1, drv[1].transferType));			
		}
		else
			idetim->bits.sitre = 1;		// enable slave timing
	}
	
	/*
	 * Reset all performance tuning bits and disable UDMA.
	 */
	idetim->word &= 0xc000;
	switch (_ideChannel) {
		case PCI_CHANNEL_PRIMARY:
			udmactl->bits.psde0 = 0;
			udmactl->bits.psde1 = 0;
			break;
		default:
			udmactl->bits.ssde0 = 0;
			udmactl->bits.ssde1 = 0;
	}

	/*
	 * Set the timings for drive 0 (master).
	 */
	if (drv[0].ideInfo.type == 0) {
		IOLog("%s: Drive 0 is not present\n", [self name]);
		return;
	}

	/*
	 * Set timings for Drive 0 (Master drive).
	 */
	if (drv[0].transferType == IDE_TRANSFER_ULTRA_DMA) {
		if (modeDrive0 > 2) modeDrive0 = 2;
		switch (_ideChannel) {
			case PCI_CHANNEL_PRIMARY:
				udmactl->bits.psde0 = 1;
				udmatim->bits.pct0 = modeDrive0;
				break;
			case PCI_CHANNEL_SECONDARY:
				udmactl->bits.ssde0 = 1;
				udmatim->bits.sct0 = modeDrive0;
				break;
			default:
				break;
		}
	}
	idetim->bits.isp = isp;
	idetim->bits.rct = rct;
	
	/* Set timings for drive 1 (Slave drive).
	 */
	if (drv[1].transferType == IDE_TRANSFER_ULTRA_DMA) {
		if (modeDrive1 > 2) modeDrive1 = 2;
		switch (_ideChannel) {
			case PCI_CHANNEL_PRIMARY:
				udmactl->bits.psde1 = 1;
				udmatim->bits.pct1 = modeDrive1;
				break;
			case PCI_CHANNEL_SECONDARY:
				udmactl->bits.ssde1 = 1;
				udmatim->bits.sct1 = modeDrive1;
				break;
			default:
				break;
		}
	}
	if (idetim->bits.sitre) {
		isp = PIIXGetISPForMode(modeDrive1, drv[1].transferType);
		rct = PIIXGetRCTForMode(modeDrive1, drv[1].transferType);	
		if (_ideChannel == PCI_CHANNEL_PRIMARY) {
			sidetim->bits.pisp1 = isp;
			sidetim->bits.prct1 = rct;
		}
		else {
			sidetim->bits.sisp1 = isp;
			sidetim->bits.srct1 = rct;
		}
	}

	/*
	 * Enable fast timings. Turn on IORDY sampling always?
	 */
	idetim->bits.time0 = 1;
	idetim->bits.ppe0  = 1;
	idetim->bits.ie0   = 1;
	if (drv[1].ideInfo.type != 0) {
		idetim->bits.time1 = 1;
		idetim->bits.ppe1  = 1;
		idetim->bits.ie1   = 1;
	}

	/*
	 * For DMA, disable fast timing for PIO.
	 */
	if (drv[0].transferType != IDE_TRANSFER_PIO)
		idetim->bits.dte0 = 1;
	if (drv[1].transferType != IDE_TRANSFER_PIO)
		idetim->bits.dte1 = 1;

	[self PIIXReportTimings:*idetim slaveTiming:*sidetim 
		isPrimary:(_ideChannel == PCI_CHANNEL_PRIMARY)];

	return;
}

/*************************************************************************
 *
 * Intel PIIX/PIIX3/PIIX4 Bus-Mastering IDE DMA support
 *
 *************************************************************************/

/*
 * Function: PIIXVirtualToPhysical
 *
 * Similar to IOPhysicalFromVirtual but with no SPLVM/SPLX and locking.
 */
static __inline__ vm_offset_t
PIIXVirtualToPhysical(struct vm_map *map, vm_offset_t vaddr)
{
	return (vm_offset_t)pmap_resident_extract(
			(pmap_t)vm_map_pmap_EXTERNAL(map),
			vaddr);
}

/*
 * Function: PIIXStartDMA
 *
 * Purpose:
 * Start the bus master by writing a 1 to the SSBM bit in BMICX register.
 *
 * Argument:
 * piix_base - base address of the I/O space mapped bus master registers
 */
static __inline__ void
PIIXStartDMA(u_short piix_base)
{
	piix_bmicx_u piix_cmd;
	
	/*
	 * Engage the bus master by writing 1 to the start bit in the
	 * Command Register.
	 */
	piix_cmd.byte = inb(piix_base + PIIX_BMICX);
	piix_cmd.bits.ssbm = 1;
	outb(piix_base + PIIX_BMICX, piix_cmd.byte);
}

/*
 * Function: PIIXStopDMA
 *
 * Purpose:
 * Stop the bus master by clearing the SSBM bit in BMICX register.
 *
 * Argument:
 * piix_base - base address of the I/O space mapped bus master registers
 */
static __inline__ void
PIIXStopDMA(u_short piix_base)
{
	piix_bmicx_u piix_cmd;
	
	/*
	 * Stop the bus master by writing 0 to the start bit in the
	 * Command Register.
	 */
	piix_cmd.byte = inb(piix_base + PIIX_BMICX);
	piix_cmd.bits.ssbm = 0;
	outb(piix_base + PIIX_BMICX, piix_cmd.byte);	
}

/*
 * Function: PIIXGetStatus
 *
 * Purpose:
 * Return the PIIX BMISX (bus master IDE status register).
 *
 * Argument:
 * piix_base - base address of the I/O space mapped bus master registers
 */
static __inline__ u_char
PIIXGetStatus(u_short piix_base)
{
	return (inb(piix_base + PIIX_BMISX));
}
			
/*
 * Function: PIIXSetupPRDTable
 *
 * Purpose:
 * Setup the PRD (descriptor) table for the current IDE transfer.
 * This table must be aligned on a DWord (4 byte) boundary.
 *
 * Arguments:
 * table    - points to the start of the PRD table
 * size     - max number of PRD entries that the table can hold
 * vaddr	- virtual address of the start of memory buffer
 * size		- size of memory buffer in bytes
 * map		- vm_map for the memory buffer
 *
 * Return:
 *	YES: table is setup and ready for use
 *	NO : table full or alignment error
 *
 */
static __inline__ BOOL
PIIXSetupPRDTable(piix_prd_t *table, u_int table_size, vm_offset_t vaddr,
	u_int size, struct vm_map *map)
{
	vm_offset_t vaddr_next;
	vm_offset_t paddr;
	vm_offset_t paddr_next;
	vm_offset_t paddr_start;
	const char *name = "PIIXSetupPRDTable";
	u_int len_prd;
	u_int index;

#ifdef DEBUG	
	piix_prd_t *table_saved = table;
#endif DEBUG

	ddm_ide_dma("  PIIXSetupPRDTable: vaddr:%08x size:%d\n",
		(u_int)vaddr, (u_int)size, 3, 4, 5);

	if (vaddr & (PIIX_BUF_ALIGN - 1)) {
		IOLog("%s: buffer is not %d byte aligned\n", name, PIIX_BUF_ALIGN);
		return NO;
	}
	
	if (size == 0) {
		IOLog("%s: zero length DMA buffer\n", name);
		return NO;
	}
	
	index = len_prd = 0;
	paddr = PIIXVirtualToPhysical(map, vaddr);
	paddr_start = paddr;
	do {
		u_int len;

		vaddr_next = trunc_page(vaddr) + PAGE_SIZE;		// next virtual page
		paddr_next = trunc_page(paddr) + PAGE_SIZE;		// next phys page
		vaddr      = vaddr_next;
		
		len = paddr_next - paddr;		// length to transfer in this page
		if (len > size) len = size;		// take the minimum
		size  -= len;					// decrement total remaining bytes
		len_prd += len;					// increment current PRD counter

		/*
		 * If there are more bytes remaining, try to append the next
		 * page into the same PRD. We must check that the next page
		 * is physically contiguous with the current one.
		 *
		 * Each PRD cannot cross 64K boundary, and is limited to 64K per PRD.
		 */
		if (size && 
		(paddr_next == (paddr = PIIXVirtualToPhysical(map, vaddr))) &&
		((paddr_start & ~(PIIX_BUF_BOUND-1))==(paddr & ~(PIIX_BUF_BOUND-1))) &&
		(len_prd <= (PIIX_BUF_LIMIT - PAGE_SIZE))) {
		continue;
		}

		/*
		 * Setup PRD entry
		 *
		 * For the length field in PRD, 0 is used to denote the max
		 * transfer size of 64K.
		 */
		table->base = paddr_start;
		table->count = (len_prd == PIIX_BUF_LIMIT) ? 0 : len_prd;
		table->eot = 0;
		table++;

		len_prd = 0;
		paddr_start = paddr;

	} while (size && (++index < table_size));
	
	if (size) {
		IOLog("%s: PRD table exhausted\n", name);
		return NO;
	} 
	
	/*
	 * Set the 'end-of-table' bit on the last PRD entry.
	 */
	--table;
	table->eot = 1;

#ifdef DEBUG
	{
	int i = 0;
	u_int *ip = (u_int *)&table_saved[0];
	do {
		ddm_ide_dma("    table[%d]  %08x:%08x\n", i, *ip, *(ip+1), 4, 5);
		ip += 2;
	} while (i++ < index);
	}
#endif DEBUG

	return YES;
}

/*
 * Function: PIIXPrepareDMA
 *
 * Purpose:
 * Prepare the PIIX bus master for a DMA transfer.
 *
 * Arguments:
 *  piix_base - base address of the I/O space mapped bus master registers
 *  table     - points to the start of the PRD table
 *  tableAddr - physical address of the table
 *  isRead    - YES for read transfers (from device to host)
 */
static __inline__ BOOL
PIIXPrepareDMA(u_short piix_base, u_int tableAddr, BOOL isRead)
{
	piix_bmicx_u piix_cmd;
	piix_bmisx_u piix_status;

	/*
	 * Provide the starting address of the PRD table by loading the
	 * PRD Table Pointer Register.
	 *
	 * For some reason, outl(piix_base + PIIX_BMIDTPX, tableAddr)
	 * will only write the lower 16-bit WORD. That's why we use
	 * two outw instructions.
	 */	
	outw(piix_base + PIIX_BMIDTPX, tableAddr & 0xffff);
	outw(piix_base + PIIX_BMIDTPX + 2, (tableAddr >> 16) & 0xffff);

	/*
	 * Set the R/W bit depending on the direction of the transfer.
	 * The controller is also STOP'ed.
	 *
	 * Arghh!!! Why does the Intel PIIX3 and PIIX4 doc have this backwards?
	 */
	piix_cmd.byte = 0;
	piix_cmd.bits.rwcon = isRead ? 1 : 0;
	outb(piix_base + PIIX_BMICX, piix_cmd.byte);
	
	/*
	 * Clear interrupt and error bits in the Status Register.
	 */
	piix_status.byte = inb(piix_base + PIIX_BMISX);
	piix_status.bits.err = piix_status.bits.ideints = 1;
//	piix_status.bits.dma0cap = piix_status.bits.dma1cap = 1;
	outb(piix_base + PIIX_BMISX, piix_status.byte);
	
	return YES;
}

/*
 * Method: PIIXInit
 *
 * Purpose:
 * Initializes the PIIX controller.
 */
- (void) PIIXInit
{
	return (PIIXStopDMA(_bmRegs));
}

/*
 * Even if an interrupt is missed, consider the transfer operation
 * successful if the PIIX status flags says so.
 */
#define TRUST_PIIX	1

/*
 * Method: PIIXPerformDMA
 *
 * Purpose:
 * Program the PIIX controller to perform DMA READ/WRITE transfers
 * based on the transfer request ideIoReq. The entire transfer is
 * translated into one or more PRD entries in the PRD table. We will
 * get a single interrupt when the entire transfer is complete.
 *
 * Note:
 * The PIIX status register should have bit 2 and bit 0 set at the
 * conclusion of the transfer. This corresponds to the case when the
 * IDE device generated an interrupt and the size of the PRD is equal
 * to the IDE device transfer size.
 *
 * If bit 1 is set, meaning that the controller encountered a target
 * or master abort, it is very likely that we told it to DMA to/from
 * an invalid piece of memory. Perhaps due to an incorrect virtual
 * to physical map conversion.
 */
- (ide_return_t) performDMA:(ideIoReq_t *)ideIoReq
{
	ideRegsVal_t	*ideRegs = &(ideIoReq->regValues);
	ideRegsAddrs_t	*rp = &_ideRegsAddrs;
    unsigned char	status;
	piix_bmisx_u	piix_status;
	ide_return_t	rtn = IDER_SUCCESS;
	unsigned 		cmd = ideIoReq->cmd;

	ddm_ide_dma("DMA block:%d count:%d read:%d map:%d rp:%x\n",
		ideIoReq->block,
		ideIoReq->blkcnt,
		(ideIoReq->cmd == IDE_READ_DMA),
		ideIoReq->map,
		rp->data);

	if ((cmd != IDE_READ_DMA) && (cmd != IDE_WRITE_DMA)) {
		IOLog("%s: ideDmaRwCommon: unknown command %d\n", [self name], cmd);
		return IDER_REJECT;
	}

	/*
	 * wait for BSY = 0 and DRDY = 1
	 */
    rtn = [self waitForDeviceReady];
    if (rtn != IDER_SUCCESS) {
		IOLog("%s: drive not ready\n", [self name]);
		return (rtn);
    }

	/*
	 * Set up PRD descriptor table
	 */
	if (PIIXSetupPRDTable(_prdTable.ptr,
		PIIX_DT_BOUND/sizeof(piix_prd_t),
		(vm_offset_t)ideIoReq->addr,
		ideIoReq->blkcnt * IDE_SECTOR_SIZE,
		(struct vm_map *)ideIoReq->map) == NO) {
		return NO;
	}

	/*
	 * Prepare the PIIX controller for the current transfer.
	 */
	if (PIIXPrepareDMA(_bmRegs, _tablePhyAddr,
		(ideIoReq->cmd == IDE_READ_DMA)) == NO) {
		IOLog("%s: PIIXPrepareDMA error\n", [self name]);
		return IDER_CMD_ERROR;
	}
	
	/*
	 * Program the drive (task file).
	 * Recall that _driveNum must be set prior to calling logToPhys.
	 * This is already done in the method ideExecuteCmd which calls
	 * this method. testDMA also calls this method with _driveNum set.
	 */
	*ideRegs = [self logToPhys:ideIoReq->block numOfBlocks:ideIoReq->blkcnt];
    outb(rp->drHead,  ideRegs->drHead);
    outb(rp->sectNum, ideRegs->sectNum);
    outb(rp->sectCnt, ideRegs->sectCnt);
    outb(rp->cylLow,  ideRegs->cylLow);
    outb(rp->cylHigh, ideRegs->cylHigh);

	ddm_ide_dma(
		"DMA drHead:%02x sectNum:%02x sectCnt:%02x cylLow:%02x cylHigh:%02x\n",
		ideRegs->drHead,
		ideRegs->sectNum,
		ideRegs->sectCnt,
		ideRegs->cylLow,
		ideRegs->cylHigh);

	/*
	 * Issue DMA READ/WRITE command to drive.
	 */
//	[self enableInterrupts];
//	[self clearInterrupts];
    outb(rp->command, cmd);

	/*
	 * Start the PIIX bus master.
	 */
	PIIXStartDMA(_bmRegs);
	
	/* Wait for interrupt to signal the completion of the transfer.
	 */
    rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];	
	piix_status.byte = PIIXGetStatus(_bmRegs);
	PIIXStopDMA(_bmRegs);
	
#ifdef TRUST_PIIX
	if ((piix_status.byte & PIIX_STATUS_MASK) == PIIX_STATUS_OK) {

			if (rtn != IDER_SUCCESS) {
				/* Interrupt timed-out, but PIIX claims that transaction
				 * was completed without errors.
				 * This may require more testing. Always trust PIIX?
				 * First, read status from the drive.
				 */
				if ((rtn = [self waitForNotBusy]) != IDER_SUCCESS)
					return IDER_CMD_ERROR;
				status = inb(rp->status);
			}			 
#else
	if ((rtn == IDER_SUCCESS) && ((piix_status.byte & PIIX_STATUS_MASK) == 
		PIIX_STATUS_OK)) {
#endif TRUST_PIIX

		    if (status & (ERROR | WRITE_FAULT)) {
				[self getIdeRegisters:ideRegs Print:"DMA error"];
				return IDER_CMD_ERROR;
	    	}

	    	if (status & ERROR_CORRECTED) {
				IOLog("%s: Error during data transfer (corrected).\n", 
			    	[self name]);
		    }
	}
	else {
		[self getIdeRegisters:ideRegs Print:NULL];
		IOLog("%s: PIIX status:0x%02x error code:%d\n",
			[self name], piix_status.byte, rtn);
		rtn = IDER_CMD_ERROR;
	}

	ddm_ide_dma(
		"END drHead:%02x sectNum:%02x sectCnt:%02x cylLow:%02x cylHigh:%02x\n",
    	inb(rp->drHead),
    	inb(rp->sectNum),
    	inb(rp->sectCnt),
    	inb(rp->cylLow),
    	inb(rp->cylHigh));

	return (rtn);
}

#define MAX_BUSY_WAIT 				(1000*100)

/*
 * Perform DMA transfers for ATAPI devices.
 */
- (sc_status_t) performATAPIDMA:(atapiIoReq_t *)atapiIoReq
	buffer:(void *)buffer 
	client:(struct vm_map *)client
{
	piix_bmisx_u piix_status;
	unsigned char cmd = atapiIoReq->atapiCmd[0];
	ide_return_t	rtn;
    unsigned char	status;
	ideRegsAddrs_t	*rp = &_ideRegsAddrs;
	int	i;

	//IOLog("DMA transfer\n");

	atapiIoReq->bytesTransferred = 0;
	
	/*
	 * Set up PRD descriptor table
	 */
	if (PIIXSetupPRDTable(_prdTable.ptr,
		PIIX_DT_BOUND/sizeof(piix_prd_t),
		(vm_offset_t)buffer,
		atapiIoReq->maxTransfer,
		client) == NO)
		{
		atapiIoReq->scsiStatus = STAT_CHECK;
		return SR_IOST_CMDREJ;
	}

	if (PIIXPrepareDMA(_bmRegs, _tablePhyAddr, atapiIoReq->read) == NO) {
		IOLog("%s: PIIXPrepareDMA error\n", [self name]);
		atapiIoReq->scsiStatus = STAT_CHECK;
		return SR_IOST_CMDREJ;
	}

	/*
	 * Start the PIIX bus master.
	 */
	PIIXStartDMA(_bmRegs);

	if (atapiIoReq->timeout > IDE_INTR_TIMEOUT) {
		u_int current_timeout = [self interruptTimeOut];
		
		//IOLog("using SCSI timeout:%d\n", atapiIoReq->timeout);
		[self setInterruptTimeOut:atapiIoReq->timeout];
		rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
		[self setInterruptTimeOut:current_timeout];
	}
	else {
		rtn = [self ideWaitForInterrupt:cmd ideStatus:&status];
	}

	piix_status.byte = PIIXGetStatus(_bmRegs);
	PIIXStopDMA(_bmRegs);

	/*
	 * This is stupid but the Chinon drive fires off an interrupt first
	 * and then updates the status register. It appears that any drive
	 * based on Western Digital chipset will do this. At any rate, this
	 * code is harmless and should be left here. 
	 */
	for (i = 0; i < MAX_BUSY_WAIT; i++)	{
		if (status & BUSY)
			IODelay(10);
		else
			break;
		status = inb(_ideRegsAddrs.status);	
	}

#ifdef TRUST_PIIX
	if ((piix_status.byte & PIIX_STATUS_MASK) == PIIX_STATUS_OK) {
		if (rtn != IDER_SUCCESS) {
			/* Interrupt timed-out, but PIIX claims that transaction
			 * was completed without errors.
			 * This may require more testing. Always trust PIIX?
			 * First, read status from the drive.
			 */
			if ((rtn = [self waitForNotBusy]) != IDER_SUCCESS) {
				IOLog("%s: FATAL: ATAPI Drive: %d Command %x failed.\n", 
					[self name], _driveNum, atapiIoReq->atapiCmd[0]);
				[self getIdeRegisters:NULL Print:"ATAPI DMA"];
				IOLog("%s: transfer size: %d\n",
					[self name], atapiIoReq->maxTransfer);
				[self atapiSoftReset:_driveNum];
				atapiIoReq->scsiStatus = STAT_CHECK;
				return SR_IOST_CHKSNV;
			}
			status = inb(rp->status);
		}
#else
	if ((rtn == IDER_SUCCESS) && ((piix_status.byte & PIIX_STATUS_MASK) == 
		PIIX_STATUS_OK)) {
#endif TRUST_PIIX

		if (status & ERROR) {
			atapiIoReq->scsiStatus = STAT_CHECK;
			return SR_IOST_CHKSNV;	
		}
	}
	else {
		[self getIdeRegisters:NULL Print:"ATAPI DMA"];
		IOLog("%s: PIIX status:0x%02x error code:%d\n",
			[self name], piix_status.byte, rtn);
		IOLog("%s: transfer size: %d\n", [self name], atapiIoReq->maxTransfer);
		[self atapiSoftReset:_driveNum];
		atapiIoReq->scsiStatus = STAT_CHECK;
		return SR_IOST_CHKSNV;
	}

	atapiIoReq->bytesTransferred = atapiIoReq->maxTransfer;
	atapiIoReq->scsiStatus = STAT_GOOD;
	return SR_IOST_GOOD;
}

@end
