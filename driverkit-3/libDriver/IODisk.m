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
 * IODisk.m - implementation of generic disk object.
 *
 * HISTORY
 * 31-Jan-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <driverkit/return.h>
#import <mach/kern_return.h>
#import <driverkit/IODisk.h>
#import <driverkit/Device_ddm.h>
#ifdef	KERNEL
#import <kernserv/prototypes.h>
#import <mach/mach_interface.h>
#else	KERNEL
#import <mach/mach.h>
#import <bsd/libc.h>
#endif	KERNEL
#import <driverkit/IOLogicalDisk.h> 
#import <driverkit/IODiskPartition.h>
#import <driverkit/volCheck.h>
#import <machkit/NXLock.h>
#import <driverkit/generalFuncs.h>
#import <bsd/sys/errno.h>
#import <driverkit/IODeviceDescription.h>

#ifdef ppc //bknight - 12/16/97 - Radar #2004660
#define GROK_APPLE 1
#endif //bknight - 12/16/97 - Radar #2004660

/*
 * For stringFromReturn:.
 */
static IONamedValue diskIoReturnValues[] = { 
        {IO_R_NO_LABEL,		"No Label"		},
        {IO_R_UNFORMATTED,	"Disk Not Formatted"	},
	{IO_R_NO_DISK,		"Disk Not Present"	},
	{IO_R_NO_BLOCK_ZERO,	"Can\'t Read Block 0"	},
	{IO_R_NO_NEXT_PART,	"No NeXT partition"	},
	{0,			NULL			}
};

#ifdef	DDM_DEBUG
/*
 * For xpr's.
 */
IONamedValue readyStateValues[] = { 
        {IO_Ready,		"Ready"		},
	{IO_NotReady,		"Not Ready"	},
	{IO_NoDisk,		"No Disk"	},
	{IO_Ejecting,		"Ejecting"	},
	{0,			NULL		}
};
#endif	DDM_DEBUG

@implementation IODisk

/*
 * Public methods.
 */

/*
 * Public methods to get and set disk parameters. May be overridden by
 * subclass.
 */
 
- (unsigned)diskSize
{
	return(_diskSize);
}

- (unsigned)blockSize
{
	return(_blockSize);
}

- (IOReturn)setFormatted : (BOOL)formattedFlag
{
	
	/*
	 * This is illegal if any attached logicalDisks are open. 
	 * In the Unix world, this is normally only done on the raw
	 * device, an IODiskPartition, which overrides this method. 
	 */
#ifdef	KERNEL
	IOPanic("setFormatted: on IODisk");
#else	KERNEL
	id targ = (id)self;

	xpr_disk("DiskObject %s: setFormatted flag = %d\n",
		[self name], formattedFlag, 3,4,5);
		
	if(_nextLogicalDisk && [_nextLogicalDisk isOpen]) {
		xpr_err("%s: setFormatted with logicalDisk open\n",
			[self name], 2,3,4,5);
		return(IO_R_BUSY);
	}
	if(formattedFlag) {
		/*
		 * Update our internal device parameters.
		 * FIXME - maybe check for respondsTo:.
		 */
		[targ updatePhysicalParameters];
		_formatted = 1;
	}
	else
		_formatted = 0;
	/*
	 * Let any attached logical disks know about this, using the internal
	 * version of setFormatted to avoid having our setFormattedInternal:
	 * being called back..
	 */
	if(_nextLogicalDisk)
		[_nextLogicalDisk setFormattedInternal:formattedFlag];
#endif	KERNEL
	return(IO_R_SUCCESS);
}

- (BOOL)isFormatted
{
	return(_formatted);
}

- (BOOL)isRemovable
{
	return(_removable);
}

- (const char *)driveName
{
	return(_driveName);
}

- (BOOL)isPhysical
{
	return(_isPhysical);
}

- (BOOL)isWriteProtected
{
	return(_writeProtected);
}

/*
 * Eject current disk.
 */
