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
#include <powermac.h>
#include <powermac_gestalt.h>
#include <machdep/ppc/proc_reg.h>
#include <mach/ppc/vm_types.h>
#include <sys/systm.h>
#include <string.h>
#include <machdep/ppc/DeviceTree.h>
#include <families/powersurge.h>
#include <families/powerexpress.h>
#include <families/gossamer.h>
#include <families/powerstar.h>
#include <families/yosemite.h>
#include "IOProperties.h"

/* Local declarations */
vm_offset_t	get_io_base_addr(void);
vm_offset_t	get_mem_cntlr_base_addr(void);
vm_offset_t	get_int_cntlr_base_addr(void);
vm_offset_t	get_dma_offset();
vm_offset_t	get_serial_offset();
vm_offset_t	get_scsi_int_offset();
vm_offset_t	get_scsi_int_dma_offset();
vm_offset_t	get_scsi_ext_offset();
vm_offset_t	get_audio_offset();
vm_offset_t	get_floppy_offset();
vm_offset_t	get_ethernet_offset();
vm_offset_t	get_via_offset();
vm_offset_t	get_nvram_addr_offset();
vm_offset_t	get_nvram_data_offset();
vm_offset_t	get_ide_offset(int channel);
int		get_clock_frequency(void);
int		get_machine_id(void);
void		InitNVRAMPartitions(void);


/* Globals */
char		cpu_model[65];	// used in bsd
char		machine[65];	// used in bsd

/* External declarations */
extern boot_args	our_boot_args;

/* External Functions
extern void DetermineClockSpeeds(void);
extern int  InitBacksideL2(void);
extern IOReturn ReadNVRAM( unsigned int offset, unsigned int length, unsigned char * buffer );


/* identify_machine1:
 *
 *   Sets up platform class.
 *   Returns:    nothing
 */
void identify_machine1()
{
	/* Everything starts out zeroed... */
	bzero((void*) &powermac_info,           sizeof(powermac_info_t));

	/* 
	 * First, find out the gestalt number for this machine.
	 */

	powermac_info.machine               = get_machine_id();

	/* 
	 * Set up the machine class values based on the
	 * gestalt number.
	 */
	switch (powermac_info.machine) {
	case gestaltPowerMac7500:
	case gestaltPowerMac8500:
	case gestaltPowerMac9500:
		powermac_info.class		= POWERMAC_CLASS_POWERSURGE;
		powermac_io_info.io_size	= GRAND_CENTRAL_SIZE;
		powermac_init_p = &powersurge_init;
		break;

	case gestaltPowerExpress:
		powermac_info.class		= POWERMAC_CLASS_POWEREXPRESS;
		powermac_io_info.io_size	= HEATHROW_SIZE;
		powermac_init_p = &powerexpress_init;
		break;

	case gestaltGossamer:
	case gestaltWallstreet:
		powermac_info.class		= POWERMAC_CLASS_GOSSAMER;
		powermac_io_info.io_size	= HEATHROW_SIZE;
		powermac_init_p = &gossamer_init;
		break;

	case gestaltPowerMac5400:
	case gestaltPowerMac5500:
	case gestaltPowerBook2400:
	case gestaltPowerBook3400:
	case gestaltKanga:
		powermac_info.class		= POWERMAC_CLASS_POWERSTAR;
		powermac_io_info.io_size	= OHARE_SIZE;
		powermac_init_p = &powerstar_init;
		break;
	  
	case gestaltCHRP_Version1:
		powermac_info.class		= POWERMAC_CLASS_YOSEMITE;
		powermac_io_info.io_size	= HEATHROW_SIZE;
		powermac_init_p = &yosemite_init;
		break;

	default:
		// If it gets here there will be no output for the panic.
		panic("Bad gestalt number.\n");
	}

	/* Setup the pointer to the dbdma channel numbers */
	powermac_dbdma_channels = powermac_init_p->powermac_dbdma_channels;
}

/* identify_machine2:
 *
 *   Sets up platform parameters.
 *   Returns:    nothing
 */
