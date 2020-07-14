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
 * Copyright (c) 1998 Apple Computer, Inc. All Rights Reserved
 *
 *		MODIFICATION HISTORY (most recent first):
 *
 *	   20-Jul-1998	Don Brady		New today.
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <paths.h>

#import <bsd/dev/disk.h>
#import <bsd/fcntl.h>
#import <bsd/sys/errno.h>

#import "MacOSTypes.h"
#import "HFSVolumes.h"
#import "HFSBtreesPriv.h"

#include "newfs_hfs.h"

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif


/*
 * The following constant sets the default block size.
 * This constant must be a power of 2 and meet the following constraints:
 *	MINBSIZE <= DFL_BLKSIZE <= MAXBSIZE
 *	sectorsize <= DFL_BLKSIZE
 */
#define HFSOPTIMALBLKSIZE	4096
#define HFSMINBSIZE			512
#define	DFL_BLKSIZE			HFSOPTIMALBLKSIZE


/*
 * The minimum HFS Plus volume is 32 MB
 */
#define	MINHFSPLUSSIZEMB	32


/* bek 5/20/98 - [2238320] - unistd.h is missing "extern int optreset" */

#define Radar_2238320 1

#if Radar_2238320
extern int optreset;
#endif // Radar_2238320



void	usage __P((void));

int hfs_newfs(char *device, char *volumename, int forceHFS);
void printparms(char *device, const DriveInfo *driveInfo, char* volumeName, HFSPlusDefaults * defaults);

static void MakeHFSPlusDefaults (UInt32 sectorCount, UInt32 sectorSize, HFSPlusDefaults *defaults);
static void MakeHFSDefaults (UInt32 sectorCount, UInt32 sectorSize, HFSDefaults *defaults);
static UInt32 CalcBTreeClumpSize (UInt32 blockSize, UInt32 nodeSize, UInt32 driveBlocks);


char	*progname;
char	gVolumeName[MAXPATHLEN] = {"untitled"};
char	gDeviceName[MAXPATHLEN];
UInt32	gBlockSize = DFL_BLKSIZE;
int		gPrintParms = FALSE;

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	int ch;
	int forceHFS;
	char *dev;
	
	if (progname = strrchr(*argv, '/'))
		++progname;
	else
		progname = *argv;

	forceHFS = FALSE;

	optind = optreset = 1;		/* Reset for parse of new argv. */
	while ((ch = getopt(argc, argv, "HNb:v:")) != EOF)
        switch (ch) {
		case 'H':
			forceHFS = TRUE;
			break;

		case 'N':
			gPrintParms = TRUE;
			break;

		case 'b':
			gBlockSize = atoi(optarg);
			/*
			 * make sure its at least 512 and a power of two
			 */
			if (gBlockSize < HFSMINBSIZE || (gBlockSize & gBlockSize-1) != 0)
				fatal("%s: bad allocation block size (must be a power of two)", optarg);

			if (gBlockSize != HFSOPTIMALBLKSIZE)
				printf("Warning: %ld is a non-optimal block size (4096 would be a better choice)\nFile system R/W performance may be impaired.\n", gBlockSize);
			break;

		case 'v':
			strcpy(gVolumeName, optarg);
			if (strrchr(gVolumeName, ':') != 0)
				fatal("\"%s\" is an invalid hfs name (has colons)", optarg);
			break;

		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

    if (argc != 1) {
		usage();
	}

    dev = argv[0];

	if (strrchr(dev, '/') == 0) {
		/*
		 * No path prefix; try /dev/%s.
		 */
		(void)sprintf(gDeviceName, "%s%s", _PATH_DEV, dev);
		dev = gDeviceName;
	}


    if (hfs_newfs(dev, gVolumeName, forceHFS) < 0) {
        err(1, NULL);
	};

    exit(0);
}


