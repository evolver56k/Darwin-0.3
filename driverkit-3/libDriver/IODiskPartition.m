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
 * IODiskPartition.m - implementation of NeXT-style logical disk.
 *
 * HISTORY
 * 22-Jan-98	radar 1669467 - ISO 9660 CD support - jwc
 * 15-Dec-92	Sam Streeper (sam) at NeXT
 *	Added support for a NeXT disk on a MS-DOS partition
 * 01-May-91    Doug Mitchell at NeXT
 *      Created.
 */

#define DRIVER_PRIVATE 1

#import <driverkit/return.h>
#import <driverkit/IODiskPartition.h>
#import <driverkit/generalFuncs.h>
#import <bsd/dev/disk_label.h>
#import <bsd/sys/disktab.h>
#ifdef	KERNEL
#import <kernserv/prototypes.h>
#import <driverkit/kernelDiskMethods.h>
#import <driverkit/kernelDiskMethodsPrivate.h>
#import <driverkit/kernelDriver.h>
#import <machkit/NXLock.h>
#else	KERNEL
#import <bsd/libc.h>
#endif	KERNEL
#import <kern/time_stamp.h>
#import <driverkit/Device_ddm.h>
#import <mach/vm_param.h>
#import <driverkit/disk_label.h>
#import <driverkit/diskstruct.h>
#ifdef	i386
#import <bsd/dev/i386/disk.h>
#endif	i386
#import <architecture/byte_order.h>
#import "label_subr.h"
#import <driverkit/IODeviceDescription.h>
#import <bsd/dev/voldev.h>
#ifdef	ppc
#import <bsd/dev/ppc/disk.h>
#endif

/* Define GROK_DOS if scsi driver is to understand MS-DOS partitions.
 * It only finds the partition with the NeXT name.  Live partition is
 * not affected.  -sam
 *
 * define GROK_APPLE if we want to look to a Rhapsody partition on a
 * Apple Partitioned disk
 */
#ifdef i386
#define	GROK_DOS
#endif

#ifdef ppc
#define GROK_APPLE 1
#endif

// seconds to wait for disk ready at probe time
#define DELAY_AT_PROBE	0

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

#define HFS_PART_TYPE "Apple_HFS"

#endif //bknight - 12/3/97 - Radar #2004660


@interface IODiskPartition(Private)

/*
 * Private methods.
 */				  
/*
 * Examine a known good label, initialize LogicalDisk parameters for the
 * raw device (on which this method is invoked) and create block devices
 * as appropriate.
 *
 * Invoked by LogicalDisk +probe, upon disk insertion, and when a new label
 * is written.
 */
- (void) _probeLabel		: (disk_label_t *)labelp;

/*
 * Assign logical disk parameters for a partition instance.
 */
- (void)_initPartition		: (int)partNum
              physicalPartition : (int)physNum	
                        disktab : (struct disktab *)dtp;
				  
/*
 * Free all partition instances other than partition 0.
 */
- (IOReturn)_freePartitions;

/*
 * Determine if any block devices in the logicalDisk chain are open.
 */
- (BOOL)isAnyBlockDevOpen;

/*
 * Verify that it's safe to do an operation which will result in a call
 * to _freePartitions.
 */
- (IOReturn)checkSafeConfig	: (const char *)op;

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

/*
 * IODiskPartition knows how to
 * Create HFS Devices From Apple Partition Map.
 */
+ (void)CreateHFSDevicesFromApplePartitionMap : (id)physicalDisk;

/*
 * Assign logical disk parameters for a partition instance.
 */

- (void)	_initHFSPartition : (int) partitionNum
			physicalPartNum : (int) physNum
			diskSize : (int) diskSize
			partitionBase : (int) partitionBase;

#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
				  
@end


@implementation IODiskPartition

+ (IODeviceStyle)deviceStyle
{
	return IO_IndirectDevice;
}

/*
 * The protocol we need as an indirect device.
 */
static Protocol *protocols[] = {
	@protocol(IOPhysicalDiskMethods),
	@protocol(IODiskReadingAndWriting),
	nil
};

+ (Protocol **)requiredProtocols
{
	return protocols;
}

/*
 * Attempt to read label of physical device. If valid label, create an 
 * instance of LogicalDisk for each valid partition as well as for the 
 * Unix raw device. Returns YES if any instances were created. 
 */
