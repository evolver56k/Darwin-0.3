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
 * Copyright (c) 1993, 1994 NeXT Computer, Inc.
 *
 * Kernel Bus Resource Object(s).
 *
 * HISTORY
 *
 * 1 Jul 1994 ? at NeXT
 *	Modifications to support shared interrupts.
 * 16 Mar 1994 ? at NeXT
 *	Added range mapping.
 * 24 Feb 1994 ? at NeXT
 *	Major rewrite.
 * 31 Oct 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <objc/HashTable.h>

#import <driverkit/KernBus.h>
#import <driverkit/KernBusPrivate.h>
#import <driverkit/KernDeviceDescription.h>

@implementation KernBusItemResource

- initWithItemCount:	(unsigned int)count
	    itemBase:	(unsigned int)base
	    itemKind:	kind
	    owner:	owner
{
    [super init];

    /*
     * Check for base wrap
     * and invalid count.
     */
    if (count > 1024 || (base + count) <= base)
    	return [self free];

    _owner = owner;
    _kind = (kind == nil)? [KernBusItem class]: kind;
    _items = (void *)IOMalloc(count * sizeof (KernBusItem *));
    _itemCount = 0;
    bzero(_items, count * sizeof (KernBusItem *));
    _count = count; _base = base;

    return self;
}

- initWithItemCount:	(unsigned int)count
	    itemKind:	kind
	    owner:	owner
{
    return [self initWithItemCount:count itemBase:0 itemKind:kind owner:owner];
}

- init
{
    return [self free];
}

- free
{
    if (_itemCount > 0)
    	return self;

    if (_count > 0 && _items)
    	IOFree(_items, sizeof (_count * sizeof (KernBusItem *)));

    return [super free];
}

- reserveItem:	(unsigned int)item
{
    KernBusItem		**items = _items;
    id			*object;

    /*
     * Check for invalid
     * item.
     */ 
    if (item < _base || item >= (_base + _count))
    	return nil;
	
    object = &items[item - _base];

    if (*object == nil) {
    	if ((*object = [[_kind alloc]
				initForResource:self
					    item:item
					shareable:NO]) != nil) {
	    if (_itemCount++ == 0)
	    	[_owner _resourceActive];
	}
    }
    else
    	return nil;
    
    return *object;
}

- shareItem:	(unsigned int)item
{
    KernBusItem		**items = _items;
    id			*object;

    /*
     * Check for invalid
     * item.
     */ 
    if (item < _base || item >= (_base + _count))
    	return nil;
	
    object = &items[item - _base];
	
    if (*object == nil)	{
    	if ((*object = [[_kind alloc]
				initForResource:self
					    item:item
					shareable:YES]) != nil) {
	    if (_itemCount++ == 0)
	    	[_owner _resourceActive];
	}
    }
    else
    	return [*object share];
	
    return *object;
}

#ifdef	notdef
- reserveAnyItem
{
    id			*object = _items;
    int			i;
    
    if (_itemCount >= _count)
    	return nil;

    for (i = 0; i < _count; i++, object++) {
    	if (*object == nil) {
	    if ((*object = [[_kind alloc]
				    initForResource:self
				    		item:i
					    shareable:NO]) != nil) {
		if (_itemCount++ == 0)
		    [_owner _resourceActive];

		return *object;
	    }
	}
    }
	
    return nil;
}

- shareAnyItem
{
    id			*object = _items;
    int			i;
    
    for (i = 0; i < _count; i++, object++) {
    	if (*object == nil) {
	    if ((*object = [[_kind alloc]
				    initForResource:self
				    		item:i
					    shareable:YES]) != nil) {
		if (_itemCount++ == 0)
		    [_owner _resourceActive];

		return *object;
	    }
	}
	else if ([*object share])
	    return *object;
    }
    
    return nil;
}
#endif

- (unsigned int) findFreeItem
{
    KernBusItem		**items = _items;
    unsigned int	i;

    for (i = 0; i < _count; i++)
	if (items[i] == nil)
	    break;

    /*
     * If there are no free items, we return an invalid item.
     */
    return i + _base;
}

@end

@implementation KernBusItemResource(Private)

- _destroyItem:		object
{
    KernBusItem		**item, **items = _items;
    KernBusItem_	*iv = (KernBusItem_ *)object;
    
    if (![object isKindOf:_kind])
    	return nil;
	
    item = &items[iv->_item - _base];
    if (*item == nil)
    	return nil;
	
    *item = nil;
    if (--_itemCount == 0)
    	[_owner _resourceInactive];
    
    return [object dealloc];
}

@end

@implementation KernBusItem

- initForResource:	resource
	item:		(unsigned int)item
	shareable:	(BOOL)shareable
{
    if (resource == nil)
	return [super free];

    [super init];

    _resource = resource;
    _item = item;
    _useCount = 1;
    _shareable = shareable;
    
    return self;
}

- init
{
    return [super free];
}

- free
{
    if (--_useCount > 0)
    	return nil;
	
    return [_resource _destroyItem:self];
}

