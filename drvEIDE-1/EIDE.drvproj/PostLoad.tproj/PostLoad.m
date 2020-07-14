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
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * Post-Load command for IDE disks.  Creates device nodes /dev/hd[0-3][a-h]
 * and /dev/rhd[0-3][a-h].
 * units.
 *
 * HISTORY
 * 30-Sep-94    Rakesh Dubey at NeXT
 *      Created from SCSI tape source. 
 */

#import <streams/streams.h>
#import <driverkit/IODeviceMaster.h>
#import <driverkit/IODevice.h>
#import <errno.h>
#import <libc.h>

#import <ata_extern.h>

#define PATH_NAME_SIZE 			10
#define DEV_STRING 			"/dev/"
#define IDE_INIT_ERR_STRING 		"Error initializing IDE driver"

#define DEV_MOD_CHAR 			020640
#define DEV_MOD_BLOCK 			060640

#define DEV_UMASK 			0

#define NIDE_PARTITIONS 		8

#define NIDE_DEVICES			(MAX_IDE_DRIVES * MAX_IDE_CONTROLLERS)

static int makeNode(char *deviceName, int iUnit, int major, int num,
			unsigned short mode);

int main(int argc, char **argv)
{
    IOReturn			ret;
    IOObjectNumber		tag;
    IOString			kind;
    unsigned int		count;
    IODeviceMaster		*devMaster;
    int				blockMajor, characterMajor;
    int				i, iUnit, iRet;
    char 			path[PATH_NAME_SIZE];

    iRet = 0;
    
    devMaster = [IODeviceMaster new];

    /*
     * The kernel creates first two. This needs some investigation. FIXME.
     */
    for (iUnit = 2; iUnit < NIDE_DEVICES; iUnit ++) {

	bzero(path, PATH_NAME_SIZE);
	sprintf(path, "%s%s%d", DEV_STRING, "hd", iUnit);

	/*
	 * Find this instance of the IDE driver
	 */

	ret = [devMaster lookUpByDeviceName: path + strlen(DEV_STRING)
	    objectNumber: &tag 
	    deviceKind: &kind];

#ifdef DEBUG
printf ("unit %d, obj name %s\n", iUnit, path);
printf ("ret = %d\n", ret);
#endif DEBUG

	/*
	 * If this device does not exist then we don't want to create nodes
	 * for it. This saves some time.
	 */
	if (ret != IO_R_SUCCESS) {
	    continue;
	}
	
	/*
	 * Query the object for its major device number 
	 */
	blockMajor = -1;
	ret = [devMaster getIntValues:&blockMajor
	       forParameter:"BlockMajor" objectNumber:tag
	       count:&count];
	if (ret != IO_R_SUCCESS) {
	    printf("%s: couldn't get block major number:  Returned %d.\n",
		   IDE_INIT_ERR_STRING, ret);
	    iRet = -1;
	}
	
	characterMajor = -1;
	ret = [devMaster getIntValues:&characterMajor
	       forParameter:"CharacterMajor" objectNumber:tag
	       count:&count];
	if (ret != IO_R_SUCCESS) {
	    printf("%s: couldn't get char major number:  Returned %d.\n",
		   IDE_INIT_ERR_STRING, ret);
	    iRet = -1;
	}
	
	/*
	 * Remove old devs and create new ones for this unit. 	 
	 */
	for (i = 0; i < NIDE_PARTITIONS; i++) {
	    iRet = makeNode("hd", iUnit, blockMajor, i, DEV_MOD_BLOCK);
	    iRet = makeNode("rhd", iUnit, characterMajor, i, DEV_MOD_CHAR);
	}
    }
    
    exit(iRet);
}

static int makeNode(char *deviceName, int iUnit, int major, int num,
			unsigned short mode)
{
    int	iRet;
    int minor;
    char path[PATH_NAME_SIZE];
    
    iRet = -1;
    
#ifdef notdef
printf ("makeNode %s, iUnit %d num %d\n", deviceName, iUnit, num);
#endif notdef

    bzero(path, PATH_NAME_SIZE);
    sprintf(path, "%s%s%d%c", DEV_STRING, deviceName, iUnit, num + 'a');

    minor = iUnit * NIDE_PARTITIONS + num;
    
    if (unlink(path)) {
	if (errno != ENOENT) {
	    printf("%s: could not delete old %s.  Errno is %d\n",
		IDE_INIT_ERR_STRING, path, errno);
	    iRet = -1; 
	}
    }

    umask(DEV_UMASK);
    if (mknod(path, mode, (major << 8) | minor)) {
	printf("%s: could not create %s.  Errno is %d\n",
	    IDE_INIT_ERR_STRING, path, errno);
	iRet = -1;
    }
    
    return iRet;
}
