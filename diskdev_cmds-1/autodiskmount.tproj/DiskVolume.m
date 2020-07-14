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
 * DiskVolume.m
 * - objects for DiskVolume and DiskVolumes
 * - a convenient way to store information about filesystems (volumes)
 *   and perform actions on them
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
#import <unistd.h>
#import <string.h>
#import <driverkit/IODeviceMaster.h>
#import <driverkit/IOProperties.h>
#import <sys/ioctl.h>
#import <bsd/dev/disk.h>
#import <errno.h>
#import <sys/wait.h>
#import <sys/param.h>
#import <sys/mount.h>
#import <grp.h>
#import <ufs/ufs/ufsmount.h>

#import <objc/Object.h>
#import	<mach/boolean.h>
#import <objc/List.h>
#import <kernserv/loadable_fs.h>

#import "DiskVolume.h"

#define MAXNAMELEN	256

static void
eject_media(char * dev)
{
    int		fd;
    char	specName[MAXNAMELEN];

    sprintf(specName, "/dev/r%s%c", dev, 
	    (dev[0] == 'f' && dev[1] == 'd') ? 'b' : 'h');
    fd = open(specName, O_RDONLY | O_NDELAY);
    if (fd > 0) {
	if (ioctl(fd, DKIOCEJECT, 0) < 0)
	    fprintf(stderr, "eject %s failed: %s\n", dev, strerror(errno));
	close(fd);
    }
    return;
}

static struct statfs *
get_fsstat_list(int * number)
{
    int n;
    struct statfs * stat_p;

    n = getfsstat(NULL, 0, MNT_NOWAIT);
    if (n <= 0)
	return (NULL);

    stat_p = (struct statfs *)malloc(n * sizeof(*stat_p));
    if (stat_p == NULL)
	return (NULL);

    if (getfsstat(stat_p, n * sizeof(*stat_p), MNT_NOWAIT) <= 0) {
	free(stat_p);
	return (NULL);
    }
    *number = n;
    return (stat_p);
}

static struct statfs *
fsstat_lookup_spec(struct statfs * list_p, int n, char * spec, char * fstype)
{
    char 		alt_spec[MAXNAMELEN];
    int 		i;
    struct statfs * 	scan;

    sprintf(alt_spec, "/private%s", spec);
    for (i = 0, scan = list_p; i < n; i++, scan++) {
	if (strcmp(scan->f_fstypename, fstype) == 0
	    && (strcmp(scan->f_mntfromname, spec) == 0
		|| strcmp(scan->f_mntfromname, alt_spec) == 0))
	    return (scan);
    }
    return (NULL);
}

#if 0
static struct statfs *
fsstat_lookup_mount(struct statfs * list_p, int n, char * mount)
{
    struct statfs * scan;
    int i;

    for (i = 0, scan = list_p; i < n; i++, scan++) {
	if (strcmp(scan->f_mntonname, mount) == 0)
	    return (scan);
    }
    return (NULL);
}

static void
print_fsstat_list(struct statfs * stat_p, int number)
{
    int i;

    for (i = 0; i < number; i++) {
	struct statfs * p = stat_p + i;
	printf("%s (%x %x) on %s from %s\n", p->f_fstypename, 
	       p->f_fsid.val[0], p->f_fsid.val[1], p->f_mntonname, 
	       p->f_mntfromname);
    }
}
#endif

static boolean_t
get_label(char * dev, struct disk_label * l)
{
    char specName[MAXNAMELEN];
    int lfd;
    
    sprintf(specName, "/dev/r%s%c", dev, 
	    (dev[0] == 'f' && dev[1] == 'd') ? 'b' : 'h');
    lfd = open(specName, O_RDONLY | O_NDELAY);
    if (lfd <= 0) {
	return (FALSE);
    }
    if (ioctl(lfd, DKIOCGLABEL, l) < 0) {
	close(lfd);
	return (FALSE);
    }
    close(lfd);
    return (TRUE);
}

