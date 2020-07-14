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
 * ATADisk.m - Exported methods for IDE/ATA Disk device class. 
 *
 * HISTORY 
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */
 
#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/devsw.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDiskMethods.h>
#import <driverkit/IODevice.h>
#import <machkit/NXLock.h>
#import <sys/systm.h>

#import "ATADisk.h"
#import "ATADiskInternal.h"
#import "ATADiskKernel.h"

//#define DEBUG

static int diskUnit = 0;
static BOOL switchTableInited = NO;	

/*
 * List of controllers that have been already probed. We need this since each
 * Instance table lists ATADisk as well as IdeController classes. And we need
 * to create instances of disks attached to each controller only once. 
 */
static int probedControllerCount = 0;
static id probedControllers[MAX_IDE_CONTROLLERS];

@implementation ATADisk

static Protocol *protocols[] = {
    @protocol(IdeControllerPublic),
    nil
};

+ (Protocol **)requiredProtocols
{
    return protocols;
}

+ (IODeviceStyle)deviceStyle
{
    return IO_IndirectDevice;
}

/*
 * IDE drives come with a built in controller on each drive. Hence we can
 * have just one object per controller-disk pair. Probe is invoked at load
 * time. It determines what drives are on the bus and alloc's and init:'s an
 * instance of this class for each one. 
 *
 */

+ (BOOL)probe : deviceDescription
{
    id diskId;
    IODevAndIdInfo *idMap = ide_idmap();
    int unit, i;
    id controllerId = [deviceDescription directDevice];

#ifdef DEBUG
    IOLog("ATADisk probed with controller id %x\n", controllerId);
#endif DEBUG
    
    for (i = 0; i < probedControllerCount; i++)	{
    	if (probedControllers[i] == controllerId)	{
	    IOLog("ATADisk already probed for controller %x\n", controllerId);
	    return YES;
	}
    }
    probedControllers[probedControllerCount++] = controllerId;
    
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
    
	diskId = [[ATADisk alloc] initFromDeviceDescription:deviceDescription];
	[diskId initResources:controllerId];
	[diskId setDevAndIdInfo:&(idMap[diskUnit])];
	
	if ([diskId ideDiskInit:diskUnit target:unit] == NO) {
	    [diskId free];
	    continue;
	}
	
	if (([self hd_devsw_init:deviceDescription]) == NO) {
	    [diskId free];
	    IOLog("ATADisk: failed to add to devsw tables.\n");
	    return NO;
	}
	
	/*
	 * Success; we initialized a drive. Have DiskObject superclass take
	 * care of the rest. 
	 */
	[diskId setDeviceKind:"ATADisk"];
	[diskId setIsPhysical:YES];
	[diskId registerDevice];
	diskUnit += 1;
    }
    
    return YES;
}


- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias
{
    if( [super getDevicePath:path maxLength:maxLen  useAlias:doAlias]) {

	char	unitStr[ 12 ];
	int	len = maxLen - strlen( path);

	sprintf( unitStr, "/@%x", [self driveNum]);
	len -= strlen( unitStr);
	if( len < 0)
	    return( nil);
        strcat( path, unitStr);
	return( self);
    }
    return( nil);
}

- (char *) matchDevicePath:(char *)matchPath
{
    BOOL	matches = NO;
    char    *	unitStr;
    extern long int strtol(const char *nptr, char **endptr, int base);

    unitStr = [super matchDevicePath:matchPath];
    if( unitStr) {
        unitStr = strchr( unitStr, '@');
        if( unitStr) {
            matches = ([self driveNum] == strtol( unitStr + 1, &unitStr, 16));
        }
    }
    if( matches)
        return( unitStr);
    else
        return( NULL);
}

- property_IOUnit:(char *)result length:(unsigned int *)maxLen
{
    sprintf( result, "%d", [self driveNum]);
}


/*
 * Add our entry to the device switch tables. 
 */
+ (BOOL)hd_devsw_init:deviceDescription
{
    extern int seltrue();
    
    /*
     * We get called once for each IDE controller in the system; we
     * only have to call IOAddToCdevsw() once.
     */
    if (switchTableInited == YES)	{
    	return YES;
    }

#if 0	
    if ([self addToCdevswFromDescription: deviceDescription
                                    open: (IOSwitchFunc) ideopen
                                   close: (IOSwitchFunc) ideclose
                                    read: (IOSwitchFunc) ideread
                                   write: (IOSwitchFunc) idewrite
                                   ioctl: (IOSwitchFunc) ideioctl
                                    stop: (IOSwitchFunc) eno_stop
                                   reset: (IOSwitchFunc) nulldev
                                  select: (IOSwitchFunc) seltrue
                                    mmap: (IOSwitchFunc) eno_mmap
                                    getc: (IOSwitchFunc) eno_getc
                                    putc: (IOSwitchFunc) eno_putc] != YES)
    {
	    return NO;
    }

    if ([self addToBdevswFromDescription: deviceDescription
                                    open: (IOSwitchFunc) ideopen
                                   close: (IOSwitchFunc) ideclose
                                strategy: (IOSwitchFunc) idestrategy
                                   ioctl: (IOSwitchFunc) ideioctl
                                    dump: (IOSwitchFunc) eno_dump
                                   psize: (IOSwitchFunc) idesize
                                  isTape: FALSE] != YES)
    {
	    return NO;
    }
#endif

    if (  IOAddToCdevswAt( 	15,
                   		(IOSwitchFunc) ideopen,
                             	(IOSwitchFunc) ideclose,
                              	(IOSwitchFunc) ideread,
                              	(IOSwitchFunc) idewrite,
                               	(IOSwitchFunc) ideioctl,
                               	(IOSwitchFunc) eno_stop,
                               	(IOSwitchFunc) nulldev,
                               	(IOSwitchFunc) seltrue,
                               	(IOSwitchFunc) eno_mmap,
                               	(IOSwitchFunc) eno_getc,
                               	(IOSwitchFunc) eno_putc) < 0 )
    {
    	return NO;
    }
    [self setCharacterMajor: 15];

    if ( IOAddToBdevswAt(	3,
                                (IOSwitchFunc) ideopen,
                                (IOSwitchFunc) ideclose,
                                (IOSwitchFunc) idestrategy,
                                (IOSwitchFunc) ideioctl,
                                (IOSwitchFunc) eno_dump,
                                (IOSwitchFunc) idesize,
                                FALSE ) < 0 )
    {
    	return NO;
    }
    [self setBlockMajor: 3];

   ide_init_idmap(self);
    
    switchTableInited = YES;
    
//#ifdef undef
    IOLog("IDE: block major %d, character major %d\n",
	[self blockMajor], [self characterMajor]);
//#endif undef

    return YES;
}