void identify_machine2()
{
	extern int init_powermac_machine_info(void);

        unsigned int dec_rate;
        union {
	  unsigned long fixed;
	  unsigned char top;
	} tmp_fixed;

	/*
	 *  Look up as much of the I/O address from the Device Tree
	 *  as possible.
	 */

	powermac_io_info.io_base_phys       = get_io_base_addr();
	powermac_io_info.io_base_virt       = powermac_io_info.io_base_phys;  // starts off as 1-1

	powermac_io_info.mem_cntlr_base_phys = get_mem_cntlr_base_addr();
	powermac_io_info.int_cntlr_base_phys = get_int_cntlr_base_addr();

	powermac_io_info.dma_base_phys      = powermac_io_info.io_base_phys + get_dma_offset();
	powermac_io_info.serial_base_phys   = powermac_io_info.io_base_phys + get_serial_offset();
	powermac_io_info.scsi_int_base_phys = powermac_io_info.io_base_phys + get_scsi_int_offset();
	powermac_io_info.scsi_int_dma_base_phys = powermac_io_info.io_base_phys + get_scsi_int_dma_offset();
	powermac_io_info.scsi_ext_base_phys = powermac_io_info.io_base_phys + get_scsi_ext_offset();
	powermac_io_info.audio_base_phys    = powermac_io_info.io_base_phys + get_audio_offset();
	powermac_io_info.floppy_base_phys   = powermac_io_info.io_base_phys + get_floppy_offset();
	powermac_io_info.ethernet_base_phys = powermac_io_info.io_base_phys + get_ethernet_offset();
	powermac_io_info.via_base_phys      = powermac_io_info.io_base_phys + get_via_offset();
	powermac_io_info.nvram_addr_reg_phys = powermac_io_info.io_base_phys + get_nvram_addr_offset();
	powermac_io_info.nvram_data_reg_phys = powermac_io_info.io_base_phys + get_nvram_data_offset();
	powermac_io_info.ide0_base_phys = powermac_io_info.io_base_phys + get_ide_offset(0);
	powermac_io_info.ide1_base_phys = powermac_io_info.io_base_phys + get_ide_offset(1);


	/* initialize the powermac_machine_info */
	bzero((void *)&powermac_machine_info, sizeof(powermac_machine_info_t));
	init_powermac_machine_info();

	/* Do the clock speed initialization stuff. */
	DetermineClockSpeeds();

	dec_rate = powermac_info.bus_clock_rate_hz / 4;

	powermac_info.proc_clock_to_nsec_numerator   = 1000 * 4;
	powermac_info.proc_clock_to_nsec_denominator = powermac_info.bus_clock_rate_hz / 1000000;

	/* Do the lower 24 bits (ie the remainder * 1_Fixed / dec_rate) */
	tmp_fixed.fixed =
	  ((long long)(1000000000 % dec_rate) << 24) / dec_rate;

	/* Do the top 8 bits (ie the whole part of the number) */
	tmp_fixed.top = 1000000000 / dec_rate;

	powermac_info.dec_clock_period = tmp_fixed.fixed;

	/* The cache initialization stuff. */
	InitBacksideL2();

	InitNVRAMPartitions();
}

/* get_io_base_addr():
 *
 *   Get the base address of the io controller.  
 */
vm_offset_t get_io_base_addr(void)
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;

	if ((DTFindEntry("device_type", "dbdma", &entryP) == kSuccess) ||
	    (DTFindEntry("device_type", "mac-io", &entryP) == kSuccess))
	{
	    if (DTGetProperty(entryP, "AAPL,address", (void **)&address, &size) == kSuccess)
		    return(*address);

	    if (DTGetProperty(entryP, "assigned-addresses", (void **)&address, &size) == kSuccess)
		    return(*(address+2));
	}

	panic("Uhmmm.. I can't get this machine's io base address\n");
	return(kError);
}

/* get_mem_cntlr_base_addr():
 *
 *   Get the base address of the memory controller.  
 */
vm_offset_t get_mem_cntlr_base_addr(void)
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if ((DTFindEntry("name", "hammerhead", &entryP) == kSuccess ||
	     DTFindEntry("name", "fatman",     &entryP) == kSuccess))
	{
	    if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);
	}
	
	return(0);
}

/* get_int_cntlr_base_addr():
 *
 *   Get the base address of the interrupt controller.  
 */
