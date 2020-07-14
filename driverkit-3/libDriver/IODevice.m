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
 * IODevice.m - Implementation of IODevice superclass
 *
 * HISTORY
 * 13-Jan-98	Martin Minow at Apple
 *	Added createMachPort
 * 20-Jan-93	Doug Mitchell
 *	Implemented the Walrath Changes.
 * 29-Jan-91    Doug Mitchell at NeXT
 *      Created.
 */

/*
 * FIXME - should global parameter stuff be KERNEL only?
 */
 
#import <bsd/sys/types.h>
#import <mach/kern_return.h>
#import <mach/message.h>
#import <driverkit/IODevice.h>
#import <driverkit/Device_ddm.h>

#ifdef	KERNEL
#import <kernserv/prototypes.h>
#import <mach/mach_interface.h>
#else	KERNEL
#import <mach/mach.h>
#import <bsd/string.h>
#endif	KERNEL

#import <machkit/NXLock.h>
#import <bsd/sys/errno.h>

#import <driverkit/generalFuncs.h>
#import <driverkit/IODeviceParams.h>
#import <driverkit/IODeviceDescription.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/IODeviceKernPrivate.h>
#ifdef	KERNEL
#import <driverkit/KernStringList.h>
#endif
#import <kernserv/queue.h>
#import <objc/List.h>

/*
 * For stringFromReturn:.
 */
static IONamedValue IOReturn_values[] = { 
        {IO_R_SUCCESS,			"Success"			},
        {IO_R_NO_MEMORY,		"Memory Allocation error"	},
        {IO_R_RESOURCE,			"Resource Shortage"		},
        {IO_R_IPC_FAILURE,		"Mach IPC Failure"		},
        {IO_R_NO_DEVICE,		"No Such Device"		},
        {IO_R_PRIVILEGE,		"Privilege Violation"		},
        {IO_R_INVALID_ARG,		"Invalid Argument"		},
        {IO_R_LOCKED_READ,		"Device Read Locked"		},
        {IO_R_LOCKED_WRITE,		"Device Write Locked"		},
        {IO_R_EXCLUSIVE_ACCESS,		"Exclusive Access Device"	},
        {IO_R_BAD_MSG_ID,		"Bad IPC Message ID"		},
        {IO_R_UNSUPPORTED,		"Unsupported Function"		},
        {IO_R_VM_FAILURE,		"Virtual Memory error"		},
        {IO_R_INTERNAL,			"Internal Driver error"		},
        {IO_R_IO,			"I/O error"			},
        {IO_R_CANT_LOCK,		"Can\'t Acquire Desired Lock"	},
        {IO_R_NOT_OPEN,			"Device Not Open"		},
        {IO_R_NOT_READABLE,		"Device Not Readable"		},
        {IO_R_NOT_WRITABLE,		"Device Not Writeable"		},
	{IO_R_ALIGN,			"DMA ALignment error"		},
	{IO_R_MEDIA,			"Media error"			},
	{IO_R_OPEN,			"Device(s) still open"		},
	{IO_R_RLD,			"rld failure"			},
	{IO_R_DMA,			"DMA failure"			},
	{IO_R_BUSY,			"Device Busy"			},
	{IO_R_TIMEOUT,			"I/O Timeout"			},
	{IO_R_OFFLINE,			"Device Offline"		},
	{IO_R_NOT_READY,		"Not Ready"			},
	{IO_R_NOT_ATTACHED,		"Device/Channel not attached"	},
	{IO_R_NO_CHANNELS,		"No DMA Channels Available"	},
	{IO_R_NO_SPACE,			"No Address Space for Mapping"	},
	{IO_R_PORT_EXISTS,		"Device Port Already Exists"	},
	{IO_R_CANT_WIRE,		"Can\'t Wire Physical Memory"	},
	{IO_R_NO_INTERRUPT,		"No Interrupt Port Attached"	},
	{IO_R_NO_FRAMES,		"No DMA Frames Enqueued"	},
        {IO_R_INVALID,			"INVALID STATUS (Internal Error)" },
	{0,				NULL				}
};

/*
 * Struct to map "global Object number" to id.
 */
typedef struct {
  	id		instance;
	int		objectNumber;
  	queue_chain_t 	link;
} objectListEntry;

/*
 * Struct to maintain IODevice class list.
 */
typedef struct {
	id		classId;
	int 		classNumber;
	int		bmajor;
	int		cmajor;
	id		originalDeviceDescription;
	queue_chain_t	link;
} classListEntry;

/*
 * ObjectNumber-to-id mapping info. 
 */
static int globalObjectCounter;
static queue_head_t objectList;		// queue of objectListEntry's
static NXLock *objectListLock;		// protects both of the above

/*
 * IODevice class list.
 */
static int classCounter;
static queue_head_t classList;
static NXLock *classListLock;

/*
 * Static function prototypes.
 */
static IOReturn objectNumToId(IOObjectNumber objectNum, id *instance);
static IOReturn deviceNameToId(IOString name, 
	id *instance,
	IOObjectNumber *objectNum);
static objectListEntry *getObjectListEntry(id object);
static IOReturn getClassListEntryForId(
	id classId,
	classListEntry  **entry);
static IOReturn getClassListEntry(unsigned classNumber,
	classListEntry  **entry);

@implementation IODevice

static BOOL IODeviceInitFlag = NO;

/* Shouldn't this be somewhere else? */
static inline int _atoi(const char *string)
{
    int sum = 0;
    while(*string && (*string >= '0' && *string <= '9'))
	sum = (sum * 10) + (*string++ - '0');
    return sum;
}

+ initialize
{
	if(self != objc_getClass("IODevice")) {
		/*
		 * This is someone else's. Register them.
		 */
		[IODevice registerClass:self];
	}

	if(IODeviceInitFlag) {
		/*
		 * We've already been here. This shouldn't happen unless
		 * someone calls [IODevice initialize] explicitly.
		 */
		return self;
	}
	
	/*
	 * Init objectNumber-to-id list.
	 */

	objectListLock = [NXLock new];
	globalObjectCounter = 0;
  	queue_init(&objectList);
	
	/* 
	 * Init class list.
	 */
	queue_init(&classList);
	classListLock = [NXLock new];
	IODeviceInitFlag = YES;
	return self;
}

