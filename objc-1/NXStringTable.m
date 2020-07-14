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
/*
	NXStringTable.m
  	Copyright 1990 NeXT, Inc.
	Written by Bertrand Serlet, Jan 89
	Responsibility: Bertrand Serlet
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import "NXStringTable.h"

@implementation NXStringTable
+ new {
    return [self newKeyDesc:"%" valueDesc:"*"];
}

- init {
    return [self initKeyDesc:"%" valueDesc:"*"];
}

static void noFree (void *item) {}
static void freeString(void *string) { free(string);}
- free {
    [self freeKeys:noFree values:freeString];
    return [super free];
}
    
- (const char *)valueForStringKey:(const char *)aString {
    return (const char *) [super valueForKey:NXUniqueString(aString)];
}

static int skipSpace(NXStream *stream) {
    /* return first significant character */
    int	ch;
    while ((ch = NXGetc(stream)) != EOF) {
	if ((ch != ' ') && (ch != '\n') && (ch != '\t')) return ch;
    }
    return ch;
}

static int parseWord(NXStream *stream, char *word) {
    /* return '"', that is read, or EOF */
    int	length = 0;
    int	ch;
    while (((ch = NXGetc(stream)) != EOF) && (ch != '"') && (length < MAX_NXSTRINGTABLE_LENGTH)) {
	word[length++] = ch;
	if (ch == '\\') {
	    switch (ch = NXGetc(stream)) {
		case 'a':	word[length-1] = '\a'; break;
		case 'b':	word[length-1] = '\b'; break;
		case 'f':	word[length-1] = '\f'; break;
		case 'n':	word[length-1] = '\n'; break;
		case 'r':	word[length-1] = '\r'; break;
		case 't':	word[length-1] = '\t'; break;
		case 'v':	word[length-1] = '\v'; break;
		case '"':	word[length-1] = '\"'; break;
		case EOF:	break;
		case '\'':	word[length-1] = '\''; break;
		case '?':	word[length-1] = '?'; break;
		case '\\':	word[length-1] = '\\'; break;
		case 'x':
		    {
		      unsigned char c2 = 0;
		      
		      while (1)
			{
			  ch = NXGetc (stream);
			  
			  if ('0' <= ch && ch <= '7')
			    c2 = (16 * c2) + (ch - '0');
			  else if ('a' <= ch && ch <= 'f')
			    c2 = (16 * c2) + (ch - 'a' + 0xa);
			  else if ('A' <= ch && ch <= 'F')
			    c2 = (16 * c2) + (ch - 'A' + 0xa);
			  else
			    {
			      NXUngetc (stream);
			      break;
			    }
			}
		      
		      word[length-1] = c2;
		      break;
		    }
		  
		default:
		  if ('0' <= ch && ch <= '7')
		    {
		      unsigned char c2 = ch - '0';
		      unsigned int i;
		      
		      for (i = 0; i < 2; i++)
			{
			  ch = NXGetc (stream);
			  
			  if ('0' <= ch && ch <= '7')
			    c2 = (8 * c2) + (ch - '0');
			  else
			    {
			      NXUngetc (stream);
			      break;
			    }
			}
		      
		      word[length-1] = c2;
		    }
		  else
		    /* unknown escape sequence */
		    word[length-1] = ch;
		  
		  break;
	    }
	}
    }
    word[length] = 0;
    return ch;
}

static void writeWord(NXStream *stream, const char *word) {
    int	ch;
    NXPutc(stream, '"');
    while ((ch = *(word++))) {
	switch (ch) {
	    case '\a':	NXPutc(stream, '\\'); NXPutc(stream, 'a'); break;
	    case '\b':	NXPutc(stream, '\\'); NXPutc(stream, 'b'); break;
	    case '\f':	NXPutc(stream, '\\'); NXPutc(stream, 'f'); break;
	    case '\n':	NXPutc(stream, '\\'); NXPutc(stream, 'n'); break;
	    case '\r':	NXPutc(stream, '\\'); NXPutc(stream, 'r'); break;
	    case '\t':	NXPutc(stream, '\\'); NXPutc(stream, 't'); break;
	    case '\v':	NXPutc(stream, '\\'); NXPutc(stream, 'v'); break;
	    case '\"':	NXPutc(stream, '\\'); NXPutc(stream, '"'); break;
	    case '\\':	NXPutc(stream, '\\'); NXPutc(stream, '\\'); break;
	    default: NXPutc(stream, ch);
	}
    }
    NXPutc(stream, '"');
}

    
- readFromStream:(NXStream *)stream {
    int	ch;
    NXZone *zone = [self zone];
    if (!stream) return nil;
    while ((ch = skipSpace(stream)) != EOF) {
	switch (ch) {
	    case '/':
		ch = NXGetc(stream);
	    	if (ch != '*') goto nope;
		while ((ch = NXGetc(stream)) != EOF) {
		    if (ch == '*') {
			ch = NXGetc(stream);
			if (ch == '/') break;
			NXUngetc(stream);
		    }
		}
		if (ch == EOF) goto nope;
		break;
	    case '"': {
		char	key[MAX_NXSTRINGTABLE_LENGTH+1];
		char	value[MAX_NXSTRINGTABLE_LENGTH+1];
		ch = parseWord(stream, key);
		if (ch != '"') goto nope;
		ch = skipSpace(stream);
		if (ch == '=') {
		    ch = skipSpace(stream);
		    if (ch != '"') goto nope;
		    ch = parseWord(stream, value);
		    if (ch != '"') goto nope;
		    ch = skipSpace(stream);
		} else {
		    strcpy(value, key);
		}
		if (ch != ';') goto nope;
		free([self insertKey:NXUniqueString(key) 
		    value:NXCopyStringBufferFromZone(value, zone)]);
		break;
	    }
	    default:	goto nope;
	}
    }
    return self;
  nope:
    {
        extern void _NXLogError(const char *format, ...);
        if (ch == EOF)
            _NXLogError ("NXStringTable: parse error before end of stream");
        else
            _NXLogError ("NXStringTable: parse error before '%c'", ch);
        return nil;
    }
}

- readFromFile:(const char *)fileName {
    NXStream	*stream = NXMapFile(fileName, NX_READONLY);
    id retval = [self readFromStream:stream];
    if (stream) NXCloseMemory(stream, NX_FREEBUFFER);
    return retval;
}

+ newFromStream:(NXStream *)stream {
    id	table;
    if (! stream) return nil;
    table = [self new];
    if (![table readFromStream:stream])
	table = [table free];
    return table;
}
    
+ newFromFile:(const char *)fileName {
    id	table;
    table = [self new];
    if (![table readFromFile:fileName])
	table = [table free];
    return table;
}

- writeToStream:(NXStream *)stream {
    NXHashState		state = [self initState];
    NXAtom		key;
    const char		*value;
    while ([self nextState:&state key:(const void **)&key value:(void **)&value]) {
	writeWord(stream, key);
	if (strcmp(key, value)) {
	    NXPrintf(stream, "\t= ");
	    writeWord(stream, value);
	}
	NXPrintf(stream, ";\n");
    }
    return self;
}
    
- writeToFile:(const char *)fileName {
    NXStream	*stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    int		res;
    [self writeToStream:stream];
    res = NXSaveToFile(stream, fileName);
    NXCloseMemory(stream, NX_FREEBUFFER);
    return (res) ? nil : self;
}

@end
#endif /* KERNEL */