- dealloc
{
    if (_useCount > 0)
    	return self;

    return [super free];
}

- share
{
    if (!_shareable)
	return nil;
	
    _useCount++;
    
    return self;
}

- (unsigned int)item
{
    return _item;
}

@end

@implementation KernBusRangeResource

- initWithExtent:	(Range)extent
	    kind:	kind
	    owner:	owner
{
    unsigned int	end = extent.base + extent.length;
    
    [super init];

    /*
     * Check for wraparound.
     */	
    if (!rangeIsValid(extent))
    	return [self free];

    _owner = owner;
    _kind = (kind == nil)? [KernBusRange class]: kind;
    _rangeCount = 0;
    _ranges = 0;
    _end = end; _base = extent.base;
    
    return self;
}

- free
{
    if (_rangeCount > 0)
    	return self;
    
    return [super free];
}

- reserveRange:	(Range)range
{
    unsigned int	base = range.base;
    unsigned int	end = base + range.length;
    void		**prevnext = &_ranges;
    KernBusRange_	*new, *next;
    id			object = nil;

    /*
     * Check region for wrap.
     */	
    if (!rangeIsValid(range))
    	return nil;

    /*
     * Check region against
     * address space.
     */
    if (base < _base || (_end > 0 && end > _end))
	return nil;

    /*
     * Add range if no overlap
     * with existing regions.
     */	
    while (TRUE) {
    	if ((next = *prevnext) == 0 || end <= next->_base) {
	    if ((object = [[_kind alloc]
				    initForResource:self
						range:range
						shareable:NO]) != nil) {
		new = (KernBusRange_ *)object;
		new->_next = next;
		*prevnext = new;
		if (_rangeCount++ == 0)
		    [_owner _resourceActive];
	    }
	    break;
	}
	else if (next->_end > base)
	    break;
	
	prevnext = &next->_next;
    }
    
    return object;
}

- shareRange:	(Range)range
{
    unsigned int	base = range.base;
    unsigned int	end = base + range.length;
    void		**prevnext = &_ranges;
    KernBusRange_	*new, *next;
    id			object = nil;

    /*
     * Check region for wrap.
     */	
    if (!rangeIsValid(range))
    	return nil;

    /*
     * Check region against
     * address space.
     */
    if (base < _base || (_end > 0 && end > _end))
	return nil;

    /*
     * Add range if no overlap
     * with existing regions.
     */	
    while (TRUE) {
    	if ((next = *prevnext) == 0 || end <= next->_base) {
	    if ((object = [[_kind alloc]
				    initForResource:self
					range:range
					shareable:YES]) != nil) {
		new = (KernBusRange_ *)object;
		new->_next = next;
		*prevnext = new;
		if (_rangeCount++ == 0)
		    [_owner _resourceActive];
	    }
	    break;
	}
#if !ppc
	else if (base >= next->_base && end <= next->_end) {
	    object = (KernBusRange *)next; object = [object share];
	    break;
	}
	else if (next->_end > base)
	    break;
#endif
	prevnext = &next->_next;
    }
    
    return object;
}

- (Range)findFreeRangeWithSize:(unsigned int)size
	 alignment:(unsigned int)align
{
    Range 		result;
    unsigned int	base;
    unsigned int	end;
    void		**prevnext = &_ranges;
    KernBusRange_	*new, *next;
    id			object = nil;

    base = _base;
    end = base + size;
    result.base = result.length = 0;

    /*
     * Add range if no overlap
     * with existing regions.
     */
    for (base = _base, end = base + size;
         base < _end;
	 base += align, end += align) {
	while (TRUE) {
	    if ((next = *prevnext) == 0 || end <= next->_base) {
		result.base = base;
		result.length = end - base;
		break;
	    }
	    else if (next->_end > base)
		break;
	    
	    prevnext = &next->_next;
	}
	
	if (result.length)
	    break;
    }
    
    return result;
}

@end

@implementation KernBusRangeResource(Private)

- _destroyRange:	object
{
    void		**prevnext = &_ranges;
    KernBusRange_	*next;
    
    // Should check class of object
    
    if (![object isKindOf:_kind])
    	return nil;
	
    while ((next = *prevnext) != 0) {
    	if (next == (KernBusRange_ *)object) {
	    *prevnext = next->_next;
	    if (--_rangeCount == 0)
	    	[_owner _resourceInactive];

	    return [object dealloc];
	}
	
	prevnext = &next->_next;
    }
    
    return nil;
}

@end

@implementation KernBusRange

- initForResource:	resource
	range:		(Range)range
	shareable:	(BOOL)shareable
{
    if (resource == nil)
	return [super free];

    [super init];

    _resource = resource;
    _base = range.base;
    _end = _base + range.length;
    _useCount = 1;
    _shareable = shareable;
    _mappingCount = 0;
    
    return self;
}

- init
{
    return [super free];
}

- free
{
    if (_mappingCount > 0)
    	return self;

    if (--_useCount > 0)
    	return nil;
	
    return [_resource _destroyRange:self];
}

