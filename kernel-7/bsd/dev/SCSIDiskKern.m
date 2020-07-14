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

/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * SCSIDiskKern.m - UNIX front end for kernel SCSIDisk device. 
 *
 * HISTORY
 * 30-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
/*
 * Note that this file builds with KERNEL_PRIVATE and !MACH_USER_API.
 */
#import	<sys/systm.h>	/* for nblkdev and nchrdev */
#import <driverkit/SCSIDisk.h>
#import <driverkit/kernelDiskMethods.h>
#import <driverkit/SCSIDiskKern.h>
#import <driverkit/SCSIDiskPrivate.h>
#import <driverkit/SCSIDiskTypes.h>
#import <driverkit/scsiTypes.h>
#import <driverkit/SCSIStructInlines.h>
#import <bsd/dev/scsireg.h>
#import <driverkit/IODiskPartition.h>
#import <driverkit/xpr_mi.h>
#import <sys/buf.h>
#import <sys/conf.h>
#import <sys/uio.h>
#import <bsd/dev/ldd.h>
#import <sys/errno.h>
#import <sys/proc.h>
#import <vm/vm_kern.h>
#import <sys/fcntl.h>
#import <bsd/dev/disk.h>
#import <driverkit/align.h>

#import <kern/assert.h>


#ifdef ppc //bknight - 12/3/97 - Radar #2004660
#define GROK_APPLE 1
#endif //bknight - 12/3/97 - Radar #2004660

// wait until 20 seconds after probe non-ready disks
#define MAX_DISK_PROBE_TIME	20


int sdopen(dev_t dev, int flag, int devtype, struct proc * pp);
int sdclose(dev_t dev, int flag, int devtype, struct proc * pp);
int sdread(dev_t dev, struct uio *uiop, int ioflag);
int sdwrite(dev_t dev, struct uio *uiop, int ioflag);
int sdstrategy(register struct buf *bp);
int sdioctl(dev_t dev, 
	u_long cmd, 
	caddr_t data, 
	int flag, struct proc *pp);
int sdsize(dev_t dev);

/*
 * dev-to-id map array. Instances of DiskObject register their IDs in
 * this array via registerUnixDisk:.
 */
static IODevAndIdInfo SCSIDiskIdMap[NUM_SD_DEV];

/*
 * Private per-unit data.
 */
typedef struct _SCSIDisk_DataTag {
    struct buf 	physbuf;
    unsigned int maxTransfer;
} SCSIDisk_Data_t;

static SCSIDisk_Data_t *SCSIDisk_dev[NUM_SD_DEV];

/*
 * Indices of our entries in devsw's.
 */
static int sd_block_major;
static int sd_raw_major;

/*
 * prototypes for internal functions
 */
static unsigned sdminphys(struct buf *bp); 
static id sd_dev_to_id(dev_t dev);
static id sd_phys_dev_id(dev_t dev);

static void sd_prevent_eject(id physicalDisk, BOOL prevent);


#ifdef	DDM_DEBUG
static IONamedValue sdIoctlValues[] = {
	{DKIOCSFORMAT,			"DKIOCSFORMAT"	},
	{DKIOCGFORMAT,			"DKIOCGFORMAT"	},
	{DKIOCGLABEL,			"DKIOCGLABEL"	},
	{DKIOCSLABEL,			"DKIOCSLABEL"	},
	{DKIOCEJECT,			"DKIOCEJECT"	},
	{SDIOCSRQ,			"SDIOCSRQ"	},
	{SDIOCGETCAP,			"SDIOCGETCAP"	},
	{DKIOCINFO,			"DKIOCINFO"	},
	{0,				NULL		}
};

#endif	DDM_DEBUG

/*
 * Initialize id map and SCSIDisk_dev. Currently invoked by SCSIDisk probe:.
 */
void sd_init_idmap()
{
	IODevAndIdInfo *idMap = SCSIDiskIdMap;
	int unit;
	struct bdevsw *bd;
	struct cdevsw *cd;
	
	/* 
	 * figure out our major device numbers.
	 */
	for (bd = bdevsw; bd < &bdevsw[nblkdev]; bd++) {
		if (bd->d_open == (int (*)())sdopen) {
			sd_block_major = bd - bdevsw;
			break;
		}
	}
	for (cd = cdevsw; cd < &cdevsw[nchrdev]; cd++) {
		if (cd->d_open == (int (*)())sdopen) {
			sd_raw_major = cd - cdevsw;
			break;
		}
	}

	bzero(idMap, sizeof(*idMap) * NUM_SD_DEV);
	for(unit=0; unit<NUM_SD_DEV; unit++) {
		idMap->rawDev = makedev(sd_raw_major, (unit << 3));
		idMap->blockDev = makedev(sd_block_major, (unit << 3));
		idMap++;
		SCSIDisk_dev[unit] = IOMalloc(sizeof(SCSIDisk_Data_t));
		SCSIDisk_dev[unit]->physbuf.b_flags = 0;
	}
}

