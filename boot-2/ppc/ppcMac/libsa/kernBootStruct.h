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
 * kernBootStruct.h
 * What the booter leaves behind for the kernel.
 */

#define GRAPHICS_MODE	1
#define TEXT_MODE 0

#define KERNBOOTMAGIC 0xa7a7a7a7

/* The config table has room for 12 drivers if their config files
 * are the maximum size allowed.
 */
#define CONFIG_SIZE (12 * 4096)

/* Maximum number of boot drivers supported, assuming their
 * config files fit in the bootstruct.
 */
#define NDRIVERS		64

#ifndef __ASSEMBLER__

typedef struct {
    char    *address;			// address where driver was loaded
    int	    size;			// entry point for driver
} driver_config_t;

typedef struct {
    short   version;
    char    bootString[160];		// string we booted with
    int	    magicCookie;		// KERNBOOTMAGIC if struct valid
    int	    rootdev;			// booters guess as to rootdev
    int	    convmem;			// conventional memory
    char    boot_file[128];		// name of the kernel we booted
    int	    first_addr0;		// first address for kern convmem
    int	    graphicsMode;		// did we boot in graphics mode?
    int	    kernDev;			// device kernel was fetched from
    int     numBootDrivers;		// number of drivers loaded by booter    
    char    *configEnd;			// pointer to end of config files
    int	    kaddr;			// kernel load address
    int     ksize;			// size of kernel
    void    *rld_entry;			// entry point for standalone rld

    driver_config_t driverConfig[NDRIVERS];	// ??
    
    char   _reserved[4052 - sizeof(driver_config_t) * NDRIVERS];

    char   config[CONFIG_SIZE];		// the config file contents
} KERNBOOTSTRUCT;

#ifdef KERNEL
#define static_KERNBOOTSTRUCT (*KERNSTRUCT_ADDR)
extern KERNBOOTSTRUCT *KERNSTRUCT_ADDR;
#else	
/* The assumption is that the booter is the only other consumer of this */
extern KERNBOOTSTRUCT *kernBootStruct;
#undef KERNSTRUCT_ADDR
// #define KERNSTRUCT_ADDR ((KERNBOOTSTRUCT *) BOOTSTRUCT_ADDR)
#endif

#endif /* __ASSEMBLER__ */
