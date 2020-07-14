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

#ifndef	_DATAGRAM_
#define	_DATAGRAM_

#include <mach/boolean.h>
#include "transport.h"

/*
 * Datagram specific failure codes.
 */
#define DATAGRAM_ERROR_BASE		(-(TR_DATAGRAM_ENTRY * 16))
#define DATAGRAM_TOO_LARGE		(1 + DATAGRAM_ERROR_BASE)
#define DATAGRAM_SEND_FAILURE		(2 + DATAGRAM_ERROR_BASE)

/*
 * The maximum amount of data that can be placed in a datagram.
 */
extern int datagram_max_data_size;

/*
 * Exported functions.
 */
extern boolean_t datagram_init();
extern int datagram_send();

#endif	_DATAGRAM_
