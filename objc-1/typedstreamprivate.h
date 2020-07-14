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
	typedstreamprivate.h
	Copyright 1989 NeXT, Inc.
	Responsibility: Bertrand Serlet
*/

#ifndef _OBJC_TYPEDSTREAMPRIVATE_H_
#define _OBJC_TYPEDSTREAMPRIVATE_H_

/*
 *	This module provides the type definitions necessary for typedstream.h
 *	No client except typedstream should directly use this module.
 *
 */
 

#import <streams/streams.h>
#import "objc.h"
#import "hashtable.h"


/*************************************************************************
 *	Low-Level: encoding tightly information, 
 *	and sharing pointers and strings
 **************************************************************************/


typedef struct {
    NXStream	*physical;	/* the underlying stream */
    BOOL	swap;		/* should be swap bytes on reading */
    BOOL	write;		/* are we writing */
    NXHashTable	*strings;	/* maps strings to labels (vice versa for reading) */
    int		stringCounter;	/* next string label */
    int		stringCounterMax;
    NXHashTable	*ptrs;		/* maps ptrs to labels (vice versa for reading) */
    int		ptrCounter;	/* next ptr label */
    int		ptrCounterMax;
    NXZone	*scratch;
    } _CodingStream;

/* Creation, destruction */
static _CodingStream *_NXOpenEncodingStream (NXStream *physical);
	/* creates an encoding stream, given physical stream */

static _CodingStream *_NXOpenDecodingStream (NXStream *physical);
	/* creates an decoding stream, given physical stream */

static BOOL _NXEndOfCodingStream (_CodingStream *coder);
	/* TRUE iff end of stream */

static void _NXCloseCodingStream (_CodingStream *coder);

/* Encoding/Decoding of usual quantities */

static void _NXEncodeBytes (_CodingStream *coder, const char *buf, int count);
static void _NXDecodeBytes (_CodingStream *coder, char *buf, int count);

static void _NXEncodeChar (_CodingStream *coder, signed char c);
static signed char _NXDecodeChar (_CodingStream *coder);

/* static void _NXEncode (_CodingStream *coder, short x); */
static short _NXDecodeShort (_CodingStream *coder);

static void _NXEncodeInt (_CodingStream *coder, int x);
static int _NXDecodeInt (_CodingStream *coder);

static void _NXEncodeFloat (_CodingStream *coder, float x);
static float _NXDecodeFloat (_CodingStream *coder);

static void _NXEncodeDouble (_CodingStream *coder, double x);
static double _NXDecodeDouble (_CodingStream *coder);

/* low-level string coding; should never be called directly */
static void _NXEncodeChars (_CodingStream *coder, const char *str);
static char *_NXDecodeChars (_CodingStream *coder, NXZone *zone);

/* Encoding/Decoding of shared quantities.  Forces sharing of strings: identical but non shared strings at encoding will end up shared at decoding */
static void _NXEncodeSharedString (_CodingStream *coder, const char *str);
static char *_NXDecodeSharedString (_CodingStream *coder);
static const char *_NXDecodeUniqueString (_CodingStream *coder);
	/* always returns a "unique" string, that should never be freed */
	
static void _NXEncodeString (_CodingStream *coder, const char *str);
static char *_NXDecodeString (_CodingStream *coder, NXZone *zone);
	/* always returns a new string */
	
static BOOL _NXEncodeShared (_CodingStream *coder, const void *ptr);
	/* if ptr was previously encoded, encodes its previous label and returns FALSE
	if this is a new ptr, encodes that it's a new one, increases the label counter and returns TRUE ; It is assumed that the information relative to ptr is then encoded */
/* static BOOL _NXDecodeShared (_CodingStream *coder, void **pptr, int *label); */
	/* if we are reading a previously read quantity or nil, its previous ptr is set, and FALSE is returned
	if we are reading a new marker, sets the appropriate label and returns TRUE.  The data should then be read and _NXNoteShared should be called */
/* static void _NXNoteShared (_CodingStream *coder, void *ptr, int label); */

/*************************************************************************
 *	Encoding typed information, and dealing with ids
 **************************************************************************/

typedef struct {
    const char		*className;
    int			version;
    } _ClassVersion;

typedef struct {
    _CodingStream	*coder; /* the underlying stream */
    NXHashTable		*ids;	/* Set of all visited IDs */
    BOOL		write;	/* writing vs reading */
    BOOL		noteConditionals; /* when TRUE, it's a no-write pass */
    BOOL		doStatistics ; /* prints statistics */
    const char		*fileName; /* unless nil, file to be written when stream is closed */
    signed char		streamerVersion; /* changes in the meta-schema */
    int			systemVersion;	/* typically, appkit version */
    NXHashTable		*classVersions;	/* for read: [className -> version] */
    int			classVersionsCounter;
    NXZone		*objectZone;
    } TypedStream;

static void InternalWriteObject (TypedStream *s, id object);
	/* writes an object without header */
static id InternalReadObject (TypedStream *s);
	/* reads an object without header */

static void NXWriteClass (NXTypedStream *stream, Class class);
	/* Equivalent to NXWriteTypes (stream, "#", &class) */
static Class NXReadClass (NXTypedStream *stream);
	
#endif /* _OBJC_TYPEDSTREAMPRIVATE_H_ */
