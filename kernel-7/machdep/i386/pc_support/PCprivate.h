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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * PC support private declarations.
 *
 * HISTORY
 *
 * 9 Mar 1993 ? at NeXT
 *	Created.
 */
 
#ifdef KERNEL_PRIVATE

#import "PCpublic.h"

typedef struct PCprivate {
    PCshared_t		shared;
    vm_map_t		task;
    vm_offset_t		taskShared;
} *PCprivate_t;

boolean_t PCexception(thread_t	thread, thread_saved_state_t *state);
boolean_t PCbopFA(thread_t thread, thread_saved_state_t *state,
							PCbopFA_t *args);
boolean_t PCbopFC(thread_t thread, thread_saved_state_t *state,
							PCbopFC_t *args);
void PCbopFD(thread_t thread, thread_saved_state_t *state, PCbopFD_t *args);
void PCcallMonitor(thread_t thread, thread_saved_state_t *state);

#import "PCmiscInline.h"

#define	BIOS_DATA_ADDR		(0)
#define	BIOS_DATA_SIZE		(4 * 1024)
#define BIOS_ROM_ADDR		(0xf0000)
#define BIOS_ROM_SIZE		(0x10000)
#define CNV_MEM_END_ADDR	(640 * 1024)

#endif
