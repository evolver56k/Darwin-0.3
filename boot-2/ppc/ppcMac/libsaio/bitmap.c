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
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 */

#import "libsaio.h"
#import "bitmap.h"

/* load a bitmap */

#define ERROR_STR "Error %d reading bitmap from '%s'\n"

struct bitmap *
loadBitmap(char *filename)
{
	int fd, cc;
	struct bitmap *bm;
	
/* printf("Loading bitmap: %s\n",filename); */
	bm = (struct bitmap *)malloc(sizeof(struct bitmap));
	fd = open(filename, 0);
	if (fd < 0) {
		printf(ERROR_STR,1,filename);
		return 0;
	}
	cc = read(fd, (char *)bm, sizeof(*bm));
	if (cc < sizeof(*bm))
		goto error;
	bm->plane_data[0] = (unsigned char *)malloc(bm->plane_len[0]);
	bm->plane_data[1] = (unsigned char *)malloc(bm->plane_len[1]);
	cc = read(fd, bm->plane_data[0], bm->plane_len[0]);
	if (cc < bm->plane_len[0])
		goto free_error;
	cc = read(fd, bm->plane_data[1], bm->plane_len[1]);
	if (cc < bm->plane_len[1])
		goto free_error;
	close(fd);
	return bm;

free_error:
	free(bm->plane_data[0]);
	free(bm->plane_data[1]);
error:
	close(fd);
	error(ERROR_STR,2,filename);
	return 0;
}

