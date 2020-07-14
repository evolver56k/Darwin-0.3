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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * Kernel Device Description Object.
 *
 * HISTORY
 *
 * 24 Feb 1994 ? at NeXT
 *	Major rewrite.
 * 17 Jan 1994 ? at NeXT
 *	Created.
 */

#import <objc/List.h>
#import <objc/HashTable.h>
#import <driverkit/KernBus.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/IOConfigTable.h>

#define IRQ_LEVELS_KEY		"IRQ Levels"	// XXX
#define BUS_TYPE_KEY		"Bus Type"	// XXX
#define BUS_ID_KEY		"Bus ID"	// XXX

static
boolean_t
parsenum(const char *nptr, char **endptr, unsigned long *result);

@interface KernDeviceDescription (Parsing)
- (BOOL)_isShared:(const char *)key;
- _parseRangeResourceByKey :(const char *)key value:(const char *)value;
- _parseItemResourceByKey :(const char *)key value:(const char *)value;
@end

static const char *
newString(const char *oldString)
{
    return strcpy((char *)IOMalloc(strlen(oldString)+1), oldString);
}

static void
nullFunc(void *null)
{
}

static void
freeString(void *string)
{
    IOFree(string, strlen(string)+1);
}

static void
freeResources(void *_resource)
{
    id resource = (id)_resource;
    [resource freeObjects];
    [resource free];
}

static BOOL isInterrupt(const char *key)
{
    if (strcmp(key, IRQ_LEVELS_KEY) == 0)	// XXX
        return YES;
    return NO;
}

@implementation KernDeviceDescription

- initFromConfigTable:	configTable
{
    const char *busName;
    const char *str;
    unsigned long num;
    
    [super init];
    
    _configTable = configTable;
    _device = nil;
    _stringTable = [[HashTable alloc] initKeyDesc:"*" valueDesc:"*"];
    _resourceTable = [[HashTable alloc] initKeyDesc:"*" valueDesc:"@"];
    _interruptList = [[List alloc] init];
    busName = [_configTable valueForStringKey:BUS_TYPE_KEY];
    _busClass = [KernBus lookupBusClassWithName:busName];
    str = [_configTable valueForStringKey:BUS_ID_KEY];
    if (str && parsenum(str, 0, &num))
	_busId = num;
    _bus = [KernBus lookupBusInstanceWithName:busName busId:_busId];
    if (str) [_configTable freeString:str];
    if (busName) [_configTable freeString:busName];
    return self;
}

- init
{
    return [self initFromConfigTable:nil];
}

- free
{
    [_stringTable freeKeys:nullFunc values:freeString];
    [_stringTable free];
    [_resourceTable freeKeys:nullFunc values:freeResources];
    [_resourceTable free];
    [_interruptList free];
    return [super free];
}

- configTable
{
    return _configTable;
}

- setDevice:		device
{
    if (_device == nil || device == nil)
    	_device = device;
	
    if (_device != device)
    	return nil;
	
    return device;
}

- device
{
    return _device;
}

- busClass
{
    return _busClass;
}

- setBus: bus
{
    _bus = bus;
    return bus;
}

- bus
{
    return _bus;
}

- (const char *)stringForKey:(const char *)aKey
{
    const char *value;
    
    value = [_stringTable valueForKey:aKey];
    if (value == NULL) {
	value = [_configTable valueForStringKey:aKey];
	if (value != NULL)
	    [_stringTable insertKey:aKey value:(char *)value];
    }
    return value;
}

- resourcesForKey:(const char *)aKey
{
    return [_resourceTable valueForKey:aKey];
}

- setString:(const char *)aString forKey:(const char *)aKey
{
    char *old, *new;
    
    new = (char *)newString(aString);
    old = (char *)[_stringTable insertKey:aKey value:new];

    if (old)
	   freeString(old);

    return self;
}

- setResources:resources forKey:(const char *)aKey
{
    id old;
    
    if (isInterrupt(aKey)) {
	[_interruptList empty];
	[_interruptList appendList:resources];
    }
    
    old = [_resourceTable insertKey:aKey value:resources];
    if (old && old != resources)
	freeResources(old);
    return self;
}

- (BOOL)removeStringForKey:(const char *)aKey
{
    char *str;
    
    str = (char *)[_stringTable removeKey:aKey];
    if (str) {
	freeString(str);
	return YES;
    }
    return NO;
}

