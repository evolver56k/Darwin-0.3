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
#ifndef _POWERMAC_H_
#define _POWERMAC_H_

#ifndef __ASSEMBLER__

#include <mach/ppc/vm_types.h>
#include <machdep/ppc/boot.h>
#include <machdep/ppc/dbdma.h>

/*
 * Machine class 
 */

#define	POWERMAC_CLASS_PDM		1	/* PowerMac 61/71/8100 */
#define	POWERMAC_CLASS_PERFORMA		2	/* PowerMac 52/6200 */
#define	POWERMAC_CLASS_POWERSURGE	3	/* PowerMac 72/75/85/9500 */
#define	POWERMAC_CLASS_POWEREXPRESS	4	/* PowerMac 9700 */
#define	POWERMAC_CLASS_POWERBOOK	5	/* PowerBook 2300/5300 */
#define	POWERMAC_CLASS_GOSSAMER 	6	/* PowerMac G3 Desk/Mini-Twr */
#define	POWERMAC_CLASS_POWERSTAR 	7	/* PowerBook 2400/3400/G3 */
#define	POWERMAC_CLASS_YOSEMITE 	8	/* 1st Gen New World */

/* Some macros for easy machine identification */
#define IsPowerSurge()	(powermac_info.class == POWERMAC_CLASS_POWERSURGE)
 
#define IsPEx()		(powermac_info.class == POWERMAC_CLASS_POWEREXPRESS)

#define IsPowerBook()	(powermac_info.class == POWERMAC_CLASS_POWERBOOK)
 
#define IsGossamer()	(powermac_info.class == POWERMAC_CLASS_GOSSAMER)

#define IsPowerStar()	(powermac_info.class == POWERMAC_CLASS_POWERSTAR)

#define IsYosemite()	(powermac_info.class == POWERMAC_CLASS_YOSEMITE)

// This is not safe until the PMU or Cuda driver have been set up.
#define HasPMU()        (powermac_info.hasPMU)

typedef struct powermac_info {
	int		class;			/* Machine type */

	unsigned int	bus_clock_rate_hz;	/* Bus frequency */
        unsigned int    dec_clock_period;       /* Fixed point number 8.24 */

        /* to convert from real time ticks to nsec convert by this*/
	unsigned int	proc_clock_to_nsec_numerator; 
	unsigned int	proc_clock_to_nsec_denominator; 
	
	int		machine;		/* Gestalt value .. */
	int		hasPMU;                 // Flag for machines with PMU
	int		viaIRQ;                 // irq number for the via
} powermac_info_t;

extern powermac_info_t powermac_info;

/* This may be merged in struct powermac_info in the future */
typedef struct powermac_machine_info {
	unsigned long   dcache_block_size;  /* number of bytes */
	unsigned long   dcache_size;        /* number of bytes */
	unsigned long   icache_size;        /* number of bytes */
	unsigned long   caches_unified;     /* boolean_t */
	unsigned long   processor_version;  /* contents of PVR */
	unsigned long   cpu_clock_rate_hz;
	unsigned long   dec_clock_rate_hz;
	unsigned long   bus_clock_rate_hz;
	unsigned long   l2_clock_rate_hz;
	unsigned long   bus_clock_rate_hz_num;
	unsigned long   bus_clock_rate_hz_den;
	unsigned long   cpu_pll;                /* cpu pll mode * 2 */
	unsigned long   l2_pll;                 /* l2 pll mode * 2 */
	unsigned long   l2_cache_size;
	unsigned long   l2_cache_type;
} powermac_machine_info_t;

extern powermac_machine_info_t powermac_machine_info;

// Types of L2 Caches
#define L2_CACHE_NONE     (0)
#define L2_CACHE_MB       (1)
#define L2_CACHE_INLINE   (2)
#define L2_CACHE_BACKSIDE (3)


typedef struct powermac_io_info {
	int		io_size;
        vm_offset_t     io_base_phys;            /* IO region */
        vm_offset_t     io_base_virt;            /* IO region */

        vm_offset_t     io_base2;                // IO region for O'Hare 2

        vm_offset_t     mem_cntlr_base_phys;
        vm_offset_t     int_cntlr_base_phys;

        vm_offset_t     dma_base_phys;

  /* Remove All this dma stuff later */
	int		dma_buffer_alignment;	/* preallocated DMA region */
	int		dma_buffer_size;	/* preallocated DMA region */
	vm_offset_t	dma_buffer_phys;	/* preallocated DMA region */
	vm_offset_t	dma_buffer_virt;	/* preallocated DMA region */

        vm_offset_t     serial_base_phys;
        vm_offset_t     scsi_int_base_phys;
        vm_offset_t     scsi_int_dma_base_phys;
        vm_offset_t     scsi_ext_base_phys;
        vm_offset_t     audio_base_phys;
        vm_offset_t     floppy_base_phys;
        vm_offset_t     ethernet_base_phys;
        vm_offset_t     via_base_phys;

        vm_offset_t     nvram_addr_reg_phys;
        vm_offset_t     nvram_data_reg_phys;

        vm_offset_t     ide0_base_phys;
        vm_offset_t     ide1_base_phys;
	int		nvram_XPRAM_NVPartition;
	int		nvram_NameRegistry_NVPartition;
	int		nvram_OpenFirmware_NVPartition;
} powermac_io_info_t;