int hfs_newfs(char *device, char *volumename, int forceHFS)
{
	struct stat stbuf;
	struct statfs *mp;
	DriveInfo dip;
	int n;
    int fso = 0;
	int retval = 0;

	/*
	 * Check if target device is an hfs partition
	 */
	if (strstr(device, "_hfs_") == NULL)
		fatal("%s: is not an HFS partition", device);

	/*
	 * Check if target device is aready mounted
	 */
	n = getmntinfo(&mp, MNT_NOWAIT);
	if (n == 0)
		fatal("%s: getmntinfo: %s", device, strerror(errno));

	while (--n >= 0) {
		if (strcmp(device, mp->f_mntfromname) == 0)		/* XXX assumes device is of the form /dev/xxx_hfs_a */
			fatal("%s is mounted on %s", device, mp->f_mntonname);
		++mp;
	}

    fso = open( device, O_WRONLY | O_NDELAY, 0 );
	if (fso < 0)
		fatal("%s: %s", device, strerror(errno));

	if (fstat( fso, &stbuf) < 0)
		fatal("%s: %s", device, strerror(errno));
		
	if ((stbuf.st_mode & S_IFMT) != S_IFBLK)
		fatal("%s is not a block special device", device);

	if (ioctl(fso, DKIOCNUMBLKS, &dip.totalSectors) < 0)
		fatal("%s: %s", device, strerror(errno));

	if (ioctl(fso, DKIOCBLKSIZE, &dip.sectorSize) < 0)
		fatal("%s: %s", device, strerror(errno));

//	printf("stbuf.st_rdev = %d\n", stbuf.st_rdev);
//	printf("stbuf.st_mode = %o\n", stbuf.st_mode & S_IFMT);
//	printf ("\nThere are %d %d byte sectors (%ld) on device %ld\n", dip.totalSectors, dip.sectorSize, (dip.totalSectors * dip.sectorSize), stbuf.st_rdev);

	dip.fd = fso;
	dip.sectorOffset = 0;

	if (forceHFS) {
		HFSDefaults defaults;

		if (strlen(gVolumeName) > 27)
			fatal("\"%s\" is an invalid hfs name (maximum is 27 chars)", optarg);

		MakeHFSDefaults(dip.totalSectors, dip.sectorSize, &defaults);
		retval = InitHFSVolume (&dip, volumename, &defaults, 0);
		printf("newfs_hfs: Initialized %s as a %dMB HFS volume named \"%s\"\n", device, dip.totalSectors/2048, volumename);
	}
	else /* HFS Plus */ {
		HFSPlusDefaults defaults;
		
		if ((dip.totalSectors/2048) < MINHFSPLUSSIZEMB)
			fatal("%s: partition is too small (minimum is %d MB)", device, MINHFSPLUSSIZEMB);

		MakeHFSPlusDefaults(dip.totalSectors, dip.sectorSize, &defaults);
		if (gPrintParms)
			printparms(device, &dip, volumename, &defaults);
		else
			retval = InitializeHFSPlusVolume (&dip, volumename, &defaults);
		printf("newfs_hfs: Initialized %s as a %dMB HFS Plus volume named \"%s\"\n", device, dip.totalSectors/2048, volumename);
	}

	if (retval)
		fatal("%s: %s", device, strerror(errno));

    if ( fso > 0 ) {
        close(fso);
    }

	return retval;
}


void printparms(char *device, const DriveInfo *driveInfo, char* volumeName, HFSPlusDefaults * defaults)
{
	printf("Format parameters for \"%s\":\n", device);
	printf(" %d sectors at %d bytes per sector\n", driveInfo->totalSectors, driveInfo->sectorSize);

	printf(" volume name: \"%s\"\n", volumeName);
	printf(" block-size: %ld\n", defaults->blockSize);
	printf(" initial Catalog File size: %ld\n", defaults-> catalogClumpSize);
	printf(" initial Extents File size: %ld\n", defaults-> extentsClumpSize);
}


static void MakeHFSPlusDefaults (UInt32 sectorCount, UInt32 sectorSize, HFSPlusDefaults *defaults)
{
	UInt32	bitsPerBlock;
	UInt32	blockCount;


	if (gBlockSize == 0)
		gBlockSize = DFL_BLKSIZE;

	defaults->version = kHFSPlusDefaultsVersion;
	defaults->flags = 0;
	defaults->blockSize = gBlockSize;
	defaults->rsrcClumpSize = kHFSPlusRsrcClumpFactor * gBlockSize;
	defaults->dataClumpSize = kHFSPlusDataClumpFactor * gBlockSize;
	defaults->nextFreeFileID = kHFSFirstUserCatalogNodeID;

	defaults->catalogNodeSize = 4096;
	defaults->catalogClumpSize = CalcBTreeClumpSize(gBlockSize, defaults->catalogNodeSize, sectorCount);

	defaults->extentsNodeSize = 1024;
	defaults->extentsClumpSize = CalcBTreeClumpSize(gBlockSize, defaults->extentsNodeSize, sectorCount);

	defaults->attributesNodeSize = 4096;
	defaults->attributesClumpSize = CalcBTreeClumpSize(gBlockSize, defaults->attributesNodeSize, sectorCount);

	//  Calculate the number of blocks needed for bitmap (rounded up)

	blockCount = sectorSize / (gBlockSize / sectorSize);
	bitsPerBlock = 8 * gBlockSize;

	// note: the maximimum block count is 500 million with System 7 so we
	//		 don't have to worry about overflow when adding bitsPerBlock
	
	defaults->allocationClumpSize = ((blockCount + bitsPerBlock - 1) / bitsPerBlock) * gBlockSize;
}


