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
/*	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * ConfigUfs.c - Unix File System routines used by ConfigScan.
 *
 * HISTORY
 * 14-Apr-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <driverkit/userConfigServer.h>
#import "ConfigPrivate.h"
#import "ConfigUtils.h"
#import "ConfigScan.h"
#import "ConfigUfs.h"
#import <bsd/sys/dir.h>
#import <bsd/libc.h>
#include <sys/stat.h>

typedef struct {
	file_id_t 	fid;
	filename_t 	filename;
	u_short 	dev_revision;
} exec_file_t;
	

/*
 * Search for an executable appropriate for handling device specified by
 * dev_type and slot_id. Returns TRUE if executable found.
 *
 * This is totally UFS dependent. Maybe when we have an idea of what will
 * replace UFS, we can make some of this more general.
 */
#define FAKE_FILE	0
#if	FAKE_FILE
filename_t some_file;
file_id_t some_fid = 0;

boolean_t get_driver_file(
	IODeviceType dev_type,
	IOSlotId slot_id,
	filename_t *filename,		// returned
	file_id_t *fid) 		// returned
{
	xpr_common("get_driver_file\n", 1,2,3,4,5);
	/*
	 * Fake a name for now.
	 */
	if(dev_type == IO_SLOT_DEVICE_TYPE)
		sprintf(some_file, "devs_%04X_0000", slot_id);
	else
		sprintf(some_file, "devr_%04X_0000", dev_type);
	strncpy((char *)filename, (const char *)some_file, FILENAME_SIZE);
	*fid = some_fid++;
	xpr_common("...returning filename %s\n", 
		IOCopyString(some_file), 2,3,4,5);
	return(TRUE);
}

#else	FAKE_FILE

static boolean_t no_driver_dir_note = FALSE;

boolean_t get_driver_file(
	IODeviceType dev_type,
	IOSlotId slot_id,
	filename_t *filename,		// returned
	file_id_t *fid) 		// returned
{
	exec_file_t 	exec_file;
	IODeviceTypeUn 	dev_type_u;
	filename_t 	file_prefix;
	int 		prefix_length;
  	struct direct 	**namelist;
	int 		i;
	u_short 	revision;
	boolean_t 	rtn;
	struct stat 	statbuf;
	
	xpr_common("get_driver_file: dev_type 0x%x  slot_id 0x%x\n", 
		dev_type, slot_id, 3,4,5);
	
	/*
	 * Start out with a null file. 
	 */
	dev_type_u.deviceType = dev_type;
	xpr_common("dev_type_u.dev_type 0x%x  ir.dev_index 0x%x\n",
		dev_type_u.deviceType, dev_type_u.deviceTypeIr.deviceIndex,
		3,4,5);
	exec_file.fid = FID_NULL;
	exec_file.filename[0] = '\0';
	exec_file.dev_revision = 0;
	
	/*
	 * Generate the filename portion which must match exactly.
	 */
	if(dev_type == IO_SLOT_DEVICE_TYPE) {
		sprintf(file_prefix, "devs_%08X_", slot_id);
	}
	else {
		sprintf(file_prefix, "devr_%04X_", 
			(unsigned)dev_type_u.deviceTypeIr.deviceIndex);
	}
	prefix_length = strlen(file_prefix);
	
	/*
	 * stat DRIVER_PATH first to make sure it exists.
	 */
	if(stat(DRIVER_PATH, &statbuf)) {
		xpr_common("get_driver_file: %s NOT FOUND\n", 
			DRIVER_PATH,2,3,4,5);
		if(!no_driver_dir_note) {
			no_driver_dir_note = TRUE;
			printf("Warning: %s does not exist\n", DRIVER_PATH);
		}
		return FALSE;
	}
	if((statbuf.st_mode & S_IFDIR) == 0) {
		if(!no_driver_dir_note) {
			no_driver_dir_note = TRUE;
			printf("Warning: %s is not a directory\n",
				DRIVER_PATH);
		}
		return FALSE;
	}
	no_driver_dir_note = FALSE;
	
	/* 
	 * read in the DRIVER_PATH directory, check each entry. Each time
	 * we get a valid filename with a revision greater than the one
	 * we have, use that one.
	 */
	scandir(DRIVER_PATH, &namelist, NULL, NULL);
	for(i=0; namelist[i]; i++) {
		if(strncmp(namelist[i]->d_name, file_prefix, prefix_length))
			continue;		// bad prefix
		if(get_driver_rev(namelist[i]->d_name, &revision))
			continue;		// bad filename format
		if(revision >= exec_file.dev_revision) {
			/*
			 * Let's use this one.
			 */
			exec_file.fid = namelist[i]->d_fileno;
			strcpy(exec_file.filename, namelist[i]->d_name);
			exec_file.dev_revision = revision;
		}
	}
	if(exec_file.filename[0]) {
		xpr_common("get_driver_file: returning filename %s\n", 
			IOCopyString(exec_file.filename), 2,3,4,5);
		strcpy((char *)filename, exec_file.filename);
		*fid = exec_file.fid;
		rtn = TRUE;
	}
	else {
		xpr_common("get_driver_file: DRIVER NOT FOUND\n", 1,2,3,4,5);
		rtn = FALSE;
	}
	
	/*
	 * Free the namelist we got with scandir().
	 */
	for(i=0; namelist[i]; i++) {
		free(namelist[i]);
	}
	free(namelist);
	return(rtn);
}

#endif	FAKE_FILE


