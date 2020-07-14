/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* 
 * apple_partitions.c
 * - execs pdisk utility to check whether the disk has apple partitions
 */

unsigned long	apple_ufs_base;
unsigned long	apple_ufs_size;
int		apple_block_size;
int 		apple_disk = 0;

#if defined(ppc)
#include "disk.h"
#include <bsd/dev/ppc/disk.h>
#include "forkexec.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


#define PDISK_PATHNAME 	"/usr/sbin/pdisk"

static char * isPartitioned[] = {
    PDISK_PATHNAME, dev, "-isDiskPartitioned", 0
};
static char * getPartitionOfType[] = {
    PDISK_PATHNAME, dev, "-getPartitionOfType", RHAPSODY_PART_TYPE, "0", 0
};
#define PARTNO_ARGC_INDEX	3	/* argc == 3 is the partno */
static char * partitionBase[] = {
    PDISK_PATHNAME, dev, "-partitionBase", 0, 0
};
static char * partitionSize[] = {
    PDISK_PATHNAME, dev, "-partitionSize", 0, 0
};
static char * blockSize[] = {
    PDISK_PATHNAME, dev, "-blockSize", 0
};

int 
has_apple_partitions()
{
    int			n_bytes;
    unsigned char *	partno;
    unsigned char * 	results;
    int			status;

    /* does the disk have an Apple partition map? */
    results = forkexec(isPartitioned[0], isPartitioned, &status, &n_bytes);
    if (status != 0)
	goto no_partitions;

    free(results);

    /* does the disk have an Apple Rhapsody UFS partition? */
    results = forkexec(getPartitionOfType[0], getPartitionOfType, 
				&status, &n_bytes);
    if (results == NULL || status != 0)
	goto no_partitions;

    partno = results;
    
    /* get the partition base (in blocks) */
    partitionBase[PARTNO_ARGC_INDEX] = partno; /* set the partition number */
    results = forkexec(partitionBase[0], partitionBase, &status, &n_bytes);
    if (results == NULL || status != 0)
	goto no_partitions;
    apple_ufs_base = atol(results);
    free(results);
    
    /* get the partition size (in blocks) */
    partitionSize[PARTNO_ARGC_INDEX] = partno; /* set the partition number */
    results = forkexec(partitionSize[0], partitionSize, &status, &n_bytes);
    if (results == NULL || status != 0)
	goto no_partitions;
    apple_ufs_size = atol(results);
    free(results); 
    free(partno);

    /* get the block size */
    results = forkexec(blockSize[0], blockSize, &status, &n_bytes);
    if (results == NULL || status != 0)
	goto no_partitions;
    apple_block_size = atoi(results);
    free(results);
    results = 0; /* make sure we don't free twice */

    if (apple_block_size <= 0) /* don't divide by zero */
	goto no_partitions;

    printf("Apple_Rhapsody_UFS base %ld size %ld map block size %d\n", 
	   apple_ufs_base, apple_ufs_size, apple_block_size);
    return (1);

  no_partitions:
    if (results) {
	printf(results);
	free(results);
    }
    return (0);
}
#else
int has_apple_partitions()
{
    return (0);
}
#endif
