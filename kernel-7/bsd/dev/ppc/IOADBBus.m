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

/* Copyright (c) 1997 Apple Computer, Inc.  All rights reserved.
 *
 * IOADBBus.m - This file contains the definition of the
 * IOADBBus Class, which is an indirect driver designed to
 * communicate with devices on the ADB bus.
 *
 * Note: this API is very preliminary and is expected to change drastically
 * in future releases, caveat emptor.  In the future it will probably be based
 * on some extension to the PortDevices protocol.
 *
 * define FAKE_ADB_DRIVER to true if you do not want to actually talk to the
 * adb driver in the kernel, but rather talk to fake devices implemented here
 *
 *
 * HISTORY
 *  1997-12-19    Brent Schorsch (schorsch) created IOADBDevice.m
 *  1998-01-19    Dave Suurballe transformed into IOADBBus.m
 */

// DS2 #import <driverkit/ppc/IOADBDevice.h>
#import <kernserv/prototypes.h>
#import "busses.h"		// DS2
#import <dev/ppc/adb.h>
// DS2 #import <dev/ppc/adb_io.h>
#import "IOADBBus.h"		// DS2
#import "drvPMU/pmu.h"		// DS2

// DS2...

typedef struct privDataStruct
{
id		mDriver_p;	   // pointer to Cuda or PMU driver
} PrivDataT;

#define PrivDataSize (sizeof(PrivDataT))
// ...DS2

#define mDriver (((PrivDataT *) _priv)->mDriver_p)


extern void kprintf (const char * format, ...);
extern void InitializeADB(void);
extern int adb_flush(int);
extern int adb_readreg2(int, int, unsigned char *, int *);
extern int adb_writereg2(int, int, unsigned char *, int);

/* DS2...
typedef struct privDataStruct
{

    IOADBDeviceInfo mADBInfo_p;	//

#if FAKE_ADB_DRIVER
    unsigned char mReg_p[4][IO_ADB_MAX_PACKET];
    int mRegLen_p[4];
#endif    
} PrivDataT;
#define PrivDataSize (sizeof(PrivDataT))

#define mADBInfo (((PrivDataT *) _priv)->mADBInfo_p)

#if FAKE_ADB_DRIVER
#define mReg (((PrivDataT *) _priv)->mReg_p)
#define mRegLen (((PrivDataT *) _priv)->mRegLen_p)
#endif    

typedef struct _adbDeviceTableInfo
{
    IOADBDeviceInfo info;
    IOADBDevice * object;
} IOADBDeviceTableInfo;

static BOOL initialized = NO;
static IOADBDeviceTableInfo gDeviceTable [IO_ADB_MAX_DEVICE*4];
static int gDeviceCount;
...DS2 */

extern id		PMUdriver;			// in adb.m  DS2
extern adb_device_t	adb_devices[ADB_DEVICE_COUNT];	// in adb.m  DS2

/* FROM kernel/.../adb.c */
#if !FAKE_ADB_DRIVER 
extern boolean_t   	adb_initted;
extern int     	   	adb_count;

extern adb_device_t    	adb_devices[];
#endif
/* END from kernel/.../adb.c */

// DS2 static IOReturn initalize (PrivDataT *_priv);

@implementation IOADBBus

// Class methods

// DS2...
+ (BOOL) probe: (id) deviceDescription
{
id dev;
extern int kdp_flag;

if ( (dev = [ self alloc ]) == nil ) {
	return NO;
	}

if ([dev initFromDeviceDescription:deviceDescription] == nil) {
	return NO;
	}

[super init];

[dev setDeviceKind:"ADBBus"];
[dev setLocation:NULL];
[dev setName:"ADBBus"];
[dev registerDevice];

if (kdp_flag & 1)
{
	call_kdp();
}

return YES;
}


- initFromDeviceDescription:(IODeviceDescription *)deviceDescription
{
    _priv = NXZoneCalloc([self zone], 1, PrivDataSize);

    if (!_priv) {
        return [super free];
	}

    mDriver = [deviceDescription directDevice];
    PMUdriver = mDriver;
    InitializeADB();		// initialize adb and probe bus

    return self;
}

+ (IODeviceStyle) deviceStyle
{
return IO_IndirectDevice;
}

static Protocol *protocols[] = {
    @protocol(ADBservice),
    nil
};

+ (Protocol **)requiredProtocols
{
    return protocols;
}

// ...DS2

/*
 * Call this method with an array that is IO_ADB_MAX_DEVICE long.
 */