- (IOReturn) eject
{
	
#ifdef	KERNEL
	IOPanic("IODisk eject in kernel illegal\n");
	return(IO_R_UNSUPPORTED);
#else	KERNEL

	id targ = (id)self;

	if(!_removable)
		return(IO_R_UNSUPPORTED);
	xpr_disk("DiskObject %s eject:\n", [self name], 2,3,4,5);
	
	/*
	 * We can't allow this if any logical disks are attached. 
	 * Normally in the Unix world, eject is invoked on an IODiskPartition,
	 * which overrides this method and ends up calling physDev's
	 * ejectPhysical. If we're here, that means we must be a
	 * physical device.
	 */
	if(_nextLogicalDisk) {
		xpr_disk("DiskObject eject: Logical Disk(s)"
			" Attached\n", 1,2,3,4,5);
		IOLog("%s: Eject attempt with attached logical disks", 
			[self name]);
		return(IO_R_OPEN);
	}
	
	/*
	 * We know nothing about this disk...
	 */
	_formatted = 0;
	
	/* 
	 * Can't send this method to self, it's in IOPhysicalDiskMethods 
	 * protocol.
	 * FIXME - probably should check for respondsTo:.
	 */
	return([targ ejectPhysical]);
#endif	KERNEL
}

/*
 * Get/set parameters used only by subclasses.
 */		  	
- (void)setDiskSize	: (unsigned)size
{
	_diskSize = size;
}

- (void)setBlockSize : (unsigned)size
{
	_blockSize = size;
}

- (void)setIsPhysical : (BOOL)isPhysFlag
{
	_isPhysical = isPhysFlag ? YES : NO;
}

- nextLogicalDisk
{
	return(_nextLogicalDisk);
}

- (void) setRemovable : (BOOL)removableFlag
{
	_removable = removableFlag ? YES : NO;
	return;
}

- (void)setDriveName	: (const char *)name
{
	int len;
	
	len = strlen((char *)name);
	if(len >= MAXDNMLEN)
		len = MAXDNMLEN - 1;
	strncpy(_driveName, name, len);
	_driveName[MAXDNMLEN - 1] = '\0';		
}

- (IODiskReadyState)lastReadyState
{
	return(_lastReadyState);
}

- (void)setLastReadyState : (IODiskReadyState)readyState
{
	_lastReadyState = readyState;
}

- (void)setWriteProtected	: (BOOL)writeProtectedFlag
{
	_writeProtected = writeProtectedFlag ? YES : NO;
}


/*
 * internal setFormatted - avoids logical disk interaction.
 */
- (void)setFormattedInternal:(BOOL)formattedFlag
{
	xpr_disk("DiskObject %s: setFormattedInternal flag = %d\n",
		[self name], formattedFlag, 3,4,5);
	_formatted = formattedFlag ? YES : NO;
}

/*
 * Statistics support.
 */
- (void)addToBytesRead  : (unsigned)bytesRead
		    totalTime : (ns_time_t)totalTime
		    latentTime : (ns_time_t)latentTime
{
	unsigned long long ms_ll;
	
	_readOps++;
	_bytesRead += bytesRead;
	ms_ll = totalTime / (1000 * 1000);
	_readTotalTime += ms_ll;
	ms_ll = latentTime / (1000 * 1000);
	_readLatentTime += ms_ll;
}

- (void)addToBytesWritten : (unsigned)bytesWritten
		     totalTime : (ns_time_t)totalTime
		     latentTime : (ns_time_t)latentTime
{
	unsigned long long ms_ll;
	
	_writeOps++;
	_bytesWritten += bytesWritten;
	ms_ll = totalTime / (1000 * 1000);
	_writeTotalTime += ms_ll;
	ms_ll = latentTime / (1000 * 1000);
	_writeLatentTime += ms_ll;
}

- (void)incrementReadRetries
{
	_readRetries++;
}

- (void)incrementReadErrors
{	
	_readErrors++;
}

- (void)incrementWriteRetries
{
	_writeRetries++;
}

- (void)incrementWriteErrors
{	
	_writeErrors++;
}

- (void)incrementOtherRetries
{
	_otherRetries++;
}

- (void)incrementOtherErrors
{	
	_otherErrors++;
}


