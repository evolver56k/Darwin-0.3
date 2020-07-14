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
 * IODisk.h - Interface for generic Disk class.
 *
 * HISTORY
 * 31-Jan-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <driverkit/return.h>
#import <driverkit/IODevice.h>
#import <bsd/sys/disktab.h>
#import <kernserv/clock_timer.h>

#ifdef ppc //bknight - 12/3/97 - Radar #2004660
#define GROK_APPLE 1
#endif //bknight - 12/3/97 - Radar #2004660

#ifdef	KERNEL
/*
 * The Unix-level code associated with a particular subclass of IODisk
 * keeps an array of these to allow mapping from a dev_t to a IODisk
 * id. One per Unix unit (a unit is a physical disk). The _devAndIdInfo
 * instance variable for an instances of a given class of IODisk 
 * points to the one element in a static array of IODevToIdMap's for 
 * that class.
 */
typedef struct {
	id liveId;			// IODisk/... for live partition
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	/*
	 * Waste the entry for the live partition in order to successfully
	 * index into the HFS partitions.  Arguably, the liveId should have
	 * just been this distinguished array entry, anyways.
	*/

	id partitionId[ 2 * NPART ];	// for block and raw devices

#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	id partitionId[NPART-1];	// for block and raw devices

#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	dev_t rawDev;			// used by volCheck logic
	dev_t blockDev;			// ditto
} IODevAndIdInfo;

#endif	KERNEL

/*
 * Basic "usefulness" state of drive.
 */
typedef enum {
	IO_Ready, 		// Ready for r/w operations
	IO_NotReady,		// not ready (spinning up or busy)
	IO_NoDisk, 		// no disk present
	IO_Ejecting		// eject in progress
} IODiskReadyState;

/*
 * Current known disk types.
 */
typedef enum {
	IO_SCSI,
	IO_Floppy,
	IO_Other
} IODiskType;

@interface IODisk:IODevice
{
@private
	id		_nextLogicalDisk;	// next LogicalDisk object
						// in chain.
						// May be nil. 
	unsigned	_blockSize;		// in bytes 
	unsigned	_diskSize;		// in blockSize's
	BOOL		_removable;		// removable media device 
	BOOL		_formatted;		// disk is formatted
	BOOL		_isPhysical;		// this is NOT a logical disk
	BOOL		_writeProtected;
#ifdef	KERNEL
	IODevAndIdInfo	*_devAndIdInfo;		// provides dev_t to id 
						// mapping for this instance.
#endif	KERNEL
	id		_LogicalDiskLock;	// NXLock. Serializes
						// operations whcih change 
						// LogicalDisks attached to 
						// this device.
	char		_driveName[MAXDNMLEN];	// for Unix 'drive_info'
						// requests 
						
	/*
	 * The lastReadyState variable is initialized by device-specific
	 * subclass, but is subsequently only changed by the volCheck module.
	 */
	IODiskReadyState _lastReadyState;	
	
	/*
	 * Statistics. Accessed en masse via 
	 * getIntValues::DISK_STATS_ARRAY.
	 * All times in ms.
	 */
	unsigned	_readOps;
	unsigned	_bytesRead;
	unsigned	_readTotalTime;
	unsigned	_readLatentTime;
	unsigned	_readRetries;
	unsigned	_readErrors;
	unsigned	_writeOps;
	unsigned 	_bytesWritten;
	unsigned	_writeTotalTime;
	unsigned	_writeLatentTime;
	unsigned	_writeRetries;
	unsigned	_writeErrors;
	unsigned	_otherRetries;
	unsigned	_otherErrors;
	
	int		_IODisk_reserved[4];
}

/*
 * Register instance with current name space.  
 */
- registerDevice;		// nil return means failure

/*
 * Public methods to get and set disk parameters. These are implemented in
 * the IODisk class. 
 */
- (unsigned)diskSize;
- (unsigned)blockSize;
- (IOReturn)setFormatted : (BOOL)formattedFlag;
- (BOOL)isFormatted;
- (BOOL)isRemovable;
- (BOOL)isPhysical;
- (BOOL)isWriteProtected;
- (const char *)driveName;

/*
 * Two forms of eject - one for use with logical disks attached
 * (eject), so that attached NXDisks can be polled for open 
 * state; this method is implemented in IODisk but is normally overridden
 * by a subclass like IODiskPartition. The other is ejectPhysical, 
 * in the IOPhysicalDiskMethods protocol (below). 
 */
- (IOReturn) eject;

/*
 * Get/set parameters used only by subclasses.
 */		  	
- (void)setDiskSize			: (unsigned)size;
- (void)setBlockSize			: (unsigned)size;
- (void)setIsPhysical			: (BOOL)isPhysical;
- nextLogicalDisk;
- (void)setRemovable			: (BOOL)removableFlag;
- (void)setDriveName			: (const char *)name;
- (IODiskReadyState)lastReadyState;
- (void)setLastReadyState		: (IODiskReadyState)readyState;
- (void)setWriteProtected		: (BOOL)writeProtectFlag;
- (void)setFormattedInternal 		: (BOOL)formattedFlag;

/*
 * Statistics support.
 *
 * These methods are invoked by subclass during I/O.
 */
- (void)addToBytesRead		: (unsigned)bytesRead
				  totalTime  : (ns_time_t)totalTime
				  latentTime : (ns_time_t)latentTime;
- (void)addToBytesWritten	: (unsigned)bytesWritten
				  totalTime  : (ns_time_t)totalTime
				  latentTime : (ns_time_t)latentTime;