+ (BOOL)probe : deviceDescription
{
	id		physDisk = [deviceDescription directDevice];
	int 		try;
	IODiskReadyState 	ready;
	IOReturn 	rtn;
	disk_label_t 	*labelp = NULL;
	IODiskPartition *part0p;
	BOOL 		frtn = YES;	// when would this be NO?
	char 		name[30];
	id 		ld;
	const char 	*physName = [physDisk name];
	unsigned	physBlockSize = 0;
	unsigned	physDiskSize = 0;
	unsigned	formattedFlag;
	BOOL		logParams = NO;
	BOOL		logLabel = NO;
	IODiskReadyState	oldReady;
	
	xpr_disk("IODiskPartition probe\n", 1,2,3,4,5);
	/*
	 * First of all, we're only interested in physical disks.
	 * (Shouldn't this be covered by our requiredProtocols values?)
	 */
	if(![physDisk isPhysical]) {
		return NO;
	}
	
	/*
	 * Create a partition 0 instance if there isn't already one; attach 
	 * it to physDisk. There will already be a partition 0 instance if 
	 * this probe is in response to a disk insertion as opposed to 
	 * initial device configuration.
	 */
	ld = [physDisk nextLogicalDisk];
	if(ld != nil) {
		part0p = (IODiskPartition *)ld;
		
		/*
		 * Update physical parameters.
		 */
		[part0p connectToPhysicalDisk:physDisk];
		part0p->_labelValid = 0;    
        	part0p->_physicalPartition = 0;

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		part0p->_hfsValid = 0;    
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		part0p->_blockDeviceOpen = 0;
		part0p->_rawDeviceOpen = 0;
		IOLog("\n");
	}
	else {
		part0p = [IODiskPartition new];
		sprintf(name, "%sa", physName);
		[part0p setName:name];
		[part0p setDeviceKind:"IODiskPartition"];
		[part0p setDriveName:"IODiskPartition Partition"]; 
		[part0p setLocation:NULL];
                part0p->_partitionWaitLock = [[NXConditionLock alloc] init];
		IOGetTimestamp( &part0p->_probeTime);
		[part0p init];
		[part0p connectToPhysicalDisk:physDisk];
                [part0p setDeviceDescription:deviceDescription];
		[physDisk setLogicalDisk:part0p];
		part0p->_labelValid = 0;    
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		part0p->_hfsValid = 0;    
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		part0p->_blockDeviceOpen = 0;
		part0p->_rawDeviceOpen = 0;
	
#ifdef	KERNEL
		/*
		 * Register both physDisk and partition 0 with Unix 
		 * level owner.
		 */
		[physDisk registerUnixDisk:0];
		[part0p registerUnixDisk:0];
#endif	KERNEL
	}
	
	/*
	 * Give the drive a chance to spin up.
	 */
	oldReady = [physDisk lastReadyState];

#if DELAY_AT_PROBE
	if( (physName[0] == 'f') && (physName[1] == 'd'))
	    ready = [physDisk updateReadyState];
	else {
	    for(try = 0; try < DELAY_AT_PROBE; try++) {
		ready = [physDisk updateReadyState];
		switch(ready) {
		    case IO_Ready: 	
		    	goto goOn;		/* go for it */
		    case IO_NotReady:
		    	break;			/* try again */
		    case IO_NoDisk:
		    	if (try > 0)
			    IOLog("\n");
			goto done;		/* forget it */
		    case IO_Ejecting:	
			break;			/* try again */
		}
		if(try == 0)
			IOLog("%s: Waiting for drive to come ready", physName);
		else
			IOLog(".");
		IOSleep(1000);
	    }
	  goOn:
	    if (try > 0)
		IOLog("\n");
	}
#else
        ready = [physDisk updateReadyState];
#endif
	[physDisk setLastReadyState:ready];
	if(ready != IO_Ready) {
		IOLog("%s: Disk Not Ready\n", physName);
		xpr_disk("IODiskPartition probe: Disk Not Ready\n", 1,2,3,4,5);
		goto done;
	}

	/*
	 * If the disk just came ready, have driver update its physical 
	 * parameters.
	 */
	if(oldReady != IO_Ready) {
		[physDisk updatePhysicalParameters];
	}
		
	/*
	 * If disk is not formatted, forget the rest of this.
	 */
	formattedFlag = [physDisk isFormatted];
	if(!formattedFlag) {
		IOLog("%s: Disk Unformatted\n", physName);
		xpr_disk("IODiskPartitionProbe: physDisk %s not formatted\n", 
			physName, 2,3,4,5);
		goto done;
	}

	physDiskSize = [physDisk diskSize];
	physBlockSize = [physDisk blockSize];

	[part0p setPhysicalBlockSize:physBlockSize];

	logParams = YES;

	/*
	 * Try to read a label.
	 */
	labelp = IOMalloc(sizeof(disk_label_t));
	rtn = [part0p readLabel:labelp];
	if(rtn) {		
		xpr_disk("IODiskPartition probe: No Label\n", 1,2,3,4,5);
		IOLog("%s: No Valid Disk Label\n", physName);
		
		/*
		 * Set up blockSize and diskSize of partition A
		 * to be the same as our physicalDisk's for now.
		 */
		[part0p setBlockSize:physBlockSize];
		[part0p setDiskSize:[physDisk diskSize]];

		/* Remember to reset the partition base to zero,
		 * since we don't yet have any recognizable
		 * partitioning information.
		 */
		[part0p setPartitionBase:0];
	}
	else {
		logLabel = YES;
	
		/*
		 * Initialize remaining device parameters and create 
		 * additional partition instances as appropriate.
		 */
		[part0p _probeLabel:labelp];
	}

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	[self CreateHFSDevicesFromApplePartitionMap:deviceDescription];

#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	if( part0p->_partitionWaitLock) {
            [part0p->_partitionWaitLock lock];
            [part0p->_partitionWaitLock unlockWith:YES];
	}
done:
	if(logParams) {
		unsigned kbytes;
		
		IOLog("%s: Device Block Size: %u bytes\n",
			physName, physBlockSize);
		kbytes = (physDiskSize / 1024) * physBlockSize;
		if(kbytes > (10 * 1024)) {
			IOLog("%s: Device Capacity:   %u MB\n", 
				physName, kbytes / 1024);
		}
		else {
			IOLog("%s: Device Capacity:   %u KB\n", 
				physName, physDiskSize * physBlockSize / 
				1024);
		}
	}
	if(logLabel) {
		IOLog("%s: Disk Label:        %s\n", 
			physName, labelp->dl_label);
	}
	if(labelp != NULL) {
		IOFree(labelp, sizeof(disk_label_t));
	}

	if(ld == nil)
		[part0p registerDevice];

	return(frtn);
}

#ifdef GROK_DOS
/* returns the offset to the NeXT partition of an MSDOS partitioned
 * hard disk (0 if it isn't a DOS disk), or a (negative) IOReturn.  
 */
- (int) NeXTpartitionOffset
{
	IOReturn 		frtn = 0;
	IOReturn 		rtn;
	unsigned		bytesXfr;
	id 			physDisk = [self physicalDisk];
	/*
	 * Force page alignment...
	 */
	unsigned char		*blk= IOMalloc(PAGE_SIZE);
	struct disk_blk0	*blk0;
	struct fdisk_part	*fd;
	int			n;
	BOOL			validPart = NO;
	
	if ([self physicalBlockSize] != DISK_BLK0SZ) {
		frtn = 0;
		goto out;
	}
	// fixme - can I really transfer to unwired, unaligned buffer?
        rtn = [[super class] commonReadWrite
		    : physDisk
		    : YES			/* read */
		    : (unsigned long long)DISK_BLK0 * [physDisk blockSize]
		    : DISK_BLK0SZ 	/* bytes */
		    : blk
		    : IOVmTaskSelf()
		    : (void *)NULL
		    : &bytesXfr];

	if (rtn) {
		frtn = IO_R_NO_BLOCK_ZERO;
		goto out;
	}

	// Check for a valid DOS boot block.
	blk0 = (struct disk_blk0 *)blk;
	if (blk0->signature != DISK_SIGNATURE) {
		goto out;
	}

	// Check to see if the disk has been partitioned with FDISK
	// to allow DOS and ufs filesystems to exist on the same disk
	fd = (struct fdisk_part *)blk0->parts;    
	for (n = 0; n < FDISK_NPART; n++, fd++) {
		switch(fd->systid) {
		    case 0:
		    	/*
			 * Not valid partition, meaningless.
			 */
			break;
		    case FDISK_NEXTNAME:
		    	/*
			 * Valid NeXT partition.
			 */
			frtn = fd->relsect;
                        _physicalPartition = n;
			goto out;
		    default:
		    	/*
			 * Not what we're looking for, but this means that
			 * we can't use partition 0.
			 */
			validPart = YES;
			break;
		}
	}
	if(validPart) {
		frtn = IO_R_NO_NEXT_PART;
	}
	else {
		/*
		 * No valid partitions; treat same as "no valid block 0".
		 */
	}
out:
	IOFree(blk, PAGE_SIZE);
	return frtn;
}
#endif	GROK_DOS

#ifdef GROK_APPLE
/*
 * Check for an apple partition:
 *
 */
#define BLOCKSIZE_512_BYTES	512