/*
 * Probe method used by direct, indirect, and pseudo devices. 
 * Implemented by most (not all) subclasses.
 * 
 * Create an instance of subclass to be associated with specified 
 * deviceDescription. Returns a pointer to a nil-terminated array of 
 * id's of instantiated device objects if successful, else returns nil.
 */
+ (BOOL)probe : (id)deviceDescription
{
	return NO;
}

/*
 * Report basic style of driver (direct, indirect, or pseudo). Must be 
 * implemented by subclass.
 */
+ (IODeviceStyle)deviceStyle
{
	return IO_PseudoDevice;		// just a place holder
}

/*
 * Report protocols needed. Kernel-level indirect devices must implement
 * this.
 */
+ (Protocol **)requiredProtocols
{
	return NULL;
}

/*
 * Called from leaf subclass's +initialize method. 
 */
+ (void)registerClass : classId
{
	classListEntry *listEntry;
	
	listEntry = IOMalloc(sizeof(classListEntry));
	listEntry->classId = classId;
	listEntry->bmajor = listEntry->cmajor = -1;
	listEntry->originalDeviceDescription = nil;
	[classListLock lock];
	listEntry->classNumber = classCounter++;
	queue_enter(&classList, listEntry, classListEntry *, link);
	[classListLock unlock];
}

/*
 * Called if/when a class is being removed from an executable's address
 * space.
 */
+ (void)unregisterClass : classId
{
	classListEntry *classEntry;
	
	[classListLock lock];
	classEntry = (classListEntry *)queue_first(&classList);
	while(!queue_end(&classList, (queue_t)classEntry)) {
		if(classEntry->classId == classId) {
			queue_remove(&classList,
				classEntry,
				classListEntry *,
				link);
			break;
		}
		classEntry = (classListEntry *)classEntry->link.next;
	}
	[classListLock unlock];
}

/*
 * Initialize  common instance variables. Typically invoked
 * via [super init:] in subclass's init: method.
 */
- init
{
	xpr_dev("IODevice init:\n", 1,2,3,4,5);
	return [super init];
}

/* 
 * Initialize per specified deviceDescription. Returns nil on error. 
 * This is actually a nop at the IODevice level; subclasses may do with it 
 * as they see fit.
 */
- initFromDeviceDescription : deviceDescription
{
    if( [[self class] deviceStyle] == IO_IndirectDevice)
        __deviceDescription = [deviceDescription directDevice];
    else
        __deviceDescription = deviceDescription;
    return self;
}

- deviceDescription
{
    if( [[self class] deviceStyle] == IO_IndirectDevice)
	return nil;
    else
        return __deviceDescription;
}

- (void)setDeviceDescription : deviceDescription
{
    if( [[self class] deviceStyle] == IO_IndirectDevice)
        __deviceDescription = [deviceDescription directDevice];
    else
        __deviceDescription = deviceDescription;
}

/*
 * Returns a string representation of the hardware's location.
 * Indirect drivers return the direct device's path they were probed on.
 */
- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias
{
    if( [__deviceDescription respondsTo:@selector(getDevicePath:maxLength:useAlias:)])
        return( [__deviceDescription getDevicePath:path maxLength:maxLen useAlias:doAlias]);

    return( nil);
}

/*
 * Returns the tail of the matchPath parameter if the head matches the
 * device's path, else returns nil. Indirect drivers match on the
 * direct device's path they were probed on.
 */
- (char *) matchDevicePath:(char *)matchPath
{
    if( [__deviceDescription respondsTo:@selector(matchDevicePath:)])
        return( [__deviceDescription matchDevicePath:matchPath]);

    return( NULL);
}

- lookUpProperty:(const char *)propertyName
	value:(unsigned char *)value
	length:(unsigned int *)length
	selector:(SEL)selector
	isString:(BOOL)isString
{
    id		rtn = nil, prtn = nil;

    if( __deviceDescription)
        prtn = [__deviceDescription lookUpProperty:propertyName value:value length:length
		selector:(SEL)selector isString:isString];

    if( [self respondsTo:selector]) {
	// guarantee a minimal size so most methods don't have to check
	if( (isString == NO) || ((strlen( value) + 32) <= *length))
            rtn = objc_msgSend( self, selector, value, length );
    }

    return( prtn ? prtn : rtn);
}

- lookUpProperty:(const char *)propertyName
	value:(unsigned char *)value
	length:(unsigned int *)length

{
    char       		selName[ 64 ];
    SEL			selector;
    unsigned int	len = 4;

    *value = 0;
    sprintf( selName, "property_%s:length:", propertyName);
    selector = sel_getUid( selName);

    return( [self lookUpProperty:propertyName value:value length:length
		selector:selector isString:NO]);
}

- lookUpStringProperty:(const char *)propertyName
	value:(char *)value
	length:(unsigned int)maxLength
{
    char       	selName[ 64 ];
    SEL		selector;

    *value = 0;
    sprintf( selName, "property_%s:length:", propertyName);
    selector = sel_getUid( selName);

    return( [self lookUpProperty:propertyName value:value length:&maxLength
		selector:selector isString:YES]);
}

- property_IODriverNames:(char *)classes length:(unsigned int *)maxLen
{
    if( *classes)
	strcat( classes, " ");
    strcat( classes, [self name]);
    return( self);
}

- property_IODevicePath:(char *)path length:(unsigned int *)maxLen
{
    return( [self getDevicePath:path maxLength:*maxLen useAlias:YES]);
}