int 
getDeviceInfo(const char *devName, char *type, 
	      BOOL * isWritable, BOOL * isRemovable) 
{
    IODeviceMaster *	dm;
    IOObjectNumber	objNum;
    IOString 		deviceKind;
    IOReturn		rtn;
    char 		str[1024];

    dm = [IODeviceMaster new];

    rtn = [dm lookUpByDeviceName:(char *)devName
	   objectNumber:&objNum
	   deviceKind:&deviceKind];
    if (rtn == IO_R_SUCCESS) {
        rtn = [dm getStringPropertyList:objNum
	       names:IOPropDeviceType
	       results:str maxLength:sizeof(str) ];
    }
    [dm free];
    if (rtn == IO_R_SUCCESS) {
        if ( isWritable != NULL ) {
            *isWritable = strstr(str,IOTypeWriteProtectedDisk) == NULL;
        }
        
        if ( isRemovable != NULL ) {
            *isRemovable = strstr(str,IOTypeRemovableDisk) != NULL;
        }
        if ( type != NULL ) {
            char * result = "";
            if (strstr(str,IOTypeFloppy)) {
                result = "floppy";
	    } 
	    else if (strstr(str, IOTypeCDROM )) {
	        result = "cdrom";
            } 
	    else if (strstr(str,IOTypeOptical)) {
                result = "optical";
            } 
	    else if (strstr(str, IOTypeIDE)) { // ??? should we check IOTypeATAPI instead?
                result = "ide"; //** old code returned "hard" in this case
            } 
	    else if (strstr(str,IOTypeSCSI) || strstr(str,"scsi")) { //** look for "scsi" to workaround PEx bug, do so last to prefer IO string constants
                result = "scsi";
            } 
            strcpy(type,result);
        }
    }
    return rtn;
}

int
fsck_needed(char * devname)
{
    char command[1024];
    FILE * f;
    int ret;

    sprintf(command, "/sbin/fsck -q /dev/r%s", devname);
    f = popen(command, "w");
    if (f == NULL) {
	fprintf(stderr, "popen('%s') failed", command);
	return (FALSE);
    }
    fflush(f);
    ret = pclose(f);
    if (ret == 0)
	return (FALSE);
    return (TRUE);
}

#define FILESYSTEM_ERROR 		0
#define FILESYSTEM_MOUNTED 		1
#define FILESYSTEM_MOUNTED_ALREADY	2
#define FILESYSTEM_NEEDS_REPAIR 	3

/** ripped off from workspace - start **/
static void cleanUpAfterFork(void) {
    int fd, maxfd = getdtablesize();
    for (fd = 0; fd < maxfd; fd++)
	close(fd);
    if ((fd = open("/dev/tty", O_NDELAY)) >= 0) {
	ioctl(fd, TIOCNOTTY, 0);
	close(fd);
    }
    setgid(getgid());
    setuid(getuid());
    fd = open("/dev/null", O_RDONLY);
    fd = open("/dev/console", O_WRONLY);
    dup2(1, 2);
    if ((fd = open("/dev/tty", O_NDELAY)) >= 0) {
	ioctl(fd, TIOCNOTTY, 0);
	close(fd);
    }
}

static char * 
foreignLabel(char * fsName) {
    int fd;
    static char theLabel[MAXNAMELEN], buf[MAXPATHLEN];
    sprintf(buf,"%s/%s%s/%s%s", FS_DIR_LOCATION, fsName, FS_DIR_SUFFIX, fsName, FS_LABEL_SUFFIX);
    fd = open(buf, O_RDONLY, 0);
    if (fd > 0) {
	int i = read(fd,theLabel,255);
	close(fd);
	if (i > 0) {
	    theLabel[i] = '\0';
	    return (theLabel);
	}
    }
    return (fsName);
}

