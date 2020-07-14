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

/* Sym8xxInit.m created by russb2 on Sat 30-May-1998 */

/*-----------------------------------------------------------------------------*
 * This module contains initialization routines for the driver.
 *
 * Driver initialization consists of:
 * 
 * - Doing PCI bus initialization for the script engine PCI device.
 * - Setting up shared communication areas in system memory between the script
 *   and the driver.
 * - Copying the script program into the script engine on-board ram, applying 
 *   script relocation fixups as required.
 * - Setting the initial register values for the script engine.
 * - Setting up driver related storage and interfacing with driverKit.
 *
 *-----------------------------------------------------------------------------*/

/*
 * This define causes Sym8xxScript.h to include the script instructions and
 * relocation tables. Normally without this define we only will get #define
 * values for interfacing with the script.
 */
#define INCL_SCRIPT_TEXT

#import "Sym8xxController.h"

/*-----------------------------------------------------------------------------*
 * This structure contains most of the inital register settings for
 * the script engine. See Sym8xxRegs.h for the actual initialization
 * values.
 *
 *-----------------------------------------------------------------------------*/
typedef struct ChipInitRegs
{
    u_int32_t		regNum;
    u_int32_t		regSize;
    u_int32_t		regValue;

} ChipInitRegs;

static ChipInitRegs 	Sym8xxInitRegs[] =
{
	{ SCNTL0,	SCNTL0_SIZE,	SCNTL0_INIT 	},
	{ SCNTL1,	SCNTL1_SIZE,    SCNTL1_INIT	},
        { SCNTL2,	SCNTL2_SIZE,	SCNTL2_INIT	},
        { SCNTL3,     	SCNTL3_SIZE,	SCNTL3_INIT_875	},
        { SXFER,	SXFER_SIZE, 	SXFER_INIT	},
        { SDID,		SDID_SIZE,	SDID_INIT	},
        { GPREG,	GPREG_SIZE,	GPREG_INIT	},
        { SFBR,		SFBR_SIZE,	SFBR_INIT	},
        { SOCL, 	SOCL_SIZE,	SOCL_INIT	},
        { DSA,		DSA_SIZE,	DSA_INIT	},
        { ISTAT,	ISTAT_SIZE,	ISTAT_INIT	},
        { TEMP,		TEMP_SIZE,	TEMP_INIT	},
        { CTEST0,	CTEST0_SIZE,	CTEST0_INIT	},
        { CTEST3,	CTEST3_SIZE,	CTEST3_INIT_A	},
        { CTEST4,	CTEST4_SIZE,	CTEST4_INIT	},
        { CTEST5,	CTEST5_SIZE,	CTEST5_INIT_A_revB},
        { DBC,		DBC_SIZE,	DBC_INIT	},
        { DCMD,		DCMD_SIZE,	DCMD_INIT	},
        { DNAD,		DNAD_SIZE,	DNAD_INIT	},
	{ DSPS,		DSPS_SIZE,	DSPS_INIT	},
	{ SCRATCHA,	SCRATCHA_SIZE,	SCRATCHA_INIT	},
        { DMODE,	DMODE_SIZE,	DMODE_INIT_A	},
        { DIEN,		DIEN_SIZE,	DIEN_INIT	},
        { DWT,		DWT_SIZE,	DWT_INIT	},
        { DCNTL,	DCNTL_SIZE,	DCNTL_INIT_A 	},
        { SIEN,		SIEN_SIZE,	SIEN_INIT	},
        { SLPAR,	SLPAR_SIZE,	SLPAR_INIT	},
        { MACNTL,	MACNTL_SIZE,	MACNTL_INIT	},
        { GPCNTL,	GPCNTL_SIZE,	GPCNTL_INIT	},
        { STIME0,	STIME0_SIZE,	STIME0_INIT	},
        { STIME1,	STIME1_SIZE,	STIME1_INIT	},
        { RESPID0,	RESPID0_SIZE,   RESPID0_INIT	},
        { RESPID1,    	RESPID1_SIZE,   RESPID1_INIT	},
        { STEST2,	STEST2_SIZE,	STEST2_INIT	},
        { STEST3,	STEST3_SIZE,	STEST3_INIT	},
        { SODL,		SODL_SIZE,	SODL_INIT	},
        { SCRATCHB,	SCRATCHB_SIZE,	SCRATCHB_INIT	}
};


@implementation Sym8xxController(Init)

/*-----------------------------------------------------------------------------*
 *  Probe, configure board and init new instance.
 *
 *-----------------------------------------------------------------------------*/
+ (BOOL)probe:(IOPCIDevice *)deviceDescription
{
    Sym8xxController	*sym8xx = [self alloc];

//  call_kdp();	
    return ([sym8xx initFromDeviceDescription: deviceDescription] ? YES : NO);		
}

