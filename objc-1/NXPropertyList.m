/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*	NXPropertyList.m
  	Copyright 1991, NeXT, Inc.
	Bertrand, August 1991
*/
#ifndef KERNEL
#import "NXPropertyList.h"

#import <ctype.h>
#import <streams/streams.h>
#import <stdio.h>

/********	String extras		********/

@interface NXMutableString (NX_BS_Extras)
//?? SHOULD BE IN NXString, AND EFFICIENT!
- (void)appendCharacters:(const unichar *)chars length:(unsigned)length;
@end

@implementation NXMutableString (NX_BS_Extras)
- (void)appendCharacters:(const unichar *)chars length:(unsigned)length {
    NXReadWriteString	*temp = [NXReadOnlyString alloc];
    [temp initFromCharactersNoCopy:(unichar *)chars length:length freeWhenDone:NO];
    [self append:temp];
    [temp free];
}
@end

@interface Object (NX_BS_Extras)
//?? Should be in ObjC
// Assumed by ipclib
- (void)writeToStream:(NXStream *)stream;
@end

@implementation Object (NX_BS_Extras)
- (void)writeToStream:(NXStream *)stream {
    NXPrintf(stream, "0x%x", self);
}
@end

/********	Class implementation		********/

@implementation NXPropertyList

static void freeKeyAndValue(NXMapTable *table, void *key, void *value) {
    [(id)key free];
    [(id)value free];
}

- init {
    NXMapTablePrototype	proto = NXObjectMapPrototype;
    proto.free = freeKeyAndValue;
    table = NXCreateMapTable(proto, 0);
    return self;
}

- free {
    /* we test for table NULL for cases when free is called during initialization */
    if (table) NXFreeMapTable(table);
    return [super free];
}

- (unsigned)count {
    return NXCountMapTable(table);
}

- (BOOL)member:(NXString *)key {
    id	value;
    return (NXMapMember(table, key, (void **)&value) != NX_MAPNOTAKEY);
}

- get:(NXString *)key {
    return NXMapGet(table, key);
}

- insert:(NXString *)key value:value {
    id		oldValue;
    void	*oldKey = NXMapMember(table, key, (void **)&oldValue);
    if (oldKey == NX_MAPNOTAKEY) {
	oldValue = nil;
    } else {
	(void)NXMapRemove(table, oldKey);
	[(id)oldKey free];
    }
    (void)NXMapInsert(table, [key immutableCopy], value);
    return oldValue;
}

- remove:(NXString *)key {
    id		oldValue;
    void	*oldKey = NXMapMember(table, key, (void **)&oldValue);
    if (oldKey == NX_MAPNOTAKEY) return nil;
    NXMapRemove(table, oldKey);
    [(id)oldKey free];
    return oldValue;
}

- empty {
    NXResetMapTable(table);
    return self;
}

- (NXMapState)initEnumeration {
    return NXInitMapState(table);
}

- (BOOL)enumerate:(NXMapState *)state key:(NXString **)refKey value:(id *)refValue {
    return NXNextMapState(table, state, (void **)refKey, (void **)refValue) != 0;
}

@end

/********	Basic ASCII read/write of property lists	********/

@implementation NXPropertyList (Basic_IO)
- initFromStream:(NXStream *)stream {
    NXPropertyListReadContext	context = {
    			0, [NXReadWriteString new],
			[NXReadOnlyString class],
			[NXReadOnlyString class],
			[NXCleanList class],
			[NXPropertyList class],
			YES,
			[self zone],
			NULL
			};
    id	new = [self initFromStream:stream context:&context];
    [context.buffer free];
    if (context.uniquingTable) NXFreeHashTable(context.uniquingTable);
    return new;
}
			
- initFromPath:(NXString *)path {
    NXPropertyListReadContext	context = {
    			0, [NXReadWriteString new],
			[NXReadOnlyString class],
			[NXReadOnlyString class],
			[NXCleanList class],
			[NXPropertyList class],
			YES,
			[self zone],
			NULL
			};
    id	new = [self initFromPath:path context:&context];
    [context.buffer free];
    if (context.uniquingTable) NXFreeHashTable(context.uniquingTable);
    return new;
}

- (void)writeToStream:(NXStream *)stream {
    NXPropertyListWriteContext	context = {
    			4, 0,
			NO, "\n"
			};
    [self writeToStream:stream context:&context];
}

