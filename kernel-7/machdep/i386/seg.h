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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Segments.
 *
 * HISTORY
 *
 * 11 May 1992 ? at NeXT
 *	Created.
 */

#import <architecture/i386/sel.h>
#import <architecture/i386/desc.h>
#import <architecture/i386/tss.h>

/*
 * Segment selector values *used*
 * by the system.
 */

/*
 * Global descriptor table
 */

#define KCS_SEL		((sel_t)	/* CS of kernel  */	\
			    { 					\
				KERN_PRIV,			\
				SEL_GDT,			\
				0x0001				\
			    }					\
			)
#define KDS_SEL		((sel_t)	/* DS of kernel  */	\
			    {					\
				KERN_PRIV,			\
				SEL_GDT,			\
				0x0002				\
			    }					\
			)
#define TSS_SEL		((sel_t)	/* TSS of cur thread */	\
			    {					\
				KERN_PRIV,			\
				SEL_GDT,			\
				0x0003				\
			    }					\
			)
#define LDT_SEL		((sel_t)	/* LDT of cur task   */	\
			    {					\
				KERN_PRIV,			\
				SEL_GDT,			\
				0x0004				\
			    }					\
			)
#define SCALL_SEL	((sel_t)	/* U**X system calls */	\
			    {					\
			    	USER_PRIV,			\
				SEL_GDT,			\
				0x0005				\
			    }					\
			)
#define MACHCALL_SEL	((sel_t)	/* MACH kernel traps */	\
			    {					\
			    	USER_PRIV,			\
				SEL_GDT,			\
				0x0006				\
			    }					\
			)
#define MDCALL_SEL	((sel_t)	/* machine dependent calls */	\
			    {						\
			    	USER_PRIV,				\
				SEL_GDT,				\
				0x0007					\
			    }						\
			)
#define BIOSDATA_SEL	((sel_t)	/* used by prot mode WINDOWS */	\
			    {						\
			    	KERN_PRIV,				\
				SEL_GDT,				\
				0x0008					\
			    }						\
			)
#define LCODE_SEL	((sel_t)	/* entire linear code space */	\
			    {						\
				KERN_PRIV,				\
				SEL_GDT,				\
				0x0009					\
			    }						\
			)
#define LDATA_SEL	((sel_t)	/* entire linear data space */	\
			    {						\
				KERN_PRIV,				\
				SEL_GDT,				\
				0x000A					\
			    }						\
			)
#define FPSTATE_SEL	((sel_t)	/* for fp emulator */		\
			    {						\
				KERN_PRIV,				\
				SEL_GDT,				\
				0x000B					\
			    }						\
			)
#define UCODE_SEL	((sel_t)	/* alternate UCS */		\
			    {						\
				USER_PRIV,				\
				SEL_GDT,				\
				0x000C					\
			    }						\
			)
#define UDATA_SEL	((sel_t)	/* alternate UDS */		\
			    {						\
				USER_PRIV,				\
				SEL_GDT,				\
				0x000D					\
			    }						\
			)
#define DBLFLT_SEL	((sel_t)	/* TSS for double faults */	\
			    {						\
			    	KERN_PRIV,				\
				SEL_GDT,				\
				0x000E					\
			    }						\
			)
#define APMCODE32_SEL	((sel_t)	/* 32-bit code segment */	\
			    {						\
			    	KERN_PRIV,				\
				SEL_GDT,				\
				0x000F					\
			    }						\
			)
#define APMCODE16_SEL	((sel_t)	/* 16-bit code segment */	\
			    {						\
			    	KERN_PRIV,				\
				SEL_GDT,				\
				0x0010					\
			    }						\
			)
#define APMDATA_SEL	((sel_t)	/* APM data segment */		\
			    {						\
			    	KERN_PRIV,				\
				SEL_GDT,				\
				0x0011					\
			    }						\
			)
#define BIOSC32_SEL	((sel_t)	/* 32-bit BIOScode segment */	\
			    {						\
			    	KERN_PRIV,				\
				SEL_GDT,				\
				0x0012					\
			    }						\
			)
#define BIOSD32_SEL	((sel_t)	/* 32-bit BIOS data segment */	\
			    {						\
			    	KERN_PRIV,				\
				SEL_GDT,				\
				0x0013					\
			    }						\
			)

/*
 * Local descriptor table
 */

#define UCS_SEL		((sel_t)	/* User CS of cur task */	\
			    {						\
				USER_PRIV,				\
				SEL_LDT,				\
				0x0001					\
			    }						\
			)
#define UDS_SEL		((sel_t)	/* User DS of cur task */	\
			    {						\
				USER_PRIV,				\
				SEL_LDT,				\
				0x0002					\
			    }						\
			)
