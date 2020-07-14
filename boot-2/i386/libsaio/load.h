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
/* load.h */

#import "libsaio.h"

#define L_PHYSICAL		0
#define L_VIRTUAL		1

#define MAX_MISSING_DRIVERS 16
#define MAX_DRIVERS 128

struct driver_load_data {
    char	*name;
    BOOL	compressed;
    int 	fd;
    char	*buf;
    int		len;
};


extern int
openDriverReloc(
    struct driver_load_data *data
);

extern int
loadDriver(
    struct driver_load_data *data
);

extern int
linkDriver(
    struct driver_load_data *data
);