vm_offset_t get_int_cntlr_base_addr(void)
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if (DTFindEntry("device_type", "open-pic", &entryP) == kSuccess)
	{
	    if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);
	}

	return(0);
}

/* get_dma_offset():
 */
vm_offset_t get_dma_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if ((DTFindEntry("device_type", "dbdma", &entryP) == kSuccess) ||
	    (DTFindEntry("device_type", "mac-io", &entryP) == kSuccess))
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);
	
	panic("Uhmmm.. I can't get this machine's dma offset\n");
	return(kError);
}

/* get_serial_offset():
 */
vm_offset_t get_serial_offset()
{
	DTEntry 	entryP;


	if ((DTFindEntry("device_type", "escc", &entryP) == kSuccess) ||
	    (DTFindEntry("device_type", "serial", &entryP) == kSuccess))
	  return(0x12000);	// OF is wrong for some reason

	panic("Uhmmm.. I can't get this machine's serial offset\n");
	return(kError);
}

/* get_scsi_int_offset():
 */
vm_offset_t get_scsi_int_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if ((DTFindEntry("name", "mesh", &entryP) == kSuccess) ||
	    (DTFindEntry("compatible", "chrp,mesh0", &entryP) == kSuccess))
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);
	
	panic("Uhmmm.. I can't get this machine's scsi internal offset\n");
	return(kError);
}

/* get_scsi_int_dma_offset():
 */
vm_offset_t get_scsi_int_dma_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if ((DTFindEntry("name", "mesh", &entryP) == kSuccess) ||
	    (DTFindEntry("compatible", "chrp,mesh0", &entryP) == kSuccess))
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*(address+2));
	
	panic("Uhmmm.. I can't get this machine's scsi internal dma offset\n");
	return(kError);
}

/* get_scsi_ext_offset():
 */
vm_offset_t get_scsi_ext_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if (DTFindEntry("name", "53c94", &entryP) == kSuccess)
	    if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);
	
	return(0);
}

/* get_audio_offset():
 */
vm_offset_t get_audio_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if ((DTFindEntry("device_type", "davbus",  &entryP) == kSuccess) ||
	    (DTFindEntry("device_type", "sound", &entryP) == kSuccess))
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);

	panic("Uhmmm.. I can't get this machine's audio offset\n");
	return(kError);
}

/* get_floppy_offset():
 */
vm_offset_t get_floppy_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if ((DTFindEntry("AAPL,connector", "floppy", &entryP) == kSuccess) ||
	    (DTFindEntry("AAPL,connector", "swim",   &entryP) == kSuccess) ||
	    (DTFindEntry("device_type", "swim3",   &entryP) == kSuccess))
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);

	panic("Uhmmm.. I can't get this machine's floppy offset\n");
	return(kError);
}

/* get_ethernet_offset():
 */
vm_offset_t get_ethernet_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if ((DTFindEntry("AAPL,connector", "ethernet", &entryP) == kSuccess) ||
	    (DTFindEntry("network-type", "ethernet", &entryP) == kSuccess))
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);

	return(0);
}

/* get_via_offset():
 */
vm_offset_t get_via_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int             size, viaFound = 0;

	if ((DTFindEntry("device_type", "cuda", &entryP) == kSuccess) ||
	    (DTFindEntry("device_type", "via-cuda", &entryP) == kSuccess)) {
	  powermac_info.hasPMU = 0;
	  viaFound = 1;
	}

	if ((DTFindEntry("device_type", "pmu", &entryP) == kSuccess) ||
	    (DTFindEntry("device_type", "via-pmu", &entryP) == kSuccess)) {
	  powermac_info.hasPMU = 1;
	  viaFound = 1;
	}

	if (viaFound &&
	    (DTGetProperty(entryP, "reg", (void **)&address, &size) ==
	     kSuccess)) {
	    return(*address);
	}
	
	panic("Uhmmm.. I can't get this machine's via offset\n");
	return(kError);
}

/* get_nvram_addr_offset():
 */
vm_offset_t get_nvram_addr_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if (DTFindEntry("device_type", "nvram", &entryP) == kSuccess)
	    if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*address);

	return(0);
}

/* get_nvram_data_offset():
 */