#ifdef	KERNEL
- getStringPropertyList:(const char *)names
	results:(char *) results
	maxLength:(unsigned int)length
{
    int			index, maxLen, nameLen, nextLen;
    KernStringList *	nameList;
    char *		nextValue;
    const char *       	name;

    nameList = [[KernStringList alloc]
                        initWithWhitespaceDelimitedString:names];
    if( nameList) {
        nextValue = results;
        maxLen = length - (4 + 2);

        for (index = 0; index < [nameList count]; index++) {
            name = [nameList stringAt:index];
            nameLen = strlen( name);
            nextLen = maxLen - nameLen;

            if( [self lookUpStringProperty:name value:(nextValue + nameLen + 4) length:nextLen]) {

                sprintf( nextValue, "\"%s\"=", name);
                nextValue += nameLen + 3;
                *nextValue++ = '\"';
                nextLen = strlen( nextValue) + 2;
                nextValue += nextLen;
                strcpy( nextValue - 2, "\";");
                maxLen -= nextLen + nameLen + 4;
            }
        }
        [nameList free];
    }

    *nextValue = 0;
    return( self);
}

- matchStringPropertyList:(char *)values
{
    enum { 		kValueMaxSize = 256 };
    char *		valueBuf;
    KernStringList *	valueList, * lookList;
    int			look, value;
    id			rtn;
    char *		next, * end;
    BOOL		found, lookingName = YES;

    next = values;
    valueList = lookList = nil;
    valueBuf = IOMalloc( kValueMaxSize);
    if( !valueBuf)
	return( nil);

    while(
            (next = (char *) strchr( next, '\"'))
            &&
            (end = (char *) strchr( ++next, '\"'))	) {

	found = NO;
        *end = 0;		// yikes - modify caller's string

	if( lookingName) {

            rtn = [self lookUpStringProperty:next value:valueBuf length:kValueMaxSize];
            *end = '\"';	// restore caller's string
            if( !rtn)
                break;
	    [valueList free];
            valueList = [[KernStringList alloc]
                            initWithWhitespaceDelimitedString:valueBuf];
            if( !valueList)
                break;

	} else {

	    [lookList free];
            lookList = [[KernStringList alloc]
                            initWithWhitespaceDelimitedString:next];
            *end = '\"';	// restore caller's string
            if( !lookList)
                break;

            found = YES;
	    for( look = 0; found && (look < [lookList count]); look++) {
		found = NO;
                for( value = 0; !found && (value < [valueList count]); value++)
		    found = (0 == strcmp( [lookList stringAt:look], [valueList stringAt:value] ));
	    }
	    if( !found)
		break;
	}

	next = end + 1;
	lookingName ^= YES;
    }
    [valueList free];
    [lookList free];
    IOFree( valueBuf, kValueMaxSize);
    return( found ? self : nil);
}

#else 	/* !KERNEL */

- getStringPropertyList:(const char *)names
	results:(char *) results
	maxLength:(unsigned int)length
{
    return( nil);
}
- matchStringPropertyList:(char *)values
{
    return( nil);
}

#endif 	/* KERNEL */


/*
 * Free up resources used by this device; invoke Object's free. Instance 
 * will be "gone" upon return.
 */
- free
{
	[self unregisterDevice];
	return([super free]);
}

- registerLoudly
{
    return( self);
}

/*
 * Register instance with nmserver (or IOServer or ...). _deviceName and
 * _deviceKind must be valid; _location must be valid or NULL.
 */
- registerDevice
{
	objectListEntry *objectEntry;
	
	xpr_dev("IODevice register\n", 1,2,3,4,5);

	if( [self registerLoudly]) {

            IOLog("Registering: %s", _deviceName);

            if(_location[0])
                    IOLog(" at %s", _location);

            if( (_deviceName[ 0 ] != '/')
                && [__deviceDescription respondsTo:@selector(nodeName)]
		&& [__deviceDescription nodeName] ) {

		    char	slotName[ 32 ];

                    IOLog("   [%s", [__deviceDescription nodeName]);

		    if( [self lookUpStringProperty:IOPropSlotName
			value:slotName length:sizeof( slotName)])
                            IOLog(", slot %s", slotName);
                    IOLog( "]");
	    }
            IOLog( "\n");
	}

	/*
	 * Create a objectNum-to-id map entry for this instance.
	 */
	objectEntry = IOMalloc(sizeof(*objectEntry));
	objectEntry->instance = self;
	[objectListLock lock];
	objectEntry->objectNumber = globalObjectCounter++;
	queue_enter(&objectList,
		objectEntry,
		objectListEntry *,
		link);
	[objectListLock unlock];

	/*
	 * See if any indirect devices are interested in this object's
	 * protocol.
	 */
	[IODevice connectToIndirectDevices:self];
	return self;
}

- (void)unregisterDevice
{
	objectListEntry *objectEntry;
	
	xpr_dev("IODevice unregister %s\n", _deviceName, 2,3,4,5);

	/*
	 * Remove this device's objectNum-to-id map entry. This creates a 
	 * hole in the global objectNumber space which never gets filled 
	 * (in the current implementation).
	 */
	[objectListLock lock];
	objectEntry = getObjectListEntry(self);
	if(objectEntry != NULL) {
                if( [self registerLoudly])
                    IOLog("Unregistering Device: %s\n", _deviceName);
		queue_remove(&objectList, 
			objectEntry, 
			objectListEntry *, 
			link);
		IOFree(objectEntry, sizeof(*objectEntry));
	}
	[objectListLock unlock];
	
}

/*
 * Obtain public instance variables.
 */
- (const char *)name
{
	return((const char *)_deviceName);
}

- (void)setName	: (const char *)name
{
	int len;
	
	len = strlen((char *)name);
	if(len >= IO_STRING_LENGTH)
		len = IO_STRING_LENGTH - 1;
	strncpy(_deviceName, name, len);
	_deviceName[len] = '\0';		
}

- (const char *)deviceKind
{
	return((const char *)_deviceKind);
}

- (void)setDeviceKind	: (const char *)type
{
	int len;
	
	len = strlen((char *)type);
	if(len >= IO_STRING_LENGTH)
		len = IO_STRING_LENGTH - 1;
	strncpy(_deviceKind, type, len);
	_deviceKind[len] = '\0';		
}

