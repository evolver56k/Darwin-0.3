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
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * ATADiskKernel.m - UNIX front end for kernel IDE/ATA Disk driver.
 *
 * HISTORY 
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 * 19-Jun-97	Dieter Siegmund at Apple
 *	Updated to use the BSD4.4 ioctl interface that copies user
 *	buffer in/out of kernel automatically.
 * 21-Feb-98	Brent Knight at Apple
 *  Added support for HFS partitions.  Radar 2204950, 2004660
 */
 
#if (IO_DRIVERKIT_VERSION == 400)
#define _POSIX_SOURCE
#endif

/*
 * Note that this file builds with KERNEL_PRIVATE and !MACH_USER_API.
 */
#import <sys/types.h>
#import <sys/ttycom.h>
#import <sys/ucred.h>
#import <driverkit/kernelDiskMethods.h> 
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/IODiskPartition.h>
#import <sys/buf.h>
#import <sys/uio.h>
#import <bsd/dev/ldd.h>
#import <sys/errno.h>
#import <sys/proc.h>
#if (IO_DRIVERKIT_VERSION == 330)
#import <vm/vm_kern.h>
#endif
#import <sys/fcntl.h>
#import <sys/systm.h>
#import "ATADiskInternal.h"
#import "ATADisk.h"
#import "ATADiskKernel.h"

#if (IO_DRIVERKIT_VERSION != 330)
extern struct vm_map *kernel_map;
#endif

#ifdef ppc //bknight - 2/21/98 - Radar 2204950, 2004660
#define GROK_APPLE 1
#endif //bknight - 2/21/98 - Radar 2204950, 2004660

#ifdef GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660
/*
 * For consistency with bsd/dev/SCSIDiskKern.m, we use the most
 * significant bit of the 8-bit minor device number to flag HFS
 * partitions.  For SCSI devices, this meant that there could be
 * 16 (= 4 bits) SCSI disks, each with 8 (= 3 bits) partitions,
 * either UFS or HFS (= 1 bit).
 *
 * In SCSIDiskKern.h, NUM_SD_DEV is defined to be 16, which is
 * just the right constant for checking that high bit of the
 * Unit number and that's what I used in SCSIDiskKern.m.
 *
 * Unfortunately, we can't just use NUM_IDE_DEV here, because
 * it is defined in IdeDiskInternal.h to be only 4.  And since
 * it seems inappropriate to include SCSIDiskKern.h here in
 * IdeKernel.m, I am instead making this definition, which might
 * some day migrate out into IdeDiskInternal.h, or which might
 * be made obsolete by the forthcoming IOKit design.
 */
#define MAX_IDE_UNIT 16
#endif GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

IONamedValue iderValues[] = {

	{IDER_SUCCESS,		"Success"			},
	{IDER_TIMEOUT,		"Timeout occured"		},
	{IDER_MEMALLOC,		"Couldn't allocate memory"	},
	{IDER_MEMFAIL,		"Memory transfer error"		},
	{IDER_REJECT,		"Bad field in ide_ioreq"	},
	{IDER_BADDRV,		"Drive not present"		},
	{IDER_CMD_ERROR,	"Command Failed"		},
	{IDER_VOLUNAVAIL,	"Requested Volume not available"},
	{IDER_SPURIOUS,		"Spurious Interrupt"		},
	{IDER_CNTRL_REJECT,	"Controller Reject" 		},
	{0,			NULL				},
};

static int ideOpenCount = 0;

/*
 * dev-to-id map array. Instances of DiskObject register their IDs in
 * this array via registerUnixDisk:.
 */
IODevAndIdInfo IdeIdMap[NUM_IDE_DEV];

/*
 * Private per-unit data.
 */
static Ide_dev_t ide_dev[NUM_IDE_DEV] = {
	{NULL},
	{NULL},
	{NULL},
	{NULL},
};

/*
 * Indices of our entries in devsw's.
 */
static int ide_block_major;
static int ide_raw_major;

/*
 * prototypes for internal functions
 */
static unsigned ideminphys(struct buf *bp); 
static id ide_dev_to_id(dev_t dev);

/*
 * Initialize id map and Ide_dev. Currently invoked by Ide probe:.
 */