static int foreignProbe(const char *fsName, const char *devName, const char *partitionName, int removable, int writable, int forMount) {
    char cmd[] = {'-', forMount? FSUC_PROBE : FSUC_PROBEFORINIT, 0};
    char execPath[MAXPATHLEN];
    const char *childArgv[] = {execPath,cmd, devName, removable? DEVICE_REMOVABLE : DEVICE_FIXED, writable? DEVICE_WRITABLE : DEVICE_READONLY, 0};
    char fsDir[MAXPATHLEN];
    int pid;
    BOOL isHFSpartition = (strstr(partitionName,"_hfs_")!=NULL);
    if (isHFSpartition) {
        childArgv[2] = partitionName;
    }
    sprintf(fsDir,"%s/%s%s",FS_DIR_LOCATION,fsName,FS_DIR_SUFFIX);
    sprintf(execPath,"%s/%s%s",fsDir,fsName,FS_UTIL_SUFFIX);
    if (access(execPath,F_OK) == 0) {
#ifdef DEBUG
	printf("%s %s %s %s %s\n", execPath, childArgv[1], childArgv[2], childArgv[3], childArgv[4]);
#endif DEBUG
        if ((pid = fork()) == 0) {
            cleanUpAfterFork();
            chdir(fsDir);
            execve(execPath, childArgv, 0);
            exit(-127);
        } else if (pid > 0) {
	    int statusp;
            if (wait4(pid,&statusp,0,NULL) > 0) {
                if (WIFEXITED(statusp)) return (int)(char)(WEXITSTATUS(statusp));
            }
        }
    }
    return FSUR_IO_FAIL;
}

int foreignMountDevice(const char *fsName, const char *devName, const char *partitionName, int removable, int writable, const char *mountPoint) {
    char cmd[] = {'-',FSUC_MOUNT,0};
    char execPath[MAXPATHLEN];
    const char *childArgv[] = {execPath,cmd,devName, mountPoint, removable? DEVICE_REMOVABLE : DEVICE_FIXED, writable? DEVICE_WRITABLE : DEVICE_READONLY, 0};
    char fsDir[MAXPATHLEN];
    int pid;
    BOOL isHFSpartition = (strstr(partitionName,"_hfs_")!=NULL);
    if (isHFSpartition) {
        childArgv[2] = partitionName;
    }
    sprintf(fsDir,"%s/%s%s",FS_DIR_LOCATION,fsName,FS_DIR_SUFFIX);
    sprintf(execPath,"%s/%s%s",fsDir,fsName,FS_UTIL_SUFFIX);

#ifdef DEBUG
    printf("%s %s %s %s %s %s\n", execPath, childArgv[1], childArgv[2], childArgv[3], childArgv[4], childArgv[5]);
#endif DEBUG

    if ((pid = fork()) == 0) {
	cleanUpAfterFork();
	chdir(fsDir);
	execve(execPath, childArgv, 0);
	exit(-127);
    } else if (pid > 0) {
	int statusp;
	if (wait4(pid,&statusp,0,NULL) > 0) {
	    if (WIFEXITED(statusp)) {
                int i = (int)(char)(WEXITSTATUS(statusp));
		if (i == FSUR_IO_SUCCESS) {
		    return FILESYSTEM_MOUNTED;
		} else if (i == FSUR_IO_UNCLEAN) {
		    return FILESYSTEM_NEEDS_REPAIR;
		}
	    }
	}
    }
    return FILESYSTEM_ERROR;
}
/** ripped off from workspace - end **/

@implementation DiskVolume

- (boolean_t) mount_foreign
{
    int ret;

    ret = foreignMountDevice(fs_type, disk_device, dev_name, removable, 
			     writable, mount_point);
    if (ret == FILESYSTEM_MOUNTED) {
	printf("Mounted %s /dev/%s on %s\n", fs_type,
	       dev_name, mount_point);
	[self setMounted:TRUE];
	return (TRUE);
    }
    return (FALSE);
}

- (boolean_t) mount_ufs
{
    struct ufs_args 	args;
    int 		mntflags = 0;
    char 		specname[MAXNAMELEN];

    sprintf(specname, "/dev/%s", dev_name);
    args.fspec = specname;		/* The name of the device file. */
    if (writable == FALSE)
	mntflags |= MNT_RDONLY;
    if (removable)
	mntflags |= MNT_NOSUID | MNT_NODEV;
#define DEFAULT_ROOTUID	-2
    args.export.ex_root = DEFAULT_ROOTUID;
    if (mntflags & MNT_RDONLY)
	args.export.ex_flags = MNT_EXRDONLY;
    else
	args.export.ex_flags = 0;
    
    if (mount(FS_TYPE_UFS, mount_point, mntflags, &args) < 0) {
	fprintf(stderr, "mount %s on %s failed: %s\n",
		specname, mount_point, strerror(errno));
	return (FALSE);
    }
    [self setMounted:TRUE];
    printf("Mounted ufs %s on %s\n", specname, mount_point);
    return (TRUE);
}