vm_offset_t get_nvram_data_offset()
{
	DTEntry 	entryP;
	vm_offset_t 	*address;
	int		size;


	if (DTFindEntry("device_type", "nvram", &entryP) == kSuccess)
	    if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess)
		    return(*(address + 2));
	
	return(0);
}

/* get_ide_offset():
 */
vm_offset_t get_ide_offset(int channel)
{
	DTEntry 	entryP;
	vm_offset_t 	*address, offset;
	int		size;


	if (DTFindEntry("device_type", "ide", &entryP) == kSuccess) {
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess) {
	    offset = *address;
	    
	    // Wallstreet has ata1 first...
	    if (powermac_info.machine == gestaltWallstreet) {
	      if (channel == 0) offset -= 0x1000;
	    } else {
	      if (channel == 1) offset += 0x1000;
	    }
	    return offset;
	  }
	} else if (DTFindEntry("device_type", "ata", &entryP) == kSuccess) {
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess) {
	    /* Kanga has IDE1 first in the tree... dohhh */	    
	    offset = *address;
	    if (channel == 0) offset -= 0x1000;
	    return offset;
	  }
	} else if (DTFindEntry("device_type", "ATA", &entryP) == kSuccess) {
	  if (DTGetProperty(entryP, "reg", (void **)&address, &size) == kSuccess) {
	    offset = *address;
	    if (channel == 1) offset = 0;
	    return offset;
	  }
	}

	return(0);
}

int get_clock_frequency()
{
	DTEntry		entry;
	int		*freq;
	int		size;

	if (DTLookupEntry(0, "/", &entry) == kSuccess)
	    if (DTGetProperty(entry, "clock-frequency", (void **)&freq, &size) == kSuccess)
		    return(*freq);

	return(40000000);		// 45 MHz default
}

int
init_powermac_machine_info(void)
{
    DTEntry     entry;
	int     *value;
	int     size;

	value = size = 0;

	if (DTFindEntry("device_type", "cpu", &entry) == kSuccess) {
		if (DTGetProperty(entry, "d-cache-block-size",
			(void **)&value, &size) == kSuccess)
			powermac_machine_info.dcache_block_size = *value;

		if (DTGetProperty(entry, "d-cache-size",
			(void **)&value, &size) == kSuccess)
			powermac_machine_info.dcache_size = *value;

		if (DTGetProperty(entry, "i-cache-size",
			(void **)&value, &size) == kSuccess)
			powermac_machine_info.icache_size = *value;

		if (DTGetProperty(entry, "cpu-version",
			(void **)&value, &size) == kSuccess)
			powermac_machine_info.processor_version = *value;

		// These are calculated by test_clock()
#if 0
		if (DTGetProperty(entry, "clock-frequency",
			(void **)&value, &size) == kSuccess)
			powermac_machine_info.cpu_clock_rate_hz = *value;

		if (DTGetProperty(entry, "timebase-frequency",
			(void **)&value, &size) == kSuccess)
			powermac_machine_info.dec_clock_rate_hz = *value;
#endif
	} else
		return 1;

	if (DTFindEntry("device_type", "cache", &entry) == kSuccess) {
		if (DTGetProperty(entry, "cache-unified",
			(void **)&value, &size) == kSuccess)
			powermac_machine_info.caches_unified = 1;
	} else
		return 1;

	return 0;
}

/* get_machine_id():
 *
 *   get the gestalt ID of the machine.  Currently we map from the OF "compatable" string, so
 *   so we really only get the family, but it could also check the clock frequency and other 
 *   hw to narrow it down.  
 *   Returns:    gestaltID
 *               kError = unknown machine
 */