static void MakeHFSDefaults(UInt32 sectorCount, UInt32 sectorSize, HFSDefaults *defaults)
{
	UInt32	alBlkSize;
	UInt32	defaultBlockSize;

	// Compute the default allocation block size
	defaultBlockSize = sectorSize * ((sectorCount >> 16) + 1);

	// If allocation block size is undefined or invalid calculate it…
	alBlkSize = gBlockSize;

	if ( alBlkSize == 0 || (alBlkSize & 0x1FF) != 0 || alBlkSize < defaultBlockSize)
		alBlkSize = defaultBlockSize;

	if ((alBlkSize & 0x0000FFFF) == 0)			// we cannot allow the lower word to be zero!
		alBlkSize += sectorSize;				// if it is, increase by one block

	defaults->abSize = alBlkSize;

	defaults->clpSize = alBlkSize * 4;			// use 4 * allocation block size by default
	if ( defaults->clpSize > 0x100000 )			// for large volumes, just use alBlkSize
		defaults->clpSize = alBlkSize;

	*(UInt16*) defaults->sigWord = kHFSSigWord;

	defaults->nxFreeFN = kHFSFirstUserCatalogNodeID;

	defaults->btClpSize = CalcBTreeClumpSize (alBlkSize, sectorSize, sectorCount);
}


//_______________________________________________________________________
//
//	CalcBTreeClumpSize
//	
//	This routine calculates the file clump size for both the catalog and
//	extents overflow files. In general, this is 1/128 the size of the
//	volume up to a maximum of 1 MB.  For really large volumes it will be
//	just 1 allocation block.
//_______________________________________________________________________

static UInt32
CalcBTreeClumpSize(UInt32 blockSize, UInt32 nodeSize, UInt32 driveBlocks)
{
	UInt32	clumpSize;
	UInt32	maximumClumpSize;
	UInt32	nodeBitsInHeader;
	

	nodeBitsInHeader = 8 * (nodeSize - sizeof(HeaderRec) - kBTreeHeaderUserBytes - 4*sizeof(SInt16));
	maximumClumpSize = nodeBitsInHeader * nodeSize;
	
	if ( maximumClumpSize > 0x400000 )	// max out at 4MB
		maximumClumpSize = 0x400000;

	if ( blockSize >= maximumClumpSize )
	{
		clumpSize = blockSize;		// for really large volumes just use one allocation block (HFS only)
	}
	else
	{
		if ( driveBlocks > 128 )
		{
			clumpSize = (driveBlocks / 128) << kLog2SectorSize;		// the default is 1/128 of the volume size
	
			if (clumpSize > maximumClumpSize)
				clumpSize = maximumClumpSize;
		}
		else
		{
			clumpSize = blockSize * 4;		// for really small volumes (ie < 64K)
		}
		
		if ( nodeSize > blockSize )
			clumpSize = (clumpSize / nodeSize) * nodeSize;			// now truncate to nearest node
		else
			clumpSize = (clumpSize / blockSize) * blockSize;		// now truncate to nearest node and allocation block
	}

	return clumpSize;
}


/* VARARGS */
void
#if __STDC__
fatal(const char *fmt, ...)
#else
fatal(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	if (fcntl(STDERR_FILENO, F_GETFL) < 0) {
		openlog(progname, LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_ERR, fmt, ap);
		closelog();
	} else {
		vwarnx(fmt, ap);
	}
	va_end(ap);
	exit(1);
	/* NOTREACHED */
}


void usage()
{
	fprintf(stderr, "usage: %s [ -fsoptions ] special-device\n", progname);
	fprintf(stderr, "where fsoptions are:\n");
//	fprintf(stderr, "\t-D debug\n");
	fprintf(stderr, "\t-H create an HFS format filesystem (HFS Plus is the default)\n");
	fprintf(stderr, "\t-N do not create file system, just print out parameters\n");
//	fprintf(stderr, "\t-O create a 4.3BSD format filesystem\n");
//	fprintf(stderr, "\t-S sector size\n");
//	fprintf(stderr, "\t-a maximum contiguous blocks\n");
	fprintf(stderr, "\t-b allocation block size (4096 optimal)\n");
//	fprintf(stderr, "\t-c cylinders/group\n");
//	fprintf(stderr, "\t-d rotational delay between contiguous blocks\n");
//	fprintf(stderr, "\t-e maximum blocks per file in a cylinder group\n");
//	fprintf(stderr, "\t-f frag size\n");
//	fprintf(stderr, "\t-i number of bytes per inode\n");
//	fprintf(stderr, "\t-k sector 0 skew, per track\n");
//	fprintf(stderr, "\t-l hardware sector interleave\n");
//	fprintf(stderr, "\t-m minimum free space %%\n");
//	fprintf(stderr, "\t-n number of distinguished rotational positions\n");
//	fprintf(stderr, "\t-o optimization preference (`space' or `time')\n");
//	fprintf(stderr, "\t-p spare sectors per track\n");
//	fprintf(stderr, "\t-s file system size (sectors)\n");
//	fprintf(stderr, "\t-r revolutions/minute\n");
//	fprintf(stderr, "\t-t tracks/cylinder\n");
//	fprintf(stderr, "\t-u sectors/track\n");
	fprintf(stderr, "\t-v volume name\n");
//	fprintf(stderr, "\t-x spare sectors per cylinder\n");

    fprintf(stderr, "Example:\n");
    fprintf(stderr, "\t%s -b 4096 -v Untitled /dev/sd0_hfs_a \n\n", progname);

	exit(1);
}