- (BOOL)removeResourcesForKey:(const char *)aKey
{
    id old;
    
    old = [_resourceTable removeKey:aKey];
    if (old != nil) {
	if (isInterrupt(aKey)) {
	    int i;
	    for (i=0; i < [old count]; i++)
		[_interruptList removeObject:[old objectAt:i]];
	}
	freeResources(old);
    }
    return NO;
}

- interrupts
{
    return _interruptList;
}

- (NXHashState)initStringState
{
    return [_stringTable initState];
}

- (BOOL)nextStringState:(NXHashState *)aState key:(const char **)aKey 
	value:(char **)aValue
{
    return [_stringTable nextState:aState key:(void **)aKey
					  value:(void **)aValue];
}

- (NXHashState)initResourcesState
{
    return [_resourceTable initState];
}

- (BOOL)nextResourcesState:(NXHashState *)aState key:(const char **)aKey 
	value:(id *)aValue
{
    return [_resourceTable nextState:aState key:(void **)aKey
					    value:(void **)aValue];
}

- allocateResourcesForKey: (const char *)key
{
    const char		*keyValue;
    id			resourceList;
    
    keyValue = [self stringForKey:key];
    /*
     * No resources of this type are needed --
     * this is not an error.
     */
    if (keyValue == NULL)
	return self;
	
    if (strchr(keyValue, '-'))
	resourceList = [self _parseRangeResourceByKey:key value:keyValue];
    else
    	resourceList = [self _parseItemResourceByKey:key value:keyValue];

    if (resourceList == nil) {
	/*
	 * A resource was requested and was not available.
	 * This is an error.
	 */
	return nil;
    }
    
    [self setResources:resourceList forKey:key];
    return self;
}


static id
elementWithItem(
    id list,
    int item
)
{
    int i;
    id object;
    
    for (i=0; i<[list count]; i++) {
	if ([(object = [list objectAt:i]) item] == item)
	    return object;
    }
    return nil;
}

- allocateItems:(unsigned int *)aList numItems:(unsigned int)length
    forKey:(const char *)keyName
{
    id currentList, newList, reservedList;
    int i;
    BOOL		share;

    share = [self _isShared:keyName];
    currentList = [self resourcesForKey:keyName];
    /* the list that will become the new list of resources */
    newList = [[List alloc] init];
    /* the list of resources that are newly reserved */
    reservedList = [[List alloc] init];
    
    for (i=0; i < length; i++) {
	id resource, vendor;
	
	if ((resource = elementWithItem(currentList, aList[i]))) {
	    [newList addObject:resource];
	} else {
	    /* Must reserve new item */
	    vendor = [_bus _lookupResourceWithKey:keyName];
	    if (vendor == nil)
		goto fail;
	    if ((resource = (share ?
		    [vendor shareItem:aList[i]]:
		    [vendor reserveItem:aList[i]])) == nil)
		goto fail;
	    [reservedList addObject:resource];
	    [newList addObject:resource];
	}
    }
    /* Now check for items that should be freed */
    for (i=0; i < [currentList count]; i++) {
	id object;
	
	object = [currentList objectAt:i];
	if ([newList indexOf:object] == NX_NOT_IN_LIST)
	    [object free];
    }

    /* Now, empty old list, to prevent double freeing
     * of its objects.  The list itself will be freed
     * when we do a setResources.
     */
    [currentList empty];

    [self setResources:newList forKey:keyName];
    [reservedList free];
    return self;
    
fail:
    [[reservedList freeObjects] free];
    [newList free];
    return nil;
}


static id
elementWithRange(
    id list,
    Range range
)
{
    int i;
    id object;
    Range _range;
    
    for (i=0; i<[list count]; i++) {
	object = [list objectAt:i];
	_range = [object range];
	if (_range.base == range.base && _range.length == range.length)
	    return object;
    }
    return nil;
}