- (const char *)location
{
	return((const char *)_location);
}

- (void)setLocation		: (const char *)location
{
	int len;
	
	if(location == NULL) {
		_location[0] = '\0';
	}
	else {
		len = strlen((char *)location);
		if(len >= IO_STRING_LENGTH)
			len = IO_STRING_LENGTH - 1;
		strncpy(_location, location, len);
		_location[len] = '\0';		
	}
}

- (u_int)unit
{
	return(_unit);
}

- (void)setUnit		: (u_int)unit
{
	_unit = unit;
}

+ (int)blockMajor
{
    classListEntry *classEntry;
    if (getClassListEntryForId(self, &classEntry) == IO_R_SUCCESS)
	return classEntry->bmajor;
    else
    	return -1;
}

+ (void)setBlockMajor	: (int)bmajor
{
    classListEntry *classEntry;
    if (getClassListEntryForId(self, &classEntry) == IO_R_SUCCESS)
	classEntry->bmajor = bmajor;
}

+ (int)characterMajor
{
    classListEntry *classEntry;
    if (getClassListEntryForId(self, &classEntry) == IO_R_SUCCESS)
	return classEntry->cmajor;
    else
    	return -1;
}

+ (void)setCharacterMajor : (int)cmajor
{
    classListEntry *classEntry;
    if (getClassListEntryForId(self, &classEntry) == IO_R_SUCCESS)
	classEntry->cmajor = cmajor;
}

#if KERNEL

+ (BOOL)addToCdevswFromDescription: (id) deviceDescription
    open:	(IOSwitchFunc) openFunc
    close:	(IOSwitchFunc) closeFunc
    read:	(IOSwitchFunc) readFunc
    write:	(IOSwitchFunc) writeFunc
    ioctl:	(IOSwitchFunc) ioctlFunc
    stop:	(IOSwitchFunc) stopFunc
    reset:	(IOSwitchFunc) resetFunc
    select:	(IOSwitchFunc) selectFunc
    mmap:	(IOSwitchFunc) mmapFunc
    getc:	(IOSwitchFunc) getcFunc
    putc:	(IOSwitchFunc) putcFunc
{
	const char *majorString;
	int major, result;

	majorString = [[deviceDescription configTable]
			valueForStringKey:"Character Major"];
	if (majorString) {
	    major = _atoi(majorString);
	} else {
	    major = -1;
	}
	result = IOAddToCdevswAt(
	    major,
	    openFunc,
	    closeFunc,
	    readFunc,
	    writeFunc,
	    ioctlFunc,
	    stopFunc,
	    resetFunc,
	    selectFunc,
	    mmapFunc,
	    getcFunc,
	    putcFunc);
	if (result < 0) { 
	    if (major < 0)
		IOLog ("%s: could not add to cdevsw table at any major\n",
		    [self name]);
	    else
		IOLog ("%s: could not add to cdevsw table at major %d\n",
		    [self name], major);
	    return NO;
	}
	[self setCharacterMajor: result];
	return YES;
}

+ (BOOL) addToBdevswFromDescription: (id) deviceDescription
    open:	(IOSwitchFunc) openFunc
    close:	(IOSwitchFunc) closeFunc
    strategy:	(IOSwitchFunc) strategyFunc
    ioctl:	(IOSwitchFunc) ioctlFunc
    dump:	(IOSwitchFunc) dumpFunc
    psize:	(IOSwitchFunc) sizeFunc
    isTape:	(BOOL) isTape
{
	const char *majorString;
	int major, result;

	majorString = [[deviceDescription configTable]
			valueForStringKey:"Block Major"];
	if (majorString) {
	    major = _atoi(majorString);
	} else {
	    major = -1;
	}
	result = IOAddToBdevswAt(
	    major,
	    openFunc,
	    closeFunc,
	    strategyFunc,
	    ioctlFunc,
	    dumpFunc,
	    sizeFunc,
	    isTape);
	if (result < 0) { 
	    if (major < 0)
		IOLog ("%s: could not add to bdevsw table at any major\n",
		    [self name]);
	    else
		IOLog ("%s: could not add to bdevsw table at major %d\n",
		    [self name], major);
	    return NO;
	}
	[self setBlockMajor: result];
	return YES;
}

+ (BOOL) removeFromCdevsw
{
    int major = [self characterMajor];
    if (major == -1)
	return NO;

    IORemoveFromCdevsw(major);
    [self setCharacterMajor: -1];
    return YES;
}

+ (BOOL) removeFromBdevsw
{
    int major = [self blockMajor];
    if (major == -1)
	return NO;
    
    IORemoveFromBdevsw(major);
    [self setBlockMajor: -1];
    return YES;
}

- (IOReturn) serverConnect: (port_t *)   machPort   // out in IOTask space
		  taskPort: (port_t)     taskPort;  // in actually ipc_port_t
{
    *machPort = PORT_NULL;
    return IO_R_UNSUPPORTED;
}

#else /* !KERNEL */

+ (BOOL)addToCdevswFromDescription: (id) deviceDescription
    open:	(IOSwitchFunc) openFunc
    close:	(IOSwitchFunc) closeFunc
    read:	(IOSwitchFunc) readFunc
    write:	(IOSwitchFunc) writeFunc
    ioctl:	(IOSwitchFunc) ioctlFunc
    stop:	(IOSwitchFunc) stopFunc
    reset:	(IOSwitchFunc) resetFunc
    select:	(IOSwitchFunc) selectFunc
    mmap:	(IOSwitchFunc) mmapFunc
    getc:	(IOSwitchFunc) getcFunc
    putc:	(IOSwitchFunc) putcFunc
{
    return FALSE;
}

