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
	NXStringTable.m
  	Copyright 1990-1996 NeXT Software, Inc.
	Written by Bertrand Serlet, Jan 89
	Responsibility: Bertrand Serlet
*/

#if !defined(KERNEL)

#if defined(NeXT_PDO)
#import <pdo.h>		//for pdo_malloc and pdo_free defines
#define PUTC(theChar, thePlace)		putc(theChar, thePlace)
#define GETC						getc
#define UNGETC(theChar, thePlace)	ungetc(theChar, thePlace)
#define FPRINTF						fprintf
#define	STREAM						FILE
#define OPEN_STREAM_RO(fileName)	fopen(fileName, "r")
#define OPEN_STREAM_RW(fileName)	fopen(fileName, "w")
#define CLOSE_STREAM(stream)		fclose(stream)
#endif

#if defined(__MACH__)
#import <streams/streams.h>
#define PUTC(theChar, thePlace)		NXPutc(thePlace, theChar)
#define GETC						NXGetc
#define FPRINTF						NXPrintf
#define UNGETC(theChar, thePlace)	NXUngetc(thePlace)
#define STREAM						NXStream
#define OPEN_STREAM_RO(fileName)	NXMapFile(fileName, NX_READONLY)
#define OPEN_STREAM_RW(fileName)	NXOpenMemory(NULL, 0, NX_WRITEONLY)
#define CLOSE_STREAM(stream)		NXCloseMemory(stream, NX_FREEBUFFER)
#endif

#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <objc/NXStringTable.h>

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

static int skipSpace(STREAM *stream) {
    /* return first significant character */
    int	ch;
    while ((ch = GETC(stream)) != EOF) {
	if ((ch != ' ') && (ch != '\n') && (ch != '\t')) return ch;
    }
    return ch;
}

static int parseWord(STREAM *stream, char **word, unsigned wordsize) {
    /* return '"', that is read, or EOF */
    int	length = 0;
    int	ch;
    while (((ch = GETC(stream)) != EOF) && (ch != '"')) {
    	if (wordsize <= length) {
	    wordsize += 128;
	    *word = realloc(*word, wordsize);
	}
	(*word)[length++] = ch;
	if (ch == '\\') {
	    switch (ch = GETC(stream)) {
		case 'a':	(*word)[length-1] = '\a'; break;
		case 'b':	(*word)[length-1] = '\b'; break;
		case 'f':	(*word)[length-1] = '\f'; break;
		case 'n':	(*word)[length-1] = '\n'; break;
		case 'r':	(*word)[length-1] = '\r'; break;
		case 't':	(*word)[length-1] = '\t'; break;
		case 'v':	(*word)[length-1] = '\v'; break;
		case '"':	(*word)[length-1] = '\"'; break;
		case EOF:	break;
		case '\'':	(*word)[length-1] = '\''; break;
		case '?':	(*word)[length-1] = '?'; break;
		case '\\':	(*word)[length-1] = '\\'; break;
		case 'x':
		    {
		      unsigned char c2 = 0;
		      
		      while (1)
			{
			  ch = GETC (stream);
			  
			  if ('0' <= ch && ch <= '7')
			    c2 = (16 * c2) + (ch - '0');
			  else if ('a' <= ch && ch <= 'f')
			    c2 = (16 * c2) + (ch - 'a' + 0xa);
			  else if ('A' <= ch && ch <= 'F')
			    c2 = (16 * c2) + (ch - 'A' + 0xa);
			  else
			    {
			      UNGETC (ch, stream);
			      break;
			    }
			}
		      
		      (*word)[length-1] = c2;
		      break;
		    }
		  
		default:
		  if ('0' <= ch && ch <= '7')
		    {
		      unsigned char c2 = ch - '0';
		      unsigned int i;
		      
		      for (i = 0; i < 2; i++)
			{
			  ch = GETC (stream);
			  
			  if ('0' <= ch && ch <= '7')
			    c2 = (8 * c2) + (ch - '0');
			  else
			    {
			      UNGETC (ch, stream);
			      break;
			    }
			}
		      
		      (*word)[length-1] = c2;
		    }
		  else
		    /* unknown escape sequence */
		    (*word)[length-1] = ch;
		  
		  break;
	    }
	}
    }
    (*word)[length] = 0;
    return ch;
}

