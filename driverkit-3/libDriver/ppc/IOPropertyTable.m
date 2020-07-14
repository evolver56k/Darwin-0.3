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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 * HISTORY
 *
 * Simon Douglas  22 Oct 97
 * - first checked in.
 */



#include <stdlib.h>
#include <string.h>

#include <driverkit/ppc/IOPropertyTable.h>


typedef struct PTEntry PTEntry;
struct PTEntry
{
    PTEntry	*	next;
    void 	*	data;
    ByteCount		dataLength;
    OptionBits		flags;
    char		name[ 1 ];
};


struct PTable
{
	PTEntry		dummy;
};
typedef struct PTable PTable;

@implementation IOPropertyTable

void * 		PTAlloc( UInt32 size, Boolean clear )
{
    void * mem;

    mem = (void *) IOMalloc( size);
    if( mem && clear)
	bzero( mem, size);
    return( mem);
}

void PTFree( void * data, UInt32 size )
{
    IOFree( data, size);
}


- init
{
    _ptprivate = PTAlloc( sizeof( PTable), true);

    return( self);
}

-(IOReturn) deleteEntry:(PTEntry *)entry prev:(PTEntry *)prev
{
    prev->next = entry->next;
    
    if( 0 == (entry->flags & kReferenceProperty))
	PTFree( entry->data, entry->dataLength);

    PTFree( entry, strlen( entry->name) + sizeof( PTEntry));

    return( noErr);
}

- free
{
    PTable	*	table = _ptprivate;
    PTEntry	*	previous = &table->dummy;
    PTEntry	*	entry;

    while( (entry = previous->next))
	[self deleteEntry:entry prev:previous];

    PTFree( table, sizeof( PTable));
    return( [super free]);
}


- (PTEntry *) findProperty:(const char *)name prev:(PTEntry **)prev
{
    PTable	*	table = _ptprivate;
    PTEntry	*	previous = &table->dummy;
    PTEntry	*	entry;

    while( (entry = previous->next)) {
	if( 0 == strcmp( name, entry->name)) {
	    if( prev)
		*prev = previous;
	    return( entry);
	}
	previous = entry;
    }

    return( NULL);
}


-(IOReturn) getPropertyWithIndex:(UInt32)index name:(char *)name
{

    PTable	*	table = _ptprivate;
    PTEntry	*	previous = &table->dummy;
    PTEntry	*	entry;

    while( (entry = previous->next)) {
	if( (index--) == 0) {
	    strcpy( name, entry->name );
	    return( noErr);
	}
	previous = entry;
    }

    return( nrNotFoundErr);
}


-(IOReturn) setEntryProperty:(PTEntry *)entry 
		flags:(OptionBits)flags value:(void *)value length:(ByteCount)length
{
    void	*	copy;

    if( flags & kReferenceProperty) {
	copy = value;
    } else {
	copy = PTAlloc( length, false);
	if( nil == copy)
	    return( nrNotEnoughMemoryErr);
	bcopy( value, copy, length );
    }

    if( (0 == (entry->flags & kReferenceProperty))
      && entry->data )
	PTFree( entry->data, entry->dataLength);

    entry->flags = flags;
    entry->data = copy;
    entry->dataLength = length;

    return( noErr);
}


-(IOReturn) createProperty:(const char *)name flags:(OptionBits)flags 
		value:(void *)value length:(ByteCount)length
{
    IOReturn	err;
    PTable	*	table = _ptprivate;
    PTEntry	*	entry;

    if( [self findProperty:name prev:NULL] )
	return( nrPropertyAlreadyExists);

    entry = PTAlloc( strlen(name) + sizeof( PTEntry), true);
    if( NULL == entry)
	return( nrNotEnoughMemoryErr);

    err = [self setEntryProperty:entry flags:flags value:value length:length];
    if( err) {
	PTFree( entry, strlen(name) + sizeof( PTEntry));
	return( err);
    }

    entry->next = table->dummy.next;
    table->dummy.next = entry;
    strcpy( entry->name, name );

    return( noErr);

}

-(IOReturn) setProperty:(const char *)name flags:(OptionBits)flags
		value:(void *)value length:(ByteCount)length
{
    PTEntry	*	entry;

    entry = [self findProperty:name prev:NULL];
    if( entry == NULL)
	return( nrNotFoundErr);

    return( [self setEntryProperty:entry flags:flags value:value length:length] );
}

-(IOReturn) deleteProperty:(const char *)name
{
    PTEntry	*	entry;
    PTEntry	*	prev;

    entry = [self findProperty:name prev:&prev];
    if( entry == NULL)
	return( nrNotFoundErr);

    return( [self deleteEntry:entry prev:prev] );
}

-(IOReturn) getProperty:(const char *)name flags:(OptionBits)flags
		value:(void **)value length:(ByteCount *)length
{
    PTEntry	*	entry;

    entry = [self findProperty:name prev:NULL];
    if( entry == NULL)
	return( nrNotFoundErr);

    if( flags & kReferenceProperty) {
	if( value)				// can be NULL to just get the size
	    *value = entry->data;
    } else {
	bcopy( entry->data, *value, ((*length) < entry->dataLength) 
				? (*length) : entry->dataLength );
    }

    *length = entry->dataLength;

    return( noErr);
}

////////  IOConfigTable methods  ////////

- (const char *)valueForStringKey:(const char *)key
{
    IOReturn		err;
    ByteCount		length;
    char *		data;
    char *		result;
    char		nameBuf[ 88 ] = "AAPL,dk_";

    length = strlen( key);
    if( length > 80)
	length = 80;
    strncpy( nameBuf + 8, key, length);
    nameBuf[ 8 + length ] = 0;

    err = [self getProperty:nameBuf flags:kReferenceProperty
		value:(void **)&data length:&length];
    if( err)
	return( NULL);

    result = (char *) IOMalloc( length + 1);
    if( result) {
	strncpy( result, data, length);
	result[ length ] = 0;
    }

    return( result);
}

static const char * getNextStr( const char *where, UInt32 * len )
{
const char	* start;
const char	* end;

    start = strchr( where, '\"');
    if( start != NULL) {
	start++;
	end = strchr( start, '\"');
	if( end)
	    *len = end - start;
	else
	    start = NULL;
    }
    return( start);
}


- addConfigData:(const char *)configData
{

    char		nameBuf[ 88 ] = "AAPL,dk_";
    const char	*	str;
    const char	*	eol;
    const char	*	value;
    UInt32		len;

    str = configData;
    while( (str = getNextStr( str, &len))) {

	if( len > 80)
	    len = 80;
	strncpy( nameBuf + 8, str, len);
	nameBuf[ 8 + len ] = 0;

	str += len + 1;
	eol = strchr( str, ';');
	value = getNextStr( str, &len);
	if( value == NULL)
	    break;
        [self deleteProperty:(const char *)nameBuf];
	if( eol > value) {
	    [self createProperty:(const char *)nameBuf flags:0 value:value length:len];
	    str = value + len + 1;
	} else	// key with no value
	    [self createProperty:(const char *)nameBuf flags:0 value:value length:0];
    }

    return( self);
}

+ (void)freeString : (const char *)string
{
    IOFree((char *)string, strlen(string) + 1);
}

- (void)freeString : (const char *)string
{
    [IOPropertyTable freeString:string];
}	

@end

