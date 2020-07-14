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
 * Intel386 Family:	Local descriptor table.
 *
 * HISTORY
 *
 * 15 December 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <machdep/i386/ldt.h>
#import <machdep/i386/seg.h>
#import <machdep/i386/table_inline.h>
#import <machdep/i386/desc_inline.h>

void
ldt_init(
    void
)
{
    map_code(sel_to_ldt_entry(ldt, UCS_SEL),
			    VM_MIN_ADDRESS, VM_MAX_ADDRESS - VM_MIN_ADDRESS,
			    USER_PRIV,
			    FALSE				/* prot */
		    );
    map_data(sel_to_ldt_entry(ldt, UDS_SEL),
			    VM_MIN_ADDRESS, /*0x1*/00000000,
			    USER_PRIV,
			    FALSE				/* prot */
		    );
}

static ldt_t	ldt_store[LDTSZ] = { 0 };

ldt_t		*ldt = ldt_store;
