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
 * Definitions for parameters to vm_deactivate and vm_set_policy.
 *
 * Copyright (c) 1993 NeXT, Inc.
 *
 * HISTORY
 *
 *  23Jan93	Brian Pinkerton at NeXT
 *	Created.
 */

#ifndef	_VM_POLICY_H
#define	_VM_POLICY_H

/*
 *  Flags for vm_deactivate
 */
#define VM_DEACTIVATE_NOW		0x0	/* most aggressive: free mem */
#define	VM_DEACTIVATE_SOON		0x1	/* less aggressive */
#define VM_DEACTIVATE_SHARED		0x2	/* deactivate shared mem too */

/*
 *  Flags for vm_set_policy
 */
#define VM_POLICY_RANDOM		0x0	/* access pattern is random */
#define VM_POLICY_SEQ_FREE		0x1	/* free pages on sequential */
#define VM_POLICY_SEQ_DEACTIVATE	0x2	/* deactivate pages on seq */
#define VM_POLICY_PAGE_AHEAD		0x4	/* page ahead */

#define	VM_POLICY_SEQUENTIAL (VM_POLICY_SEQ_FREE | VM_POLICY_SEQ_DEACTIVATE)

#endif	/* _VM_POLICY_H */