- allocateRanges:(Range *)aList numRanges:(unsigned int)length
    forKey:(const char *)keyName
{
    id currentList, newList, reservedList;
    int i;
    BOOL		share;

    share = [self _isShared:keyName];
    currentList = [self resourcesForKey:keyName];
    /* the list that will become the new list of resources */
    newList = [[List alloc] init];
    /* the list of resources that are newly reserved */
    reservedList = [[List alloc] init];
    
    for (i=0; i < length; i++) {
	id resource, vendor;
	Range range;
	
	range = aList[i];
	if ((resource = elementWithRange(currentList, range))) {
	    [newList addObject:resource];
	} else {
	    /* Must reserve new range */
	    vendor = [_bus _lookupResourceWithKey:keyName];
	    if (vendor == nil)
		goto fail;
	    if ((resource = (share ?
		    [vendor shareRange:range] :
		    [vendor reserveRange:range])) == nil)
		goto fail;
	    [reservedList addObject:resource];
	    [newList addObject:resource];
	}
    }
    /* Now check for items that should be freed */
    for (i=0; i < [currentList count]; i++) {
	id object;
	
	object = [currentList objectAt:i];
	if ([newList indexOf:object] == NX_NOT_IN_LIST)
	    [object free];
    }
    /* Now, empty old list, to prevent double freeing
     * of its objects.  The list itself will be freed
     * when we do a setResources.
     */
    [currentList empty];

    [self setResources:newList forKey:keyName];
    [reservedList free];
    return self;
    
fail:
    [[reservedList freeObjects] free];
    [newList free];
    return nil;
}
@end

#include <limits.h>

static inline BOOL
isupper(char c)
{
    return (c >= 'A' && c <= 'Z');
}

static inline BOOL
isalpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}


static inline BOOL
isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

static inline BOOL
isdigit(char c)
{
    return (c >= '0' && c <= '9');
}

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
static
boolean_t
parsenum(nptr, endptr, result)
	const char *nptr;
	char **endptr;
	unsigned long *result;
{
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;
	int base = 0;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if (c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0)
		acc = ULONG_MAX;
	else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = any ? s - 1 : (char *)nptr;
	if (result != 0)
		*result = acc;
	return ((any > 0)? TRUE: FALSE);
}

@implementation KernDeviceDescription(Parsing)


- (BOOL)_isShared:(const char *)key
{
    char		buf[128];
    const char 		*shareKey;
    BOOL		result;

    sprintf(buf, "Share %s", key);
    if ((shareKey = [_configTable valueForStringKey:buf])) {
        result = ((*shareKey == 'y') || (*shareKey == 'Y'));
	[_configTable freeString:shareKey];
	return( result);
    } else
	return NO;
}

- _parseRangeResourceByKey: (const char *)key value:(const char *)keyValue
{
    id			resource, list;
    const char		*p;
    unsigned long	begin, end;
    BOOL		share;
    
    resource = [_bus _lookupResourceWithKey:key];
    if (resource == nil) {
    	printf("%s: Couldn't locate resource object\n", key);
    	return nil;
    }
	
    list = [[List alloc] init];

    p = keyValue;
    if (keyValue == (void *)nil)
    	return list;

    share = [self _isShared:key];

    while (parsenum(p, &p, &begin) &&
		    parsenum(++p, &p, &end)) {
	Range	range = { begin, end - begin + 1 };

	id		rangeResource;

	rangeResource = share ? [resource shareRange:range] :
				[resource reserveRange:range];	
	if ([list addObject:rangeResource] == nil) {
	    printf("%s: Couldn't reserve range %08x-%08x\n", key, begin, end);	    
	    return [[list freeObjects] free];
	}
    }
    
    return list;
}

- _parseItemResourceByKey: (const char *)key value:(const char *)keyValue
{
    id			resource, list;
    const char		*p;
    unsigned long	item;
    BOOL		share;
    
    resource = [_bus _lookupResourceWithKey:key];
    if (resource == nil) {
    	printf("%s: Couldn't locate resource object\n", key);
    	return nil;
    }
	
    list = [[List alloc] init];

    p = keyValue;
    if (keyValue == (void *)nil)
    	return list;
	
    share = [self _isShared:key];
    while (parsenum(p, &p, &item)) {	
	id		itemResource;
	
	itemResource = share ? [resource shareItem:item] :
			       [resource reserveItem:item];
	if ([list addObject:itemResource] == nil) {
	    printf("%s: Couldn't reserve %d\n",	key, item);
	    return [[list freeObjects] free];
	}
    }
    
    return list;
}

@end