static void writeWord(STREAM *stream, const char *word) {
    int	ch;
    PUTC ('"', stream);
    while ((ch = *(word++))) {
	switch (ch) {
	    case '\a':	PUTC ('\\', stream); PUTC ('a', stream); break;
	    case '\b':	PUTC ('\\', stream); PUTC ('b', stream); break;
	    case '\f':	PUTC ('\\', stream); PUTC ('f', stream); break;
	    case '\n':	PUTC ('\\', stream); PUTC ('n', stream); break;
	    case '\r':	PUTC ('\\', stream); PUTC ('r', stream); break;
	    case '\t':	PUTC ('\\', stream); PUTC ('t', stream); break;
	    case '\v':	PUTC ('\\', stream); PUTC ('v', stream); break;
	    case '\"':	PUTC ('\\', stream); PUTC ('"', stream); break;
	    case '\\':	PUTC ('\\', stream); PUTC ('\\', stream); break;
	    default: PUTC (ch, stream);
	}
    }
    PUTC ('"', stream);
}

    
- readFromStream:(STREAM *)stream {
    int	ch;
    NXZone *zone = [self zone];
    if (!stream) return nil;
    while ((ch = skipSpace(stream)) != EOF) {
	switch (ch) {
	    case '/':
		ch = GETC(stream);
	    	if (ch != '*') goto nope;
		while ((ch = GETC(stream)) != EOF) {
		    if (ch == '*') {
			ch = GETC(stream);
			if (ch == '/') break;
			UNGETC(ch, stream);
		    }
		}
		if (ch == EOF) goto nope;
		break;
	    case '"': {
		char *value = NULL, *key = malloc((MAX_NXSTRINGTABLE_LENGTH+1) * sizeof(char));
		ch = parseWord(stream, &key, MAX_NXSTRINGTABLE_LENGTH);
		if (ch != '"') {
		    free(key);
		    goto nope;
		}
		ch = skipSpace(stream);
		if (ch == '=') {
		    ch = skipSpace(stream);
		    if (ch != '"') goto nope;
		    value = malloc((MAX_NXSTRINGTABLE_LENGTH+1) * sizeof(char));
		    ch = parseWord(stream, &value, MAX_NXSTRINGTABLE_LENGTH);
		    if (ch != '"') {
		    	free(key);
		    	free(value);
		    	goto nope;
		    }
		    ch = skipSpace(stream);
		} 
		if (ch != ';') {
		    free(key);
		    if (value) free(value);
		    goto nope;
		}
		free([self insertKey:NXUniqueString(key) 
		    value:NXCopyStringBufferFromZone((value ? value : key), zone)]);
		free(key);
		if (value) free(value);
		break;
	    }
	    default:	goto nope;
	}
    }
    return self;
  nope:
    {
        OBJC_EXPORT void _NXLogError(const char *format, ...);
        if (ch == EOF)
            _NXLogError ("NXStringTable: parse error before end of stream");
        else
            _NXLogError ("NXStringTable: parse error before '%c'", ch);
        return nil;
    }
}

- readFromFile:(const char *)fileName {
    STREAM	*stream = OPEN_STREAM_RO(fileName);
    id retval = [self readFromStream:stream];
	if (stream) CLOSE_STREAM(stream);
    return retval;
}

+ newFromStream:(STREAM *)stream {
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

- writeToStream:(STREAM *)stream {
	NXHashState		state = [self initState];
    NXAtom		key;
    const char		*value;
    while ([self nextState:&state key:(const void **)&key value:(void **)&value]) {
	writeWord(stream, key);
	if (strcmp(key, value)) {
	    FPRINTF(stream, "\t= ");
	    writeWord(stream, value);
	}
	FPRINTF(stream, ";\n");
    }
    return self;
}
    
- writeToFile:(const char *)fileName {
#if defined(__MACH__)
    STREAM		*stream = OPEN_STREAM_RW(fileName);
    int		res;
    [self writeToStream:stream];
    res = NXSaveToFile(stream, fileName);
    CLOSE_STREAM(stream);
    return (res != 0) ? nil : self;
#else
	return nil;
#endif
}

@end

#endif // not KERNEL
