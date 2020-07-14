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
 *      Copyright (c) 1997 Apple Computer, Inc.
 *
 *      The information contained herein is subject to change without
 *      notice and  should not be  construed as a commitment by Apple
 *      Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *      for any errors that may appear.
 *
 *      Confidential and Proprietary to Apple Computer, Inc.
 *
 */

/* at_kdebug.h */

#import <kern/kdebug.h>

#define DBG_AT_DDP_INPUT NETDBG_CODE(DBG_NETDDP, 1)

/*
Strings:

0x02650004	Fnc_DDP_Input

*/

/* usage:
   KERNEL_DEBUG(DBG_AT_DDP_INPUT | DBG_FUNC_START, 0,0,0,0,0);
   KERNEL_DEBUG(DBG_AT_DDP_INPUT, 0,0,0,0,0);
   KERNEL_DEBUG(DBG_AT_DDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
*/