- (int) NeXTpartitionOffset
{
	unsigned char *		blk;
	Block0 *		blk0;
	unsigned long 		blocksize = [self physicalBlockSize];
	unsigned		bytesXfr;
	IOReturn 		frtn = 0;
	id	 		physDisk = [self physicalDisk];
	IOReturn 		rtn;

	blk = IOMalloc(PAGE_SIZE); /* force page alignment */
	blk0 = (Block0 *)blk;

	/* check block 0 for valid Apple partition signature */

        rtn = [[super class] commonReadWrite
            			: physDisk
				: YES			/* read */
				: (unsigned long long)0 /* byte offset */
				: PAGE_SIZE 	/* bytes */
				: blk
				: IOVmTaskSelf()
				: (void *)NULL
				: &bytesXfr];
        
        /* scan for the rhapsody apple ufs partition */
	if (rtn)
	    frtn = IO_R_NO_BLOCK_ZERO;
	else {
	    unsigned int		block = 1;
	    unsigned int		entry_within_block = 0;
	    int				n;
	    unsigned int		nentries_per_block = 1;
	    unsigned int		numPartBlocks;
	    DPME *			partEntry;
	    int				physNum;

	    /* check if there is a valid map at 512 offset */
	    partEntry = (DPME *)(blk + BLOCKSIZE_512_BYTES);
	    if (blocksize > BLOCKSIZE_512_BYTES) {
		if (NXSwapBigShortToHost(partEntry->dpme_signature) 
		    == DPME_SIGNATURE) {
		    /* there is a valid map, and the device blocksize
		     * is > 512, which means we probably have a CDROM
		     * using 2K, but having valid 512 map
		     */
		    block = 0;
		    nentries_per_block = blocksize / BLOCKSIZE_512_BYTES;
		    entry_within_block = 1;
		}
		else
		    partEntry = (DPME *)(blk + blocksize);
	    }
	  if (NXSwapBigShortToHost(partEntry->dpme_signature) == DPME_SIGNATURE) {
	    numPartBlocks = NXSwapBigLongToHost(partEntry->dpme_map_entries);
#ifdef PRINT_APT
	    IOLog("Apple Partition table (%d entries)...\n", numPartBlocks);
#endif PRINT_APT

            physNum = 1;
	    for (n = 0; n < numPartBlocks; n++) {
		int ufs_partition_set = 0;

		if (NXSwapBigShortToHost(partEntry->dpme_signature) 
		    != DPME_SIGNATURE) {
		    break;
		}
		if (strcmp(partEntry->dpme_type, RHAPSODY_PART_TYPE) == 0
		    && ufs_partition_set == 0) {
		    unsigned long part_offset;

		    part_offset 
			= NXSwapBigLongToHost(partEntry->dpme_pblock_start);
		    if ((part_offset / nentries_per_block * nentries_per_block)
			!= part_offset) {
			IOLog("IODiskPartition: " RHAPSODY_PART_TYPE
			      " partition base (%ld x 512) is not"
			      " a multiple of the devblksize %ld\n",
			      part_offset, blocksize);
			frtn = IO_R_NO_NEXT_PART;
			break;
		    }
	
		    part_offset = part_offset / nentries_per_block;
#ifdef PRINT_APT
		    IOLog("%s: UFS partition at offset %ld\n",
			  [physDisk name], part_offset);
#endif PRINT_APT
		    frtn = (IOReturn)part_offset;
		    ufs_partition_set = 1;
                    _physicalPartition = physNum;
		}
#ifdef PRINT_APT
		IOLog("Apple Partition entry %d:\n", n + 1);
		IOLog("  Name: %s, Type %s, size: %d\n", 
		       partEntry->dpme_name, partEntry->dpme_type,
		       partEntry->dpme_pblocks);
		IOLog("block %d entry %d (of %d) blocksize %d\n", 
		       block, entry_within_block + 1,
		       nentries_per_block, blocksize);
#endif PRINT_APT
		partEntry++;
                physNum++;
		if (++entry_within_block == nentries_per_block) {
		    block++;
		    entry_within_block = 0;
		    partEntry = (DPME *)blk;

		    rtn = [[super class] commonReadWrite
            			: physDisk
 				: YES			/* read */
		    : (unsigned long long)block * blocksize /* byte offset */
				: blocksize 	/* bytes */
				: blk
				: IOVmTaskSelf()
				: (void *)NULL
				: &bytesXfr];
                   
		    if (rtn) {
			frtn = IO_R_NO_NEXT_PART;
			break;
		    }
		}
	    }
	  } /* DPME signature validation */
	} /* scan for Apple rhapsody ufs partitions */
	IOFree(blk, PAGE_SIZE);
	return frtn;
}

- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias
{
    if( [[self physicalDisk] 
	getDevicePath:path maxLength:maxLen useAlias:doAlias]) {

	char partStr[ 12 ];

	sprintf( partStr, ":%x", _physicalPartition);
	strcat( path, partStr);
	return( self);
    }
    return( nil);
}

#endif GROK_APPLE

/*
 * Read disk label. label_p must does not have to be aligned; we do a
 * page-aligned read to satisfy any possible DMA requirements.
 *
 * This is only invoked on the raw device. This can be invoked on a 
 * drive with no disk present; we'll do a 'isDiskReady:YES" to get a 
 * disk if necessary.
 *
 * This reads labels as raw m68k disk_label_t's. On successful return,
 * *label_p contains a label which is valid for the current architecture;
 * the transformation is performed per the API in <driverkit/disk_label.h>.
 */