IODevAndIdInfo *sd_idmap()
{
	return SCSIDiskIdMap;
}

int sdopen(dev_t dev, int flag, int devtype, struct proc *pp)
{
	id 	diskObj;
	BOOL 	prompt;
	int	err;
	int	unit;

	// UFS partition a
	diskObj = sd_dev_to_id(dev & 0x78);
	if( diskObj)
            [diskObj waitForProbe:MAX_DISK_PROBE_TIME];

	diskObj = sd_dev_to_id(dev);
	if(diskObj == nil) {
		return(ENXIO);
	}
	xpr_sd("%s: sdopen\n", [diskObj name], 2,3,4,5);
	if(flag & O_NDELAY) {
		prompt = NO;
	}
	else {
		prompt = YES;
	}
	if([diskObj isDiskReady:prompt]) {
		return(ENXIO);
	}

	/*
	 * Register this 'Unix-level open' event for IODiskPartitions.
	 */
	if(IO_DISK_PART(dev) != SD_LIVE_PART) {
		// If this is the first open() on a removable disk,
		// whether block or raw,  on any of the partitions,
		// send a TEST UNIT READY to guarantee the disk is
		// still present (isDiskReady doesn't do this). If
		// it is, we send a PREVENT MEDIUM REMOVAL to lock
		// down the removable disk while it is open.
		//
		// Note that we don't keep track of live open()'s,
		// so we don't send TEST UNIT READY and/or PREVENT
		// MEDIUM REMOVAL on live opens.  Should not be an
		// issue (for now).

		if ( [diskObj isRemovable]    == YES &&
		     [diskObj isInstanceOpen] == NO  &&
		     [diskObj isAnyOtherOpen] == NO  )
		{
		  id physDisk = sd_phys_dev_id(dev);
		  ASSERT(physDisk);

		  if ( [physDisk updateReadyState] != IO_Ready )
		  {
		    [physDisk diskNotReady];  // not ready state
		    return ENXIO;
		  }

		  sd_prevent_eject(physDisk, YES);
		}
		if(major(dev) == sd_block_major) {
			[diskObj setBlockDeviceOpen:YES];
		}
		else {
			[diskObj setRawDeviceOpen:YES];
		}
	}

	unit = IO_DISK_UNIT(dev);
#ifdef GROK_APPLE
	if (unit >= NUM_SD_DEV)
		unit -= NUM_SD_DEV;
#endif /* GROK_APPLE */

	/* We have the physical disk now so grab a maxTransfer if necessary */
	if ( !SCSIDisk_dev[unit]->maxTransfer ) {
	    diskObj = sd_phys_dev_id(dev);	/* Get the physical disk */
	    SCSIDisk_dev[unit]->maxTransfer = [[diskObj controller] maxTransfer];
	}

	return(0);	
} /* sdopen() */

int sdclose(dev_t dev, int flag, int devtype, struct proc *pp)
{
	id diskObj = sd_dev_to_id(dev);
	id physDisk = sd_phys_dev_id(dev);
	
	if(diskObj == nil) {
		return(ENXIO);
	}
	kprintf("%s: sdclose major %d minor %d\n", [diskObj name],
		major(dev),minor(dev));
	xpr_sd("%s: sdclose\n", [diskObj name], 2,3,4,5);

	/* Issue a Synchronize-cache operation, which will force
	 * the device to flush its cached blocks.
	 */
	if (physDisk != nil) {
	    [physDisk synchronizeCache];
	}

	/*
	 * bknight - 12/3/97 - 2004660
	 * This test is safe, even when IO_DISK_PART denotes an HFS partition.
	 */

	if(IO_DISK_PART(dev) == SD_LIVE_PART) {
		return 0;
	} else if(![diskObj isInstanceOpen]) {
		return(ENXIO);
	}
	
	/*
	 * Register this 'Unix-level close' event. We won't be called unless
	 * this is the last close.
	 */
	if(major(dev) == sd_block_major) {
	 	[diskObj setBlockDeviceOpen:NO];
	}
	else {
	 	[diskObj setRawDeviceOpen:NO];
	}

	// If this is the last close() on a removable disk,
	// whether block or raw, for all of the partitions,
	// send an ALLOW MEDIUM REMOVAL to the SCSI drive.
	//
	// Note that we don't keep track of live open()'s,
	// so we don't send PREVENT/ALLOW MEDIUM REMOVAL
	// on live opens/closes either.
	//
	// Note that diskObj is an IOLogicalDisk here.

	if ( [diskObj isRemovable]    == YES &&
	     [diskObj isInstanceOpen] == NO  && // (raw and block)
	     [diskObj isAnyOtherOpen] == NO)    // (raw and block)
	{
	  sd_prevent_eject(physDisk, NO);
	}
        
	return(0);	
} /* sdclose() */