- (IOReturn) GetTable: (IOADBDeviceInfo *) table
		     : (int *) lenP
{
    IOReturn		status = IO_R_SUCCESS;
    int			index;

/* DS2...
    initalize (_priv);

    *lenP = gDeviceCount;

    for (index = 0; index < gDeviceCount; index++)
	table[index] = gDeviceTable.info;
...DS2 */

// DS2...
    *lenP = IO_ADB_MAX_DEVICE;

for (index = 0; index < IO_ADB_MAX_DEVICE; index++) {
	[self getADBInfo:index:&table[index]];
	}
// ...DS2

    return status;
}

/* DS2...
- initForDevice: (long) uniqueID
	       : (IOReturn *) result;
{
    int index;

    initalize (_priv);

    [super init];

    _priv = NXZoneCalloc([self zone], 1, PrivDataSize);
    if (!_priv)
    {
	*result = IO_R_NO_MEMORY;
        return [super free];
    }

    for (index = 0; index < gDeviceCount; index++)
	{
        if (gDeviceTable[index].info.uniqueID == uniqueID)
            break;
	}

    if (index >= gDeviceCount)
	{
        *result = IO_R_NO_DEVICE;
        return [self free];
	}

    ASSERT (gDeviceTable[index].info.uniqueID == uniqueID);

    if (gDeviceTable[index].object != NULL)
        {
        *result = IO_R_PRIVILEGE;
        return [self free];
        }

    gDeviceTable[index].object = self;

    mADBInfo = gDeviceTable[index].info;

#if FAKE_ADB_DRIVER
    mRegLen[0] = 0;
    mRegLen[1] = 0;
    mRegLen[2] = 0;
    mRegLen[3] = 0;
#endif

    *result = IO_R_SUCCESS;
    return self;
}
 ...DS2 */

- free
{
    if (_priv)
    {
	NXZoneFree([self zone], _priv);
	_priv = NULL;
    }

    return [super free];
}

- (IOReturn) getADBInfo: (int) whichDevice			// DS2
		       : (IOADBDeviceInfo *) deviceInfo
{
// DS2    deviceInfo = mADBInfo;

deviceInfo->originalAddress = adb_devices[whichDevice].a_dev_type;
deviceInfo->address = adb_devices[whichDevice].a_addr;
deviceInfo->originalHandlerID = adb_devices[whichDevice].a_dev_orighandler;
deviceInfo->handlerID = adb_devices[whichDevice].a_dev_handler;
deviceInfo->uniqueID = 0;
deviceInfo->flags = adb_devices[whichDevice].a_flags;

return IO_R_SUCCESS;
}

- (IOReturn) flushADBDevice: (int) whichDevice			// DS2
{
    IOReturn	status = IO_R_SUCCESS;

/* DS2...
#if !FAKE_ADB_DRIVER
    adb_request_t   flush;

    adb_init_request(&flush);
    ADB_BUILD_CMD2(&flush, ADB_PACKET_ADB, (ADB_ADBCMD_FLUSH_ADB | mADBInfo.address << 4));
    adb_send(&flush, TRUE);

    status = flush.a_result;
#endif
...DS2 */

adb_flush(whichDevice);						// DS2
return status;
}

/*
 * Note, the buffers must be IO_ADM_MAX_PACKET long.
 */
- (IOReturn) readADBDeviceRegister: (int) whichDevice		// DS2
		       		  : (int) whichRegister
                            	  : (unsigned char *) buffer
                            	  : (int *) length;
{
    IOReturn	status = IO_R_SUCCESS;
    
/* DS2...
#if FAKE_ADB_DRIVER
    *length = mRegLen[whichRegister];
    memcpy (buffer, mReg[whichRegister], mRegLen[whichRegister]);
#else
    adb_request_t   readreg;

    adb_init_request(&readreg);
    ADB_BUILD_CMD2(&readreg, ADB_PACKET_ADB,
                   (ADB_ADBCMD_READ_ADB | mADBInfo.address << 4 | whichRegister));

    adb_send(&readreg, TRUE);

    *length = readreg.a_reply.a_bcount;
    memcpy (buffer, readreg.a_reply.a_buffer, length);

    status = readreg.a_result;
#endif

...DS2 */

adb_readreg2(whichDevice, whichRegister, buffer, length);		// DS2
return status;
}