- (IOReturn) readLabel	: (disk_label_t *)label_p
{
	int 			label_num;
	IOReturn 		rtn;
	int 			found = 0;
	int 			blocksInLabel;
	int 			bytesInLabel;
	unsigned		bytesXfr;
	char			*raw_label;
	int 			labelBufSize;
	id 			physDisk = [self physicalDisk];
	unsigned		physBlockSize = [self physicalBlockSize];
	unsigned		physFormatted;
        unsigned		blocksize;
	const char 		*disk_name = [self name];
	/* offset of NeXT disk on dos or Apple partition */
	int			part_offset = 0;
	
	xpr_disk("IODiskPartition readLabel\n", 1,2,3,4,5);
	
	/*
	 * Note we have to 'fault in' a possible non-present disk in 
	 * order to get its physical parameters...
	 */
	rtn = [physDisk isDiskReady:YES];
	switch(rtn) {
	    case IO_R_SUCCESS:
	    	break;
	    case IO_R_NO_DISK:
		xpr_err("%s readLabel: disk not present\n", disk_name, 
			2,3,4,5);
		return(rtn);
	    default:
	    	IOLog("%s readLabel: bogus return from isDiskReady (%s)\n",
			disk_name, [self stringFromReturn:rtn]);
		return(rtn);
	}
	
	physFormatted = [physDisk isFormatted];
	if((physBlockSize == 0) || !physFormatted) {
		xpr_err("%s readLabel: physDisk UNFORMATTED\n", 
			disk_name, 2,3,4,5);
		return(IO_R_UNFORMATTED);
	}
	
#if defined(GROK_DOS) || defined(GROK_APPLE)
	part_offset = [self NeXTpartitionOffset];
	if (part_offset < 0) {
		return part_offset;
	}
#ifdef	DEBUG
	if (part_offset > 0) {
		printf("found NeXT disk partition at offset"
			", offset = %d sectors\n", part_offset);
	}
#endif	DEBUG
#endif defined(GROK_DOS) || defined(GROK_APPLE)

	/*
	 * Careful, we're reading in an m68k-style label...
	 */
	blocksInLabel = howmany(SIZEOF_DISK_LABEL_T, physBlockSize);
	bytesInLabel = blocksInLabel * physBlockSize;
	labelBufSize = round_page(bytesInLabel);
	
	/*
	 * This assumes that the memory allocator used by IOMalloc will 
	 * return one physically contiguous page if we ask it for one
	 * page...should be OK, right?
	 * This used to be vm_allocate(), but can't DMA to IOTask's
	 * virtual memory.
	 */
	raw_label = IOMalloc(labelBufSize);
	for(label_num=0; label_num<NLABELS; label_num++) {

	/* To read a disk label, we *should* use the blocksize of the
	 * enclosing partition, but we don't know what that is.
	 *
	 * As a workaround, we can use the *physical* blocksize. By
	 * doing so, we will always compute the proper offset for
	 * fixed disks using Mac, FDISK, or *no* partitioning scheme,
	 * since all these schemes assume a 512 byte blocksize.
	 * (It is possible for a Mac partition map to imply a non-512
	 * blocksize, but in practice this is rare).
	 *
	 * Using the physical blocksize would not iterate properly
	 * through all four labels on a Mac-partitioned CD-ROM, because
	 * we'd be using 2K blocks instead of the Mac partition's 512.
	 * This, however, *still* luckily works because the first label
	 * is at offset zero: offset zero is always the same no matter
	 * what you're multiplying by to get that zero!
	 *
	 * Once we find a valid label (often the one at offset zero),
	 * we immediately reset our logical blocksize, so after that
	 * point things work normally.
	 */

	blocksize = [physDisk blockSize];

       		rtn = [[super class] commonReadWrite
                    		: physDisk
				: YES			/* read */
				: ((unsigned long long)part_offset + 
					(label_num * blocksInLabel))
					    * blocksize /* byte offset */
				: bytesInLabel 	/* bytes */
				: raw_label
				: IOVmTaskSelf()
				: (void *)NULL
				: &bytesXfr];
            if((rtn == IO_R_SUCCESS) &&
		   (bytesXfr == bytesInLabel)) {
			
			/*
			 * Is it a valid disk label?
			 */
			const char *rtn;
			
			rtn = check_label(raw_label, 
				part_offset + (label_num*blocksInLabel));
			if(rtn == NULL) {
				found++;
				break;
			}
#ifdef	DEBUG
			else {
				IOLog("%s\n", rtn);
			}
#endif	DEBUG
		}
		
		/*
		 * there is one fatal error here - disk not present.
		 */
		if(rtn == IO_R_NO_DISK)
			break;
	}

	if(found) {
		/*
		 * Success. 
		 */
		xpr_disk("readLabel: Valid Label\n", 1,2,3,4,5);
		_labelValid = 1;
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		_hfsValid = 0;			// bek - 12/14/97 - Is this redundant?
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		get_disk_label(raw_label, label_p);
		rtn = IO_R_SUCCESS;
	}
	else {
		xpr_disk("readLabel: NO LABEL\n", 1,2,3,4,5);
		if(rtn != IO_R_NO_DISK)
			rtn = IO_R_NO_LABEL;
	}
	IOFree(raw_label, labelBufSize);
	return(rtn);
}

/*
 * Write disk label. Label is passed in as an actual native label per 
 * current architecture and is written as an m68k-style label.
 */

static inline void put_short(unsigned short s, void *dest)
{
	*((unsigned short *)dest) = NXSwapHostShortToBig(s);
}

- (IOReturn) writeLabel : (disk_label_t *)label_p
{
	unsigned 		size;
	unsigned short 		*cksum_p;
	unsigned short		cksum;
	ns_time_t 		timestamp;
	int 			label_num;
	unsigned		bytesXfr;
	int 			goodLabel = 0;
	IOReturn 		rtn;
	int 			blocksInLabel;
	int 			bytesInLabel;
	id 			physDisk = [self physicalDisk];
	unsigned		physBlockSize = [self physicalBlockSize];
	char		 	*raw_label = NULL;
	int 			labelBufSize = 0;	/* compiler quirk */
	unsigned		formattedFlag = 0;
	const char 		*lrtn;
	int			cksum_offset;
	/* offset of NeXT disk on dos or Apple partition */
	int			part_offset = 0;
#ifdef	i386
	/*
	 * Avoid writing first label over block 0.
	 */
	int			firstLabel = 1;
#else	i386
	int			firstLabel = 0;
#endif	i386

	xpr_disk("IODiskPartition writeLabel\n", 1,2,3,4,5);
	
	/*
	 * We can't do this if any block devices, or any other logical disk, 
	 * are currently open. Also, we can only do this on partition 0
	 * because we're going to blow away all IODiskPartitions other than the
	 * one for partition 0 before we're thru.
	 */
	rtn = [self checkSafeConfig:"writeLabel"];
	if(rtn) {
		return rtn;
	}

	/*
	 * Lock out similar destructive actions...
	 */
	[physDisk lockLogicalDisks];
	
	formattedFlag = [physDisk isFormatted];
	if(!formattedFlag) {
		xpr_disk("IODiskPartition writeLabel: UNFORMATTED DISK\n",
			1,2,3,4,5);
		rtn = IO_R_IO;
		goto done;
	}
	
	/*
	 * OK, here we go. All other partitions are now invalid. Let's
	 * get rid of them.
	 */
	[self _freePartitions];
	_labelValid = 0;			// until we're thru
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	_hfsValid = 0;			// until we're thru
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	
	if(label_p->dl_version == DL_V1 || label_p->dl_version == DL_V2) {
		size = SIZEOF_DISK_LABEL_T;
		cksum_p = &label_p->dl_checksum;
		cksum_offset = DISK_LABEL_DL_CHECKSUM;
	} else if(label_p->dl_version == DL_V3) {
		size = SIZEOF_DISK_LABEL_T - SIZEOF_DL_UN_T;
		cksum_p = &label_p->dl_v3_checksum;
		cksum_offset = DISK_LABEL_DL_UN;
	}
	else {
		IOLog("%s writeLabel: BAD LABEL\n", [self name]);
		rtn = IO_R_INVALID_ARG;
		goto done;
	}
		
	/*
	 * tag label with time.
	 */
	IOGetTimestamp(&timestamp);
	label_p->dl_tag = (unsigned)timestamp;
	
	/*
	 * prepare to validate (before converting to m68k-style label).
	 */
	label_p->dl_label_blkno = 0;
	*cksum_p = 0;
	
	/*
	 * Get an m68k-style label for validation (and which we'll eventually
	 * write to disk). We do page alignment here to satisfy the most 
	 * stringent DMA requirements downstream.
	 */
	blocksInLabel = howmany(SIZEOF_DISK_LABEL_T, physBlockSize);
	bytesInLabel = blocksInLabel * physBlockSize;
	labelBufSize = round_page(bytesInLabel);
	raw_label = IOMalloc(labelBufSize);
	put_disk_label(label_p, raw_label);

	/*
	 * Get checksum and validate. Careful, put the checksum in the 
	 * raw m68k-style label in a machine-independent manner...
	 */
	cksum = checksum16((unsigned short *)raw_label, size >> 1);
	put_short(cksum, raw_label + cksum_offset);
	if(lrtn = check_label(raw_label, 0)) {
		IOLog("%s writeLabel: BAD LABEL : %s\n", 
			[self name], lrtn);
		rtn = IO_R_INVALID_ARG;
		goto done;
	}
		
#if defined(GROK_DOS) || defined(GROK_APPLE)
	part_offset = [self NeXTpartitionOffset];
	if (part_offset < 0) {
		rtn = part_offset;
		goto done;
	}
#endif defined(GROK_DOS) || defined(GROK_APPLE)

	/*
	 * OK, the caller gave us a good label. Write NLABEL copies.
	 */
	for(label_num=firstLabel; label_num<NLABELS; label_num++) {
		*(int *)(raw_label + DISK_LABEL_DL_LABEL_BLKNO ) = 
			NXSwapHostIntToBig(part_offset + 
				(label_num * blocksInLabel));

       		rtn = [[super class] commonReadWrite
                    		: physDisk
				: NO			/* write */
				: ((unsigned long long)part_offset + 
					(label_num * blocksInLabel))
					    * physBlockSize /* byte offset */
				: bytesInLabel 	/* bytes */
				: raw_label
				: IOVmTaskSelf()
				: (void *)NULL
				: &bytesXfr];

            if((rtn == IO_R_SUCCESS) && (bytesXfr == bytesInLabel)) {
		   	goodLabel++;
		}
		/*
		 * there is one fatal error here - disk not present.
		 */
		if(rtn == IO_R_NO_DISK)
			break;
	}
	if(goodLabel) {
		_labelValid = 1;
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		_hfsValid = 0;			// bek - 12/14/97 - Is this redundant?
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		rtn = IO_R_SUCCESS;
	}
	else {
		xpr_disk("IODiskPartition writeLabel: Couldn\'t write label\n",
			1,2,3,4,5);
		if(rtn != IO_R_NO_DISK)
			rtn = IO_R_IO;
		goto done;
	}
	
	/*
	 * One more thing - update our own parameters and see if we
	 * need to create any block devices.
	 */
	[self _probeLabel: label_p];
done:
	[physDisk unlockLogicalDisks];
	if(raw_label) {
		IOFree(raw_label, labelBufSize);
	}
	return(rtn);
}