+ (BOOL) addToBdevswFromDescription: (id) deviceDescription
    open:	(IOSwitchFunc) openFunc
    close:	(IOSwitchFunc) closeFunc
    strategy:	(IOSwitchFunc) strategyFunc
    ioctl:	(IOSwitchFunc) ioctlFunc
    dump:	(IOSwitchFunc) dumpFunc
    psize:	(IOSwitchFunc) sizeFunc
    isTape:	(BOOL) isTape
{
    return FALSE;
}

+ (BOOL) removeFromCdevsw
{
	return FALSE;
}

+ (BOOL) removeFromBdevsw
{
	return FALSE;
}

- (IOReturn) serverConnect: (port_t *)   machPort   // out in IOTask space
		  taskPort: (port_t)     taskPort;  // in actually ipc_port_t
{
    return IO_R_UNSUPPORTED;
}

#endif /* KERNEL */

+ (int) driverKitVersion
{
    return IO_DRIVERKIT_VERSION;
}

#define VERSION_CLASS "Version"
#define VERSION_METHOD "driverKitVersionFor"

+ (int) driverKitVersionForDriverNamed:(char *)driverName
{
    char *nameBuf;
    int size, ret;
    id versionClass;
    SEL versionMethod;
    
    size = strlen(driverName) + strlen(VERSION_CLASS) + 1;
    nameBuf = IOMalloc(size);
    if (nameBuf == NULL)
	return -1;
    strcpy(nameBuf, driverName);
    strcat(nameBuf,VERSION_CLASS);
    versionClass = objc_getClass(nameBuf);
    IOFree(nameBuf, size);
    if (versionClass == nil)
	return -1;
    size = strlen(driverName) + strlen(VERSION_METHOD) + 1;
    nameBuf = IOMalloc(size);
    if (nameBuf == NULL)
	return -1;
    strcpy(nameBuf, VERSION_METHOD);
    strcat(nameBuf, driverName);
    versionMethod = sel_getUid(nameBuf);
    if ([versionClass respondsTo:versionMethod])
	ret = (int)[versionClass perform:versionMethod];
    else
	ret = -1;
    IOFree(nameBuf, size);
    return ret;
}



/*
 * Get/set parameters.
 */
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count	// in/out
{
	if(strcmp(parameterName, IO_UNIT) == 0) {
		parameterArray[0] = _unit;
		*count = 1;
		return IO_R_SUCCESS;
	} else if(strcmp(parameterName, IO_BLOCK_MAJOR) == 0) {
		classListEntry *classEntry;
		if (getClassListEntryForId([self class], &classEntry) ==
			IO_R_SUCCESS) {
		    parameterArray[0] = classEntry->bmajor;
		    *count = 1;
		    return IO_R_SUCCESS;
		} else
		    return IO_R_UNSUPPORTED;
	} else if(strcmp(parameterName, IO_CHARACTER_MAJOR) == 0) {
		classListEntry *classEntry;
		if (getClassListEntryForId([self class], &classEntry) ==
			IO_R_SUCCESS) {
		    parameterArray[0] = classEntry->cmajor;
		    *count = 1;
		    return IO_R_SUCCESS;
		} else
		    return IO_R_UNSUPPORTED;
	}
	return IO_R_UNSUPPORTED;
}

- (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count	// in/out
{
	const char *name;
	unsigned length;
	int maxCount = *count;
	
	if(maxCount == 0) {
		maxCount = IO_MAX_PARAMETER_ARRAY_LENGTH;
	}
	if(strcmp(parameterName, IO_CLASS_NAME) == 0) {
		name = [[self class] name];
		goto name_out;
	}
	else if(strcmp(parameterName, IO_DEVICE_NAME) == 0) {
		name = _deviceName;
		goto name_out;
	}
	else if(strcmp(parameterName, IO_DEVICE_KIND) == 0){
		name = _deviceKind;
name_out:
		length = strlen(name);
		if(length >= maxCount) {
			length = maxCount - 1;
		}
		*count = length + 1;
		strncpy(parameterArray, name, length);
		parameterArray[length] = '\0';
		return IO_R_SUCCESS;
	}
	else {
		return IO_R_UNSUPPORTED;
	}
}
		
- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count
{
	return IO_R_UNSUPPORTED;
}

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count
{
	return IO_R_UNSUPPORTED;
}

/*
 * Convert an IOReturn to text. Subclasses which add additional
 * IOReturn's should override this method and call 
 * [super stringFromReturn] if the desired value is not found.
 */
- (const char *)stringFromReturn : (IOReturn)rtn
{
	return [IODevice stringFromReturn:rtn];
}

+ (const char *)stringFromReturn : (IOReturn)rtn
{
	return IOFindNameForValue(rtn, IOReturn_values);
}

/*
 * Convert an IOReturn to an errno.
 */
- (int)errnoFromReturn : (IOReturn)rtn
{
	switch(rtn) {
	    case IO_R_SUCCESS:
	    	return(0);
	    case IO_R_NO_MEMORY:
	    case IO_R_RESOURCE:
	    	return(ENOMEM);
	    case IO_R_NO_DEVICE:
	    	return(ENXIO);
	    case IO_R_PRIVILEGE:
	    case IO_R_LOCKED_READ:
	    case IO_R_LOCKED_WRITE:
	    case IO_R_EXCLUSIVE_ACCESS:
	    case IO_R_CANT_LOCK:
	    case IO_R_NOT_READABLE:
	    case IO_R_NOT_OPEN:
	    	return(EACCES);
	    case IO_R_INVALID_ARG:
	    	return(EINVAL);
	    case IO_R_UNSUPPORTED:
		return(EOPNOTSUPP);
	    case IO_R_IO:
	    case IO_R_MEDIA:
	    case IO_R_DMA:
	    	return(EIO);
	    case IO_R_NOT_WRITABLE:
	    	return(EROFS);
	    case IO_R_OPEN:
	    case IO_R_BUSY:
	    	return(EBUSY);
	    default:
	    	return(EIO);
	}
}

@end	/* IODevice */

@implementation IODevice(Internal)

@end	/* IODevice(Internal) */

/*
 * Get/set parameter RPC support. In the kernel, these methods are mainly 
 * called by the driverServer (although they cal be used by anyone); they 
 * map an objectNumber to an id and invoke the appropriate methods. Usage in
 * user space TBD.
 */
 
@implementation IODevice(GlobalParameter)

#if KERNEL
+ (IOReturn) serverConnect: (port_t *)       machPort   // out - IOTask space
	      objectNumber: (IOObjectNumber) objectNumber
		  taskPort: (port_t)         taskPort;  // in  - ipc_port_t
{
    IODevice *instance;
    IOReturn rtn = IO_R_SUCCESS;

    [objectListLock lock];
    rtn = objectNumToId(objectNumber, (id) &instance);
    [objectListLock unlock];

    if (rtn == IO_R_SUCCESS)
	rtn = [instance serverConnect: machPort taskPort: taskPort];

    return rtn;
}
#endif /* KERNEL */

/*
 * Lookup by IOObjectNumber.
 */
+ (IOReturn)lookupByObjectNumber	 : (IOObjectNumber)objectNumber
			      deviceKind : (IOString *)deviceKind
			      deviceName : (IOString *)deviceName;
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, &instance);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		strncpy(*deviceKind, [instance deviceKind], IO_STRING_LENGTH);
		strncpy(*deviceName, [instance name], IO_STRING_LENGTH);
	}
	return rtn;
}