- (void)incrementReadRetries;
- (void)incrementReadErrors;
- (void)incrementWriteRetries;
- (void)incrementWriteErrors;
- (void)incrementOtherRetries;
- (void)incrementOtherErrors;

/*
 * For gathering cumulative statistics.
 */
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out

/*
 * Obtain statistics in array defined by IODiskStatIndices.
 */
#define IO_DISK_STATS		"IODiskStats"

/*
 * RPC equivalent of isKindOfClassNamed:IODisk. Used in 
 * getParameterInt. Returns no data; just returns IO_R_SUCCESS.
 */
#define IO_IS_A_DISK		"IOIsADisk"

/*
 * RPC equivalent of _isPhysDevice. Used in getParameterInt. Returns one 
 * int, the value of _isPhysDevice.
 */
#define IO_IS_A_PHYSICAL_DISK	"IOIsAPhysicalDisk"

/*
 * Indices into array obtained via getParameterInt : IO_DISK_STATS.
 */
typedef enum {
	IO_Reads,
	IO_BytesRead,
	IO_TotalReadTime,
	IO_LatentReadTime,
	IO_ReadRetries,
	IO_ReadErrors,
	IO_Writes,
	IO_BytesWritten,
	IO_TotalWriteTime,
	IO_LatentWriteTime,
	IO_WriteRetries,
	IO_WriteErrors,
	IO_OtherRetries,
	IO_OtherErrors,
} IODiskStatIndices;
	
#define IO_DISK_STAT_ARRAY_SIZE		(IO_OtherErrors + 1)

/*
 * Register a connection with LogicalDisk.
 */
- (void) setLogicalDisk	: diskId;

/*
 * Lock/Unlock device for LogicalDisk-specific methods. Invoked only by
 * LogicalDisks which are attached to this device.
 */
- (void)lockLogicalDisks;
- (void)unlockLogicalDisks;

/*
 * Convert an IOReturn to text. Overrides superclass's method of same name
 * to allow for additional IOReturn's defined in this file.
 */
- (const char *)stringFromReturn	: (IOReturn)rtn;

/*
 * Request a "please insert disk" panel.
 */
- (void)requestInsertionPanelForDiskType : (IODiskType)diskType;

/*
 * Notify volCheck thread that specified device is in a "disk ejecting" state.
 */
- (void)diskIsEjecting : (IODiskType)diskType;

/*
 * Notify volCheck that disk has gone not ready. Typically called on
 * gross error detection.
 */
- (void)diskNotReady;

/*
 * To be optionally overridden by subclass. IODisk version returns NO.
 * If subclass's version returns NO, and the drive is a removable media 
 * drive, the drive will not be polled once per second while ready state
 * is IO_NotReady or IO_NoDisk; instead; polling will only occur when
 * an DKIOCCHECKINSERT ioctl is executed on the vol driver.
 */
- (BOOL)needsManualPolling;

@end

/* End of IODisk interface. */

/*
 * The IOPhysicalDiskMethods protocol must be implemented by each bottom-level 
 * physical disk subclass of IODisk.
 */

@protocol IOPhysicalDiskMethods

/*
 * Get physical parameters (dev_size, block_size, etc.) from new disk. Called 
 * upon disk insertion detection or other transition to RS_READY.
 */
- (IOReturn)updatePhysicalParameters;

/*
 * Called by volCheck thread when WS has told us that a requested disk is
 * not present. Pending I/Os which require a disk to be present must be 
 * aborted.
 */
- (void)abortRequest;

/*
 * Called by the volCheck thread when a transition to "ready" is detected.
 * Pending I/Os which require a disk may proceed.
 */
- (void)diskBecameReady;

/*
 * Inquire if disk is present; if not, and 'prompt' is TRUE, ask for it. 
 * Returns IO_R_NODISK if:
 *    prompt TRUE, disk not present, and user cancels request for disk.
 *    prompt FALSE, disk not present.
 * Else returns IO_R_SUCCESS.
 */
- (IOReturn)isDiskReady	: (BOOL)prompt;

/*
 * Device-specific eject method, only called on physical device.
 */
- (IOReturn) ejectPhysical;

/*
 * Determine basic state of device. This method should NOT implement any
 * retries. It also should not return RS_EJECTING (That's only used in the
 * lastReadyState instance variable).
 */
- (IODiskReadyState)updateReadyState;

@end

/* end of IOPhysicalDiskMethods protocol */

/*
 * Standard IODisk read/write protocol. Offsets are in blocks.
 * Lengths are in bytes.
 * FIXME - readAsyncAt should NOT copy data back to caller on return in
 * Distrubuted Objects implementation.
 */
@protocol IODiskReadingAndWriting

#ifndef	KERNEL
 
- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength;
				  
- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending;	// untyped
				  
- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength;
				  	
- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending;
#else	KERNEL

- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength 
				  client : (vm_task_t)client;

- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client;
		
- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength 
				  client : (vm_task_t)client;
		  
- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client;
				  
#endif	KERNEL

@end

/* end of DiskDeviceRw protocol */


/*
 * IOReturn's specific to IODisk.
 */
#define IO_R_NO_LABEL		(-1100)		/* no label present */
#define IO_R_UNFORMATTED	(-1101)		/* disk not formatted */
#define IO_R_NO_DISK		(-1102)		/* disk not present */
#define IO_R_NO_BLOCK_ZERO	(-1103)		/* can't read first sector */
#define IO_R_NO_NEXT_PART	(-1104)		/* No NeXT partition on */
						/* DOS disk */
extern IONamedValue readyStateValues[];