- (IOReturn) writeADBDeviceRegister: (int) whichDevice			// DS2
		       		   : (int) whichRegister
                             	   : (unsigned char *) buffer
                             	   : (int) length;
{
    IOReturn	status = IO_R_SUCCESS;
    
/* DS2...
#if FAKE_ADB_DRIVER
    mRegLen[whichRegister] = length;
    memcpy (mReg[whichRegister], buffer, length);
#else
    adb_request_t   writereg;

    if (length > IO_ADB_MAX_PACKET) length = IO_ADB_MAX_PACKET;

    adb_init_request(&writereg);
    ADB_BUILD_CMD2_BUFFER(&writereg, ADB_PACKET_ADB,
                   (ADB_ADBCMD_WRITE_ADB | mADBInfo.address << 4 | whichRegister),
                   length, buffer);

    adb_send(&writereg, TRUE);

    status = writereg.a_result;
#endif

...DS2 */

adb_writereg2(whichDevice, whichRegister, buffer, length);	// DS2
return status;
}

/* The state functions below are not currently implemented */
/*
 * Set the state for the port device.
 */
- (IOReturn) setState: (int) whichDevice			// DS2
		     : (IOADBDeviceState) state
		     : (IOADBDeviceState) mask;
{
    return IO_R_UNSUPPORTED;
}

/*
 * Get the state for the port device.
 */
- (IOADBDeviceState) getState: (int) whichDevice		// DS2
{
    return IO_R_UNSUPPORTED;
}

// DS2...
- (IOReturn) adb_register_handler: (int) type		// DS2
        			 : (autopoll_callback) handler
{
//kprintf("[IOADBBus adb_register_handler: %08x handler: %08x]\n",
//	type, handler);

  adb_register_handler(type, handler);
  return IO_R_SUCCESS;
}
// ...DS2
/*
 * Wait for the atleast one of the state bits defined in mask to be equal
 * to the value defined in state.
 * Check on entry then sleep until necessary.
 */
- (IOReturn) watchState: (int) whichDevice		// DS2
		       : (IOADBDeviceState *) state
                       : (IOADBDeviceState) mask
{

return IO_R_UNSUPPORTED;
}


@end

/* DS2...
static IOReturn initalize (PrivDataT *_priv)
{
    IOReturn	status = IO_R_SUCCESS;

    if (initialized)
    {
        if (gDeviceCount != adb_count)
        {
            IOLog ("Error! adb seems to have been reset, IOADBDevice is invalid!");
            return IO_R_INTERNAL;
	}

        return IO_R_SUCCESS;
    }

    // build the table
#if FAKE_ADB_DRIVER
    gDeviceCount = 1;
    gDeviceTable[0].info.originalAddress = 3;
    gDeviceTable[0].info.address = 3;
    gDeviceTable[0].info.originalHandlerID = 1;
    gDeviceTable[0].info.handlerID = 4;
    gDeviceTable[0].info.uniqueID = 1;
    gDeviceTable[0].info.flags = kIOADBDeviceAvailable;

    gDeviceTable[0].object = NULL;

#else
    int			index;
    int			address;
    adb_device_t    	*adbDevice;

    if (!adb_initted) return IO_R_NOT_OPEN;

    gDeviceCount = adb_count;
    address = 0;

    for (index = 0; index < gDeviceCount && address < IO_ADB_MAX_DEVICE; index++)
    {
        // scan thru adb addresses, looking for once that is present
        while ((adb_devices[address].a_flags & ADB_FLAGS_PRESENT) == 0)
            address++;

        if (address >= IO_ADB_MAX_DEVICE)
            IOLog ("IOADBDevice->initialize error: adb devices count mismatch");

        gDeviceTable[index].info.originalAddress = adb_devices[address].a_dev_type;
        gDeviceTable[index].info.address = adb_devices[address].a_addr; // (== address)
        gDeviceTable[index].info.originalHandlerID = adb_devices[address].a_dev_orighandler;
        gDeviceTable[index].info.handlerID = adb_devices[address].a_dev_handler;
        gDeviceTable[index].info.uniqueID = index + 1;
        gDeviceTable[index].info.flags = 0;

        // if the device is not registered, and not unresolved, they we can access it
	if ((adb_devices[address].a_flags & ADB_FLAGS_REGISTERED) == 0) &&
            (adb_devices[address].a_flags & ADB_FLAGS_UNRESOLVED) == 0))
            gDeviceTable[index].info.flags |= kIOADBDeviceAvailable;

        gDeviceTable[index].object = NULL;
    }
#endif
    
    initialized = YES;

    return status;
}

...DS2 */