- (boolean_t) mount
{
    if (strcmp(fs_type, FS_TYPE_UFS) == 0)
	return [self mount_ufs];
    if (strcmp(fs_type, FS_TYPE_HFS) == 0)
	return [self mount_foreign];
    if (strcmp(fs_type, FS_TYPE_CD9660) == 0)
	return [self mount_foreign];
    return (FALSE);
}

#define FORMAT_STRING		"%-6s %-6s %-8s %-10s %-5s %-5s %-16s %-16s\n"
- print
{
    printf(FORMAT_STRING, disk_device, dev_type, fs_type, dev_name,
	   !removable ? "yes" : "no",
	   writable ? "yes" : "no",
	   disk_name, 
	   mounted ? mount_point : "[not mounted]");
    return self;
}

- set:(char * *) var Str:(char *)val
{
    if (*var)
	free(*var);
    if (val == NULL)
	*var = NULL;
    else 
	*var = strdup(val);
    return (self);
}

- setDeviceName:(char *)d
{
    return [self set:&dev_name Str:d];
}

- setDeviceType:(char *)t
{
    return [self set:&dev_type Str:t];
}

- setFSType:(char *)t
{
    return [self set:&fs_type Str:t];
}

- setDiskName:(char *)d
{
    return [self set:&disk_name Str:d];
}

- setDiskDeviceName:(char *)d
{
    return [self set:&disk_device Str:d];
}

- setMountPoint:(char *)m
{
    return [self set:&mount_point Str:m];
}

- setMounted:(boolean_t)val
{
    mounted = val;
    return self;
}

- setWritable:(boolean_t)val
{
    writable = val;
    return self;
}

- setRemovable:(boolean_t)val
{
    removable = val;
    return self;
}

- setDirtyFS:(boolean_t)val
{
    dirty = val;
    return self;
}

- free
{
    int 	i;
    char * * 	l[6] = { 
	&fs_type, &dev_type, &dev_name, &disk_device,
	&disk_name, &mount_point,
    };

    for (i = 0; i < 6; i++) {
	if (*(l[i]))
	    free(*(l[i]));
	*(l[i]) = NULL;
		 
    }
    return [super free];
}
@end

@implementation DiskVolumes

- getMountedVolume:(char *)devName Type:(char *)type
 FSSpec:(struct statfs *)fs_p Writable:(boolean_t)w Removable:(boolean_t)r
{
    id		disk;
    char * 	dev = strrchr(fs_p->f_mntfromname, '/');

    if (dev == NULL)
	dev = fs_p->f_mntfromname;
    else
	dev++;
    disk = [[DiskVolume alloc] init];
    [disk setDiskDeviceName:devName];
    [disk setFSType:fs_p->f_fstypename];
    [disk setDiskName:fs_p->f_mntonname + 1];
    [disk setDeviceType:type];
    [disk setWritable:w];
    [disk setRemovable:r];
    [disk setDeviceName:dev];
    [disk setMounted:TRUE];
    [disk setMountPoint:fs_p->f_mntonname];
    return (disk);
}