/*
 * Common read/write methods. These are used directly in the kernel; user-level
 * methods using remote objects as defined in IODevice.h in turn call these.
 */
- (IOReturn) readAt		: (unsigned)offset 
				    length : (unsigned)length 
				    buffer : (unsigned char *)buffer
				    actualLength : (unsigned *)actualLength 
				    client : (vm_task_t)client
{
    IOReturn rtn;
	
    rtn = [self deviceRwCommon : IDEC_READ
	    block : offset
	    length : length 
	    buffer : buffer
	    client: client
	    pending : NULL
	    actualLength : actualLength];
    return(rtn);
}				  

- (IOReturn) readAsyncAt	: (unsigned)offset 
				    length : (unsigned)length 
				    buffer : (unsigned char *)buffer
				    pending : (void *)pending
				    client : (vm_task_t)client
{
    IOReturn rtn;
	
    rtn = [self deviceRwCommon : IDEC_READ
	    block : offset
	    length : length 
	    buffer : buffer
	    client : client
	    pending : (void *)pending
	    actualLength : NULL];
    return(rtn);
}				  
		
- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength 
				  client : (vm_task_t)client
{
    IOReturn rtn;
    
    rtn = [self deviceRwCommon : IDEC_WRITE
	    block : offset
	    length : length 
	    buffer : buffer
	    client: client
	    pending : NULL
	    actualLength : actualLength];

    return(rtn);
}				  
		  
- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
				  client : (vm_task_t)client
{
    IOReturn rtn;
    
    rtn = [self deviceRwCommon : IDEC_WRITE
	    block : offset
	    length : length 
	    buffer : buffer
	    client : client
	    pending : (void *)pending
	    actualLength : NULL];
    return(rtn);
}				  

- (IOReturn)updatePhysicalParameters
{
    // we have got everything we need during initialization

    return(IO_R_SUCCESS);
}

- (void)abortRequest
{
    ideBuf_t *ideBuf;
    IOReturn rtn;
    
    ideBuf = [self allocIdeBuf:NULL];
    ideBuf->command = IDEC_ABORT;
    ideBuf->buf = NULL;
    ideBuf->needsDisk =  0;
    ideBuf->oneWay = 0;
    rtn = [self enqueueIdeBuf:ideBuf];
    [self freeIdeBuf:ideBuf];
}

- (void)diskBecameReady
{
    [_ioQLock lock];
    [_ioQLock unlockWith:WORK_AVAILABLE];
}

- (IOReturn)isDiskReady	: (BOOL)prompt
{
    return(IO_R_SUCCESS);
}

- (IODiskReadyState)updateReadyState
{
    return([self lastReadyState]);
}

- (IOReturn) ejectPhysical
{
    return(IO_R_UNSUPPORTED);
}

- (int)deviceOpen:(u_int)intentions
{
    return(0);
}

- (void)deviceClose
{
    return;
}

- (ideDriveInfo_t)ideGetDriveInfo
{
    return(_ideInfo);
}

- (id)cntrlr
{
    return _cntrlr;
}

- (unsigned)driveNum
{
    return _driveNum;
}

- (IOReturn)getIntValues:(unsigned int *)values
	    forParameter:(IOParameterName)parameter
	    count:(unsigned int *)count
{
    int maxCount = *count;
    int blockMajor, characterMajor;

    if (maxCount == 0) {
	maxCount = IO_MAX_PARAMETER_ARRAY_LENGTH;
    }
    
    if (strcmp(parameter, "BlockMajor") == 0) {
        ide_block_char_majors(&blockMajor, &characterMajor);
	values[0] = blockMajor;
	*count = 1;
	return IO_R_SUCCESS;
    }
    if (strcmp(parameter, "CharacterMajor") == 0) {
        ide_block_char_majors(&blockMajor, &characterMajor);
	values[0] = characterMajor;
	*count = 1;
	return IO_R_SUCCESS;
    }
    
    /*
     * Pass to superclass what we can't handle. 
     */
    return [super getIntValues:values forParameter:parameter
		count:&maxCount];
}

@end