- dealloc
{
    if (_useCount > 0)
    	return self;

    return [super free];
}

- share
{
    if (!_shareable)
	return nil;
	
    _useCount++;
    
    return self;
}

- (Range)range
{
    Range	range;
    
    range.base = _base;
    range.length = _end - _base;
    
    return range;
}

@end

@implementation KernBusRange(Private)

- _addMapping
{
    _mappingCount++;
    
    return self;
}

- _destroyMapping:	object
{
    _mappingCount--;
    
    return [object free];
}

@end

@implementation KernBusRangeMapping

- initWithRange:	range
	subRange:	(Range)subRange
{
    Range		resourceRange;
    unsigned int	resourceBase, resourceEnd;
    unsigned int	mappedBase, mappedEnd;

    if (range == nil)
    	return [self free];

    [super init];

    if (!rangeIsValid(subRange))
    	return [self free];

    /*
     * Convert range into
     * endpoints.
     */
    resourceRange = [range range];
    resourceBase = resourceRange.base;
    resourceEnd = resourceBase + resourceRange.length;

    /*
     * Convert subrange into
     * absolute endpoints.
     */	
    mappedBase = resourceBase + subRange.base;
    mappedEnd = resourceBase + subRange.base + subRange.length;

    /*
     * Check subrange against
     * range.
     */
    if (mappedBase < resourceBase ||
    		(resourceEnd > 0 && mappedEnd > resourceEnd))
	return [self free];

    _range = range;
    _mappedRange.base = mappedBase;
    _mappedRange.length = subRange.length;

    [range _addMapping];

    return self;
}

- free
{
    id			range = _range;

    if (range == nil)
    	return [super free];
	
    _range = nil;

    return [range _destroyMapping:self];
}

- (Range)mappedRange
{
    return _mappedRange;
}

@end

@implementation KernBus

static HashTable	*_busClasses, *_busInstances;

+ initialize
{
    if (_busClasses == nil) {
	_busClasses = [[HashTable alloc] initKeyDesc:"*"];
    }
    if (_busInstances == nil) {
	_busInstances = [[HashTable alloc] initKeyDesc:"*"];
    }
    
    return self;
}

+ registerBusClass:busClass name:(char *)className
{
    [_busClasses insertKey:className value:busClass];
    
    return busClass;
}

+ registerBusInstance:busInstance name:(char *)busName  busId:(int)index
{
    HashTable *t;
    
    t = [_busInstances valueForKey:busName];
    if (t == NULL) {
	t = [[HashTable alloc] initKeyDesc:"i"];
	[_busInstances insertKey:busName value:t];
    }
    [t insertKey:(void *)index value:busInstance];
    return busInstance;
}

+ lookupBusClassWithName:(char *)busName
{
    return [_busClasses valueForKey:busName];
}

+ lookupBusInstanceWithName:(char *)busName busId:(int)index
{
    HashTable *t;
    id obj = nil;
    
    t = [_busInstances valueForKey:busName];
    if (t) {
	obj = [t valueForKey:(void *)index];
    }
    return obj;
}

- init
{
    [super init];
    
    if (_resources == nil)
	_resources = [[HashTable alloc] initKeyDesc:"*"];

    return self;
}

- free
{
    if (_activeResourceCount > 0)
    	return self;
	
    return [super free];
}

- (BOOL)areResourcesActive
{
    return _activeResourceCount > 0;
}

+ deviceDescriptionFromConfigTable:table
{
    return [[KernDeviceDescription alloc] initFromConfigTable:table];
}

/*
 * Allocate resources for bus.
 * Return nil if there is an error allocating a resource.
 */
- allocateResourcesForDeviceDescription:descr
{
    const char **names;
    
    for (names = [self resourceNames]; names && *names; names++) {
	if ([descr allocateResourcesForKey:*names] == nil)
	    return nil;
    }
    return descr;
}

- _insertResource:	object
	withKey:	(const char *)key
{
    HashTable	*resources = (HashTable *)_resources;
    
    if ([resources isKey:key])
    	return nil;
	
    [resources insertKey:key value:object];
    
    return object;
}

- _deleteResourceWithKey:	(const char *)key
{
    HashTable	*resources = (HashTable *)_resources;
    id		object;

    if ((object = [resources valueForKey:key]) == nil)
    	return nil;
	
    [resources removeKey:key];
    
    return object;
}

- _lookupResourceWithKey:	(const char *)key
{
    HashTable	*resources = (HashTable *)_resources;

    return [resources valueForKey:key];
}

- (const char **)resourceNames
{
    static const char *names[1];

    return names;
}

+ (BOOL)configureDriverWithTable:configTable
{
    return NO;
}

- (int)busId
{
    return _busId;
}

- (void)setBusId:(int)busId
{
    _busId = busId;
}

+ (BOOL)probeBus:configTable
{
    [[self alloc] init];
    return YES;
}

@end

@implementation KernBus(Private)

- (void)_resourceActive
{
    _activeResourceCount++;
}

- (void)_resourceInactive
{
    _activeResourceCount--;
}

@end
