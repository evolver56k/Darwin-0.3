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
 * Copyright 1995, NeXT Computer, Inc. All Rights Reserved.
 *
 * HISTORY :
 *	30-Apr-97 : Umesh Vaishampayan (umeshv@NeXT.com)
 *		Cleanup.
 */

#ifndef _MACHDEP_PPC_FPU_H_
#define _MACHDEP_PPC_FPU_H_

#if defined(KERNEL_PRIVATE)
#include <machdep/ppc/thread.h>

/* Save the floating point state for a thread */

extern void fpu_save(void);
extern void fpu_disable(void);

static inline void fp_state_save(thread_t th)
{
	fpu_save();
	fpu_disable();
}

#endif /* KERNEL_PRIVATE */

#endif /* _MACHDEP_PPC_FPU_H_ */
 