/*
 * Raw I/O uses standard UNIX physio routine, resulting in async I/O requests
 * via sdstrategy().
 *
 * For now, we do DMA alignment in both disk driver's phys read and 
 * physwrite as well as in the ioctl ops which do DMA. Ugh.
 */
#define SD_PHYS_ALIGN		1
#if	SD_PHYS_ALIGN

#define FORCE_PAGE_ALIGN	1
#if	FORCE_PAGE_ALIGN
int 	forceSdPageAlign = 0;
#endif	FORCE_PAGE_ALIGN
#endif	SD_PHYS_ALIGN
			
int sdread(dev_t dev, struct uio *uiop, int ioflag)
{
	id 		diskObj = sd_dev_to_id(dev);
	int 		unit = IO_DISK_UNIT(dev);
	int 		rtn;
#if	SD_PHYS_ALIGN
	void 		*freePtr = NULL;	// memory to free
	void 		*alignedPtr = NULL;	// DMA target
	int 		freeCnt = 0;		// size to free
	void		*userPtr = NULL;	// user spec'd pointer 
	BOOL 		didAlign = NO;
	struct iovec	*iov;
	int		userCnt = 0;
	int 		userSegflg = 0;
	IODMAAlignment 	dmaAlign;
	id 		liveId = sd_phys_dev_id(dev);
#endif	SD_PHYS_ALIGN

	if(diskObj == nil) {
		return(ENXIO);
	}
	xpr_sd("sdread %s\n", [diskObj name], 2,3,4,5);

	/*
	 * Catch unformatted disks right now, since blockSize is zero in
	 * that case.
	 */
	if(![diskObj isFormatted]) {
		return EINVAL;
	}

#if	SD_PHYS_ALIGN

	/*
	 * Note - this is temporary. We assume one vector and a well aligned
	 * byte count.
	 */
	ASSERT(uiop->uio_iovcnt == 1);	
	iov = uiop->uio_iov;
	[[liveId controller] getDMAAlignment:&dmaAlign];
#if	FORCE_PAGE_ALIGN
	if(forceSdPageAlign) {
		dmaAlign.readStart = PAGE_SIZE;
	}
#endif	FORCE_PAGE_ALIGN
	if( ( (dmaAlign.readStart > 1) && 
	      !IOIsAligned(iov->iov_base, dmaAlign.readStart)
	    ) ||
	    ( (dmaAlign.readLength > 1) && 
	      !IOIsAligned(iov->iov_len, dmaAlign.readLength)
	    ) 
#if sparc
			|| 1
#endif
	  ) {
		didAlign = YES;
		alignedPtr = [[liveId controller] allocateBufferOfLength:
				iov->iov_len
			actualStart:&freePtr
			actualLength:&freeCnt];
		userPtr = (void *)iov->iov_base;
		iov->iov_base = (caddr_t)alignedPtr;
		userCnt = iov->iov_len;
		
		/*
		 * DMA to kernel space now...
		 */
		userSegflg = uiop->uio_segflg;
		uiop->uio_segflg = UIO_SYSSPACE;
	}
#endif	SD_PHYS_ALIGN

#ifdef GROK_APPLE
	if (unit >= NUM_SD_DEV)
		unit -= NUM_SD_DEV;
#endif /* GROK_APPLE */

	rtn = physio(sdstrategy, 
		(struct buf *) SCSIDisk_dev[unit], 
		dev, 
		B_READ, 
		sdminphys, 
		uiop, 
		[diskObj blockSize]);
		
#if	SD_PHYS_ALIGN
	if(didAlign) {
		if(userSegflg == UIO_SYSSPACE) {
			bcopy(alignedPtr, userPtr, userCnt);
		} 
		else {
			copyout(alignedPtr, userPtr, userCnt);
		}
		IOFree(freePtr, freeCnt);
	}
#endif	SD_PHYS_ALIGN

	return rtn;

} /* sdread() */

