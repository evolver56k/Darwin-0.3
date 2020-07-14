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
 * DiskVolume.h
 * - objects for DiskVolume and DiskVolumes
 * - a convenient way to store information about filesystems (volumes)
 */
@interface DiskVolume : Object
{
@public
    char *	fs_type;
    char *	disk_device;
    char *	dev_name;
    char *	dev_type;
    char *	disk_name;
    char *	mount_point;
    boolean_t	removable;
    boolean_t	writable;
    boolean_t	dirty;
    boolean_t	mounted;
}

- print;
- setDiskDeviceName:(char *)d;
- setDeviceName:(char *)d;
- setDeviceType:(char *)t;
- setFSType:(char *)t;
- setDiskName:(char *)n;
- setMountPoint:(char *)m;
- setRemovable:(boolean_t)val;
- setWritable:(boolean_t)val;
- setDirtyFS:(boolean_t)val;
- setMounted:(boolean_t)val;
- free;
- (boolean_t) mount;
@end

@interface DiskVolumes : Object
{
    id		list;
}
- init:(boolean_t)do_removable Eject:(boolean_t)eject;
- free;
- objectAt:(unsigned)i;
- volumeWithMount:(char *) path;
- (boolean_t) setVolumeMountPoint:vol;
- print;
@end

#define FS_TYPE_HFS	"hfs"
#define FS_TYPE_UFS	"ufs"
#define FS_TYPE_CD9660	"cd9660"