int get_machine_id(void)
{
	DTEntry entry;
	char 	*family;
	int	size, cnt;

	if (DTLookupEntry(0, "/", &entry) == kSuccess)
	{
	    DTGetProperty(entry, "compatible", (void **)&family, &size);

	    // Convert all "/"'s in family to "-"'s
	    for (cnt = 0; family[cnt] != '\0'; cnt++) {
	      if (family[cnt] == '/') family[cnt] = '-';
	      if (family[cnt] == ' ') family[cnt] = '-';
	    }

	    // these are globals currently needed by the kernel

	    if (strncmp(cpu_model, "AAPL,",  5) == 0)
	      strcpy(cpu_model, family+5);
	    else
	      strcpy(cpu_model, family);

	    strcpy(machine, "Power Macintosh");
	    
	    if      (strcmp( family, "AAPL,6100") == 0)		return(gestaltPowerMac6100_60);
	    else if (strcmp( family, "AAPL,7100") == 0)		return(gestaltPowerMac7100_66);
	    else if (strcmp( family, "AAPL,8100") == 0)		return(gestaltPowerMac8100_80);

	    else if (strcmp( family, "AAPL,7300") == 0)		return(gestaltPowerMac7300);
	    else if (strcmp( family, "AAPL,7500") == 0)		return(gestaltPowerMac7500);
	    else if (strcmp( family, "AAPL,8500") == 0)		return(gestaltPowerMac8500);
	    else if (strcmp( family, "AAPL,9500") == 0)		return(gestaltPowerMac9500);

	    else if (strcmp( family, "AAPL,9700") == 0)		return(gestaltPowerExpress);

	    else if (strcmp( family, "AAPL,Gossamer") == 0)	return(gestaltGossamer);
	    else if (strcmp( family, "AAPL,PowerMac-G3") == 0)	return(gestaltGossamer);
	    else if (strcmp( family, "AAPL,PowerBook1998") == 0)	return(gestaltWallstreet);

	    else if (strcmp( family, "AAPL,e407") == 0)	return(gestaltPowerMac5400);
	    else if (strcmp( family, "AAPL,e411") == 0)	return(gestaltPowerMac5500);
	    else if (strcmp( family, "AAPL,3400-2400") == 0)	return(gestaltPowerBook3400);
	    else if (strcmp( family, "AAPL,3500") == 0)	return(gestaltKanga);
	    else if (strcmp( family, "iMac") == 0)	return(gestaltCHRP_Version1);
	    else if (strcmp( family, "PowerMac1,1") == 0)	return(gestaltCHRP_Version1);
	    else if (strcmp( family, "PowerBook1,1") == 0)	return(gestaltCHRP_Version1);
	    else
		panic("Unsupported machine\n");
	}

	panic("Uhmmm.. I can't get this machine's id\n");
	return(kError);		// kError = unknown
}


void identify_via_irq(void)
{
  DTEntry     entryP;
  int         size, *irq;

  if ((DTFindEntry("device_type", "cuda", &entryP) == kSuccess) ||
      (DTFindEntry("device_type", "via-cuda", &entryP) == kSuccess) ||
      (DTFindEntry("device_type", "pmu", &entryP) == kSuccess) ||
      (DTFindEntry("device_type", "via-pmu", &entryP) == kSuccess))
    if ((DTGetProperty(entryP, "AAPL,interrupts", (void **)&irq, &size) == kSuccess) ||
	(DTGetProperty(entryP, "interrupts", (void **)&irq, &size) == kSuccess))
      *irq = powermac_info.viaIRQ ^ 0x18;
}

int set_ethernet_irq(int newIRQ)
{
  DTEntry     entryP;
  int         size, *irq, hasOHare2 = 0;

  if ((DTFindEntry("name", "pci106b,7", &entryP) == kSuccess)) {
    hasOHare2 = 1;
  }

  if ((DTFindEntry("name", "pci1011,14", &entryP) == kSuccess))
    if ((DTGetProperty(entryP, "AAPL,interrupts", (void **)&irq, &size) == kSuccess) ||
	(DTGetProperty(entryP, "interrupts", (void **)&irq, &size) == kSuccess)) {
      if (!hasOHare2) {
	newIRQ = *irq ^ 0x18;
      }
      
      *irq = newIRQ ^ 0x18;
    }

    return hasOHare2; 
}

/* DriverKit calls this for PExpert to change DT entries
 * before they are used by DK
 *
 * index is an iterator over properties in one DTEntry.
 * propData is copied by reference. Return nil to delete.
 */

