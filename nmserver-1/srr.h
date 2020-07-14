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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef	_SRR_
#define	_SRR_

#include <mach/boolean.h>

#include "transport.h"

/*
 * srr specific failure codes.
 */
#define SRR_SUCCESS		(0)
#define SRR_ERROR_BASE		(-(TR_SRR_ENTRY * 16))
#define SRR_TOO_LARGE		(1 + SRR_ERROR_BASE)
#define SRR_FAILURE		(2 + SRR_ERROR_BASE)
#define SRR_ENCRYPT_FAILURE	(3 + SRR_ERROR_BASE)


/*
 * The maximum amount of data that can be placed in a request or a response.
 */
extern int srr_max_data_size;

/*
 * Exported functions.
 */
extern boolean_t srr_init();
extern int srr_send();

#endif	_SRR_