/*
 * Before we die, tell owner about this. LogicalDisk takes care of freeing
 * chained logicalDisks.
 */
- free
{
#ifdef	KERNEL
	[self unregisterUnixDisk: _partition];
#endif	KERNEL

	if( _partitionWaitLock) {
	    [_partitionWaitLock free];
	    _partitionWaitLock = nil;
	}
	return([super free]);
}
		  
/*
 * Handle Disk Eject request. Overrides IODisk's method of same name. 
 * Invoked upon the partition 0 instance by DiskObject's eject. This 
 * will be rejected at the DiskObject level if any block devices are open. 
 * We require this to be done only on partition 0 since we're going to blow
 * away all of the other partitions.
 */
- (IOReturn)eject
{
	IOReturn rtn;
	id phys = [self physicalDisk];
	
	xpr_disk("IODiskPartition eject\n", 1,2,3,4,5);

	/*
	 * We can't do this if any block devices, or any other logical disk, 
	 * are currently open. Also, we can only do this on partition 0
	 * because we're going to blow away all IODiskPartitions other than the
	 * one for partition 0 before we're thru.
	 */
	rtn = [self checkSafeConfig:"eject"];
	if(rtn) {
		return rtn;
	}

	/*
	 * Kill all other partitions. Have IODisk clear our
	 * "formatted" flag.
	 */
	[self _freePartitions];
	_labelValid = 0;
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	_hfsValid = 0;
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	[super setFormattedInternal:0];

	/*
	 * Cancel possible outstanding "manual poll" request. This covers
	 * the case in which the open() performed immediately preceeding
	 * this eject command did an implied vol_check_set_poll() in the
	 * driver's open() routine.
	 */
	if([phys needsManualPolling]) {
		vol_check_manual_poll();
	}
	
	/*
	 * Pass this down to physDisk using the internal versions to avoid
	 * getting callbacks.
	 */
	return([phys ejectPhysical]);
}

- (IOReturn) requestRelease
{
	IOReturn rtn;
	
	/*
	 * We can't do this if any block devices, or any other logical disk, 
	 * are currently open. Also, we can only do this on partition 0
	 * because we're going to blow away all IODiskPartitions other than the
	 * one for partition 0 before we're thru.
	 */

	rtn = [self checkSafeConfig:"disown"];
	if(rtn) {
		return rtn;
	}

	/*
	 * Kill all other partitions. Have IODisk clear our
	 * "formatted" flag.
	 */
	[self _freePartitions];
	_labelValid = 0;
#ifdef GROK_APPLE
	_hfsValid = 0;
#endif GROK_APPLE
	[super setFormattedInternal:0];

	return(rtn);
}

/*
 * read/write methods. These are illegal if _labelValid is false, otherwise
 * they're just passed up to LogicalDisk.
 */
#ifdef	KERNEL
- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (out unsigned *)actualLength
				  client : (vm_task_t)client
{
	xpr_disk("IODiskPartition read\n", 1,2,3,4,5);
	
#if 0 // radar 1669467 - ISO 9660 CD support
// Read is NOT illegal if _labelValid is false - ISO 9660 CDs do not have 
// standard disk label.  In order to get read-only ISO 9660 CDROM support
// working, we have disabled the check for a valid label on the read path 
// only.  However, the ISO 9660 file system will not attempt to write to 
// the device and no other client should be able to open the device for 
// writing while the file system has it open, so the above scenario should 
// not occur.  bknight.
//
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Read attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
#endif // radar 1669467 - ISO 9660 CD support

	return([super readAt : offset 
		      length : length 
		      buffer : buffer
		      actualLength : actualLength
		      client : client]);
}
				  
- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client
{
	xpr_disk("IODiskPartition readAsync\n", 1,2,3,4,5);
#if 0 // radar 1669467 - ISO 9660 CD support
// Read is NOT illegal if _labelValid is false - ISO 9660 CDs do not have 
// standard disk label.  In order to get read-only ISO 9660 CDROM support
// working, we have disabled the check for a valid label on the read path 
// only.  However, the ISO 9660 file system will not attempt to write to 
// the device and no other client should be able to open the device for 
// writing while the file system has it open, so the above scenario should 
// not occur.  bknight.
//
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Read attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
#endif // radar 1669467 - ISO 9660 CD support

	return([super readAsyncAt : offset 
		      length : length 
		      buffer : buffer
		      pending: pending
		      client : client]);
}
				  
- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (out unsigned *)actualLength
				  client : (vm_task_t)client
{
	xpr_disk("IODiskPartition writeAt\n", 1,2,3,4,5);
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Write attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
	return([super writeAt : offset 
		      length : length 
		      buffer : buffer
		      actualLength : actualLength
		      client : client]);
}
				  	
- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client
{
	xpr_disk("IODiskPartition writeAsync\n", 1,2,3,4,5);
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Write attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
	return([super writeAsyncAt : offset 
		      length : length 
		      buffer : buffer
		      pending: pending
		      client : client]);
}

#else	KERNEL

- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (out unsigned *)actualLength
{
	xpr_disk("IODiskPartition read\n", 1,2,3,4,5);
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Read attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
	return([super readAt : offset 
		      length : length 
		      buffer : buffer
		      actualLength : actualLength]);
}
				  
- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
{
	xpr_disk("IODiskPartition readAsync\n", 1,2,3,4,5);
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Read attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
	return([super readAsyncAt : offset 
		      length : length 
		      buffer : buffer
		      pending: pending]);
}
				  
- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength
{
	xpr_disk("IODiskPartition writeAt\n", 1,2,3,4,5);
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Write attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
	return([super writeAt : offset 
		      length : length 
		      buffer : buffer
		      actualLength : actualLength]);
}
				  	
- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
{
	xpr_disk("IODiskPartition writeAsync\n", 1,2,3,4,5);
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( _labelValid || _hfsValid ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! _labelValid ) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s: Write attempt with no valid label\n", 
			[self name]);
		return(IO_R_INVALID_ARG);
	}
	return([super writeAsyncAt : offset 
		      length : length 
		      buffer : buffer
		      pending: pending]);
}
#endif	KERNEL

/*
 * Public setFormatted, overrides the same method in IODisk.
 * This is the normal way a setFormatted operation is done in a 
 * Unix environment.
 */
- (IOReturn)setFormatted : (BOOL)formattedFlag
{
	id physDisk;
	IOReturn rtn;
	
	/*
	 * We can't do this if any block devices, or any other logical disk, 
	 * are currently open. Also, we can only do this on partition 0
	 * because we're going to blow away all IODiskPartitions other than the
	 * one for partition 0 before we're thru.
	 */
	rtn = [self checkSafeConfig:"setFormatted"];
	if(rtn) {
		return rtn;
	}

	/*
	 * Pass this down to physDisk using the internal versions to avoid
	 * getting callbacks.
	 */
	physDisk = [self physicalDisk];
	[physDisk setFormattedInternal:formattedFlag];
	if(formattedFlag) {
		/*
		 * Going to a newly formatted state; have physical
		 * device update block size and so forth.
		 */
		[physDisk updatePhysicalParameters];
	}
	
	/*
	 * Strange, but we'll set the internal formatted flag again in case 
	 * physical device driver disagrees with what our caller is saying.
	 * The caller wins in this case.
	 */
	[physDisk setFormattedInternal:formattedFlag];
	[self setFormattedInternal:formattedFlag];
	return IO_R_SUCCESS;
}

/*
 * internal setFormatted - avoids logical disk interaction. Called by
 * the exported setFormatted method both here (for normal Unix use) and
 * from physicalDisk.
 */
- (void)setFormattedInternal:(BOOL)formattedFlag
{
	xpr_disk("%s: setFormattedInternal %d\n", [self name], 
		formattedFlag, 3,4,5);
	[self _freePartitions];
	_labelValid = 0;

	/*
	 * Finally, the low-impact "just set the flag" method in
	 * IODisk...
	 */
	[super setFormattedInternal:formattedFlag];
}


/*
 * Get/set "device open" flags.
 */
- (BOOL)isBlockDeviceOpen
{
	return _blockDeviceOpen;
}

- (void)setBlockDeviceOpen		: (BOOL)openFlag
{
   	 _blockDeviceOpen = openFlag ? YES : NO;
	[self setInstanceOpen: (_blockDeviceOpen || _rawDeviceOpen)];
}

- (BOOL)isRawDeviceOpen
{
	return _rawDeviceOpen;
}

- (void)setRawDeviceOpen		: (BOOL)openFlag
{
	_rawDeviceOpen = openFlag ? YES : NO;
	[self setInstanceOpen: (_blockDeviceOpen || _rawDeviceOpen)];
}

- waitForProbe:(int) seconds
{
    int		attempts;
    BOOL	ready;
    ns_time_t	now;

    if( nil == _partitionWaitLock)
	return( nil);

    [_partitionWaitLock lock];
    ready = [_partitionWaitLock condition];
    [_partitionWaitLock unlock];

    if( NO == ready) do {
        IOGetTimestamp( &now);
        attempts = seconds - (int)((now - _probeTime) / 1000 / 1000 / 1000);
        if( attempts <= 0)
	    continue;

        IOLog( "%s: Waiting for drive to come ready", [self name]);
        for( ; (NO == ready) && attempts; attempts--) {
            IOLog( ".");
            IOSleep( 1000);
            [_partitionWaitLock lock];
            ready = [_partitionWaitLock condition];
            [_partitionWaitLock unlock];
        }
        IOLog( "\n");
        if( NO == ready)
            IOLog( "%s: Disk Not Ready\n", [self name]);

    } while( NO);

    return( ready ? self : nil);
}

@end

@implementation IODiskPartition(Private)


/*
 * Examine a known good label, initialize LogicalDisk parameters for the
 * partition '0' instance (on which this method is invoked) and create 
 * additional partition instances as appropriate.
 *
 * Invoked by LogicalDisk +probe, upon disk insertion, and when a new label
 * is written.
 */
- (void) _probeLabel : (disk_label_t *)labelp
{
	IODiskPartition *partInst;
	int part;
	struct partition *partition_p;
	id physDisk = [self physicalDisk];
	id lastIODiskPartition = self;
	
	xpr_disk("IODiskPartition  _probeLabel\n", 1,2,3,4,5);
	
	if(_partition != 0) {
		IOLog("%s:  _probeLabel on partition != 0\n", 
			[self name]);
		return;
	}
	
	/*
	 * Update internal state for partition 0.
	 */
	[self 	_initPartition:0 
		physicalPartition:_physicalPartition
		disktab:&labelp->dl_dt];
	
	/*
	 * Create & init an additional IODiskPartition instance per valid 
	 * partition. Partition NPART-1 is never used by convention (it's 
	 * the live partition).
	 */
	for(part=1; part<NPART-1; part++) {
		partition_p = &labelp->dl_dt.d_partitions[part];
		if(partition_p->p_size > 0) {
		
			/*
			 * A valid logical disk partition.
			 */
			partInst = [IODiskPartition new];
			[partInst connectToPhysicalDisk:physDisk];
			[partInst _initPartition : part
				physicalPartition: _physicalPartition
				disktab : &labelp->dl_dt];
			[partInst init];
			[partInst registerDevice];
				
			/*
			 * Link to logical disk chain, and notify 
			 * physDisk as well.
			 */
			[lastIODiskPartition setLogicalDisk:partInst];
			[[self physicalDisk] setLogicalDisk:partInst];
			lastIODiskPartition = partInst;
		}
	}
	return;
}