/*-----------------------------------------------------------------------------*
 *
 *
 *-----------------------------------------------------------------------------*/
- initFromDeviceDescription:(IOPCIDevice *) deviceDescription
{

    if ( [super initFromDeviceDescription:deviceDescription] == nil)
    {
      goto abort;
    }

    if ( [self Sym8xxInit: deviceDescription] == NO )
    {
      goto abort;
    }

    /*
     *  
     */
    interruptPortKern = IOConvertPort(	[self interruptPort],
                                        IO_KernelIOTask,
                                        IO_Kernel);

    Sym8xxTimerReq( self );

    [self resetSCSIBus];

    /*
     *  Now that everything has succeeded, enable interrupts, and go,
     */
    [self registerDevice];	
    return self;

abort:
    [self free];
    return NULL;
}

/*-----------------------------------------------------------------------------*
 * Script Initialization
 *
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxInit:(IOPCIDevice *)deviceDescription
{
    /*
     * Perform PCI related initialization
     */
    if ( [self Sym8xxInitPCI: (IOPCIDevice *)deviceDescription] == NO )
    { 
        return NO;
    }

    /*
     * Allocate/initialize driver resources
     */
    if ( [self Sym8xxInitVars] == NO )
    {
        return NO;
    }

    /*
     * Initialize the script engine registers
     */
    if ( [self Sym8xxInitChip] == NO )
    {
        return NO;
    }

    /* 
     * Apply fixups to script and copy script to script engine's on-board ram
     */
    if ( [self Sym8xxInitScript] == NO )
    {
        return NO;
    }

    [self enableAllInterrupts];

    /*
     * Start script execution
     */
    Sym8xxWriteRegs( chipBaseAddr, DSP, DSP_SIZE, (u_int32_t) &chipRamAddrPhys[Ent_select_phase] );

    return YES;
}

/*-----------------------------------------------------------------------------*
 * Script engine PCI initialization
 *
 * This routine determines the chip version/revision, enables the chip address
 * ranges and allocates a virtual mapping to the script engine's registers and
 * on-board ram.
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxInitPCI: (IOPCIDevice *)deviceDescription
{
    IORange		*ioRange;
    unsigned long	pciReg0, pciReg8;
    u_int32_t		chipRev;
    u_int32_t		pciRamSlot=-1, pciRegSlot=-1;
    u_int32_t		n;

    /*
     * Determine the number of memory ranges for the PCI device.
     * 
     * The hardware implementation may or may not have a ROM present
     * accounting for the difference in the number of ranges.
     */
    n = [deviceDescription numMemoryRanges];
    if ( !( n == 3  ||  n == 4 )  )
    {
      return NO;
    }

    /*
     * Determine the hardware version. Check the deviceID and
     * RevID in the PCI config regs.
     */
    ioRange = [deviceDescription memoryRangeList];

    [deviceDescription configReadLong:0x00 value:&pciReg0];
    [deviceDescription configReadLong:0x08 value:&pciReg8];

    chipRev = pciReg8 & 0xff;

    if ( (pciReg0 & 0xf0000) == 0xf0000 )
    {
      chipType = 0x875;
    }
    else
    {
      chipType = 0x825;
      return NO;
    }

    kprintf( "SCSI(Symbios8xx): Chip type = %04x Chip rev = %02x\n\r", chipType, chipRev );

    /*
     * Assume 80Mhz external clock rate for motherboard 875 implementations
     * and 40Mhz for others.
     */
    if ( !strcmp([deviceDescription nodeName], "apple53C8xx") )
    {
      chipClockRate = CLK_80MHz;
    }
    else
    {
      chipClockRate = CLK_40MHz;
    }

    /*
     * BUS MASTER, MEM I/O Space, MEM WR & INV
     */
    [deviceDescription configWriteLong:0x04 value:0x16];

    /*
     *  set Latency to Max , cache 32
     */
    [deviceDescription configWriteLong:0x0C value:0x2008];

    switch ( n )
    {
      case 3:
        pciRegSlot = 2;
        pciRamSlot = 1;
        break;
      case 4:
        pciRegSlot = 3;
        pciRamSlot = 2;
    }

    /*
     * get chip register block mapped into pci memory
     */
    ioRange[pciRegSlot].size = round_page(ioRange[pciRegSlot].size);
    [deviceDescription setMemoryRangeList: ioRange num: n];
    ioRange = [deviceDescription memoryRangeList];

    chipBaseAddrPhys 	= (u_int8_t *)ioRange[pciRegSlot].start;
    [self mapMemoryRange: pciRegSlot to:(vm_address_t *)&chipBaseAddr findSpace:YES cache:IO_CacheOff];

    kprintf( "SCSI(Symbios8xx): Chip Base addr = %08x(p) %08x(v)\n\r", 
	     (u_int32_t)chipBaseAddrPhys, (u_int32_t)chipBaseAddr );

    chipRamAddrPhys 	= (u_int8_t *)ioRange[pciRamSlot].start;
    [self mapMemoryRange: pciRamSlot to:(vm_address_t *)&chipRamAddr findSpace:YES cache:IO_CacheOff];

    kprintf( "SCSI(Symbios8xx): Chip Ram  addr = %08x(p) %08x(v)\n\r",  
	     (u_int32_t)chipRamAddrPhys,  (u_int32_t)chipRamAddr );

    /*
     * Setup pointers to the script engine's registers and on-board ram for
     * inspection by the kernel mini-mon utility.
     */