/*
 * For gathering cumulative statistics.
 */
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out
{
	unsigned int paramArray[IO_DISK_STAT_ARRAY_SIZE];
	int i;
	int maxCount = *count;
	
	if(maxCount == 0) {
		maxCount = IO_MAX_PARAMETER_ARRAY_LENGTH;
	}
	if(strcmp(parameterName, IO_DISK_STATS) == 0) {
		paramArray[IO_Reads]  		= _readOps;
		paramArray[IO_BytesRead]     	= _bytesRead;
		paramArray[IO_TotalReadTime]	= _readTotalTime;
		paramArray[IO_LatentReadTime]	= _readLatentTime;
		paramArray[IO_ReadRetries]      = _readRetries;
		paramArray[IO_ReadErrors]       = _readErrors;
		paramArray[IO_Writes]  	 	= _writeOps;
		paramArray[IO_BytesWritten]   	= _bytesWritten;
		paramArray[IO_TotalWriteTime]   = _writeTotalTime;
		paramArray[IO_LatentWriteTime]  = _writeLatentTime;
		paramArray[IO_WriteRetries]     = _writeRetries;
		paramArray[IO_WriteErrors]      = _writeErrors;
		paramArray[IO_OtherRetries]     = _otherRetries;
		paramArray[IO_OtherErrors]      = _otherErrors;
		
		*count = 0;
		for(i=0; i<IO_DISK_STAT_ARRAY_SIZE; i++) {
			if(*count == maxCount)
				break;
			parameterArray[i] = paramArray[i];
			(*count)++;
		}
		return IO_R_SUCCESS;
	}
	else if(strcmp(parameterName, IO_IS_A_DISK) == 0) {
		/*
		 * No data; just let caller know we're a disk.
		 */
		*count = 0;
		return IO_R_SUCCESS;
	}
	else if(strcmp(parameterName, IO_IS_A_PHYSICAL_DISK) == 0) {
		*count = 1;
		parameterArray[0] = _isPhysical ? 1 : 0;
		return IO_R_SUCCESS;
	}
	else {
		return [super getIntValues : parameterArray
			forParameter : parameterName
			count : count];

	}
}					

/*
 * Register a connection with LogicalDisk. If we already have a logicalDisk,
 * this is a nop - subclasses might override this method to perform 
 * other operations in that case. 
 */
- (void) setLogicalDisk	: diskId
{
	xpr_disk("setLogicalDisk: log %s self %s oldLog %s\n",
		(diskId ? [diskId name] : "nil"), 
		[self name], 
		(_nextLogicalDisk ? [_nextLogicalDisk name] : "nil"), 4,5);
#ifdef GROK_APPLE //bknight - 12/16/97 - Radar #2004660
	/*
	 * bknight - 12/16/97 - This was changed in order to prevent a
	 * space leak caused by IODiskPartition instances for HFS partitions
	 * on disks that get ejected.  It changes the meaning of this call
	 * to be either "set to nil" or "insert at the end of the list beginning
	 * at", depending on the value of the argument.  With this change,
	 * it becomes important that the _nextLogicalDisk field gets set to nil
	 * before being used - including being used by this routine, which means
	 * that it must be set to nil before setting it to a non-nil value.
	 * Otherwise, there would be a use-before-definition bug.  Anyhow, it
	 * it appears that all the callers do a setLogicalDisk:nil before
	 * ever calling it with a non-nil argument, so this is safe.
	 * However, if new call sites are added, this needs to be taken into
	 * account, or we need to split it into two methods each having the
	 * appropriate functionality: either "set the field" or "add to the list".
	 */
	if( ! diskId ) {
		_nextLogicalDisk = diskId;
	}
	else if ( ! _nextLogicalDisk && self != diskId ) {
		_nextLogicalDisk = diskId;
	}
	else if ( _nextLogicalDisk ) {
		[_nextLogicalDisk setLogicalDisk: diskId];
	}
#else GROK_APPLE //bknight - 12/16/97 - Radar #2004660
	if(_nextLogicalDisk == nil) {
		_nextLogicalDisk = diskId;
	}
#endif GROK_APPLE //bknight - 12/16/97 - Radar #2004660

}

/*
 * Invoked by instance of subclass upon completion of device initialization.
 * The code here is only executed for physical disks.
 *
 * All device parameters (_diskSize, _blockSize, _formatted, _removable,
 * _lastReadyState) must be valid at this time.
 */
#define ALWAYS_VCREGISTER	1