/*
 * Assign logical disk parameters for an IODiskPartition instance based on
 * specified partition number on disktab. Caller must have already 
 * done a connectToPhysicalDisk on this instance, and specified partition
 * is known to be valid.
 */
- (void)_initPartition		: (int)partNum
              physicalPartition : (int)physNum	
                        disktab : (struct disktab *)dtp;
{
	struct partition *pp = &dtp->d_partitions[partNum];
	int partBase;
	char name[30];
	id physDisk = [self physicalDisk];

	/*
	 * Set IODevice-class instance variables.
	 */
	sprintf(name, "%s%c", [physDisk name], 'a' + partNum);
	[self setName:name];
	[self setDriveName:"IODiskPartition Partition"];
	[self setLocation:NULL];
	xpr_disk("%s: _initPartition\n", IOCopyString(name), 2,3,4,5);
	[self setDiskSize:pp->p_size];
	[self setBlockSize:dtp->d_secsize];
	[self setUnit:[physDisk unit]];		// probably unnecessary

	// Ensure partition 0 always has the correct value
	// for this instance variable, since connectToPhysicalDisk
	// is only called once for that partition.
	[self setWriteProtected:[physDisk isWriteProtected]];

	/*
	 * LogicalDisk instance variables are taken care of in
	 * connectToPhysicalDisk.
	 * Set up local instance variables.
	 *
	 * Careful, partitionBase is not necessarily in physical blocksize;
	 * for old disks, it's in d_secsize (usually DEV_BSIZE) from 
	 * disktab...
	 */
	partBase = pp->p_base + dtp->d_front;
	partBase *= dtp->d_secsize / [self physicalBlockSize];
	[self setPartitionBase:partBase];
	_partition =  partNum;
	_physicalPartition = physNum;
	
	/*
	 * The low-impact "just set the flag" method in IODisk...Our
	 * override of this method frees partitions.
	 */
	[super setFormattedInternal:1];
	_labelValid = 1;
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	_hfsValid = 0;			// bek - 12/14/97 - Is this redundant?
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	
#ifdef	KERNEL
	/*
	 * Let Unix layer know about us.
	 */
	[self registerUnixDisk : partNum];
#endif	KERNEL
	return;
}				  
		
/*
 * Free all partition instances other than partition 0. The others must
 * not be open, and this must be invoked on partition 0.
 */
- (IOReturn)_freePartitions;
{
	id nextPart = [self nextLogicalDisk];
	
	if(_partition != 0) {
		IOLog("%s: _freePartitions on partition != 0\n",
			[self name]);
		return IO_R_BUSY;	
	}	
	if(nextPart == nil)
		return IO_R_SUCCESS;
	if([nextPart isOpen]) {
		/* 
		 * this shouldn't happen - this was already checked
		 * in checkSafeConfig.
		 */
		IOLog("%s: _freePartitions with open partitions\n",
			[self name]);
		return IO_R_BUSY;
	}
	[nextPart free];
	[self setLogicalDisk:nil];
	return IO_R_SUCCESS;
}

/*
 * Determine if any block devices in the logicalDisk chain are open.
 */
- (BOOL)isAnyBlockDevOpen
{
	id logDisk = [[self physicalDisk] nextLogicalDisk];
	
	while(logDisk) {
		if([logDisk isBlockDeviceOpen]) {
			return YES;
		}
		logDisk = [logDisk nextLogicalDisk];
	}
	return NO;
}


/*
 * Verify that it's safe to do an operation which will result in a call
 * to _freePartitions.
 */
- (IOReturn)checkSafeConfig	: (const char *)op
{
	if(_partition != 0) {
		return IO_R_BUSY;
	}
	if([self isAnyBlockDevOpen]) {
		return IO_R_BUSY;
	}
	if([self isAnyOtherOpen]) {
		return IO_R_BUSY;
	}
	return IO_R_SUCCESS;
}


#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

/*
 * Check for an apple partition:
 *
 */
#define BLOCKSIZE_512_BYTES	512

