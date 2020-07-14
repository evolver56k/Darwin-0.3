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
	File:		ExtractRaw.c

	Contains:	An MPW tool that extracts code from an XCOFF file into a raw binary
				output file such that the output file contains only the code and no
				XCOFF junk.  Extracts only the first XCOFF section from the input file.

	Version:	Maxwell

	Copyright:	© 1996 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Alan Mimms

		Other Contact:		Stanford Au

		Technology:			MacOS

	Writers:

		(ABM)	Alan Mimms

	Change History (most recent first):

		 <1>	 4/16/96	ABM		First checked in.
*/

#include <stdio.h>
#include <stdlib.h>


enum {
	kOffset_s_size = 0x24			// Offset within XCOFF file of s_size field
};


static void usage (char *cmd, char *msg)
{
	printf ("Usage:\"%s\" input-filespec output-filespec\n\t(%s)\n", cmd, msg);
	exit (9999);
}


void main (int argc, char **argv)
{
	FILE *inf;
	FILE *outf;
	
	int sectionSize;
	int sectionOffset;
	int c;

	if (argc != 3) usage (argv[0], "wrong number of args");
	
	inf = fopen (argv[1], "rb");
	if (!inf) usage (argv[0], "can't open input file");
	
	outf = fopen (argv[2], "wb");
	if (!outf) usage (argv[0], "can't create/open output file");

	fseek (inf, kOffset_s_size, SEEK_SET);
	sectionSize = getw (inf);				// Get s_size value: size of code section
	sectionOffset = getw (inf);				// Get s_scnptr value: offset to start of code section
	
	fseek (inf, sectionOffset, SEEK_SET);	// Seek to start of section
	
	// Copy sectionSize bytes from inf to outf
	while (--sectionSize >= 0 && (c = fgetc (inf)) != EOF) fputc (c, outf);

	fclose (inf);
	fclose (outf);
}