int sdwrite(dev_t dev, struct uio *uiop, int ioflag)
{
	id 		diskObj = sd_dev_to_id(dev);
	int 		unit = IO_DISK_UNIT(dev);
	int 		rtn;
#if	SD_PHYS_ALIGN
	void 		*freePtr = NULL;	// memory to free
	void 		*alignedPtr = NULL;	// DMA target
	int 		freeCnt = 0;		// size to free
	void		*userPtr = NULL;	// user spec'd pointer 
	BOOL 		didAlign = NO;
	struct iovec	*iov;
	IODMAAlignment 	dmaAlign;
	id 		liveId = sd_phys_dev_id(dev);
#endif	SD_PHYS_ALIGN
		
	if(diskObj == nil)
		return(ENXIO);
	xpr_sd("sd_write %s\n", [diskObj name], 2,3,4,5);
	
	/*
	 * Catch unformatted disks right now, since blockSize is zero in
	 * that case.
	 */
	if(![diskObj isFormatted]) {
		return EINVAL;
	}

#if	SD_PHYS_ALIGN

	/*
	 * Note - this is temporary. We assume one vector and a well aligned
	 * byte count.
	 * We force a copyin to kernel space for raw user-level I/O to 
	 * ensure that rawVerify can work.
	 */
	ASSERT(uiop->uio_iovcnt == 1);	
	iov = uiop->uio_iov;
	[[liveId controller] getDMAAlignment:&dmaAlign];
#if	FORCE_PAGE_ALIGN
	if(forceSdPageAlign) {
		dmaAlign.writeStart = PAGE_SIZE;
	}
#endif	FORCE_PAGE_ALIGN
	if( ( (dmaAlign.writeStart > 1) && 
	      !IOIsAligned(iov->iov_base, dmaAlign.writeStart)
	    ) ||
	    ( (dmaAlign.writeLength > 1) && 
	      !IOIsAligned(iov->iov_len,  dmaAlign.writeLength)
	    ) 
#if sparc
			|| 1
#endif
		) {
		
		didAlign = YES;
		alignedPtr = [[liveId controller] allocateBufferOfLength:
				iov->iov_len
			actualStart:&freePtr
			actualLength:&freeCnt];
		userPtr = (void *)iov->iov_base;
		iov->iov_base = (caddr_t)alignedPtr;
		if(uiop->uio_segflg == UIO_SYSSPACE) {
			bcopy(userPtr, alignedPtr, iov->iov_len);
		}
		else {
			copyin(userPtr, alignedPtr, iov->iov_len);
			uiop->uio_segflg = UIO_SYSSPACE;
		}
		
	}
#endif	SD_PHYS_ALIGN

#ifdef GROK_APPLE
	if (unit >= NUM_SD_DEV)
		unit -= NUM_SD_DEV;
#endif /* GROK_APPLE */

	rtn = physio(sdstrategy, 
		(struct buf *) SCSIDisk_dev[unit], 
		dev, 
		B_WRITE, 
		sdminphys, 
		uiop, 
		[diskObj blockSize]);
		
#if	SD_PHYS_ALIGN
	if(didAlign) {
		IOFree(freePtr, freeCnt);
	}
#endif	SD_PHYS_ALIGN

	return rtn;

} /* sdwrite() */