#if 0
    if ( !gRegs875 )
    {
        gRegs875 	= (u_int32_t)chipBaseAddr; 
        gRegs875Phys	= (u_int32_t)chipBaseAddrPhys;
        gRam875  	= (u_int32_t)chipRamAddr;
        gRam875Phys 	= (u_int32_t)chipRamAddrPhys;
    }
#endif

    return YES;
}


/*-----------------------------------------------------------------------------*
 * This routine allocates/initializes shared memory for communication between 
 * the script and the driver. In addition other driver resources semaphores, 
 * queues are initialized here.
 *
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxInitVars
{
    kern_return_t		kr;
    u_int32_t			i;

    kr = kmem_alloc_wired(IOVmTaskSelf(), (vm_offset_t *) &adapter, page_size );
    if ( kr != KERN_SUCCESS )
    {
        return NO;
    }
    bzero( adapter, page_size );

    /*
     * We keep two copies of the Nexus pointer array. One contains physical addresses and
     * is located in the script/driver shared storage. The other copy holds the corresponding
     * virtual addresses to the active Nexus structures and is located in the drivers instance
     * data.
     * Both tables can be accessed through indirect pointers in the script/driver communication
     * area. This is the preferred method to access these arrays.
     */ 
    adapter->nexusPtrsVirt = (Nexus **)nexusArrayVirt;
    adapter->nexusPtrsPhys = (Nexus **)adapter->nexusArrayPhys;

    for (i=0; i < MAX_SCSI_TAG; i ++ )
    {
        adapter->nexusPtrsVirt[i] = (Nexus *) -1;
        adapter->nexusPtrsPhys[i] = (Nexus *) -1;
    }

    IOPhysicalFromVirtual((vm_task_t)IOVmTaskSelf(), (vm_offset_t)adapter, (vm_offset_t *)&adapterPhys );
 
    /*
     * The script/driver communication area also contains a 16-entry table clock
     * settings for each target.
     */ 
    for (i=0; i < MAX_SCSI_TARGETS; i++ )
    {
        targets[i].targetTagSem = [[NXLock alloc] init];
        targets[i].flags        = kTFXferSyncAllowed | kTFXferWide16Allowed | kTFCmdQueueAllowed;

        adapter->targetClocks[i].scntl3Reg = SCNTL3_INIT_875;
    }

    /*
     * Allocate/init various driver semaphores
     */
    cmdQTagSem      = [[NXLock alloc] init];
    abortBdrSem     = [[NXLock alloc] init];
    resetQuiesceSem = [[NXLock alloc] init];

    
    /*
     *  Initialize the IOThread command queue.
     */
    queue_init(&srbPendingQ);
    srbPendingQLock = [[NXLock alloc] init];

    /*
     * Initialize the SRB Pool   
     */
    queue_init(&srbPool);
    srbPoolLock = [[NXLock alloc] init];

    /*
     * Reserve our host-adapter ID so the driverKit SCSI classes do not probe it.  
     */
    for ( i = 0; i < MAX_SCSI_LUNS; i++ )
    {
        [ self reserveTarget: kHostAdapterSCSIId lun: i forOwner: self ];
    }

    /*
     * Start up the SRB Pool grow thread.
     */
    srbPoolGrowLock = [[NXConditionLock alloc] initWith: kSRBGrowPoolIdle];
    IOForkThread( (IOThreadFunc) Sym8xxGrowSRBPool, (void *)self );

    return YES;
}


