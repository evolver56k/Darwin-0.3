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
 * autodiskmount.m
 * - fscks and mounts all currently unmounted ufs filesystems, and mounts any
 *   hfs or cd9660 filesystems
 * - by default, only filesystems on fixed disks are mounted
 * - option '-a' means mount removable media too
 * - option '-e' means eject removable media (overrides '-a')
 * - option '-n' means don't mount anything, can be combined
 *   with other options to eject all removable media without mounting anything
 * - option '-v' prints out what's already mounted (after doing mounts)
 */

/*
 * Modification History:
 *
 * Dieter Siegmund (dieter@apple.com) Thu Aug 20 18:31:29 PDT 1998
 * - initial revision
 * Dieter Siegmund (dieter@apple.com) Thu Oct  1 13:42:34 PDT 1998
 * - added support for hfs and cd9660 filesystems
 */
#import <libc.h>
#import <stdlib.h>
#import <string.h>
#import <driverkit/IODeviceMaster.h>
#import <driverkit/IOProperties.h>
#import <sys/ioctl.h>
#import <bsd/dev/disk.h>
#import <errno.h>
#import <sys/param.h>
#import <sys/mount.h>
#import <ufs/ufs/ufsmount.h>

#import <objc/Object.h>
#import	<mach/boolean.h>
#import <objc/List.h>
#import "DiskVolume.h"


boolean_t
fsck_vols(id vols) 
{
    char 		command[1024];
    int			dirty_fs = 0;
    int 		i;
    DiskVolume * 	vol;

    for (i = 0; i < [vols count]; i++) {
	vol = (DiskVolume *)[vols objectAt:i];
	if (vol->writable && vol->dirty && vol->mount_point == NULL) {
	    if (dirty_fs == 0)
		sprintf(command, "/sbin/fsck -y");
	    strcat(command, " /dev/r");
	    strcat(command, vol->dev_name);
	    dirty_fs++;
	}
    }
    if (dirty_fs) {
	FILE *		f;
	int 		ret;
	printf("command to execute is '%s'\n", command);
	f = popen(command, "w");
	if (f == NULL) {
	    fprintf(stderr, "popen('%s') failed", command);
	    return (FALSE);
	}
	fflush(f);
	ret = pclose(f);
	if (ret == 0)
	    return (TRUE);
	return (FALSE);
    }
    return (TRUE);
}

boolean_t 
mount_vols(id vols)
{
    int i;

    for (i = 0; i < [vols count]; i++) {
	DiskVolume * d = (DiskVolume *)[vols objectAt:i];

	/* already mounted, skip this volume */
	if (d->mounted)
	    continue;

	/* determine/create the mount point */
	if ([vols setVolumeMountPoint:d] == FALSE) {
	    continue;
	}
	[d mount];
    }
    return (TRUE);
}

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

int
main(int argc, char * argv[])
{
    boolean_t eject_removable = FALSE;
    boolean_t do_removable = FALSE;
    boolean_t verbose = FALSE;
    boolean_t do_mount = TRUE;
    char * progname;
    char ch;
    id vols;

    progname = argv[0];

    if (getuid() != 0) {
	fprintf(stderr, "%s: must be run as root\n", progname);
	exit(1);
    }
    while ((ch = getopt(argc, argv, "avne")) != -1) {
	switch (ch) {
	  case 'a':
	    do_removable = TRUE;
	    break;
	  case 'v':
	    verbose = TRUE;
	    break;
	  case 'n':
	    do_mount = FALSE;
	    break;
	  case 'e':
	    eject_removable = TRUE;
	    break;
	}
    }
    if (eject_removable)
	do_removable = FALSE; /* sorry, eject overrides use */
    vols = [[DiskVolumes alloc] init:do_removable Eject:eject_removable];
    if (do_mount) {
	if (fsck_vols(vols) == FALSE) {
	    fprintf(stderr, "fsck failed - exiting");
	    exit(1);
	}
	if (mount_vols(vols) == FALSE) {
	    fprintf(stderr, "mounts failed - exiting");
	    exit(2);
	}
    }
    if (verbose)
	[vols print];
    [vols free];
    exit(0);
}