int sdstrategy(struct buf *bp)
{
	id diskObj = sd_dev_to_id(bp->b_dev);
	u_int offset;
	u_int bytes_req;
	void *bufp;
	u_int block_size;
	IOReturn rtn;
	vm_task_t client;
	
	xpr_sd("%s: sdstrategy\n", [diskObj name], 2,3,4,5);
	if(diskObj == nil) {
		xpr_sd("sdstrategy: bad unit\n", 1,2,3,4,5);
		bp->b_error = ENXIO;
		goto bad;
	}
	if((bp->b_flags & (B_PHYS|B_KERNSPACE)) == B_PHYS) {
		/*
		 * Physical I/O to user space.
		 */
	    	client = ((task_t)bp->b_proc->task)->map;
	}
	else {
		/*
		 * Either block I/O (always kernel space) or physical I/O
		 * to kernel space (e.g., loadable file system).
		 */
	    	client = kernel_map;
	}
	block_size = [diskObj blockSize];
	if(block_size == 0) {
		xpr_sd("sdstrategy %s: zero block_size\n", 
			[diskObj name], 2,3,4,5);
		bp->b_error = ENXIO;
		goto bad;
	}
	offset = bp->b_blkno;
	bytes_req = bp->b_bcount;
	bufp = bp->b_un.b_addr;
	if(bp->b_flags & B_READ) {
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
	if(rtn) {
		/*
		 * Check to see if 'IODisk' object has called completeTransfer.
		 * if so then don't try to repair the bp structure just return.
		 */
		if (bp->b_flags & B_DONE) {
			xpr_sd("sdstrategy: COMMAND REJECT\n", 1,2,3,4,5);
			return -1;
		}
		bp->b_error = [diskObj errnoFromReturn:rtn];
		goto bad;
	}
	xpr_sd("sdstrategy: SUCCESS\n", 1,2,3,4,5);
	return(0);
	
bad:
	bp->b_flags |= B_ERROR;
	rtn = -1;
	biodone(bp);
	xpr_sd("sdstrategy: COMMAND REJECT\n", 1,2,3,4,5);
	return(rtn);

} /* sdstrategy() */

/*
 * Ops which are common to all IODiskPartitions are done on the raw device; 
 * SCSI-specific ioctls are done directly to the live SCSIDisk object.
 */
int sdioctl(dev_t dev, 
	u_long cmd, 
	caddr_t data, 
	int flag, 
	struct proc *pp)
{
	int 		unit = IO_DISK_UNIT(dev);
	int 		part = IO_DISK_PART(dev);
	id 		diskObj;
	IODevAndIdInfo	*idmap;
	int 		rtn = 0;
	IOReturn 	irtn = IO_R_SUCCESS;
	char 		*userData = *(char **)data;	// user src/dest
	void 		*alignedPtr = NULL;		// kernel src/des
	unsigned 	alignedLength;
	int 		i;
	IOSCSIRequest 	scsiReq;		// new style - to controller
	struct scsi_req *srp;			// old style - from caller
	int 		nblk;
	void 		*freeBuf;
	int 		freeLength;
	
	xpr_sd("sdioctl: unit %d cmd %s\n", unit, 
		IOFindNameForValue(cmd, sdIoctlValues), 3,4,5);

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	/* Compensate for an HFS partition. */

	if ( ! ( unit < NUM_SD_DEV ) ) {
		unit -= NUM_SD_DEV;
		part += NUM_SD_PART;
	}

#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	if(!(unit < NUM_SD_DEV))
		return (ENXIO);

#if 0
	if(major(dev) != sd_raw_major) {
	 	xpr_sd("sdioctl: not raw device\n", 1,2,3,4,5);
		return(ENXIO);  
	}
#endif
	idmap = &SCSIDiskIdMap[unit];

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

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
		/*
		 * For an HFS partition, return the # of blocks and block
		 * size in the partition, rather than on the whole device.
		 */
		diskObj = idmap->partitionId[part];
		switch ( cmd )
		{
			case DKIOCBLKSIZE:
                               *(int *)data = [diskObj blockSize];
                               return (0);
			break;

			case DKIOCNUMBLKS:
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
	
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	/* 
	 * Get appropriate device. Note we sometimes use the live 
	 * device partition (partition 7) here even if another partition
	 * was opened; we're just directing the I/O request to the correct
	 * object.
	 *
	 * Also note that several of these ioctls can fail if not invoked
	 * upon partition0, or if any block devices are open, or if any
	 * other partitions are open. That's handled elsewhere.
	 */
	switch (cmd) {
	    case DKIOCSFORMAT:
	    case DKIOCGFORMAT:
	    case DKIOCGLABEL:
	    case DKIOCSLABEL:
	    case DKIOCEJECT:
		/* 
		 * These must be performed on a valid raw device, 
		 * but if caller asked for live device (as some disk 
		 * diags are liable to do), we'll given them raw
		 * partition a.
		 */
		{

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

			/* Treat HFS partitions like the live partition, too. */

			if ( part >= SD_LIVE_PART ) {
				part = 0;
			}

#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660

			if(part == SD_LIVE_PART) {
				part = 0;
			}

#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

			diskObj = idmap->partitionId[part];
		}
		break;

	    case DKIOCGLOCATION:
                    diskObj = idmap->partitionId[part];
		break;

	    case SDIOCSRQ:
	    case SDIOCGETCAP:
	    case DKIOCINFO:
	    case DKIOCBLKSIZE:
	    case DKIOCNUMBLKS:
		diskObj = idmap->liveId;
		break;
	    default:
	    	xpr_sd("sdioctl: BAD cmd (0x%x)\n", cmd,2,3,4,5);
	    	return(EINVAL);
	}
	if(diskObj == nil)
		goto nodev;
	
	switch (cmd) {
	    case DKIOCSFORMAT:
	    	/*
		 * This can fail if block devices attached to this disk are 
		 * open.
		 */
	    	irtn = [diskObj setFormatted:(*(u_int *)data)];
	    	break;
		
	    case DKIOCGFORMAT:
		{
			*(int *)data = [diskObj isFormatted];
			break;
		}
		
	    case DKIOCGLABEL:
	    	{
			struct disk_label *labelp;
			
			labelp = IOMalloc(sizeof(*labelp));
			irtn = [diskObj readLabel:labelp];
			if(irtn == IO_R_SUCCESS) {
			*(struct disk_label *)data = *labelp;
			}
			IOFree(labelp, sizeof(*labelp));
			break;
		}
		
	    case DKIOCSLABEL:
	    	{
			struct disk_label *labelp;
			
			labelp = IOMalloc(sizeof(*labelp));
			*labelp = *(struct disk_label *)data;
			irtn = [diskObj writeLabel:labelp];
			IOFree(labelp, sizeof(*labelp));
			break;
		}
		
	    case DKIOCINFO:
	        {
			struct drive_info info;
			
			bzero(&info, sizeof(info));
			strcpy(info.di_name, [diskObj driveName]);
			info.di_devblklen = [diskObj blockSize];
			info.di_maxbcount = SCSIDisk_dev[unit]->maxTransfer;
			if(info.di_devblklen) {
				/*
				 * Careful, blockSize might be 0 for an
				 * unformatted disk.
				 */
				nblk = howmany(sizeof(struct disk_label),
					info.di_devblklen);
				for (i = 0; i < NLABELS; i++)
					info.di_label_blkno[i] = nblk * i;
			}
			*(struct drive_info *)data = info;
			break;
		}
			
	    case DKIOCEJECT:
		/*
		 * This is a (legal) nop for non-removable drives. This 
		 * merely accomodates a WSM quirk left over from
		 * m68k develolpment; it's handled here instead of
		 * in IODiskPartition to avoid propagating further...
		 */
		if(![diskObj isRemovable]) {
			break;
		}
		// Send an ALLOW MEDIUM REMOVAL to the SCSI drive
		// to permit the ejection of the disk,  unless we
		// detect another active open on this disk (aside
		// from the one issuing this eject).  The [eject]
		// method also does this check, don't worry.   We
		// just don't want to send spurious PREVENT/ALLOW
		// commands while other folks are doing I/O.
		//
		// Note that on successful ejection, a 2nd ALLOW
		// MEDIUM REMOVAL will be sent after this on the
		// close() of this device.  This is not an issue.

		if ( [diskObj isAnyOtherOpen]    == NO  &&
		     [diskObj isBlockDeviceOpen] == NO )
		{
		  // Note that eject ioctls arrive on raw opens,
		  // so we only check for outstanding block opens
		  // on our device.
		  sd_prevent_eject(idmap->liveId, NO);

		  // Eject the disk now.
		  irtn = [diskObj eject];

		  // Send a PREVENT MEDIUM REMOVAL to the SCSI drive
		  // to re-prevent the ejection of the disk if the
		  // eject has failed for any reason.
		  if (irtn) sd_prevent_eject(idmap->liveId, YES);
		} else {
	    	  irtn = [diskObj eject];
		}
		break;
		
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
		    irtn = IO_R_UNSUPPORTED;
		break;
		
	    case SDIOCSRQ:
	       
   		srp = (struct scsi_req *)data;
		
		/* if user expects to do some DMA, get some well-aligned
		 * memory. Copy in the user's data if a DMA write is 
		 * expected. By using allocateBufferOfLength we guarantee 
		 * that there is enough space in the buffer we pass to the 
		 * controller to handle end-of-buffer alignment, although
		 * we won't copy more than sr_dma_max to or from the 
		 * caller.
		 */
		if(srp->sr_dma_max != 0) {
			IODMAAlignment dmaAlign;
			id controller = [idmap->liveId controller];
			unsigned alignment;
			
			[controller getDMAAlignment:&dmaAlign];
			if(srp->sr_dma_dir == SR_DMA_WR) {
				alignment = dmaAlign.writeLength;
			}
			else {
				alignment = dmaAlign.readLength;
			}
			if(alignment > 1) {
				alignedLength = IOAlign(unsigned,
					srp->sr_dma_max,
					alignment);
			}
			else {
				alignedLength = srp->sr_dma_max;
			}
			alignedPtr = [controller
					allocateBufferOfLength:alignedLength
				actualStart:&freeBuf
				actualLength:&freeLength];
			if(srp->sr_dma_dir == SR_DMA_WR) {
				rtn = copyin(srp->sr_addr, alignedPtr,
					srp->sr_dma_max);
				if(rtn) {
				    xpr_sd(" ...copyin() returned %d\n",
					    rtn, 2,3,4,5);
				    srp->sr_io_status = SR_IOST_MEMF;
				    goto err_exit;
				}
			}
		} else {
			alignedLength = 0;
			alignedPtr = 0;
		}

		/*
		 * Generate a contemporary version of scsi_req.
		 */
		bzero(&scsiReq, sizeof(scsiReq));
		scsiReq.target = [diskObj target];
		scsiReq.lun    = [diskObj lun];
		/*
		 * Careful. this assumes that the old and new cdb structs are
		 * equivalent...
		 */
		scsiReq.cdb = srp->sr_cdb;
		scsiReq.read = (srp->sr_dma_dir == SR_DMA_RD) ? 
			YES : NO;
		scsiReq.maxTransfer 	= alignedLength;
		scsiReq.timeoutLength 	= srp->sr_ioto;
		scsiReq.disconnect	= srp->sr_discon_disable ? 0 : 1;
		scsiReq.cmdQueueDisable	= srp->sr_cmd_queue_disable;
		scsiReq.syncDisable 	= srp->sr_sync_disable;
		scsiReq.cdbLength 	= srp->sr_cdb_length;
		
		/*
		 * Go for it.
		 */
		if(srp->sr_dma_dir == SR_DMA_WR) {
			irtn = [diskObj sdCdbWrite:&scsiReq
				buffer : alignedPtr
				client : kernel_map];
		}
		else {
			irtn = [diskObj sdCdbRead:&scsiReq
				buffer : alignedPtr
				client : kernel_map];
		}

		/*
		 * Copy status back to user. 
		 */
		srp->sr_io_status = scsiReq.driverStatus;
		if(srp->sr_io_status == SR_IOST_BADST) {
			/* 
			 * dmitch 9 Jun 94: Warning: this code is 
			 * bogus, though benign. Lower levels will give us
			 * SR_IOST_CHKSNV or SR_IOST_CHKSV in case of 
			 * STAT_CHECK.
			 */
			if(scsiReq.scsiStatus == STAT_CHECK)
				srp->sr_io_status = SR_IOST_CHKSNV;
		}
		srp->sr_scsi_status = scsiReq.scsiStatus;
		srp->sr_dma_xfr = scsiReq.bytesTransferred;
		if(srp->sr_dma_xfr > srp->sr_dma_max) {
			srp->sr_dma_xfr = srp->sr_dma_max;
		}
		srp->sr_exec_time.tv_sec = srp->sr_exec_time.tv_usec = 0;
		
		/*
		 * Copy read data back to user if appropriate.
	 	 */
		if((srp->sr_dma_dir == SR_DMA_RD) && 
		   (scsiReq.bytesTransferred != 0)) {
			rtn = copyout(alignedPtr, 
				srp->sr_addr, 
				srp->sr_dma_xfr);
		}
		/*
		 * Copy sense data back to user if appropriate.
		 */
		if(srp->sr_io_status == SR_IOST_CHKSV) {
			srp->sr_esense = scsiReq.senseData;
		}
err_exit:
		if (srp->sr_dma_max != 0) {
			IOFree(freeBuf, freeLength);
		}
		break;

	    case SDIOCGETCAP:
	    {
			irtn = [diskObj updatePhysicalParameters];
			if(irtn)
				break;
			scsi_crp_setup((struct capacity_reply *) data,
				[diskObj blockSize], [diskObj diskSize] - 1);
		}
		break;
		
	    default:
	    	xpr_sd("sdioctl: BAD cmd (0x%x)\n", cmd,2,3,4,5);
	    	return(EINVAL);
	}

	if(irtn)
		rtn = [diskObj errnoFromReturn:irtn];
	xpr_sd("%s sdioctl: returning %s (errno %d)\n", 
		[diskObj name], [diskObj stringFromReturn:irtn],  
		rtn, 4,5);
	return(rtn);
	
nodev:
	xpr_sd("sdioctl: no such device (dev = 0x%x)\n", 
		dev, 2,3,4,5);
	return(ENXIO);
	
} /* sdioctl() */

/*
 * Obtain physical block size.
 */
int sdsize(dev_t dev)
{
	id diskObj = sd_dev_to_id(dev);
	
	if(diskObj == nil) {
		xpr_sd("sdsize: bad unit\n", 1,2,3,4,5);
		return -1;
	}
	return [diskObj blockSize];
	
}

static unsigned sdminphys(struct buf *bp)
{
	SCSIDisk_Data_t *dataBP = (SCSIDisk_Data_t *) bp;

	if (bp->b_bcount > dataBP->maxTransfer)
		bp->b_bcount = dataBP->maxTransfer;
	return bp->b_bcount;
}

/*
 * Map dev_t to id. A nil return indicates ENXIO.
 */

static id sd_dev_to_id(dev_t dev)
{
	id					rtn;
	int					unit = IO_DISK_UNIT(dev);
	int					part = IO_DISK_PART(dev);
	IODevAndIdInfo	*	idmap;

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	if ( ! ( unit < NUM_SD_DEV ) ) {
		/* Modify the unit # under the assumption that it is an HFS device. */

		unit -= NUM_SD_DEV;
		
		/* Is it an HFS device? */

		if ( ! ( unit < NUM_SD_DEV ) ) {
			rtn = nil;
			goto Return;
		}

		/* Yes, it is an HFS device. */
		
		/* But it doesn't have a raw variant. */
		
		if ( major(dev) == sd_raw_major ) {
			rtn = nil;
			goto Return;
		}

		/* Modify the partition # to indicate an HFS partition and then proceed. */

		part += NUM_SD_PART;

	}

#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	if((unit >= NUM_SD_DEV) || (part >= NUM_SD_PART)) {
		return nil;
	}

#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	idmap = &SCSIDiskIdMap[unit];

	if(part == SD_LIVE_PART) {
	   	if(major(dev) == sd_block_major) {
	 		/*
			 * Live partition on the block device not permitted.
			 */
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

/*
 * Obtain the id of the physical disk assiociated with specified dev.
 */

static id sd_phys_dev_id(dev_t dev)
{
	int unit = IO_DISK_UNIT(dev);
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	id rtn;
	
	if ( ! ( unit < NUM_SD_DEV ) ) {
		/* Could it be an HFS partition? */

		unit -= NUM_SD_DEV;

		if ( ! (unit < NUM_SD_DEV) ) {
			rtn = nil;
			goto Return;
		}
		
		/* Yes, it is an HFS partition.  Use adjusted <unit>. */
		
	}

	rtn = SCSIDiskIdMap[unit].liveId;

Return:

	return rtn;

#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if(unit > NUM_SD_DEV) {
		return nil;
	}
	return SCSIDiskIdMap[unit].liveId;
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
}

void sd_prevent_eject(id physicalDisk, BOOL prevent)
{
  //
  // Sends a SCSI PREVENT/ALLOW MEDIUM REMOVAL command to the given
  // disk (target and lun) in order to prevent or permit the manual
  // ejection of removable disk(s) inside the SCSI drive.
  //

  IOSCSIRequest scsiReq;

  //
  // Set up the SCSI REQUEST structure.  Note that cdbLength is an
  // optional field and is left as zero.
  //

  bzero(&scsiReq, sizeof(scsiReq));

  scsiReq.cdb.cdb_c6.c6_opcode = C60P_PREVENTALLOW;
  scsiReq.cdb.cdb_c6.c6_len    = (prevent ? 0x01 : 0x00);
  scsiReq.target               = [physicalDisk target];
  scsiReq.lun                  = [physicalDisk lun];
  scsiReq.read                 = YES;
  scsiReq.timeoutLength        = SD_TIMEOUT_SIMPLE;

  //
  // Execute the request.
  //

  [physicalDisk sdCdbRead : &scsiReq
                   buffer : NULL
                   client : kernel_map];
}

/* end of SCSIDiskKern.m */