/*-----------------------------------------------------------------------------*
 * This routine makes a temporary copy of the script program, applies script fixups,
 * initializes the script local data table at the top of the script image, and
 * copies the modified script image to the script engine's on-board ram.
 *
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxInitScript
{
    u_int32_t	 	i;
    u_int32_t		scriptPgm[sizeof(BSC_SCRIPT)/sizeof(u_int32_t)];

    /*
     * Make a copy of the script
     */
    bcopy( BSC_SCRIPT, scriptPgm, sizeof(scriptPgm) );
    bzero( scriptPgm, R_ld_size );

    /*
     * Apply fixups to the script copy
     */
    for ( i=0; i < sizeof(Rel_Patches)/sizeof(u_int32_t); i++ )
    {
        scriptPgm[Rel_Patches[i]] += (u_int32_t)chipRamAddrPhys;
    }
    for ( i=0; i < sizeof(LABELPATCHES)/sizeof(u_int32_t); i++ )
    {
        scriptPgm[LABELPATCHES[i]] += (u_int32_t)chipRamAddrPhys;
    }
 
    /*
     * Initialize the script working variables with pointers to the script/driver
     * communications area.
     */
    scriptPgm[R_ld_sched_mlbx_base_adr >> 2] 	= (u_int32_t)&adapterPhys->schedMailBox;
    scriptPgm[R_ld_nexus_array_base >> 2]    	= (u_int32_t)&adapterPhys->nexusArrayPhys;
    scriptPgm[R_ld_device_table_base_adr >> 2] 	= (u_int32_t)&adapterPhys->targetClocks;

    /*
     * Load the script image into the script engine's on-board ram.
     */
    [self Sym8xxLoadScript: scriptPgm count:sizeof(scriptPgm)/sizeof(u_int32_t)];

    return YES;
}


/*-----------------------------------------------------------------------------*
 * This routine transfers the script program image into the script engine's
 * on-board ram
 *
 *-----------------------------------------------------------------------------*/
- (void) Sym8xxLoadScript:(u_int32_t *)scriptPgm count:(u_int32_t)scriptWords
{
    u_int32_t			i;
    volatile u_int32_t		*ramPtr = (volatile u_int32_t *)chipRamAddr;

    for ( i = 0; i < scriptWords; i++ )
    {
        ramPtr[i] = EndianSwap32(scriptPgm[i]);
    }
}

/*-----------------------------------------------------------------------------*
 * This routine initializes the script engine's register block.
 *
 *-----------------------------------------------------------------------------*/
- (BOOL) Sym8xxInitChip
{
    u_int32_t			i;

    /*
     * Reset the script engine
     */
    Sym8xxWriteRegs( chipBaseAddr, ISTAT, ISTAT_SIZE, RST );
    IODelay( 25 );
    Sym8xxWriteRegs( chipBaseAddr, ISTAT, ISTAT_SIZE, ISTAT_INIT );
  
    /*
     * Load our canned register values into the script engine
     */
    for ( i = 0; i < sizeof(Sym8xxInitRegs)/sizeof(ChipInitRegs); i++ )
    {
        Sym8xxWriteRegs( chipBaseAddr, Sym8xxInitRegs[i].regNum, Sym8xxInitRegs[i].regSize, Sym8xxInitRegs[i].regValue );
        IODelay( 10 );
    }

    /*
     * For hardware implementations that have a 40Mhz SCLK input, we enable the chip's on-board
     * clock doubler to bring the clock rate upto 80Mhz which is required for Ultra-SCSI timings.
     */
    if ( chipType == 0x875 )
    {
        if ( chipClockRate == CLK_40MHz )
        {
            /*
             *   Clock doubler setup for 875 (rev 3 and above).
             */
            /* set clock doubler enabler bit */
            Sym8xxWriteRegs( chipBaseAddr, STEST1, STEST1_SIZE, STEST1_INIT | DBLEN);
            IODelay(30);  
            /* halt scsi clock */
            Sym8xxWriteRegs( chipBaseAddr, STEST3, STEST3_SIZE, STEST3_INIT | HSC );
            IODelay(10);
            Sym8xxWriteRegs( chipBaseAddr, SCNTL3, SCNTL3_SIZE, SCNTL3_INIT_875);
            IODelay(10);
            /* set clock doubler select bit */
            Sym8xxWriteRegs( chipBaseAddr, STEST1, STEST1_SIZE, STEST1_INIT | DBLEN | DBLSEL);
            IODelay(10);
            /* clear hold on scsi clock */
            Sym8xxWriteRegs( chipBaseAddr, STEST3, STEST3_SIZE, STEST3_INIT);
        }
    }
    else
    {
        Sym8xxWriteRegs( chipBaseAddr, SCNTL3, SCNTL3_SIZE, SCNTL3_INIT);
    }

    /*  
     * Set our host-adapter ID in the script engine's registers
     */
    initiatorID = kHostAdapterSCSIId;

    if ( initiatorID > 7 )
    {
        Sym8xxWriteRegs( chipBaseAddr, RESPID1, RESPID1_SIZE, 1 << (initiatorID-8));
    }
    else
    {
        Sym8xxWriteRegs( chipBaseAddr, RESPID0, RESPID0_SIZE, 1 << initiatorID);
    }

    Sym8xxWriteRegs( chipBaseAddr, SCID, SCID_SIZE, SCID_INIT | initiatorID );

    return YES;
}


@end