- init:(boolean_t)do_removable Eject:(boolean_t)eject
{
    IODeviceMaster *	dm = nil;
    char *		devName;
    IOReturn		rtn;
    char        	query[sizeof(IOPropDeviceClass)+sizeof(IOClassDisk)+10];
    char 		str[512];
    boolean_t		success = FALSE;
    struct statfs * 	stat_p;
    int			stat_number;
    char		type[512];
    BOOL		isWritable, isRemovable;
    
    if ([super init] == nil)
	return nil;

    list = [[List alloc] init];
    stat_p = get_fsstat_list(&stat_number);
    if (stat_p == NULL || stat_number == 0)
	goto err;

    dm = [IODeviceMaster new];
    if (dm == nil)
	goto err;
    sprintf(query, "\"%s\"=\"%s\"", IOPropDeviceClass, IOClassDisk);
    rtn = [dm lookUpByStringPropertyList:query
	   results:str maxLength:sizeof(str)];
    if (rtn == IO_R_SUCCESS) {
        devName = strtok(str," ");
        while (devName != NULL) {
	    struct disk_label label;

	    getDeviceInfo(devName, type, &isWritable, &isRemovable);
	    if (isRemovable && do_removable == FALSE) {
		if (eject)
		    eject_media(devName);
	    }
	    else {
		id 		disk = nil;
		struct statfs *	fs_p;
		int 		part;
		char		devPart[MAXNAMELEN];
		int 		ret;
		char		specName[MAXNAMELEN];

		/* check for ufs partitions */
		if (get_label(devName, &label)) { /* ufs partitions */
		    struct disktab * 	dt = &(label.dl_dt);
		    
#ifdef DEBUG
		    printf("get_label %s succeeded\n", devName);
#endif DEBUG
		    for (part = 0; part < NPART; part++) {
			struct partition * 	pp;
			
			pp = &dt->d_partitions[part];
			if (pp->p_newfs == 0 || pp->p_base == -1)
			    continue;
			disk = [[DiskVolume alloc] init];
			[disk setDiskDeviceName:devName];
			[disk setFSType:FS_TYPE_UFS];
			[disk setDiskName:label.dl_label];
			[disk setDeviceType:type];
			[disk setWritable:isWritable];
			[disk setRemovable:isRemovable];
			sprintf(devPart, "%s%c", devName, 'a' + part);
			[disk setDeviceName:devPart];
			sprintf(specName, "/dev/%s", devPart);
			fs_p = fsstat_lookup_spec(stat_p, stat_number, 
						  specName, FS_TYPE_UFS);
			if (fs_p) {
			    [disk setMounted:TRUE];
			    [disk setMountPoint:fs_p->f_mntonname];
			}
			else if (isWritable)
			    [disk setDirtyFS:fsck_needed(devPart)];
			[list addObject:disk];
		    }
		}
		/* check for hfs volumes */
		for (part = 'a'; part < 'h'; part++) {
		    int		fd;
		    
		    sprintf(devPart, "%s_hfs_%c", devName, part);
		    sprintf(specName, "/dev/%s", devPart);
		    fd = open(specName, O_RDONLY | O_NDELAY);
		    if (fd <= 0) {
			if (errno == EBUSY) {
			    sprintf(specName, "/dev/%s", devPart);
			    fs_p = fsstat_lookup_spec(stat_p, stat_number,
						      specName, FS_TYPE_HFS);
			    if (fs_p) {
				disk = [self getMountedVolume:devName Type:type
				        FSSpec:fs_p Writable:isWritable
				        Removable:isRemovable];
				if (disk != nil)
				    [list addObject:disk];
			    }
			}
		    }
		    else {
			close(fd);
			ret = foreignProbe(FS_TYPE_HFS, devName, devPart, 
					   isRemovable, isWritable, TRUE);
			if (ret == FSUR_RECOGNIZED || ret == -9) {
			    char * volume_name = foreignLabel(FS_TYPE_HFS);
			    
			    disk = [[DiskVolume alloc] init];
			    [disk setDiskDeviceName:devName];
			    [disk setFSType:FS_TYPE_HFS];
			    [disk setDiskName:volume_name];
			    [disk setDeviceType:type];
			    [disk setWritable:isWritable];
			    [disk setRemovable:isRemovable];
			    [disk setDeviceName:devPart];
			    [list addObject:disk];
			}
		    }
		}
		/* check for cd9660 volume */
		sprintf(devPart, "%sa", devName);
		sprintf(specName, "/dev/%s", devPart);
		fs_p = fsstat_lookup_spec(stat_p, stat_number, 
					  specName, FS_TYPE_CD9660);
		if (fs_p) { /* already mounted */
		    disk = [self getMountedVolume:devName Type:type
			    FSSpec:fs_p Writable:isWritable
			    Removable:isRemovable];
		    if (disk != nil)
			[list addObject:disk];
		}
		else { /* check if it's there */
		    int fd = open(specName, O_NDELAY | O_RDONLY);
		    if (fd <= 0)
			;
		    else {
			close(fd);
			ret = foreignProbe(FS_TYPE_CD9660, devName, devName,
					   isRemovable, isWritable, TRUE);
			if (ret == FSUR_RECOGNIZED || ret == -9) {
			    char * volume_name = foreignLabel(FS_TYPE_CD9660);
			    
			    disk = [[DiskVolume alloc] init];
			    [disk setDiskDeviceName:devName];
			    [disk setFSType:FS_TYPE_CD9660];
			    [disk setDiskName:volume_name];
			    [disk setDeviceType:type];
			    [disk setWritable:isWritable];
			    [disk setRemovable:isRemovable];
			    [disk setDeviceName:devPart];
			    [list addObject:disk];
			}
#ifdef DEBUG
			printf("ret for %s is %d\n", devPart, ret);
#endif DEBUG
		    }
		}
	    }
	    devName = strtok(NULL," ");
	}
    }
    success = TRUE;
  err:
    [dm free];
    if (stat_p)
	free(stat_p);
    if (success)
	return self;
    return [self free];
}