- (BOOL)writeToPath:(NXString *)path safely:(BOOL)safe {
    char	cpath[MAXPATHLEN];
    char	temp[MAXPATHLEN];
    NXStream	*stream;
    BOOL	ok;
    if (! path) return NO;
    [path getCString:cpath]; cpath[[path length]] = 0;
    stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    strcpy(temp, cpath);
    if (safe) strcat(temp, "~");
    [self writeToStream:stream];
    ok = ! NXSaveToFile(stream, temp);
    NXCloseMemory(stream, NX_FREEBUFFER);
    if (safe && ok) ok = ! rename(temp, cpath);
    return ok;
}

- (BOOL)writeToPath:(NXString *)path {
    return [self writeToPath:path safely:YES];
}

@end

/********	Read/write utilities		********/

// constants
#define BEGIN_PAR	'('
#define END_PAR		')'
#define BEGIN_CURLY	'{'
#define END_CURLY	'}'

static int NXGetNonSpace(NXStream *stream, int *line) {
    int		ch;
    while ((ch = NXGetc(stream)) != EOF) {
	if (ch == '\n') (*line)++;
	if (ch == '/') {
	    if ((ch = NXGetc(stream)) == '/') {
		while ((ch = NXGetc(stream)) != EOF && ch != '\n') {};
		if (ch == '\n') (*line)++;
	    } else if (ch == '*') {
		while ((ch = NXGetc(stream)) != EOF) {
		    if (ch == '*') {
			ch = NXGetc(stream);
			if (ch == '/') break;
			NXUngetc(stream);
		    } else if (ch == '\n') (*line)++;
		}
	    } else {
		NXUngetc(stream);
		return '/';
	    }
	} else if (! isspace(ch)) return ch;
    }
    return EOF;
}

static inline int isTokenChar(int ch) {
    return (isalnum(ch) || ch == '_' || ch == '$' || ch == ':' || ch == '.' || ch == '/') ? 1 : 0;
}

static int NXGetSlashedChar(NXStream *stream, int *line) {
    int	ch;
    switch (ch = NXGetc(stream)) {
	case '0':
	case '1':	
	case '2':	
	case '3':	
	case '4':	
	case '5':	
	case '6':	
	case '7':  {
			int		num = ch - '0';
			/* three digits maximum to avoid reading \000 followed by 5 as \5 ! */
			if ((ch = NXGetc(stream)) >= '0' && ch <= '7') {
			    num = (num << 3) + ch - '0';
			    if ((ch = NXGetc(stream)) >= '0' && ch <= '7') {
				num = (num << 3) + ch - '0';
			    } else NXUngetc(stream);
			} else NXUngetc(stream);
			return num;
		    }
	case 'a':	return '\a';
	case 'b':	return '\b';
	case 'f':	return '\f';
	case 'n':	return '\n';
	case 'r':	return '\r';
	case 't':	return '\t';
	case 'v':	return '\v';
	case '"':	return '\"';
	case '\n':	(*line)++;
			return '\n';
    }
    return ch;
}

static id readValue(NXStream *stream, NXPropertyListReadContext *context) {
    int	ch = NXGetNonSpace(stream, &context->line);
    if (ch == BEGIN_CURLY || ch == BEGIN_PAR) {
	id	factory;
	int	endch;
	id	value;
	if (ch == BEGIN_CURLY) {
	    factory = context->propertyListValueFactory;
	    endch = END_CURLY;
	} else {
	    factory = context->listValueFactory;
	    endch = END_PAR;
	}
	value = [[factory allocFromZone:context->zone] initFromStream:stream context:context];
	if (! value) return nil;
	ch = NXGetNonSpace(stream, &context->line);
	if (ch != endch) {
	    [value free];
	    return nil;
	}
	return value;
    } else {
	NXUngetc(stream);
	return [[context->stringValueFactory allocFromZone:context->zone] initFromStream:stream context:context];
    }
}

static void writeSpaces(NXStream *stream, int spaces) {
    while (spaces >= 8) {
	NXPutc(stream, '\t'); spaces -= 8;
    }
    while (spaces--) NXPutc(stream, ' ');
}

/********	A list that really frees its elements	********/

@implementation NXCleanList:List
- free {
    [self freeObjects];
    return [super free];
}
@end

/********	Fancy ASCII read/write of property lists	********/