int PEEditDTEntry( DTEntry dtEntry, char * nodeName, int index,
                    char ** propName , void ** propData, int * propSize)
{
    int		err = -1;
    extern int  kdp_flag;
    DTEntry	entry;

    if( 0 == strcmp( nodeName, "53c94")) switch( index) {

	case 0:
            *propName = IOPropConnectorName;
	    *propSize = 1 + strlen("External");
	    *propData = "External";
	    err = 0;
	    break;

    } else if( 0 == strcmp( nodeName, "mesh")) switch( index) {

	case 0:
            *propName = IOPropConnectorName;
	    *propSize = 1 + strlen("External");
	    if( powermac_info.class == POWERMAC_CLASS_POWERSURGE)
                *propData = "Internal";
	    else if( kSuccess == DTFindEntry("device_type", "pmu", &entry) )	// is PBook?
                *propData = "External";
	    else {
                *propSize = 1 + strlen("Internal/External");			// G3
                *propData = "Internal/External";
	    }
	    err = 0;
	    break;

    } else if( 
	   (0 == strcmp( nodeName, "chips65550")) 
	|| (0 == strcmp( nodeName, "pci1011,14")) 
	|| (0 == strcmp( nodeName, "pci106b,7"))
	|| (0 == strncmp( nodeName, "ATY,mach64_3DU", strlen("ATY,mach64_3DU")))) switch( index) {

	case 0:
	{
            static int zero;
                *propName = "AAPL,slot-name";
                *propData = &zero;
                *propSize = 3;
                err = 0;
                break;
	}
    } else if( (kdp_flag & 2) && (0 == strcmp( nodeName, "ch-a"))) switch( index) {

	case 0:
            *propName = *propData = "AAPL,ignore";
	    *propSize = 0;
	    err = 0;
	    break;

    } else if( 0 == strcmp( nodeName, "device-tree")) switch( index) {

	case 0:
            *propName = IOPropMachineClass;
	    *propSize = sizeof( int);
	    *propData = &powermac_info.class;
	    err = 0;
	    break;

	case 1:
            *propName = IOPropMachineGestalt;
	    *propSize = sizeof( int);
	    *propData = &powermac_info.machine;
	    err = 0;
	    break;

	case 2:
	{
	    static IOSlotConfiguration sixVSlotConfig = {
		0, IO_SLOTS_VERTICAL, IO_SLOTS_TOP2BOTTOM, 6
	    };
	    static IOSlotConfiguration threeVSlotConfig = {
		0, IO_SLOTS_VERTICAL, IO_SLOTS_TOP2BOTTOM, 3
	    };
	    static IOSlotConfiguration threeHSlotConfig = {
		0, IO_SLOTS_HORIZONTAL, IO_SLOTS_LEFT2RIGHT, 3
	    };
	    static IOSlotConfiguration threeQSlotConfig = {
		0, IO_SLOTS_UNKNOWN, IO_SLOTS_LEFT2RIGHT, 3
	    };

            *propName = IOPropSlotConfiguration;
	    *propSize = sizeof( int);

	    switch( powermac_info.machine) {

                case gestaltGossamer:
                    *propData = &threeQSlotConfig;
		    break;

                case gestaltPowerMac7500:
                    *propData = &threeHSlotConfig;
		    break;

                case gestaltPowerMac8500:
                    *propData = &threeVSlotConfig;
		    break;

                case gestaltPowerMac9500:
                case gestaltPowerExpress:
                    *propData = &sixVSlotConfig;
		    break;
	    }
	    err = 0;
	    break;
	}
    }

    return( err);
}

void InitNVRAMPartitions(void)
{
  long curOffset = 0;
  char buf[17];

  if (IsYosemite()) {
    // Look at the NVRAM partitons and find the right ones.

    buf[16] = '\0';
    while (curOffset < 0x2000) {
      ReadNVRAM(curOffset, 16, buf);

      if (strcmp(buf + 4, "common") == 0) {
	NVRAM_OpenFirmware_Offset = curOffset + 16;
      } else if (strcmp(buf + 4, "APL,MacOS75") == 0) {
	NVRAM_XPRAM_Offset = curOffset + 16;
	NVRAM_NameRegistry_Offset = NVRAM_XPRAM_Offset + 0x100;
      }
      
      curOffset += ((short *)buf)[1] * 16;
    }
  } else {
    // Fixed offset for legacy machines.
    NVRAM_XPRAM_Offset = 0x1300;
    NVRAM_NameRegistry_Offset = 0x1400;
    NVRAM_OpenFirmware_Offset = 0x1800;
  }
}