+ (IOReturn)lookupByObjectNumber	: (IOObjectNumber)objectNumber
			       instance : (id *)instance
{
	IOReturn rtn;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, instance);
	[objectListLock unlock];
	return rtn;
}

/*
 * Lookup by deviceName.
 */
+ (IOReturn)lookupByDeviceName		: (IOString)deviceName
			   objectNumber : (IOObjectNumber *)objectNumber
			     deviceKind : (IOString *)deviceKind;
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = deviceNameToId(deviceName, &instance, objectNumber);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		strncpy(*deviceKind, [instance deviceKind], IO_STRING_LENGTH);
	}
	return rtn;
}

+ (IOReturn) lookUpByStringPropertyList:(char *)values
	results:(char *)results
	maxLength:(unsigned int)length
{
    IOReturn		err;
    int			objectNum = 0;
    id			dev;
    const char *	newName;

    *results = 0;
    [objectListLock lock];
    do {
        err = objectNumToId(objectNum++, &dev);
        if( err != IO_R_SUCCESS)
            continue;

        if( [dev matchStringPropertyList:values]) {

	    newName = [dev name];
	    if( (1 + 1 + strlen( newName)) < length) {		// space & zero chars
		if( results[ 0 ]) {
                    strcat( results, " ");
		    length--;
		}
                strcat( results, newName );
		length -= strlen( newName);
	    } else
		err = IO_R_RESOURCE;
	}
    } while( (err == IO_R_SUCCESS) || (err == IO_R_OFFLINE));

    [objectListLock unlock];

    if( err == IO_R_NO_DEVICE)
	err = IO_R_SUCCESS;

    return( err);
}

+ (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count	// in/out
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, &instance);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		rtn = [instance getIntValues : parameterArray
			forParameter : parameterName
			count : count];
	}
	return rtn;

}

+ (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count	// in/out
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, &instance);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		rtn = [instance getCharValues : parameterArray
			forParameter : parameterName
			count : count];
	}
	return rtn;

}

+ (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, &instance);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		rtn = [instance setIntValues : parameterArray
			forParameter : parameterName
			count : count];
	}
	return rtn;

}


+ (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, &instance);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		rtn = [instance setCharValues : parameterArray
			forParameter : parameterName
			count : count];
	}
	return rtn;

}

+ (IOReturn) getStringPropertyList     	: (IOObjectNumber)objectNumber
			   names        : (const char *)names
			   results      : (char *)results
			   maxLength    : (unsigned int)length
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, &instance);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		instance = [instance getStringPropertyList : names
			results : results
			maxLength : length];
		if( !instance)
		    rtn = IO_R_UNSUPPORTED;
	}
	return rtn;
}

+ (IOReturn) getByteProperty     	: (IOObjectNumber)objectNumber
			   name         : (const char *)name
			   results      : (char *)results
			   maxLength    : (unsigned int *)length
{
	id instance;
	IOReturn rtn = IO_R_SUCCESS;
	
	[objectListLock lock];
	rtn = objectNumToId(objectNumber, &instance);
	[objectListLock unlock];
	if(rtn == IO_R_SUCCESS)  {
		instance = [instance lookUpProperty : name
			value : results
			length : length];
		if( !instance)
		    rtn = IO_R_UNSUPPORTED;
	}
	return rtn;
}

+ (IOReturn)callDeviceMethod		: (IOString)methodName
		    inputParams		: (unsigned char *)inputParams
		    inputCount		: (unsigned)inputCount
		    outputParams	: (unsigned char *)outputParams
		    outputCount		: (unsigned *)outputCount	// in/out
		    privileged		: (host_priv_t *)privileged
		    objectNumber	: (IOObjectNumber)objectNumber

{
    id 		instance;
    IOReturn 	rtn = IO_R_SUCCESS;
    SEL		selector;
    
    [objectListLock lock];
    rtn = objectNumToId(objectNumber, &instance);
    [objectListLock unlock];
    if(rtn == IO_R_SUCCESS)  {

	selector = sel_getUid( methodName );
	if( NO == [instance respondsTo:selector])
	    rtn = IO_R_UNSUPPORTED;

	else if( *outputCount) {
	    if( inputCount) {
		rtn = (IOReturn) objc_msgSend( instance, selector,
			inputParams, inputCount, outputParams, outputCount,
			privileged );
	    } else {
		rtn = (IOReturn) objc_msgSend( instance, selector,
			outputParams, outputCount,
			privileged );
	    }
	} else {
	    if( inputCount) {
		rtn = (IOReturn) objc_msgSend( instance, selector,
			inputParams, inputCount,
			privileged );
	    } else {
		rtn = (IOReturn) objc_msgSend( instance, selector,
			privileged );
	    }
	}
    }
    return rtn;
}