@implementation NXPropertyList (Fancy_IO)
- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context {
    int		ch = NXGetNonSpace(stream, &context->line);
    BOOL	ok = YES;
    [self init];
    while (ok && (ch == '"' || isTokenChar(ch))) {
	NXString	*key;
	id		value;
	NXUngetc(stream);
	key = [[context->keyFactory allocFromZone:context->zone] initFromStream:stream context:context];
	if (! key) return NO;
	ch = NXGetNonSpace(stream, &context->line);
	if (ch == '=') {
	    value = readValue(stream, context);
	    if (! value) {
		printf("*** NXPropertyList: Syntax error line %u\n", context->line);
		[self free];
		return nil;
	    }
	    [self insert:key value:value];
	    ch = NXGetNonSpace(stream, &context->line);
	} else {
	    [self insert:key value:(context->noValueIsSame) ?  [key immutableCopy] : nil];
	}
	[key free];
	ok = NO;
	while (ch == ';') {
	    ok = YES; ch = NXGetNonSpace(stream, &context->line);
	}
    }
    NXUngetc(stream);
    return self;
}

- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context {
    NXMapState	state = [self initEnumeration];
    id		key;
    id		value;
    NXPropertyListWriteContext	original = *context;
    unsigned	spaces = context->indent;
    if (original.topLevelBrackets) NXPutc(stream, BEGIN_CURLY);
    while ([self enumerate:&state key:&key value:&value]) {
	context->topLevelBrackets = YES;
	context->pairSeparator = original.pairSeparator;
	writeSpaces(stream, spaces);
	if (key == value) {
	    [key writeToStream:stream context:context];
	    NXPrintf(stream, ";");
	} else if ([value isKindOf:[NXPropertyList class]]) {
	    [key writeToStream:stream context:context];
	    NXPrintf(stream, " = {%s", original.pairSeparator);
	    context->topLevelBrackets = NO;
	    context->indent = spaces + context->indentDelta;
	    [value writeToStream:stream context:context];
	    writeSpaces(stream, spaces);
	    NXPrintf(stream, "};");
	} else if (value) {
	    [key writeToStream:stream context:context];
	    NXPrintf(stream, " = ");
	    [value writeToStream:stream context:context];
	    NXPrintf(stream, ";");
	}
	NXPrintf(stream, "%s", original.pairSeparator);
    }
    if (original.topLevelBrackets) NXPutc(stream, END_CURLY);
}
- initFromPath:(NXString *)path context:(NXPropertyListReadContext *)context {
    char	cpath[MAXPATHLEN];
    NXStream	*stream;
    id		new;
    if (! path) goto nope;
    [path getCString:cpath]; cpath[[path length]] = 0;
    stream = NXMapFile(cpath, NX_READONLY);
    if (! stream) goto nope;
    new = [self initFromStream:stream context:context];
    if (NXGetc(stream) != EOF) {
	char	cpath[MAXPATHLEN];
	[path getCString:cpath];
	printf("NXPropertyList: discarded input at line %u of file %s\n", context->line, cpath);
    }
    NXCloseMemory(stream, NX_FREEBUFFER);
    return new;
  nope:
    [self free];
    return nil;
}

@end

@implementation NXCleanList (Fancy_IO)
- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context {
    BOOL	ok = YES;
    int		ch = NXGetNonSpace(stream, &context->line);
    while (ok && (ch == '"' || isTokenChar(ch) || ch == BEGIN_CURLY || ch == BEGIN_PAR)) {
	id	value;
	NXUngetc(stream);
	value = readValue(stream, context);
	if (! value) {
	    printf("*** NXCleanList: Syntax error line %u\n", context->line);
	    [self free]; 
	    return nil;
	}
	[self addObject:value];
	ch = NXGetNonSpace(stream, &context->line);
	ok = NO;
	while (ch == ',') {
	    ok = YES; ch = NXGetNonSpace(stream, &context->line);
	}
    }
    NXUngetc(stream);
    return self;
}


- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context {
    unsigned	index = 0;
    unsigned	count = [self count];
    BOOL	top = context->topLevelBrackets;
    if (top) NXPutc(stream, BEGIN_PAR);
    while (index < count) {
	context->topLevelBrackets = YES;
	context->pairSeparator = "";
	context->indent = 0;
	[[self objectAt:index] writeToStream:stream context:context];
	if (index != count-1) NXPrintf(stream, ", ");
	index++;
    }
    if (top) NXPutc(stream, END_PAR);
}

@end

@implementation NXString (Fancy_IO)

#define MAX_TOKEN	1024