- registerDevice
{
	id targ = (id)self;	// IODisk doesn't implement updateReadyState
	id ret;
	
	xpr_disk("DiskObject registerDisk\n", 1,2,3,4,5);
	if(!_isPhysical) {
                ret = [super registerDevice];
		goto out;
	}
	
	/*
	 * Do some trivial initialization.
	 */

	_nextLogicalDisk = nil;
	_LogicalDiskLock = [NXLock new];
		
	_readOps	 = 0;
	_bytesRead       = 0;
	_readTotalTime   = 0;
	_readLatentTime  = 0;
	_readRetries     = 0;
	_readErrors	 = 0;
	_writeOps        = 0;
	_bytesWritten    = 0;
	_writeTotalTime  = 0;
	_writeLatentTime = 0;
	_writeRetries    = 0;
	_writeErrors	 = 0;
	_otherRetries    = 0;
	_otherErrors	 = 0;
	
	/*
	 * For physical disks, this will cause probes of IODiskPartition.
	 */
	ret = [super registerDevice];

	/*
	 * sign up for disk insertion/ready detect notification.
	 */
	if(ret != nil && (
	   ALWAYS_VCREGISTER || 
	   _removable || 
	   ([targ updateReadyState] != IO_Ready))) {
	 	/*
		 * Make sure the disk conforms to the appropriate protocol.
		 */
		if(! [[self class] 
		    conformsTo:@protocol(IOPhysicalDiskMethods)] ) {
		    	IOLog("Warning: %s, class %s, does not conform to "
				"IOPhysicalDiskMethods\n",
				[self name], [[self class] name]);
			goto out;	
		} 
#ifdef	KERNEL
		volCheckRegister(self,
			_devAndIdInfo->blockDev,
			_devAndIdInfo->rawDev);
#else	KERNEL
		volCheckRegister(self, 0, 0);
#endif	KERNEL
	}
	
out:
	return ret;
}

/*
 * Lock/Unlock device for LogicalDisk-specific methods. Invoked only by
 * LogicalDisks which are attached to this device.
 */
- (void)lockLogicalDisks
{
	[_LogicalDiskLock lock];
}

- (void)unlockLogicalDisks
{
	[_LogicalDiskLock unlock];
}

/*
 * Convert an IOReturn to text. Overrides superclass's method of same name
 * to allow for additional IOReturn's defined in IODisk.h.
 */
- (const char *)stringFromReturn	: (IOReturn)rtn
{
	IONamedValue *valArray = diskIoReturnValues;
	
	for( ; valArray->name; valArray++) {
		if(valArray->value == rtn)
			return(valArray->name);
	}
	/* 
	 * Not found here. Pass up to superclass.
	 */
	return([super stringFromReturn:rtn]);
}

/*
 * Convert an IOReturn to an errno.
 */
- (int)errnoFromReturn : (IOReturn)rtn
{
	switch(rtn) {
	    case IO_R_NO_DISK:
	    	return(ENXIO);
	    case IO_R_NO_LABEL:
	    	return(ENXIO);		// ???
	    case IO_R_UNFORMATTED:
	    	return(EINVAL);
	    default:
		/* 
		 * Not found here. Pass up to superclass.
		 */
		return([super errnoFromReturn:rtn]);

	}
}

/* 
 * volCheck module support.
 */
 
static inline int 
diskTypeToPR(IODiskType diskType)
{
	switch(diskType) {
	    case IO_SCSI:
	    	return PR_DRIVE_SCSI;
	    case IO_Floppy:
	    	return PR_DRIVE_FLOPPY;
	    case IO_Other:
	    	return -1;
	}
}

/*
 * Request a "please insert disk" panel.
 */
- (void)requestInsertionPanelForDiskType : (IODiskType)diskType
{
	volCheckRequest(self, diskTypeToPR(diskType));
}

/*
 * Notify volCheck thread that specified device is in a "disk ejecting" state.
 */
- (void)diskIsEjecting : (IODiskType)diskType
{
	volCheckEjecting(self, diskTypeToPR(diskType));

}

/*
 * Notify volCheck that disk has gone not ready. Typically called on
 * gross error detection.
 */
- (void)diskNotReady
{
	volCheckNotReady(self);
}

/*
 * To be optionally overridden by subclass. IODisk version returns NO.
 * If subclass's version returns NO, and the drive is a removable media 
 * drive, the drive will not be polled once per second while ready state
 * is IO_NotReady or IO_NoDisk; instead; polling will only occur when
 * an DKIOCCHECKINSERT ioctl is executed on the vol driver.
 */
- (BOOL)needsManualPolling
{
	return NO;
}	

- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
    if( [self isRemovable])
        strcat( types, " "IOTypeRemovableDisk);
    if( [self isWriteProtected])
        strcat( types, " "IOTypeWriteProtectedDisk);

    return( self);
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    strcpy( classes, IOClassBlock);
    if( [self isPhysical])
        strcat( classes, " "IOClassDisk);
    return( self);
}

@end