@end	/* IODevice(GlobalParameter) */

#ifdef	KERNEL

/*
 * We leave this enabled for user level programs for now; it may be useful...
 */
@implementation IODevice(kernelPrivate)

/*
 * A new class has been loaded into the kernel. Connect the new class
 * with all existing classes, probing as appropriate.
 * 
 * All instance variables in deviceDescription must be valid or 
 * null. For newly loaded indirect devices, this method will determine
 * appropriate directDevice values. 
 *
 * Returns IO_R_NO_DEVICE if no devices were instantiated, else IO_R_SUCCESS.
 */
+ (IOReturn)addLoadedClass : newClass
	       description : deviceDescription
{
	IOReturn 	rtn = 0;
	Protocol 	**protos;
	IOObjectNumber 	objectNum = 0;
	id 		dirDevice;
	BOOL 		objsCreated = NO;
	int 		i;
	classListEntry *classEntry;
	
	if (getClassListEntryForId(newClass, &classEntry) == IO_R_SUCCESS)
	    classEntry->originalDeviceDescription = deviceDescription;

	switch([newClass deviceStyle]) {
	    case IO_IndirectDevice:
	    	/* 
		 * First probe this class once for all other running objects
		 * which export the protocol(s) required by this indirect
		 * device. 
		 * Note all deviceStyles can export a protocol, not just direct 
		 * devices.
		 */
	    	protos = [newClass requiredProtocols];
		if((protos == NULL) || (*protos == nil)) {
			/*
			 * This indirect device does not require any
			 * protocols. Huh???
			 */
			IOLog("Loaded class %s returns nil for "
				"+requiredProtocols\n",
				[newClass name]);
			break;
		}
		
		/*
		 * Cycle thru all known devices.
		 */
		for(objectNum=0; rtn!=IO_R_NO_DEVICE; objectNum++) {
		    rtn = objectNumToId(objectNum, &dirDevice);
		    switch(rtn) {
			case IO_R_OFFLINE:
			    /*
			     * No device here, but more to go.
			     */
			    continue;
			    
			case IO_R_NO_DEVICE: 
			default:
			    /*
			     * No more.
			     */
			    break;
			
			case IO_R_SUCCESS:
			    /*
			     * See if this thing supports all the 
			     * protocols newClass needs.
			     */
			    for(i=0; protos[i]; i++) {
			         if(![dirDevice conformsTo:protos[i]]) {
				     goto next_object; /* to next objectNum */
				 }
			    }
			    
			    /*
			     * Looks good. Cook up an IODeviceDescription
			     * we can pass to newClass.
			     */
			    [deviceDescription setDirectDevice:dirDevice];
			    if([newClass probe:deviceDescription]) {
			    	objsCreated = YES;
			    }
			    break;
		    }   /* switch rtn */
next_object:
		    continue;
		}	/* for each objectNum */
		
		break;	/* from case IO_STYLE_INDIRECT */
    
	    case IO_DirectDevice:
	    case IO_PseudoDevice:
		if (![newClass respondsTo:@selector(probe:)]) {
			IOLog("addLoadedClass: Class %s does not "
				"respond to probe:\n",
				[newClass name]);
			break;
		}
		if([newClass probe:deviceDescription]) {
		    objsCreated = YES;
		}
		break;
	    
	} /* switch deviceStyle */
	
	if(objsCreated) {
		return IO_R_SUCCESS;
	}
	else {
		return IO_R_NO_DEVICE;
	}
}    	

/*
 * Probe for any indirect device classes in the system which are 
 * interested in connecting to a newly instantiated device of any kind.
 * Used only by -registerDevice (so far).
 */
+(void)connectToIndirectDevices : newObject
{
	unsigned 	classNumber = 0;
	Protocol 	**protos;
	IOReturn 	rtn;
	classListEntry	*classEntry;
	int 		i;
	id		deviceDescription;
	
	/*
	 * We hold classListLock throughout this routine, except for when
	 * we actually probe an indirect device class.
	 */
	[classListLock lock];
	for(classNumber=0; ;classNumber++) {
		/*
		 * Check out each indirect driver class in classList.
		 */
		rtn = getClassListEntry(classNumber, &classEntry);
		switch(rtn) {
		    case IO_R_SUCCESS:
		    	break;			// proceed
		    case IO_R_NO_DEVICE:	
		    	goto done;		// normal termination
		    case IO_R_OFFLINE:
		    	goto next_class;	// hole in classList space
		}
		if([classEntry->classId deviceStyle] != IO_IndirectDevice) {
			goto next_class;
		}
		
		/*
		 * We have an indirect device class. See if the protocols
		 * it requires are supported by the new object.
		 */
		protos = [classEntry->classId requiredProtocols];
		if((protos == NULL) || (*protos == nil)) {
			/*
			 * This indirect device does not require any
			 * protocols. Huh???
			 */
			IOLog("Loaded class %s returns nil for "
				"+requiredProtocols\n",
				[classEntry->classId name]);
			goto next_class;
		}
		for(i=0; protos[i]; i++) {
			if(![newObject conformsTo:protos[i]]) {
			    	goto next_class;
			}
		}
		
		/*
		 * Looks good. Cook up an IODeviceDescription we can pass 
		 * to the indirect driver class and probe away.
		 */
		if (classEntry->originalDeviceDescription != nil)
		    deviceDescription = classEntry->originalDeviceDescription;
		else
		    deviceDescription = [[IODeviceDescription alloc] init];
		[deviceDescription setDirectDevice:newObject];
		[classListLock unlock];
		if ([classEntry->classId probe:deviceDescription] == NO)
		    [deviceDescription free];
		[classListLock lock];

next_class:
		continue;
	}
done:
 	[classListLock unlock];
	return;
}