+ (void)CreateHFSDevicesFromApplePartitionMap: (id) devDesc
{
	id			physicalDisk = [devDesc directDevice];
	unsigned char	*	blk;
	Block0			*	blk0;
	unsigned long 		blocksize;
	unsigned			bytesXfr;
	IOReturn	 		rtn;
	
    unsigned int		block = 1;
    unsigned int		entry_within_block = 0;
    int					n;
    unsigned int		nentries_per_block = 1;
    unsigned int		numPartBlocks;
    DPME			*	partEntry;
    int				physNum;

	IODiskPartition	*	partitionInstance;
	IODiskPartition	*	lastIODiskPartition = nil; // mandatory initialization
	unsigned			part = 0;

	blocksize = 512;

	blk = IOMalloc(PAGE_SIZE); /* force page alignment */
	blk0 = (Block0 *)blk;

	/* check block 0 for valid Apple partition signature */

	rtn = [[super class] commonReadWrite
            			: physicalDisk
				: YES			/* read */
				: (unsigned long long)0	/* byte offset */
				: PAGE_SIZE 	/* bytes */
				: blk
				: IOVmTaskSelf()
				: (void *)NULL
				: &bytesXfr];

        if (rtn)
	{
	    goto Return;
	}
	
	/* scan for apple hfs partitions */

    /* check if there is a valid map at 512 offset */

    partEntry = (DPME *)(blk + BLOCKSIZE_512_BYTES);

    if (blocksize > BLOCKSIZE_512_BYTES)
    {
		if (NXSwapBigShortToHost(partEntry->dpme_signature) == DPME_SIGNATURE)
		{
		    /* there is a valid map, and the device blocksize
		     * is > 512, which means we probably have a CDROM
		     * using 2K, but having valid 512 map
		     */
		    block = 0;
		    nentries_per_block = blocksize / BLOCKSIZE_512_BYTES;
		    entry_within_block = 1;
		}
		else
		{
		    partEntry = (DPME *)(blk + blocksize);
		}
    }

    if (NXSwapBigShortToHost(partEntry->dpme_signature) != DPME_SIGNATURE)
    {
#if 1 // bknight - 7/2/98 - Radar #2240631
	/* Make a single giant "_hfs_a" partition to support floppies, DOS disks, ISO9660 CDs. */

	partitionInstance = [IODiskPartition new];
	[partitionInstance connectToPhysicalDisk: physicalDisk];
	[partitionInstance setDeviceDescription: devDesc];
	[partitionInstance _initHFSPartition: 0
		physicalPartNum: 0 // bknight - intentionally illegal phys part num
		diskSize: ( [physicalDisk diskSize] * ([physicalDisk blockSize] / BLOCKSIZE_512_BYTES) )
		partitionBase: 0];
	[partitionInstance init]; //bknight - [super init] all the way up to Object
	[partitionInstance registerDevice]; //bknight - doesn't do anything since (!_isPhysical)
			
	/* No need to link onto the end of nonexistent chain of logical partitions.
	   Just notify the physical disk. */
		 
	[physicalDisk setLogicalDisk: partitionInstance]; //bknight - doesn't override if already set
#endif // bknight - 7/2/98 - Radar #2240631
	goto Return;
    }

    numPartBlocks = NXSwapBigLongToHost(partEntry->dpme_map_entries);
    physNum = 1;
    for (n = 0; n < numPartBlocks; n++)
    {
		if (NXSwapBigShortToHost(partEntry->dpme_signature) != DPME_SIGNATURE)
		{
		    break;
		}

		if ( strcmp(partEntry->dpme_type, HFS_PART_TYPE) == 0 )
		{
		    /*
		     * Mount HFS volume only if this isn't "MOSX_OF3_Booter" partition.
		     */
		    if ( strcmp(partEntry->dpme_name, "MOSX_OF3_Booter") != 0 )
		    {
			    unsigned long part_offset;
	
			    part_offset = NXSwapBigLongToHost(partEntry->dpme_pblock_start);
			    if ((part_offset / nentries_per_block * nentries_per_block) != part_offset)
			    {
					IOLog("IODiskPartition: " HFS_PART_TYPE
					      " partition base (%ld x 512) is not"
					      " a multiple of the devblksize %ld\n",
					      part_offset, blocksize);
					break;
			    }
	
		    part_offset = part_offset / nentries_per_block;

			/* A valid logical disk partition. */

			partitionInstance = [IODiskPartition new];
			[partitionInstance connectToPhysicalDisk: physicalDisk];
			[partitionInstance setDeviceDescription:devDesc];
			[partitionInstance
				_initHFSPartition: part
				physicalPartNum: physNum
				diskSize: partEntry->dpme_pblocks
				partitionBase: partEntry->dpme_pblock_start];
			[partitionInstance init]; //bknight - [super init] all the way up to Object
			[partitionInstance registerDevice]; //bknight - doesn't do anything since (!_isPhysical)
				
			/* Link to logical disk chain, and notify the physicalDisk. */
			 
			if ( lastIODiskPartition != nil )
			{
				[lastIODiskPartition setLogicalDisk: partitionInstance];
			}
			lastIODiskPartition = partitionInstance;
			[physicalDisk setLogicalDisk: partitionInstance]; //bknight - doesn't override if already set
			
			/* Increment our partition counter. */
			
			part++;
		    } // if not "MOSX_OF3_Booter" partition flag set
		} // if HFS_PART_TYPE

                physNum++;
		partEntry++;
		if (++entry_within_block == nentries_per_block)
		{
		    block++;
		    entry_within_block = 0;
		    partEntry = (DPME *)blk;
       		    rtn = [[super class] commonReadWrite
                        	: physicalDisk
				: YES			/* read */
		    : (unsigned long long)block * blocksize /* byte offset */
				: blocksize 	/* bytes */
				: blk
				: IOVmTaskSelf()
				: (void *)NULL
				: &bytesXfr];
                    if (rtn)
		    {
				break;
		    }

		} // if

    } // for

Return:

	IOFree(blk, PAGE_SIZE);
	return;

} // CreateHFSDevicesFromApplePartitionMap

#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

#if GROK_APPLE //bknight - 12/3/97 - Radar #2004660

/*
 * Assign logical disk parameters for an IODiskPartition instance based on
 * specified partition number on disktab. Caller must have already 
 * done a connectToPhysicalDisk on this instance, and specified partition
 * is known to be valid.
 */
- (void)	_initHFSPartition	: (int) partitionNum
			physicalPartNum	: (int) physNum
			diskSize	: (int) diskSize
			partitionBase : (int) partitionBase;
{
	int						hfsPartitionNum = partitionNum + NPART;
	char					name[30]; //bknight - hardcoded
	id						physicalDisk = [self physicalDisk];

	/* IODevice initialization */

	sprintf(name, "%s_hfs_%c", [physicalDisk name], 'a' + partitionNum);
	[self setUnit: [physicalDisk unit]]; // IODevice //bknight - probably unnecessary ???
	[self setName: name];
	[self setLocation: NULL];

	/* IODisk initialization */

	[self setDriveName: "IODiskPartition Partition"];

	// always measured in 512-byte blocks, regardless of the device block size

	[self setDiskSize: diskSize]; //bknight - in 512-byte blocks

	// hardcoded per Radar #2211264

	[self setBlockSize: 512]; 

	/* bknight - WHAT DOES THIS COMMENT / LINE MEAN ??? */

	// Ensure partition 0 always has the correct value
	// for this instance variable, since connectToPhysicalDisk
	// is only called once for that partition.
	[self setWriteProtected:[physicalDisk isWriteProtected]];

	/*
	 * IOLogicalDisk instance variables are taken care of in connectToPhysicalDisk.
	 * Set up IODiskPartition instance variables.
	 *
	 */
	[self setPartitionBase: partitionBase];
	_partition = hfsPartitionNum;
        _physicalPartition = physNum;

	/*
	 * The low-impact "just set the flag" method in IODisk...Our
	 * override of this method frees partitions.
	 */
	[super setFormattedInternal:1];
	_hfsValid = 1;
	_labelValid = 0;
	
#ifdef	KERNEL
	/*
	 * Let Unix layer know about us.
	 */
	[self registerUnixDisk: hfsPartitionNum];
#endif	KERNEL

Return:

	return;

} // _initHFSPartition
		
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
#if GROK_APPLE
    if( _hfsValid)
        strcat( types, " "IOTypeHFS);
    else
#endif GROK_APPLE
        strcat( types, " "IOTypeUFS);
    return( self);
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    [super property_IODeviceClass:classes length:maxLen];
    strcat( classes, " "IOClassDiskPartition);
    return( self);
}

- property_IOPartitionNumber:(char *)result length:(unsigned int *)maxLen
{
    sprintf( result, "0x%x", _physicalPartition);
    return( self);
}

- registerLoudly
{
    return( nil);
}

@end

#ifndef	KERNEL
/*
 * Ripped off from kernel's next/checksum16.c
 */
#define	ADDCARRY(x)  (x > 65535 ? x -= 65535 : x)
#define	REDUCE { 				\
	l_util.l = sum; 			\
	sum = l_util.s[0] + l_util.s[1];	\
	ADDCARRY(sum);				\
}

u_short checksum_16(u_short *wp, int num_shorts)
{
	int sum = 0;
	union {
		u_short s[2];
		long	l;
	} l_util;

	while (num_shorts--)
		sum += *wp++;
	REDUCE;
	return (sum);
}
#endif	KERNEL
