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
 *	Copyright (c) 1996 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 *	File: cfg.c
 */
#define RESOLVE_DBG
#include <sysglue.h>
#include <at/appletalk.h>
#include <lap.h>
#include <routing_tables.h>
#define _AURP
#include <at/aurp.h>
#include <at_aurp.h>

extern atlock_t aurpgen_lock;
static int aurp_inited = 0;
static char aurp_minor_no[4];

int aurp_open(gref)
	gref_t *gref;
{
	extern void AURPcmdx();
	int i, s;

	if (!aurp_inited) {
		aurp_inited = 1;
		ATLOCKINIT(aurpgen_lock);
	}

	for (i=1; i < sizeof(aurp_minor_no); i++) {
		if (aurp_minor_no[i] == 0) {
			aurp_minor_no[i] = (char )i;
			break;
		}
	}
	if (i == sizeof(aurp_minor_no))
		return EAGAIN;
	if (i == 1) {
		aurp_gref = gref;
		if (ddp_AURPfuncx(AURPCODE_REG, AURPcmdx, 0)) {
			aurp_gref = 0;
			aurp_minor_no[i] = 0;
			return EPROTO;
		}
	}

	gref->info = (void *)&aurp_minor_no[i];
	return 0;
}

int aurp_close(gref)
	gref_t *gref;
{
	if (*(char *)gref->info == 1) {
		aurp_gref = 0;
		aurp_inited = 0;
		ddp_AURPfuncx(AURPCODE_REG, 0, 0);
	}

	*(char *)gref->info = 0;
	gref->info = 0;
	return 0;
}