static inline void append1(NXMutableString *buffer, BOOL *bufferUsed, int ch, unichar *buf, unsigned *buflen) {
    if (*buflen == MAX_TOKEN) {
	if (! *bufferUsed) [buffer replaceWith:@""];
	*bufferUsed = YES;
	[buffer appendCharacters:buf length:MAX_TOKEN];
	*buflen = 0;
    }
    buf[(*buflen)++] = ch;
}

static inline NXString *init(NXString *self, NXMutableString *buffer, BOOL bufferUsed, unichar *buf, unsigned *buflen) {
    if (bufferUsed) {
	[buffer appendCharacters:buf length:*buflen];
	return [self initFromString:buffer];
    }
    return [self initFromCharacters:buf length:*buflen];
}

- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context {
    int		ch = NXGetNonSpace(stream, &context->line);
    if (ch == '"') {
	BOOL		bufferUsed = NO;
	unichar		buf[MAX_TOKEN];
	unsigned	buflen = 0;
	while (((ch = NXGetc(stream)) != EOF) && (ch != '"')) {
	    if (ch == '\n') (context->line)++;
	    if (ch == '\\') ch = NXGetSlashedChar(stream, &context->line);
	    append1(context->buffer, &bufferUsed, ch, buf, &buflen);
	}
	if (ch == EOF) {
	    [self free];
	    return nil;
	}
	return init(self, context->buffer, bufferUsed, buf, &buflen);
    } else if (isTokenChar(ch)) {
	BOOL		bufferUsed = NO;
	unichar		buf[MAX_TOKEN];
	unsigned	buflen = 0;
	append1(context->buffer, &bufferUsed, ch, buf, &buflen);
	while (((ch = NXGetc(stream)) != EOF) && isTokenChar(ch)) {
	    append1(context->buffer, &bufferUsed, ch, buf, &buflen);
	}
	NXUngetc(stream);
	return init(self, context->buffer, bufferUsed, buf, &buflen);
    } else {
	NXUngetc(stream);
	[self free];
	return nil;
    }
}

- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context {
    unsigned	index = 0;
    unsigned	count = [self length];
    BOOL	token = (count != 0);
    while (index < count) {
	int	ch = [self characterAt:index];
	if (! isTokenChar(ch)) { token = NO; break; }
	index ++;
    }
    if (token) {
	NXPrintf(stream, "%@", self);
    } else {
	NXPutc(stream, '"');
	index = 0;
	while (index < count) {
	    int	ch = [self characterAt:index];
	    int	ch1 = '\\', ch2 = 0;
	    switch (ch) {
		case '\\':	ch2 = ch; break;
		case '"':	ch2 = ch; break;
		case '\a':	ch2 = 'a'; break;
		case '\b':	ch2 = 'b'; break;
		case '\f':	ch2 = 'f'; break;
		case '\n':	ch2 = 'n'; break;
		case '\t':	ch2 = 't'; break;
		case '\v':	ch2 = 'v'; break;
		default:
		    if (ch >= ' ' && ch <= '~') {
			ch1 = ch;
		    } else {
			/* attention here: if the next character is a number, we need to avoid \0 followed by 5 that would become \05.  We do that by printing 3 digits */
			NXPrintf(stream, "\\%+03o", ch);
 			ch1 = 0;
		    }
	    }
	    if (ch1) {
		NXPutc(stream, ch1);
		if (ch2) NXPutc(stream, ch2);
	    }
	    index ++;
	}
	NXPutc(stream, '"');
    }
}

@end

@implementation NXReadOnlyString (Fancy_IO)
    // Only this class tries uniquing the strings
static unsigned hashString(const void *info, const void *data) {
    return [(NXString *)data hash];
}
static int isEqualString(const void *info, const void *data1, const void *data2) {
    return [(NXString *)data1 isEqual:(NXString *)data2];
}
static void freeString(const void *info, const void *data) {
    [(NXString *)data free];
}

- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context {
    NXString	*new = [super initFromStream:stream context:context];
    NXString	*original;
    if (! new) return new;
    if (! context->uniquingTable) {
	NXHashTablePrototype stringSetProto = {hashString, isEqualString, freeString, 0};
	context->uniquingTable = NXCreateHashTable(stringSetProto, 0, 0);
    }
    original = NXHashGet(context->uniquingTable, new);
    if (! original) {
	NXHashInsert(context->uniquingTable, [new immutableCopy]);
	return new;
    } else {
	[new free];
	return [original immutableCopy];
    }
}
@end

@implementation Object (Fancy_IO)
- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context {
    [self writeToStream:stream];
}
@end

#endif