__private_extern__ void ide_init_idmap(id self)
{
    IODevAndIdInfo *idMap = IdeIdMap;
    Ide_dev_t *ide_devp = ide_dev;
    int unit;
    
    /* 
     * figure out our major device numbers.
     */
    ide_block_major = [self blockMajor];
    ide_raw_major = [self characterMajor];

    bzero((char *)idMap, sizeof(IODevAndIdInfo) * NUM_IDE_DEV);
    for (unit = 0; unit < NUM_IDE_DEV; unit++) {
	idMap->rawDev   = makedev(ide_raw_major, (unit << 3));
	idMap->blockDev = makedev(ide_block_major, (unit << 3));
	idMap++;
	ide_devp->physbuf = (struct buf *)IOMalloc(sizeof(struct buf));
	ide_devp->physbuf->b_flags = 0;
	ide_devp++;
    }
}

__private_extern__ IODevAndIdInfo *ide_idmap()
{
    return IdeIdMap;
}

__private_extern__ int
ideopen(dev_t dev, int flag, int devtype, struct proc * pp)
{
    id diskObj = ide_dev_to_id(dev);
    
    if(diskObj == nil)
	return(ENXIO);

    if([diskObj isDiskReady:NO])
	return(ENXIO);
		
    /*
     * Register this 'Unix-level open' event for IODiskPartitions.
     */
    if(IO_DISK_PART(dev) != IDE_LIVE_PART) {
	if(major(dev) == ide_block_major) {
	    [diskObj setBlockDeviceOpen:YES];
            ideOpenCount++;
	}
	else {
	    [diskObj setRawDeviceOpen:YES];
	}
    }
    return(0);	
} /* ideopen() */

__private_extern__ int
ideclose(dev_t dev, int flag, int devtype, struct proc * pp)
{
    id diskObj = ide_dev_to_id(dev);
    
    if(diskObj == nil)
	return(ENXIO);
    if(IO_DISK_PART(dev) == IDE_LIVE_PART) { 
	return 0;
    } else 	{
	if(![diskObj isInstanceOpen])
	    return(ENXIO);
    }
	    
    /*
     * Register this 'Unix-level close' event. We won't be called unless
     * this is the last close.
     */
    if(major(dev) == ide_block_major) {
	[diskObj setBlockDeviceOpen:NO];
        if ( ideOpenCount )
        {
            if ( !--ideOpenCount )
            {
                IOSleep(1000);
            }
        }
    }
    else {
	[diskObj setRawDeviceOpen:NO];
    }
    
    return(0);	
} /* ideclose() */

/*
 * Raw I/O uses standard UNIX physio routine, resulting in async I/O requests
 * via idestrategy().
 */

__private_extern__ int
ideread(dev_t dev, struct uio *uiop, int ioflag)
{
    id 		diskObj = ide_dev_to_id(dev);
    int 	unit = IO_DISK_UNIT(dev);
    int 	rtn;

    if(diskObj == nil) {
	return(ENXIO);
    }


    rtn = physio(idestrategy, 
	ide_dev[unit].physbuf, 
	dev, 
	B_READ, 
	ideminphys, 
	uiop, 
	[diskObj blockSize]);
	    
    return rtn;
} /* ideread() */

__private_extern__ int
idewrite(dev_t dev, struct uio *uiop, int ioflag)
{
    id 		diskObj = ide_dev_to_id(dev);
    int 	unit = IO_DISK_UNIT(dev);
    int 	rtn;
	    
    if(diskObj == nil)
	return(ENXIO);
    

    rtn = physio(idestrategy, 
	ide_dev[unit].physbuf, 
	dev, 
	B_WRITE, 
	ideminphys, 
	uiop, 
	[diskObj blockSize]);
	    
    return rtn;
} /* idewrite() */

