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


// move to event defs
#define VC_LED_CAPSLOCK 1
#define VC_LED_SCROLLLOCK 2
#define VC_LED_NUMLOCK 4


struct bus_device {
	int	unused;
};
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef NULL
#define NULL 0
#endif

#define NADB 1
#define NCUDA 1

typedef int io_return_t;
typedef int io_req_t;
typedef int dev_mode_t;
typedef int dev_flavor_t;
typedef int dev_status_t;

#define D_SUCCESS       0
#define D_IO_ERROR      2500    /* hardware IO error */
#define D_WOULD_BLOCK       2501    /* would block, but D_NOWAIT set */
#define D_NO_SUCH_DEVICE    2502    /* no such device */
#define D_ALREADY_OPEN      2503    /* exclusive-use device already open */
#define D_DEVICE_DOWN       2504    /* device has been shut down */
#define D_INVALID_OPERATION 2505    /* bad operation for device */
#define D_INVALID_RECNUM    2506    /* invalid record (block) number */
#define D_INVALID_SIZE      2507    /* invalid IO size */
#define D_NO_MEMORY     2508    /* memory allocation failure */
#define D_READ_ONLY     2509    /* device cannot be written to */
#define D_OUT_OF_BAND       2510    /* out-of-band condition on device */
#define D_NOT_CLONED        2511    /* device cannot be cloned */