+ (List *)objectsForClass:(Class *)driverClass
{
	id list;
	objectListEntry *objectEntry = 
		(objectListEntry *)queue_first(&objectList);
	
	list = [[List alloc] init];
	[objectListLock lock];
	while(!queue_end(&objectList, (queue_t)objectEntry)) {
		if((Class *)[objectEntry->instance class] == driverClass) {
			[list addObject:objectEntry->instance];
		}
		objectEntry = (objectListEntry *)objectEntry->link.next;
	}
	[objectListLock unlock];
	return list;
}

#if 0
+ (HashTable *)classTableFromConfigTable : configTable
{
    char *classNameList, *className, *ptr, *str;
    int len;
    HashTable *hash;
    
    classNameList = (char *)[configTable valueForStringKey:"Class Names"];
    if (classNameList == NULL)
	classNameList = (char *)[configTable valueForStringKey:"Driver Name"];
    if (classNameList == NULL)
	return nil;

    len = strlen(classNameList);
    hash = [[HashTable alloc] initKeyDesc:"*"];
    className = classNameList;
    while (*className) {
	ptr = className;
	while (*ptr && *ptr != ' ' && *ptr != '\t')
	    ptr++;
	len = ptr - className + 1;
	str = IOMalloc(len);
	strncpy(str, className, len - 1);
	str[len-1] = '\0';
	[hash insertKey:str value:objc_getClass(str)];
	while (*ptr && (*ptr == ' ' || *ptr == '\t'))
	    ptr++;
	className = ptr;
    }
    [configTable freeString:classNameList];

    return hash;
}
#endif

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    return( nil);
}

- property_IODeviceType:(char *)classes length:(unsigned int *)maxLen
{
    return( nil);
}

@end	/* IODevice(kernelPrivate) */

/*
 * Lookup by deviceName, kernel internal version. Returns id of specified
 * instance in deviceId. 
 */
IOReturn IOGetObjectForDeviceName(
	IOString deviceName,
	id *deviceId)				// returned
{
	IOReturn rtn;
	IOObjectNumber objectNumber;
	
	[objectListLock lock];
	rtn = deviceNameToId(deviceName, deviceId, &objectNumber);
	[objectListLock unlock];
	return rtn;
}

#endif	KERNEL


/*
 * Static functions.
 */
 
/*
 * Get id of specified global objectNumber. objectListLock must be held on 
 * entry.
 * Returns IO_R_OFFLINE if a hole is found in IOObjectNumber space at 
 * specified objectNumber. Returns IO_R_NO_DEVICE if objectNumber is out
 * of range.
 */
static IOReturn objectNumToId(IOObjectNumber objectNumber, id *instance)
{
	objectListEntry *objectEntry = 
		(objectListEntry *)queue_first(&objectList);
	
	if(objectNumber >= globalObjectCounter)
		return IO_R_NO_DEVICE;
	while(!queue_end(&objectList, (queue_t)objectEntry)) {
		if(objectEntry->objectNumber == objectNumber) {
			*instance = objectEntry->instance;
			return IO_R_SUCCESS;
		}
		else
			objectEntry = 
				(objectListEntry *)objectEntry->link.next;
	}
	
	/*
	 * Not found. This implies a "hole" in the objectNumber space.
	 */
	return IO_R_OFFLINE;
} 

/*
 * Get id of device with specified name. objectListLock must be held on entry.
 */
static IOReturn deviceNameToId(IOString name, 
	id *instance,
	IOObjectNumber *objectNumber)
{
	objectListEntry *objectEntry = 
		(objectListEntry *)queue_first(&objectList);
	
	while(!queue_end(&objectList, (queue_t)objectEntry)) {
		if(strncmp(name, [objectEntry->instance name],
		    IO_STRING_LENGTH) == 0) {
			*instance     = objectEntry->instance;
			*objectNumber = objectEntry->objectNumber;
			return IO_R_SUCCESS;
		}
		else {
			objectEntry = 
				(objectListEntry *)objectEntry->link.next;
		}
	}
	
	/*
	 * Not found. 
	 */
	return IO_R_NO_DEVICE;
} 

/*
 * Get an instance's objectNum-to-id map entry. objectListLock must be held
 * on entry.
 */
static objectListEntry *getObjectListEntry(id object)
{
	objectListEntry *objectEntry;
	
	objectEntry = (objectListEntry *)queue_first(&objectList);
	while(!queue_end(&objectList, (queue_t)objectEntry)) {
		if(objectEntry->instance == object) {
			goto out;
		}
		else {
			objectEntry = 
				(objectListEntry *)objectEntry->link.next;
		}
	}
	objectEntry = NULL;
out:
	return objectEntry;
}

/*
 * Get a classListEntry for specified class id.
 */
static IOReturn getClassListEntryForId(
    id classId,
    classListEntry **entry
)
{
    classListEntry *classEntry;
    
    [classListLock lock];
    classEntry = (classListEntry *)queue_first(&classList);
    while(!queue_end(&classList, (queue_t)classEntry)) {
	if(classEntry->classId == classId) {
	    *entry = classEntry;
	    [classListLock unlock];
	    return IO_R_SUCCESS;
	}
	classEntry = (classListEntry *)classEntry->link.next;
    }
    [classListLock unlock];
    return IO_R_OFFLINE;
}

/*
 * Get a classListEntry for specified classNumber. classListLock must be held
 * on entry. 
 * Returns IO_R_OFFLINE if a hole is found in classNumber space at 
 * specified classNumber. Returns IO_R_NO_DEVICE if classNumber is out
 * of range.
 */
static IOReturn getClassListEntry(unsigned classNumber,
	classListEntry  **entry)
{
	classListEntry *classEntry;
	
	if(classNumber > classCounter) {
		return IO_R_NO_DEVICE;
	}
	classEntry = (classListEntry *)queue_first(&classList);
	while(!queue_end(&classList, (queue_t)classEntry)) {
		if(classEntry->classNumber == classNumber) {
			*entry = classEntry;
			return IO_R_SUCCESS;
		}
		classEntry = (classListEntry *)classEntry->link.next;
	}
	return IO_R_OFFLINE;
}

/* end of IODevice.m */