extern powermac_io_info_t powermac_io_info;

/* For compatability right now, keep these defines */
#define PCI_IO_BASE_PHYS        (powermac_io_info.io_base_phys)

#define PCI_DMA_BASE_PHYS       (powermac_io_info.dma_base_phys)

/* SCC registers (serial line) - physical addr is for probe */
#define PCI_SCC_BASE_PHYS       (powermac_io_info.serial_base_phys)

/* ASC registers (external scsi) - physical address for probe */
#define PCI_ASC_BASE_PHYS       (powermac_io_info.scsi_ext_base_phys)

/* MESH (internal scsi) controller */
#define PCI_MESH_BASE_PHYS      (powermac_io_info.scsi_int_base_phys)
#define PCI_MESH_DMA_BASE_PHYS  (powermac_io_info.scsi_int_dma_base_phys)

/* audio controller */
#define PCI_AUDIO_BASE_PHYS     (powermac_io_info.audio_base_phys)

/* floppy controller */
#define PCI_FLOPPY_BASE_PHYS    (powermac_io_info.floppy_base_phys)

/* Ethernet controller */
#define PCI_ETHERNET_BASE_PHYS  (powermac_io_info.ethernet_base_phys)
#define PCI_ETHERNET_ADDR_PHYS  (0xF3019000)

/* VIA controls, misc stuff (including CUDA) */
#define PCI_VIA_BASE_PHYS       (powermac_io_info.via_base_phys)

#define	PCI_VIA1_AUXCONTROL	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x01600))
#define	PCI_VIA1_T1COUNTERLOW	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00800))
#define	PCI_VIA1_T1COUNTERHIGH	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00A00))
#define	PCI_VIA1_T1LATCHLOW	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00C00))
#define	PCI_VIA1_T1LATCHHIGH	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00E00))
#define PCI_VIA1_IER            (POWERMAC_IO(powermac_io_info.via_base_phys + 0x01c00))
#define PCI_VIA1_IFR            (POWERMAC_IO(powermac_io_info.via_base_phys + 0x01a00))
#define PCI_VIA1_PCR            (POWERMAC_IO(powermac_io_info.via_base_phys + 0x01800))

/* NVRAM Addrs and Partitions */
#define PCI_NVRAM_ADDR_PHYS     (powermac_io_info.nvram_addr_reg_phys)
#define PCI_NVRAM_DATA_PHYS     (powermac_io_info.nvram_data_reg_phys)

#define NVRAM_XPRAM_Offset (powermac_io_info.nvram_XPRAM_NVPartition)
#define NVRAM_NameRegistry_Offset (powermac_io_info.nvram_NameRegistry_NVPartition)
#define NVRAM_OpenFirmware_Offset (powermac_io_info.nvram_OpenFirmware_NVPartition)



/* IDE controller */
#define PCI_IDE0_BASE_PHYS	(powermac_io_info.ide0_base_phys)
#define PCI_IDE1_BASE_PHYS	(powermac_io_info.ide1_base_phys)


extern void NO_ENTRY(void);

typedef struct {
	void (*configure_machine)(void);
	void (*machine_initialize_interrupt)(void);
	void (*machine_initialize_network)(void);
	void (*machine_initialize_processors)(boot_args *args);
	int  (*machine_initialize_rtclock)(void);
	powermac_dbdma_channels_t *powermac_dbdma_channels;
} powermac_init_t;

extern powermac_init_t *powermac_init_p;


/* Macro to convert from a physical I/O address into a virtual I/O address */
#define POWERMAC_IO(addr)	(powermac_io_info.io_base_virt +	\
				 (addr - powermac_io_info.io_base_phys))

/*
 * prototypes
 */

/* Used to initialise IO once DMA and IO virtual space has been assigned */
extern void powermac_io_init(vm_offset_t powermac_io_base_v);

extern void powermac_powerdown(void);
extern void powermac_reboot(void);

/* Non-cached version of bcopy */
extern void	bcopy_nc(char *from, char *to, int size);

/* Some useful typedefs for accessing control registers */

typedef volatile unsigned char	v_u_char;
typedef volatile unsigned short v_u_short;
typedef volatile unsigned int	v_u_int;
typedef volatile unsigned long  v_u_long;

/* And some useful defines for reading 'volatile' structures,
 * don't forget to be be careful about sync()s and eieio()s
 */
#define reg8(reg) (*(v_u_char *)reg)
#define reg16(reg) (*(v_u_short *)reg)
#define reg32(reg) (*(v_u_int *)reg)

/* VIA1_AUXCONTROL values same across several powermacs */
#define	VIA1_AUX_AUTOT1LATCH 0x40	/* Autoreload of T1 from latches */

#endif /* !(__ASSEMBLER__) */
#endif /* _POWERMAC_H_ */
