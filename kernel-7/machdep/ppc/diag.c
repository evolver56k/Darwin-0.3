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
 	File:		diag.c

 	Contains:	Kernel diagnostics

 	Copyright:	1997 by Apple Computer, Inc., all rights reserved

	File Ownership:

		DRI:               Thomas Mason

		Other Contact:     Brett Halle

		Technology:        Kernel/IO

	Writers:

		(tjm)	Thomas Mason

	Change History (most recent first):
*/

#include <machdep/ppc/exception.h>
#include <kern/thread.h>
#include <machdep/ppc/PseudoKernel.h>
#include <kern/ipc_tt.h>
#include <ipc/ipc_port.h>

#define printf kprintf

#if	DEBUG

/*
**	Profiling Counters
*/
unsigned long shandler_cnt = 0;
unsigned long thandler_cnt = 0;
unsigned long ihandler_cnt = 0;
unsigned long gdbhandler_cnt = 0;
unsigned long fpu_switch_cnt = 0;

#define	TST_BCOPY_SIZE	(256)
#define	TST_MOVE_SIZE	(TST_BCOPY_SIZE/2)
unsigned long sb[TST_BCOPY_SIZE];		/* source buffer */
unsigned long db[TST_BCOPY_SIZE];		/* destination buffer */


/*
**
**	test_bcopy()
**
**	Test the various configurations of bcopy for correctness
**	this does the table first, then cache aligned, the byte < 7
**
*/
test_bcopy()
{
    int	i, j, k, l;
    char	 *src, *dst, *cmpsrc, *cmpdst;
    boolean_t	succeeded;


    cmpsrc = (char *)sb;
    /* Initialize the source buffer */
    for (i = 0; i < TST_BCOPY_SIZE; i++)
    {
        *cmpsrc++ = (char)i;
    }

    /* test the variants */

    /* setup initial src and dst ptrs */
    dst = (char *)db;
    for (i = 0; i < 4; i++)
    {
	src = (char *)sb;
	for (j = 0; j < 4; j++)
	{
            for (k = TST_MOVE_SIZE; k < TST_MOVE_SIZE+4; k++)
            {
		cmpdst = (char *)db;
		/* jumble the destination buffer */
		for (l = 0; l < TST_BCOPY_SIZE; l++)
		{
		    *cmpdst++ = 0xA5;
		}

		cmpsrc = src;
		cmpdst = dst;
		bcopy(src, dst, k);
		succeeded = TRUE;
		for (l = 0; l < k; l++)
		{
		    if (*cmpsrc++ != *cmpdst++)
		    {
			succeeded = FALSE;
		    }
		}
		printf("mm%ds%dc%d (src=0x%x,dst=0x%x,llen=%d) ",((int)dst&3),((int)src&3),(k&3),src, dst, k);
		switch (succeeded) {
		case FALSE:
		    printf("FAILED!\n");
		    break;
		case TRUE:
		    printf("SUCCEEDED!\n");
		    break;
		}
	    }
	    src++;
	}
	dst++;
    }
    /* update to original ptrs */
    src = (char *)sb;
    dst = (char *)db;

    /* do cache aligned */
    cmpsrc = src;
    cmpdst = dst;
    bcopy(src, dst, TST_MOVE_SIZE);
    succeeded = TRUE;
    for (l = 0; l < TST_MOVE_SIZE; l++)
    {
	if (*cmpsrc++ != *cmpdst++)
	{
	    succeeded = FALSE;
	}
    }
    printf("cache aligned bcopy (src=0x%x,dst=0x%x,len=%d) ",src,dst,TST_MOVE_SIZE);
    switch (succeeded) {
    case FALSE:
	printf("FAILED!\n");
	break;
    case TRUE:
	printf("SUCCEEDED!\n");
	break;
    }

    /* do byte only */
    cmpsrc = src;
    cmpdst = dst;
    bcopy(src, dst, 7);
    succeeded = TRUE;
    for (l = 0; l < 7; l++)
    {
	if (*cmpsrc++ != *cmpdst++)
	{
	    succeeded = FALSE;
	}
    }
    printf("byte only bcopy(src=0x%x,dst=0x%x,len=%d) ",src,dst,7);
    switch (succeeded) {
    case FALSE:
	printf("FAILED!\n");
	break;
    case TRUE:
	printf("SUCCEEDED!\n");
	break;
    }

    /* do copy in place */
    cmpsrc = src;
    cmpdst = dst+4;
    bcopy(dst, dst+4, 7);
    succeeded = TRUE;
    for (l = 0; l < 7; l++)
    {
	if (*cmpsrc++ != *cmpdst++)
	{
	    succeeded = FALSE;
	}
    }
    printf("copy in place bcopy(src=0x%x,dst=0x%x,len=%d) ",dst,dst+4,7);
    switch (succeeded) {
    case FALSE:
	printf("FAILED!\n");
	break;
    case TRUE:
	printf("SUCCEEDED!\n");
	break;
    }
}


struct bcopy_callers {
	int	caller;
	int	count;
};

typedef struct bcopy_callers bcopy_callers_t, *bcopy_callers_tPtr;

bcopy_callers_t	bcc[1024];
boolean_t bcc_inited = FALSE;


void bcopy_diag (
    int	caller
    )
{
    int i;


    if (bcc_inited == FALSE)
    {
	for (i = 0; i < 1024; i++)
	{
	    bcc[i].caller = 0;
	    bcc[i].count = 0;
	    bcc_inited = TRUE;
	}
    }
    for (i = 0; i < 1024; i++)
    {
	if ( bcc[i].caller == caller )
	{
	    bcc[i].count++;
	    return;
	}
	else if ((bcc[i].caller == 0) && (i < 1024))
	{
	    bcc[i].caller = caller;
	    bcc[i].count = 1;
	    return;
	}
    }
}

#endif /* DEBUG */