__private_extern__ void
idestrategy(struct buf *bp)
{
    id diskObj = ide_dev_to_id(bp->b_dev);
    u_int offset;
    u_int bytes_req;
    void *bufp;
    u_int block_size;
    IOReturn rtn;
    vm_task_t client;
	
    if(diskObj == nil) {
	bp->b_error = ENXIO;
	goto bad;
    }
    
    if((bp->b_flags & (B_PHYS|B_KERNSPACE)) == B_PHYS) {
	/*
	 * Physical I/O to user space.
	 */
	client = IOVmTaskForBuf(bp);
    }
    else {
	/*
	 * Either block I/O (always kernel space) or physical I/O
	 * to kernel space (e.g., loadable file system).
	 */
	client = IOVmTaskSelf();
    }
    
    block_size = [diskObj blockSize];
    if (block_size == 0) {
	bp->b_error = ENXIO;
	goto bad;
    }
    offset = bp->b_blkno;
    bytes_req = bp->b_bcount;
    bufp = bp->b_un.b_addr;

    if (bp->b_flags & B_READ) {
	rtn = [diskObj readAsyncAt:offset
		length:bytes_req
		buffer:bufp
		pending:bp
		client:client];
    }
    else {
	rtn = [diskObj writeAsyncAt:offset
		length:bytes_req
		buffer:bufp
		pending:bp
		client:client];
    }

    if (rtn) {
	bp->b_error = [diskObj errnoFromReturn:rtn];
	goto bad;
    }
    return(0);
    
bad:
    bp->b_flags |= B_ERROR;
    rtn = -1;
    biodone(bp);
    
    return(rtn);
} /* fdstrategy() */

/*
 * Ops which are common to all IODiskPartitions are done on the raw device; 
 * Ide-specific ioctls are done directly to the live Ide object.
 */