- print
{
    int i;

    printf(FORMAT_STRING, "Disk", "Type", "Filesys", "Dev", "Fixed",
	   "Write", "Volume Name", "Mounted On");
    for (i = 0; i < [list count]; i++)
	[[list objectAt:i] print];
    return self;
}

- (unsigned) count
{
    return [list count];
}

- objectAt:(unsigned)i
{
    return [list objectAt:i];
}

- free
{
    [[list freeObjects] free];
    list = nil;
    return [super free];
}

- volumeWithMount:(char *) path
{
    int i;

    for (i = 0; i < [list count]; i++) {
	DiskVolume * d = (DiskVolume *)[list objectAt:i];
	if (d->mounted && d->mount_point && strcmp(d->mount_point, path) == 0){
	    return (d);
	}
    }
    return (nil);
}

- (boolean_t) setVolumeMountPoint:d
{
    DiskVolume *	disk = (DiskVolume *)d;
    int 		i = 1;
    boolean_t		is_hfs = (strcmp(disk->fs_type, FS_TYPE_HFS) == 0);
    mode_t		mode = 0755;
    char 		mount_path[MAXLBLLEN + 32];
    struct stat 	sb;

    sprintf(mount_path, "/%s", disk->disk_name);
    while (1) {
	if (stat(mount_path, &sb) < 0) {
	    if (errno == ENOENT)
		break;
	    else {
		fprintf(stderr, "stat(%s) failed, %s\n", mount_path,
			strerror(errno));
		return (FALSE);
	    }
	}
	else if ([self volumeWithMount:mount_path])
	    ;
	else if (rmdir(mount_path) == 0) {
	    /* it was an empty directory */
	    break;
	}
	sprintf(mount_path, "/%s %d", disk->disk_name, i);
	i++;
    }
    /*
     * For HFS and HFS+ volumes, we change the mode of the mount point.
     * The reason is that this mode is used to determine the default
     * mode for files and directories which have no mode (Mac OS created
     * them). We also set the group owner to "macos" to Blue Box users
     * can open and edit these modeless files.
     */
    if (is_hfs)
	mode = 0775; /* Radar 2286518 */
    if (mkdir(mount_path, mode) < 0) {
	fprintf(stderr, 
		"mountDisks: mkdir(%s) failed, %s\n",
		mount_path,
		strerror(errno));
	return (FALSE);
    }
    if (is_hfs) { /* Radar 2286518 */
	struct group * grent = getgrnam("macos");

	/* change the group to macos (if it exists) */
	if (grent) {
	    if (chown(mount_path, 0, grent->gr_gid) < 0) {
		fprintf(stderr, "mountDisks: chown(%s) failed: %s\n",
			mount_path, strerror(errno));
	    }
	}
	if (chmod(mount_path, mode) < 0) {
	    fprintf(stderr, "mountDisks: chmod(%s) failed: %s\n",
		    mount_path, strerror(errno));
	}
    }
    [disk setMountPoint:mount_path];
    return (TRUE);
}

@end