__private_extern__ int
ideioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc * pp)
{
    int     unit = IO_DISK_UNIT(dev);
#ifdef GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660
    int     part = IO_DISK_PART(dev);
#endif GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660
    id      diskObj;
    IODevAndIdInfo *idmap;
    int     rtn = 0;
    IOReturn irtn = IO_R_SUCCESS;
    struct ucred cred;
    u_short acflags;

    // user src/dest
    int     i;
    int     nblk;
    ideIoReq_t *ideIoReq;
    int     error;
    void   *userPtr;

    // in ideIoReq_t
    BOOL wrFlag = NO;
    unsigned bSize = 0;
    unsigned char *alignedPtr;


#ifdef GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

	if ( ! ( unit < MAX_IDE_UNIT ) )
	{
		unit -= MAX_IDE_UNIT;
		part += NUM_IDE_PART;
	}

#endif GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660
    if (unit > NUM_IDE_DEV)
	return (ENXIO);
#if 0
#ifdef GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

    if ((major(dev) != ide_raw_major) || (part >= 2*NUM_IDE_PART)) {

#else GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

    if ((major(dev) != ide_raw_major) || (part >= NUM_IDE_PART)) {

#endif GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660
	return (ENXIO);
    }
#endif

    idmap = &IdeIdMap[unit];

#ifdef GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

	/*
	 * Special cases for HFS partitions.
	 * DKIOCNUMBLKS - return # blocks on partition, not the whole device
	 * DKIOCSFORMAT - not permitted
	 * DKIOCGFORMAT - not permitted
	 * DKIOCGLABEL - not permitted
	 * DKIOCSLABEL - not permitted
	 */

	/* Is it an HFS partition ? */

	if ( ! ( part < NPART ) )
	{
		switch ( cmd )
		{
			case DKIOCNUMBLKS:
				/*
				 * For an HFS partition, return the # of blocks in the
				 * partition, rather than # of blocks on the whole device.
				 */
			
				diskObj = idmap->partitionId[part];
				*(int *)data = [diskObj diskSize];
				return (0);
				
			break;

		    case DKIOCSFORMAT:
		    case DKIOCGFORMAT:
		    case DKIOCGLABEL:
		    case DKIOCSLABEL:
		    	return(EINVAL);
		    break;
		}
	}
	
#endif GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

    /*
     * First verify valid device. 
     */
    switch (cmd) {
      case DKIOCSFORMAT:
      case DKIOCGFORMAT:
      case DKIOCGLABEL:
      case DKIOCSLABEL:

	/*
	 * Raw device, whatever the caller asked for. 
	 */
	diskObj = idmap->partitionId[0];
	break;

      case DKIOCGLOCATION:
        diskObj = idmap->partitionId[part];
	break;

      case IDEDIOCREQ:
      case IDEDIOCINFO:
      case DKIOCINFO:
      case DKIOCBLKSIZE:
      case DKIOCNUMBLKS:

	diskObj = idmap->liveId;
	break;

      default:
	return (EINVAL);
    }
    if (diskObj == nil)
	goto nodev;

    switch (cmd) {
      case DKIOCSFORMAT:

	/*
	 * This can fail if block devices attached to this disk are open. 
	 */
	irtn = [diskObj setFormatted:(*(u_int *) data)];
	break;

      case DKIOCGFORMAT:
	{
	    *(int *)data = [diskObj isFormatted];
	    break;
	}

      case DKIOCGLABEL:
	{
	    struct disk_label *labelp;

	    labelp = (struct disk_label *)IOMalloc(sizeof(*labelp));
	    irtn = [diskObj readLabel:labelp];
	    if (irtn == IO_R_SUCCESS) {
		*(struct disk_label *)data = *labelp;
	    }
	    IOFree(labelp, sizeof(*labelp));
	    break;
	}

      case DKIOCSLABEL:
	{
	    struct disk_label *labelp;

	    labelp = (struct disk_label *)IOMalloc(sizeof(*labelp));
	    *labelp = *(struct disk_label *)data;
	    irtn = [diskObj writeLabel:labelp];
	    IOFree(labelp, sizeof(*labelp));
	    break;
	}

      case DKIOCINFO:
	{
	    struct drive_info info;

	    bzero(&info, sizeof(info));
	    strcpy(info.di_name,[diskObj driveName]);
	    info.di_devblklen = [diskObj blockSize];
	    info.di_maxbcount = IDE_MAX_PHYS_IO;
	    if (info.di_devblklen) {
		nblk = howmany(sizeof(struct disk_label),
			       info.di_devblklen);
	    } else {
		nblk = 0;
	    }
	    for (i = 0; i < NLABELS; i++)
		info.di_label_blkno[i] = nblk * i;
	    *(struct drive_info *) data = info;
	    break;
	}

      case DKIOCBLKSIZE:
	*(int *)data = [diskObj blockSize];
	break;

      case DKIOCNUMBLKS:
	*(int *)data = [diskObj diskSize];
	break;

      case DKIOCGLOCATION:
        if( nil == [diskObj getDevicePath:((struct drive_location *)data)->location
                    maxLength:sizeof(((struct drive_location *)data)->location)
		    useAlias:YES]	)
                irtn = IO_R_NO_MEMORY;
        break;

      case IDEDIOCREQ:

	/*
	 * Perform specified I/O.
	 */
	ideIoReq = (ideIoReq_t *) data;
//	if (!suser() && (ideIoReq->cmd != IDE_IDENTIFY_DRIVE))
	if (suser(pp->p_ucred, &pp->p_acflag) &&
		(ideIoReq->cmd != IDE_IDENTIFY_DRIVE)) {
//	    return (u.u_error);
	    return (EINVAL);
	}

	if ((ideIoReq->cmd == IDE_WRITE_DMA) ||
	    (ideIoReq->cmd == IDE_READ_DMA)) {
	    if ([[diskObj cntrlr] isDmaSupported:[diskObj driveNum]] != TRUE)
		return (EINVAL);
	}
	
	if (ideIoReq->cmd == IDE_IDENTIFY_DRIVE)
	    ideIoReq->blkcnt = 1;

	userPtr = (void *)ideIoReq->addr;

	alignedPtr = ideIoReq->addr;
	if ((ideIoReq->cmd == IDE_WRITE) ||
	    (ideIoReq->cmd == IDE_READ) ||
	    (ideIoReq->cmd == IDE_READ_MULTIPLE) ||
	    (ideIoReq->cmd == IDE_WRITE_MULTIPLE) ||
	    (ideIoReq->cmd == IDE_READ_DMA) ||
	    (ideIoReq->cmd == IDE_WRITE_DMA) ||
	    (ideIoReq->cmd == IDE_IDENTIFY_DRIVE)) {
	    if (ideIoReq->blkcnt != 0) {
		bSize = [diskObj blockSize];
		wrFlag = ((ideIoReq->cmd == IDE_WRITE) ||
			  (ideIoReq->cmd == IDE_WRITE_MULTIPLE) ||
			  (ideIoReq->cmd == IDE_WRITE_DMA));

		alignedPtr = (unsigned char *)
		    IOMalloc(ideIoReq->blkcnt * bSize);
		if (alignedPtr == 0) {
		    ideIoReq->status = IDER_MEMALLOC;
		    return (ENOMEM);
		}
		if (wrFlag) {
		    error = copyin(ideIoReq->addr,
				   alignedPtr,
				   ideIoReq->blkcnt * bSize);
		    if (error) {
			ideIoReq->status = IDER_MEMFAIL;
			goto err_exit;
		    }
		}
	    }
	    ideIoReq->addr = (caddr_t) alignedPtr;
	    ideIoReq->map = (struct vm_map *)IOVmTaskSelf();
	}


	[diskObj ideXfrIoReq:ideIoReq];

	/*
	 * Note if we got this far, we'll return 0; any errors are in
	 * ideIoReq->status. 
	 */
	if ((ideIoReq->cmd == IDE_WRITE) ||
	    (ideIoReq->cmd == IDE_READ) ||
	    (ideIoReq->cmd == IDE_READ_MULTIPLE) ||
	    (ideIoReq->cmd == IDE_WRITE_MULTIPLE) ||
	    (ideIoReq->cmd == IDE_READ_DMA) ||
	    (ideIoReq->cmd == IDE_WRITE_DMA) ||
	    (ideIoReq->cmd == IDE_IDENTIFY_DRIVE)) {

	    ideIoReq->addr = userPtr;
	    if (((ideIoReq->cmd == IDE_READ) ||
		 (ideIoReq->cmd == IDE_READ_MULTIPLE) ||
		 (ideIoReq->cmd == IDE_IDENTIFY_DRIVE) ||
		 (ideIoReq->cmd == IDE_READ_DMA)) &&
		(ideIoReq->blocks_xfered != 0)) {
		error = copyout(alignedPtr,
				userPtr,
				ideIoReq->blocks_xfered * bSize);
		if (error) {
		    ideIoReq->status = IDER_MEMFAIL;
		}
	    }
    err_exit:

	    /*
	     * if we malloc'd any memory, free it. 
	     */
	    if (ideIoReq->blkcnt != 0)
		IOFree(alignedPtr, ideIoReq->blkcnt * bSize);
	}
	break;

      case IDEDIOCINFO:
	{
	    ideDriveInfo_t *idep = (ideDriveInfo_t *) data;

	    *idep = (ideDriveInfo_t)[diskObj ideGetDriveInfo];
	    break;
	}
      default:
	return (EINVAL);
    }

    if (irtn)
	rtn = [diskObj errnoFromReturn:irtn];
    return (rtn);

nodev:
    return (ENXIO);

} /* ideioctl() */

/*
 * Obtain physical block size.
 */
__private_extern__ int
idesize(dev_t dev)
{
    id diskObj = ide_dev_to_id(dev);
    
    if(diskObj == nil) {
	return -1;
    }
    return [diskObj blockSize];
}

/*
 * Returns block and char major nums. This is used so that the PostLoad
 * program can create block and character nodes for IDE. 
 */
__private_extern__ void
ide_block_char_majors(int *blockmajor, int *charmajor)
{
    *blockmajor = ide_block_major;
    *charmajor = ide_raw_major;
}

static unsigned ideminphys(struct buf *bp)
{
    if (bp->b_bcount > IDE_MAX_PHYS_IO)
	bp->b_bcount = IDE_MAX_PHYS_IO;
    return(bp->b_bcount);
}

/*
 * Map dev_t to id. A nil return indicates ENXIO.
 */
static id ide_dev_to_id(dev_t dev)
{
    id rtn;
    int unit = IO_DISK_UNIT(dev);
    int part = IO_DISK_PART(dev);
    IODevAndIdInfo *idmap;

#ifdef GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

	if ( ! ( unit < MAX_IDE_UNIT ) )
	{
		/* Modify the unit # under the assumption that it is an HFS device. */

		unit -= MAX_IDE_UNIT;
		
		/* Is it an HFS device? */

		if ( ! ( unit < MAX_IDE_UNIT ) ) {
			rtn = nil;
			goto Return;
		}

		/* Yes, it is an HFS device. */
		
		/* But it doesn't have a raw variant. */
		
		if ( major(dev) == ide_raw_major ) {
			rtn = nil;
			goto Return;
		}

		/* Modify the partition # to indicate an HFS partition and then proceed. */

		part += NUM_IDE_PART;

	}

#else GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

    if((unit >= NUM_IDE_DEV) || (part >= NUM_IDE_PART)) {
		return nil;
    }

#endif GROK_APPLE //bknight - 2/21/98 - Radar 2204950, 2004660

    idmap = &IdeIdMap[unit];
    
    if(part == IDE_LIVE_PART) {
		if(major(dev) == ide_block_major) {
		    rtn = nil;  
		}
		else {
		    rtn = idmap->liveId;
		}
    }
    else {
		rtn = idmap->partitionId[part];
    }
Return:
    return rtn;
}

/* end of IdeKern.m */
