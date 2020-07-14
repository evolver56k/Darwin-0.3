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
    typedstream.m
    Pickling data structures
    Copyright 1989, NeXT, Inc.
    Responsability: Bertrand Serlet
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdarg.h>
#import <mach/mach.h>
#import <sys/file.h>
#import "objc-private.h"
#import "Object.h"
#import "objc-class.h"
#import "objc-runtime.h"
#import "typedstream.h"
#import "typedstreamprivate.h"
#import "hashtable.h"

#import <architecture/byte_order.h>

/*************************************************************************
 *	Utilities
 *************************************************************************/

static void checkRead (TypedStream *s) {
    if (s->write) 
	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, "expecting a reading stream", s);
    };
    
static void checkWrite (TypedStream *s) {
    if (! s->write) 
	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, "expecting a writing stream", s);
    };

static void checkExpected (const char *readType, const char *wanted) {
    if (readType == wanted) return;
    if (! readType || strcmp (readType, wanted)) {
	char	*buffer = malloc (
		((readType) ? strlen (readType) : 0) + strlen (wanted) + 100);
	sprintf (buffer, "file inconsistency: read '%s', expecting '%s'", 
	    readType, wanted);
	NX_RAISE (TYPEDSTREAM_FILE_INCONSISTENCY, buffer, 0);
	};
    };
    
static volatile void classError (const char *className, const char *message) {
    char	*buffer = malloc (100);
    sprintf (buffer, "class error for '%s': %s", className, message);
    NX_RAISE (TYPEDSTREAM_CLASS_ERROR, buffer, 0);
    };
    
static volatile void typeDescriptorError (char ch, const char *message) {
    char	*buffer = malloc (100);
    sprintf (buffer, "type descriptor error for '%c': %s", ch, message);
    NX_RAISE (TYPEDSTREAM_TYPE_DESCRIPTOR_ERROR, buffer, 0);
    };
    
static volatile void writeRefError (const char *message) {
    NX_RAISE (TYPEDSTREAM_WRITE_REFERENCE_ERROR, message, 0);
    };
    
static BOOL sameSexAsCube () {
    union {int	ii; char cc[sizeof(int)];}	uu;
    
    uu.ii = 1;
    if (uu.cc[sizeof(int)-1] == 1) return TRUE;
    if (uu.cc[0] == 1) return FALSE;
    NX_RAISE (TYPEDSTREAM_FILE_INCONSISTENCY, 
    	"typedstream: snail? cannot recognize byte sex.", 
	(void *) (int) uu.cc[0]);
    };
    
/*************************************************************************
 *	Low-Level: encoding tightly information, 
 *	and sharing pointers and strings
 *************************************************************************/

#define LONG2LABEL	-127
#define LONG4LABEL	-126
#define REALLABEL	-125
#define NEWLABEL	-124
#define NULLLABEL	-123
#define EOOLABEL	-122
/* more reserved labels */
#define SMALLESTLABEL	-110
#define Bias(x) (x - SMALLESTLABEL)

/* Following constants are not universal constants, but used for the coding */
#define BIG_INT +2147483647L
#define SMALL_INT -2147483647L
#define BIG_SHORT +32767
#define SMALL_SHORT -32767

#define INIT_NSTRINGS 252
#define INIT_NPTRS 252
#define PTRS(x) ((void **)x)
#define STRINGS(x) ((char **)x)

static inline int inc_stringCounter(_CodingStream *coder) {
  if (coder->stringCounter == coder->stringCounterMax) {
    coder->stringCounterMax += INIT_NSTRINGS;
    coder->strings = (NXHashTable *)
	NXZoneRealloc(coder->scratch, coder->strings, coder->stringCounterMax * sizeof(char *));
  }
  return coder->stringCounter++;
}

static int inc_ptrCounter(_CodingStream *coder) {
  if (coder->ptrCounter == coder->ptrCounterMax) {
    coder->ptrCounterMax += INIT_NPTRS;
    coder->ptrs = (NXHashTable *)
	NXZoneRealloc(coder->scratch, coder->ptrs, coder->ptrCounterMax * sizeof(void *));
  }
  return coder->ptrCounter++;
}

typedef struct {
    const char	*key;
    int		value;
    } StrToInt;

static void freeStrToInt(const void *info, void *data) {
    free((char *)((StrToInt *)data)->key);
    free(data);
}

typedef struct {
    const void	*key;
    int		value;
    } PtrToInt;

typedef struct {
    int		key;
    void	*value;
    } IntToPtr;

   
/* Creation, destruction of the low-level streams */
static _CodingStream *_NXOpenEncodingStream (NXStream *physical) {
    _CodingStream		*coder;
    NXHashTablePrototype	proto = NXStrStructKeyPrototype;
    
    coder = (_CodingStream *) malloc (sizeof (_CodingStream));
    coder->physical = physical;
    coder->swap = NO;
    coder->write = YES;
    proto.free = freeStrToInt;
    coder->strings = NXCreateHashTable (proto, 7, NULL);
    coder->stringCounter = SMALLESTLABEL;
    coder->ptrs = NXCreateHashTable (NXPtrStructKeyPrototype, 15, NULL);
    coder->ptrCounter = SMALLESTLABEL;
    return coder;
    };

static _CodingStream *_NXOpenDecodingStream (NXStream *physical) {
    _CodingStream		*coder;
    NXZone *zone;
    
    zone = NXCreateZone(vm_page_size, vm_page_size, 1);
    coder = (_CodingStream *) NXZoneMalloc (zone, sizeof (_CodingStream));
    coder->scratch = zone;
    coder->physical = physical;
    coder->swap = NO;
    coder->write = NO;
    coder->strings = (NXHashTable *)
	NXZoneMalloc(coder->scratch, INIT_NSTRINGS * sizeof(char *));
    coder->stringCounter = 0;
    coder->stringCounterMax = INIT_NSTRINGS;
    coder->ptrs = (NXHashTable *)
	NXZoneMalloc(coder->scratch, INIT_NPTRS * sizeof(void *));
    coder->ptrCounter = 0;
    coder->ptrCounterMax = INIT_NPTRS;
    return coder;
    };

static BOOL _NXEndOfCodingStream (_CodingStream *coder) {
    char	ch = NXGetc (coder->physical);
    NXUngetc (coder->physical);
    return ch == -1;
    };

static void _NXCloseCodingStream (_CodingStream *coder) {
  if (coder->write) {
	NXFreeHashTable(coder->strings);
	NXFreeHashTable(coder->ptrs);
	free(coder);
    } else 
	NXDestroyZone(coder->scratch);
  };

/* Byte Swapping utilities */

static __inline__ unsigned long long
_NXSwapCard8(_CodingStream *coder, unsigned long long llin) {
    if (coder->swap)
	return NXSwapLongLong(llin);
    else
	return llin;
    };

static __inline__ unsigned int
_NXSwapCard4(_CodingStream *coder, unsigned int iin) {
    if (coder->swap)
	return NXSwapInt(iin);
    else
	return iin;
    };

static __inline__ unsigned short
_NXSwapCard2(_CodingStream *coder, unsigned short sin) {
    if (coder->swap)
	return NXSwapShort(sin);
    else
	return sin;
    };

static __inline__ float
_NXSwapFloat(_CodingStream *coder, NXSwappedFloat fin) {
    if (coder->swap) {
	return NXConvertSwappedFloatToHost(NXSwapFloat(fin));
    } else
	return NXConvertSwappedFloatToHost(fin);
    };

static __inline__ double
_NXSwapDouble(_CodingStream *coder, NXSwappedDouble din) {
    if (coder->swap) {
	return NXConvertSwappedDoubleToHost(NXSwapDouble(din));
    } else
	return NXConvertSwappedDoubleToHost(din);
    };


/* Encoding/Decoding of usual quantities */

static void _NXEncodeBytes (_CodingStream *coder, const char *buf, int count) {
    if (coder->physical) NXWrite (coder->physical, buf, count);
    };
    
static void _NXDecodeBytes (_CodingStream *coder, char *buf, int count) {
    NXRead (coder->physical, buf, count);
    };

static void _NXEncodeChar (_CodingStream *coder, signed char c) {
    if (coder->physical) NXPutc (coder->physical, c);
    };
    
static signed char _NXDecodeChar (_CodingStream *coder) {
    return NXGetc (coder->physical);
    };

/* all the following should (of course) be made machine independent */
static void _NXEncodeShort (_CodingStream *coder, short x) {
    if (x>=SMALLESTLABEL && x<=127)
	_NXEncodeChar (coder, (signed char) x);
    else {
	_NXEncodeChar (coder, LONG2LABEL); 
	_NXEncodeBytes (coder, (char *) &x, 2);
	};
    };
    
static short _NXDecodeShort (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    short	x;
    if (ch != LONG2LABEL) return (short) ch;
    _NXDecodeBytes (coder, (char *) &x, 2);
    x = _NXSwapCard2(coder, x);
    return x;
    };
	
static void _NXEncodeInt (_CodingStream *coder, int x) {
    if (x >= SMALLESTLABEL && x <= 127)
	_NXEncodeChar (coder, (signed char) x);
    else if (x >= SMALL_SHORT && x <= BIG_SHORT) {
	short	sh = (short) x;
	_NXEncodeChar (coder, LONG2LABEL);
	_NXEncodeBytes (coder, (char *) &sh, 2);
	}
    else {
	_NXEncodeChar (coder, LONG4LABEL);
	_NXEncodeBytes (coder, (char *) &x, 4);
	};
    };

/* Finishes to decode an int. ch can be only be LONG2LABEL, LONG4LABEL or the int itself. */
static int FinishDecodeInt (_CodingStream *coder, signed char ch) {
    switch (ch) {
    	case LONG2LABEL: {
	    short	x;
	    _NXDecodeBytes (coder, (char *) &x, 2);
	    x = _NXSwapCard2(coder, x);
	    return (int) x;
	    };
	case LONG4LABEL: {
	    int		x;
	    _NXDecodeBytes (coder, (char *) &x, 4);
	    x = _NXSwapCard4(coder, x);
	    return x;
	    };
	default: return (int) ch;
	};
    };
    
static int _NXDecodeInt (_CodingStream *coder) {
    return FinishDecodeInt (coder, _NXDecodeChar (coder));
    };
	
static int FloorOrZero (double x) {
    if (x >= (double) BIG_INT-1) return 0;
    if (x < (double) SMALL_INT+1) return 0;
    return (int) (float) x;
    };
    
static void _NXEncodeFloat (_CodingStream *coder, float x) {
    int		flore = FloorOrZero ((double) x);
    /* we are conservative */
    if ((float) flore == x) _NXEncodeInt (coder, flore); 
    else {
        NXSwappedFloat sf = NXConvertHostFloatToSwapped(x);
	_NXEncodeChar (coder, REALLABEL);
        _NXEncodeBytes (coder, (char *) &sf, sizeof(NXSwappedFloat));
	};
    };

static float _NXDecodeFloat (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    NXSwappedFloat x;
    if (ch != REALLABEL) return (float) FinishDecodeInt (coder, ch);
    _NXDecodeBytes (coder, (char *) &x, sizeof(NXSwappedFloat));
    return _NXSwapFloat(coder, x);
    };
	
static void _NXEncodeDouble (_CodingStream *coder, double x) {
    int		flore = FloorOrZero (x);
    if ((double) flore == x) _NXEncodeInt (coder, flore);
    else {
        NXSwappedDouble sd = NXConvertHostDoubleToSwapped(x);
	_NXEncodeChar (coder, REALLABEL);
        _NXEncodeBytes (coder, (char *) &sd, sizeof(NXSwappedDouble));
	};
    };

static double _NXDecodeDouble (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    NXSwappedDouble	x;
    if (ch != REALLABEL) return (double) FinishDecodeInt (coder, ch);
    _NXDecodeBytes (coder, (char *) &x, sizeof(NXSwappedDouble));
    return _NXSwapDouble(coder, x);
    };
	
static void _NXEncodeChars (_CodingStream *coder, const char *str) {
    int		len;
    if (! str) {_NXEncodeChar (coder, NULLLABEL); return;};
    len = strlen (str);
    _NXEncodeInt (coder, len);
    _NXEncodeBytes (coder, str, len);
    };

static char *_NXDecodeChars (_CodingStream *coder, NXZone *zone) {
    signed char	ch = _NXDecodeChar (coder);
    int		len;
    STR		str;
    if (ch == NULLLABEL) return NULL;
    len = FinishDecodeInt (coder, ch);
    str = (STR) NXZoneMalloc (zone, len+1);
    _NXDecodeBytes (coder, str, len);
    str[len] = '\0';
    return str;
    };
    	
/* Encoding/Decoding of shared quantities.  The trick is that a label int cannot start by encoding NEWLABEL */
static void _NXEncodeSharedString (_CodingStream *coder, const char *str) {
    StrToInt	sti;
    
    if (! str) {_NXEncodeChar (coder, NULLLABEL); return;};
    sti.key = str;
    if (NXHashMember (coder->strings, &sti)) {
	int	value = ((StrToInt *) NXHashGet (coder->strings, &sti))->value;
	_NXEncodeInt (coder, value);
	return;
	}
     else {
	 StrToInt	*new = (StrToInt *) malloc (sizeof (StrToInt));
    	_NXEncodeChar (coder, NEWLABEL); _NXEncodeChars (coder, str);
	new->key = NXCopyStringBuffer(str); 
	new->value = coder->stringCounter++;
	(void) NXHashInsert (coder->strings, new);
	return;
	};
    };
    
static char *_NXDecodeSharedString (_CodingStream *coder) {
    signed char	ch = _NXDecodeChar (coder);
    
    if (ch == NULLLABEL) return NULL;
    if (ch == NEWLABEL) {
	char *s = _NXDecodeChars (coder, coder->scratch);
	STRINGS(coder->strings)[inc_stringCounter(coder)] = s;
	return s;
	};
    return STRINGS(coder->strings)[Bias(FinishDecodeInt(coder, ch))];
    };

static const char *_NXDecodeUniqueString (_CodingStream *coder) {
    return NXUniqueString (_NXDecodeSharedString (coder));
    };
    
static BOOL _NXEncodeShared (_CodingStream *coder, const void *ptr) {
    PtrToInt	pti;
    
    if (! ptr) {
	_NXEncodeChar (coder, NULLLABEL);
	return NO;
	};
    pti.key = ptr;
    if (NXHashMember (coder->ptrs, &pti)) {
	int	value = ((PtrToInt *) NXHashGet (coder->ptrs, &pti))->value;
	_NXEncodeInt (coder, value);
	return NO;
	}
     else {
	 PtrToInt	*new = (PtrToInt *) malloc (sizeof (PtrToInt));
	_NXEncodeChar (coder, NEWLABEL);
	new->key = ptr; new->value = coder->ptrCounter++;
	(void) NXHashInsert (coder->ptrs, new);
	return YES;
	};
    };
    
/*
static BOOL _NXDecodeShared (_CodingStream *coder, void **pptr, int *label) {
    signed char	ch = _NXDecodeChar (coder);
    
    if (ch == NULLLABEL) {
	*pptr = NULL;
	return NO;
	};
    if (ch == NEWLABEL) {
	*label = coder->ptrCounter++;
	return YES;
	}
    else {
	IntToPtr	itp;
	
	*label = FinishDecodeInt (coder, ch);
	itp.key = *label;
	*pptr = ((IntToPtr *) NXHashGet (coder->ptrs, &itp))->value;
	return NO;
	};
    };
*/
    
//?? we have a minor leak here (8 bytes per unarchived Font or Bitmap) that could be fixed by replacing for the entry label the previous value with object, instead of allocating a new entry.  If that was done, we could check in _NXNoteShared that there was no previous value.
/*
static void _NXNoteShared (_CodingStream *coder, void *ptr, int label) {
    IntToPtr	*new = (IntToPtr *) malloc (sizeof (IntToPtr));
    
    if (! ptr) 
    	NX_RAISE (TYPEDSTREAM_INTERNAL_ERROR, 
		"_NXNoteShared: nil shared", (void *) label);
    new->key = label; new->value = ptr;
    new = NXHashInsert (coder->ptrs, new);
    if (new) free (new);
    };
*/
    
static void _NXEncodeString (_CodingStream *coder, const char *str) {
    if (_NXEncodeShared (coder, str)) _NXEncodeSharedString (coder, str);
    };

/* except in the case of nil, always returns a freshly allocated string */
static char *_NXDecodeString (_CodingStream *coder, NXZone *zone) {
  char	*str;
  char ch;
  switch (ch = NXGetc(coder->physical)) {
  case NULLLABEL: return NULL;
  case NEWLABEL: {
    str = _NXDecodeSharedString (coder);
    PTRS(coder->ptrs)[inc_ptrCounter(coder)] = str;
    /* we make a real copy (let's not take risks!) */
    return NXCopyStringBufferFromZone (str, zone);
  }
  default: return NXCopyStringBufferFromZone (PTRS(coder->ptrs)[Bias(FinishDecodeInt(coder, ch))], zone);
  }
}

/***********************************************************************
 *	Creation, destruction and other global operations
 **********************************************************************/

#define FIRSTSTREAMERVERSION	3
#define NXSTREAMERVERSION	4

/* The two following strings are magic: they decide of the byte oredering encoding.  It is very important that they remain small, so that only bytes are written for writting the string themselves */
#define NXSTREAMERNAME		"typedstream"
#define NXNAMESTREAMER		"streamtyped"

/* A typed stream always encodes the type of the information before its value.  Objects are specially coded because first the class is coded (along with the nbytesIndexedVars, then all its data (by sending a write: or read: message is sent to the object, and finally an end of Object token (NULLLABEL). */

NXTypedStream *NXOpenTypedStream (NXStream *physical, int mode) {
    TypedStream		*s;
    
    if (mode != NX_WRITEONLY && mode != NX_READONLY)
    	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, 
		"NXOpenTypedStream: invalid mode", (void *) mode);
    if (! physical)
    	NX_RAISE (TYPEDSTREAM_CALLER_ERROR, 
		"NXOpenTypedStream: null stream", 0);
    s = (TypedStream *) malloc (sizeof (TypedStream));
    s->coder = (mode == NX_WRITEONLY) 
	? _NXOpenEncodingStream (physical) : _NXOpenDecodingStream (physical);
    s->ids = NULL;
    s->write = mode == NX_WRITEONLY;
    s->noteConditionals = NO;
    s->doStatistics = NO;
    s->fileName = NULL;
    s->classVersions = NULL;
    s->objectZone = NXDefaultMallocZone();
    if (s->write) {
	_NXEncodeChar (s->coder, NXSTREAMERVERSION);
	_NXEncodeChars (s->coder, (sameSexAsCube()) ? NXSTREAMERNAME : NXNAMESTREAMER);
	_NXEncodeInt (s->coder, NXSYSTEMVERSION);
	s->streamerVersion = NXSTREAMERVERSION;
	s->systemVersion = NXSYSTEMVERSION;
	}
    else {
    	STR	header;
	
	s->streamerVersion = _NXDecodeChar (s->coder);
	if (s->streamerVersion < FIRSTSTREAMERVERSION ||
		s->streamerVersion > NXSTREAMERVERSION) {
	    /* reading an old version or reading a non-archive file */
	    NXUngetc (s->coder->physical);
	    NXCloseTypedStream (s);
	    return NULL;
	    };
	header = _NXDecodeChars (s->coder, s->coder->scratch);
	if (strcmp (header, NXSTREAMERNAME) == 0)
	    s->coder->swap = ! sameSexAsCube();
	else if (strcmp (header, NXNAMESTREAMER) == 0)
	    s->coder->swap = sameSexAsCube();
	else {
	    /* we are not reading a typedstream file! */
	    NXCloseTypedStream (s);
	    return NULL;
	    };
	/* we are now ready for int reading */
	s->systemVersion = _NXDecodeInt (s->coder);
	s->classVersions = NXCreateHashTableFromZone (
	    NXStrStructKeyPrototype, 0, NULL, s->coder->scratch);
	free (header);
	};
    return s;
    };

extern void NXSetTypedStreamZone(NXTypedStream *stream, NXZone *zone)
{
    TypedStream	*s = (TypedStream *) stream;
    s->objectZone = zone;
}

extern NXZone *NXGetTypedStreamZone(NXTypedStream *stream)
{
    TypedStream	*s = (TypedStream *) stream;
    return s->objectZone;
}

BOOL NXEndOfTypedStream (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    checkRead (s);
    return _NXEndOfCodingStream (s->coder);
    };
    
void NXFlushTypedStream (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    if (s->coder->physical) NXFlush (s->coder->physical);
    };

void NXCloseTypedStream (NXTypedStream *stream) {
    int saveToFile = 0;
    TypedStream	*s = (TypedStream *) stream;
    if (s->write) NXFlushTypedStream (stream);
    if (s->classVersions) NXFreeHashTable (s->classVersions);
    if (s->fileName) {
	NXStream	*physical;
	physical = s->coder->physical;
	s->coder->physical = NULL;
	if (s->write) saveToFile = NXSaveToFile (physical, s->fileName);
	NXCloseMemory (physical, NX_FREEBUFFER);
	};
    _NXCloseCodingStream (s->coder);
    if (s->ids) NXFreeHashTable (s->ids);
    free (s);
    if (saveToFile) NX_RAISE (TYPEDSTREAM_CALLER_ERROR, "file cannot be saved", NULL);
    };

/***********************************************************************
 *	Writing and reading arbitrary data: type string utilities
 **********************************************************************/

// Handle both new- and old-style aggregate type descriptors.
// One of "{iiii}", "{foo=iiii}", or "{?=iiii}".

static const char *skipAggregateName (const char *string)
{
  const char *type = string;
  
  while (1)
    switch (*type++)
      {
      case '=':		// Succeeded: end of aggregate name
	return type;
	
      case '}':		// Failed: end of struct descriptor
      case '{':		// Failed: start of nested struct descriptor
      case ')':		// Failed: end of union descriptor
      case '(':		// Failed: start of nested union descriptor
      case '\0':	// Failed: end of descriptor!
	return string;
      }
}

static inline unsigned int roundUp (unsigned int size, unsigned int align)
{
  return align * ((size + align - 1) / align);
}

static inline unsigned int max (unsigned int x, unsigned int y)
{
  if (x >= y)
    return x;
  else
    return y;
}

// Some machines enforce a minimum structure alignment.  This should
// parallel the STRUCTURE_SIZE_BOUNDARY macro in the GNU compiler.

#if defined (m68k) && defined (NeXT)
#define MIN_STRUCT_ALIGN 2
#else
#define MIN_STRUCT_ALIGN 1
#endif

// Store the size and alignment of "type" into "sizePtr" and "alignPtr".

static const char *
SizeOfType (const char *type, unsigned int *sizePtr, unsigned int *alignPtr)
{
  char c = *type++;
  
  switch (c)
    {
    case 'c':
    case 'C':
      *sizePtr = sizeof (char);
      *alignPtr = __alignof (char);
      break;
      
    case 's':
    case 'S':
      *sizePtr = sizeof (short);
      *alignPtr = __alignof (short);
      break;
      
    case 'i':
    case 'I':
    case '!':
      *sizePtr = sizeof (int);
      *alignPtr = __alignof (int);
      break;
      
    case 'l':
    case 'L':
      *sizePtr = sizeof (long);
      *alignPtr = __alignof (long);
      break;
      
    case 'f':
      *sizePtr = sizeof (float);
      *alignPtr = __alignof (float);
      break;
      
    case 'd':
      *sizePtr = sizeof (double);
      *alignPtr = __alignof (double);
      break;
      
    case '@':
      *sizePtr = sizeof (id);
      *alignPtr = __alignof (id);
      break;
      
    case '*':
    case '%':
      *sizePtr = sizeof (char *);
      *alignPtr = __alignof (char *);
      break;
      
    case ':':
      *sizePtr = sizeof (SEL);
      *alignPtr = __alignof (SEL);
      break;
      
    case '#':
      *sizePtr = sizeof (Class);
      *alignPtr = __alignof (Class);
      break;
      
    case '[':
      {
	unsigned int count = 0;
	unsigned int size, align;
	
	while ('0' <= *type && *type <= '9')
	  count = 10 * count + (*type++ - '0');
	
	type = SizeOfType (type, &size, &align);
	
	*sizePtr = count * roundUp (size, align);
	*alignPtr = align;
	
	c = *type++;
	if (c != ']')
	  typeDescriptorError (c, "missing ']' in type descriptor");
	
	break;
      }
      
    case '{':
      {
        unsigned int structSize = 0;
	unsigned int structAlign = MIN_STRUCT_ALIGN;
	
	type = skipAggregateName (type);
	
	while (*type != '}')
	  {
	    unsigned int size, align;
	    
	    type = SizeOfType (type, &size, &align);
	    
	    structSize = roundUp (structSize, align);
	    structSize += size;
	    structAlign = max (structAlign, align);
	  }
	
	*sizePtr = roundUp (structSize, structAlign);
	*alignPtr = structAlign;
	
	type++;
	break;
      }
      
    case '(':
      {
        unsigned int unionSize = 0;
	unsigned int unionAlign = 1;
	
	type = skipAggregateName (type);
	
	while (*type != ')')
	  {
	    unsigned int size, align;
	    
	    type = SizeOfType (type, &size, &align);
	    
	    unionSize = max (unionSize, size);
	    unionAlign = max (unionAlign, align);
	  }
	
	*sizePtr = roundUp (unionSize, unionAlign);
	*alignPtr = unionAlign;
	
	type++;
	break;
      }
      
    default:
      typeDescriptorError (c, "unknown type descriptor");
    }
  
  return type;
}


/***********************************************************************
 *	Writing and reading arbitrary data: the real Write/Read of values
 **********************************************************************/

// Write one object described by "type" and store it into "arg".
// Returns the unread portion of "type".

static const char *
WriteValue (NXTypedStream *stream, const char *type, const void *arg)
{
  TypedStream *s = (TypedStream *) stream;
  char c = *type++;
  
  switch (c)
    {
    case 'c':
    case 'C':
      _NXEncodeChar (s->coder, *((signed char *) arg));
      break;
      
    case 's':
    case 'S': 
      _NXEncodeShort (s->coder, *((short *) arg));
      break;
      
    case 'i':
    case 'I':
    case 'l':
    case 'L':
      _NXEncodeInt (s->coder, *((int *) arg));
      break;
      
    case 'f': 
      _NXEncodeFloat (s->coder, *((float *) arg));
      break;
      
    case 'd':
      _NXEncodeDouble (s->coder, *((double *) arg));
      break;
      
    case '@':
      InternalWriteObject (s, *((id *) arg));
      break;
      
    case '*':
      _NXEncodeString (s->coder, *((const char **) arg));
      break;
      
    case '%':
      _NXEncodeSharedString (s->coder, *((const char **) arg));
      break;
      
    case ':':
      _NXEncodeSharedString (s->coder, sel_getName (*((SEL *) arg)));
      break;
      
    case '#': 
      NXWriteClass (stream, *((Class *) arg));
      break;
      
    case '[':
      {
	unsigned int size, align;
	unsigned int i, count = 0;
	const char *elementType;
	
	while ('0' <= *type && *type <= '9')
	  count = 10 * count + (*type++ - '0');
	
	elementType = type;
	type = SizeOfType (elementType, &size, &align);
	
	for (i = 0; i < count; i++)
	  WriteValue (stream, elementType, ((char *) arg) + (i * size));
	
	c = *type++;
	if (c != ']')
	  typeDescriptorError (c, "missing ']' in type descriptor");
	
	break;
      }
      
    case '{':
      {
        unsigned int offset = 0;
	
	type = skipAggregateName (type);
	
	while (*type != '}')
	  {
	    unsigned int size, align;
	    
	    SizeOfType (type, &size, &align);
	    
	    offset = roundUp (offset, align);
	    
	    type = WriteValue (stream, type, ((char *) arg) + offset);
	    
	    offset += size;
	  }
	
	type++;
	break;
      }
      
    case '(':
      {
	unsigned int i, size, align;
	const char *newType;
	
	type = skipAggregateName (type);
	
	type = SizeOfType (type - 1, &size, &align);
	
	for (i = 0; i < size; i++)
	  newType = WriteValue (stream, "C", ((char *) arg) + i);
	
	break;
      }
      
    case '!': 
      break;
      
    default:
      typeDescriptorError (c, "unknown type descriptor");
    }
  
  return type;
}


// Read one object described by "type" ponted to by "arg".
// Returns the unread portion of "type".

static const char *
ReadValue (NXTypedStream *stream, const char *type, void *arg)
{
  TypedStream *s = (TypedStream *) stream;
  char c = *type++;
  
  switch (c)
    {
    case 'c':
    case 'C':
      *((char *) arg) = _NXDecodeChar (s->coder);
      break;
      
    case 's':
    case 'S':
      *((short *) arg) = _NXDecodeShort (s->coder);
      break;
      
    case 'i':
    case 'I':
    case 'l':
    case 'L':
      *((int *) arg) = _NXDecodeInt (s->coder);
      break;
      
    case 'f':
      *((float *) arg) = _NXDecodeFloat (s->coder);
      break;
      
    case 'd':
      *((double *) arg) = _NXDecodeDouble (s->coder);
      break;
      
    case '@':
      *((id *) arg) = InternalReadObject (s);
      break;
      
    case '*':
      *((const char **) arg) = _NXDecodeString (s->coder, s->objectZone);
      break;
      
    case '%':
      *((const char **) arg) = _NXDecodeUniqueString (s->coder);
      break;
      
    case ':':
      *((SEL *) arg) = sel_registerName (_NXDecodeSharedString (s->coder));
      break;
      
    case '#':
      *((Class *) arg) = NXReadClass (s);
      break;
      
    case '[':
      {
	unsigned int size, align;
	unsigned int i, count = 0;
	const char *elementType;
	
	while ('0' <= *type && *type <= '9')
	  count = 10 * count + (*type++ - '0');
	
	elementType = type;
	type = SizeOfType (elementType, &size, &align);
	
	for (i = 0; i < count; i++)
	  ReadValue (stream, elementType, ((char *) arg) + (i * size));
	
	c = *type++;
	if (c != ']')
	  typeDescriptorError (c, "missing ']' in type descriptor");
	
	break;
      }
      
    case '{':
      {
        unsigned int offset = 0;
	
	type = skipAggregateName (type);
	
	while (*type != '}')
	  {
	    unsigned int size, align;
	    
	    SizeOfType (type, &size, &align);
	    
	    offset = roundUp (offset, align);
	    
	    type = ReadValue (stream, type, ((char *) arg) + offset);
	    
	    offset += size;
	  }
	
	type++;
	break;
      }
      
    case '(':
      {
	unsigned int i, size, align;
	
	type = skipAggregateName (type);
	
	type = SizeOfType (type - 1, &size, &align);
	
	for (i = 0; i < size; i++)
	  ReadValue (stream, "C", ((char *) arg) + i);
	
	break;
      }
      
    case '!': 
      break;
      
    default:
      typeDescriptorError (c, "unknown type descriptor");
    }
  
  return type;
}

/***********************************************************************
 *	Writing and reading arbitrary data: API functions
 **********************************************************************/

void NXWriteType (NXTypedStream *stream, const char *type, const void *data)
{
  TypedStream *s = (TypedStream *) stream;
  
  checkWrite (s);
  _NXEncodeSharedString (s->coder, type);
  type = WriteValue (stream, type, data);
  if (*type)
    typeDescriptorError (*type, "excess characters in type descriptor");
}

void NXReadType (NXTypedStream *stream, const char *type, void *data)
{
  TypedStream *s = (TypedStream *) stream;
  const char *readType;
  
  checkRead (s);
  readType = _NXDecodeSharedString (s->coder);
  checkExpected (readType, type);
  type = ReadValue (stream, type, data);
  if (*type)
    typeDescriptorError (*type, "excess characters in type descriptor");
}
    
// NXWriteTypes() should be changed to write the type of each component
// before that component rather than writing all the types followed
// by all of the components.  This way the data could be read back with
// a series of NXReadType() calls or a single NXReadTypes() call.

void NXWriteTypes (NXTypedStream *stream, const char *type, ...)
{
  TypedStream *s = (TypedStream *) stream;
  va_list ap;
  
  checkWrite (s);
  _NXEncodeSharedString (s->coder, type);
  va_start (ap, type);
  
  while (*type)
    type = WriteValue (stream, type, va_arg (ap, void *));
  
  va_end (ap);
}

void NXReadTypes (NXTypedStream *stream, const char *type, ...)
{
  TypedStream *s = (TypedStream *) stream;
  const char *readType;
  va_list ap;
  
  checkRead (s);
  readType = _NXDecodeSharedString (s->coder);
  checkExpected (readType, type);
  va_start (ap, type);
  
  while (*type)
    type = ReadValue (stream, type, va_arg (ap, void *));
  
  va_end (ap);
}
    
/***********************************************************************
 *	Conveniences for writing and reading common types of data.
 **********************************************************************/
 
void NXWriteArray (NXTypedStream *stream, const char *type, int count, const void *data)
{
  TypedStream *s = (TypedStream *) stream;
  char *arrayType = malloc (15 + strlen (type));	/* enough room */
  
  checkWrite (s);
  sprintf (arrayType, "[%d%s]", count, type);
  _NXEncodeSharedString (s->coder, arrayType);
  free (arrayType);
  
  if (*type == 'c' && *(type+1) == 0)
    {
      _NXEncodeBytes (s->coder, data, count);
      type++;
    }
  else
    {
      unsigned int size, align, i;
      const char *elementType = type;
      char descriptorToUse[ strlen( elementType ) + 3];
      
      if ( *elementType && *elementType != '{' && *(elementType+1) ) {
        strcpy( descriptorToUse, "{" );
        strcat( descriptorToUse, elementType );
        strcat( descriptorToUse, "}" );
        elementType = descriptorToUse;
      }
      
      type = SizeOfType (elementType, &size, &align);

      for (i = 0; i < count; i++)
	WriteValue (stream, elementType, ((char *) data) + (i * size));
    }
  
  if (*type)
    typeDescriptorError (*type, "excess characters in type descriptor");
}

void NXReadArray (NXTypedStream *stream, const char *type, int count, void *data)
{
  TypedStream *s = (TypedStream *) stream;
  const char *readType;
  char	*arrayType = malloc (15 + strlen (type));	/* enough room */
  
  checkRead (s);
  sprintf (arrayType, "[%d%s]", count, type);
  readType = _NXDecodeSharedString (s->coder);
  checkExpected (readType, arrayType);
  free (arrayType);
  
  if (*type == 'c' && *(type+1) == 0)
    {
      _NXDecodeBytes (s->coder, data, count);
      type++;
    }
  else
    {
      unsigned int size, align, i;
      const char *elementType = type;
      char descriptorToUse[ strlen( elementType ) + 3];

      if ( *elementType && *elementType != '{' && *(elementType+1) ) {
        strcpy( descriptorToUse, "{" );
        strcat( descriptorToUse, elementType );
        strcat( descriptorToUse, "}" );
        elementType = descriptorToUse;
      }

      type = SizeOfType (elementType, &size, &align);

      for (i = 0; i < count; i++)
	ReadValue (stream, elementType, ((char *) data) + (i * size));
    }
  
  if (*type)
    typeDescriptorError (*type, "excess characters in type descriptor");
}

/* Note, the writing of the class should be moved to [Object write].
   This way an object can choose to write another object in its place.
   No change was made for 3.0 since it a sensitive area and we were able
   to work around the problem in NXConstantString.  This should definitley
   be reexamined for 4.0!  (mself 10/8/91) */

static void InternalWriteObject (TypedStream *s, id object) {
    if ([object respondsTo:@selector(startArchiving:)])
	object = [object startArchiving: s];
    if (s->noteConditionals) {
	if (NXHashMember (s->ids, object)) return; /* already visited */
	(void) NXHashInsert (s->ids, object);
	};
    if (_NXEncodeShared (s->coder, object)) {
	int	addr = 0;
	Class	class = object->isa;
	NXWriteClass (s, class);
	if (! s->noteConditionals && s->doStatistics)
	    addr = NXTell(s->coder->physical);
	/* a misfeature in the first version */
	if (s->streamerVersion == FIRSTSTREAMERVERSION)
	    _NXEncodeInt (s->coder, 0); 
	[object write: s];
	_NXEncodeChar (s->coder, EOOLABEL);
	if (! s->noteConditionals && s->doStatistics)
	    printf ("\tWritten %s object %s: %ld bytes\n", 
		[[object class] name], [object name], 
		NXTell (s->coder->physical)-addr);
	};
    };
    
void NXWriteObject (NXTypedStream *stream, id object) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    if (s->streamerVersion > FIRSTSTREAMERVERSION)
	_NXEncodeSharedString (s->coder, "@");
    InternalWriteObject (s, object);
    };
    
static id InternalReadObject (TypedStream *s) {
  id object;
  char ch;
  switch (ch = NXGetc(s->coder->physical)) {
  case NULLLABEL: return NULL;
  case NEWLABEL: {
	Class	class;
	int     size;
	int label = inc_ptrCounter(s->coder);
	class = NXReadClass (s);
	if (s->streamerVersion == FIRSTSTREAMERVERSION)
	    size = _NXDecodeInt (s->coder);
	if (! class) classError ("NULL", "found null class");
#if 0
	/* We would like to use allocFromZone: so that objects
	   may place themselves in special zones.  Before making
	   this change we must ensure that all objects which ever
	   appear in archives have a valid allocFromZone: method.  */
	object = [class allocFromZone: s->objectZone];
#else
	object = class_createInstanceFromZone (class, 0, s->objectZone);
#endif
	PTRS(s->coder->ptrs)[label] = object;
	[object read: s];
	[object awake];
	if ([object respondsTo:@selector(finishUnarchiving)]) {
	    id		new = [object finishUnarchiving];
	    if (new) {
	    	object = new;
		PTRS(s->coder->ptrs)[label] = object;
		};
	    };
	if (_NXDecodeChar (s->coder) != EOOLABEL) 
	    NX_RAISE (TYPEDSTREAM_FILE_INCONSISTENCY, 
	     "NXReadObject: inconsistency between written data and read:", 0); 
	return object;
	};
  default: return PTRS(s->coder->ptrs)[Bias(FinishDecodeInt(s->coder, ch))];
  }
}

id NXReadObject (NXTypedStream *stream) {
    const char	*readType;
    TypedStream	*s = (TypedStream *) stream;
    
    checkRead (s);
    if (s->streamerVersion > FIRSTSTREAMERVERSION) {
	readType = _NXDecodeSharedString (s->coder);
        checkExpected (readType, "@");
	};
    return InternalReadObject (s);
    };
    
//?? We hack around % classes (pose as?)
static Class RealSuperClass (Class class) {
    while ((class = class->super_class) && (class->name[0]=='%')) {};
    return class;
    };
    
static void NXWriteClass (NXTypedStream *stream, Class class) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    while (_NXEncodeShared (s->coder, class)) {
    	_NXEncodeSharedString (s->coder, class->name);
	_NXEncodeInt (s->coder, class->version);
	//?? Hack for skipping PoseAs classes (indicated by %)
	class = RealSuperClass (class);
	};
    };

static Class NXReadClass (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    char ch;
    
    checkRead (s);
    switch (ch = NXGetc(s->coder->physical)) {
    case NULLLABEL: return NULL;
    case NEWLABEL: {
      const char	*className = _NXDecodeSharedString (s->coder);
      int		version = _NXDecodeInt (s->coder);
      Class		superClass;
      Class		class = (Class) objc_getClass ((char *) className);
      _ClassVersion	*cv = (_ClassVersion *) 
	   NXZoneMalloc (s->coder->scratch, sizeof(_ClassVersion));
      /* explicit class initialization */
      (void) [(id) class self];
	cv->className = className; cv->version = version;
	(void) NXHashInsert (s->classVersions, cv);
      if (! class) classError (className, "class not loaded");
      PTRS(s->coder->ptrs)[inc_ptrCounter(s->coder)] = class;
      superClass = NXReadClass (s);
      if (superClass != RealSuperClass (class)) 
	classError (className, "wrong super class");
      return class;
    };
    default: return PTRS(s->coder->ptrs)[Bias(FinishDecodeInt(s->coder, ch))];
    }
  };
     

/***********************************************************************
 *	Writing and reading back pointers.
 **********************************************************************/

void NXWriteRootObject (NXTypedStream *stream, id object) {
    TypedStream	*s = (TypedStream *) stream;
    _CodingStream	*olds;
    
    checkWrite (s);
    if (s->noteConditionals) 
	writeRefError ("NXWriteRootObject: already done");
    s->noteConditionals = YES;
    if (! s->ids) 
	s->ids = NXCreateHashTable (NXPtrPrototype, 0, NULL);
    olds = s->coder;
    s->coder = _NXOpenEncodingStream ((NXStream *) nil);
    NXWriteObject (stream, object);
    _NXCloseCodingStream (s->coder);
    s->coder = olds;
    s->noteConditionals = NO;
    NXWriteObject (stream, object);
    NXFlushTypedStream (stream);
    };

void NXWriteObjectReference (NXTypedStream *stream, id object) {
    TypedStream	*s = (TypedStream *) stream;
    
    checkWrite (s);
    if (s->noteConditionals) return;
    if (! s->ids) 
	writeRefError ("NXWriteObjectReference: NXWriteRootObject has not been previously done");
    if (! object) {NXWriteObject (stream, nil); return;};
    NXWriteObject (stream, (NXHashMember (s->ids, object)) ? object : nil);
    };

/***********************************************************************
 *	Conveniences for writing and reading files and buffers.
 **********************************************************************/
 
NXTypedStream *NXOpenTypedStreamForFile (const char *fileName, int mode) {
    TypedStream	*s;
    NXStream		*physical;
    int fd;
    switch (mode) {
	case NX_WRITEONLY: 
            /* workaround NXMapFile bug */
            fd = open (fileName, O_CREAT | O_WRONLY, 0666);
            if (fd < 0) return NULL;
            close(fd);
	    physical = NXOpenMemory (NULL, 0, NX_WRITEONLY);
	    break;
	case NX_READONLY:
	    physical = NXMapFile (fileName, mode);
	    if (! physical) return NULL;
	    break;
	default: NX_RAISE (TYPEDSTREAM_CALLER_ERROR, 
		"NXOpenTypedStreamForFile: invalid mode", (void *) mode);
	};
   s = (TypedStream *) NXOpenTypedStream (physical, mode);
   if (! s) return NULL;
   s->fileName = fileName;
   return s;
   };

char *NXWriteRootObjectToBuffer (id object, int *length) {
    char 		*buffer;
    int 		max;
    NXStream		*physical = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    NXTypedStream	*stream = NXOpenTypedStream (physical, NX_WRITEONLY);
    NXWriteRootObject (stream, object);
    NXCloseTypedStream (stream);
    NXGetMemoryBuffer(physical, &buffer, length, &max);
    NXCloseMemory(physical, NX_TRUNCATEBUFFER);
    return buffer;
    };

id NXReadObjectFromBuffer (const char *buffer, int length) {
    return NXReadObjectFromBufferWithZone(buffer, length, NXDefaultMallocZone());
    };

id NXReadObjectFromBufferWithZone (const char *buffer, int length, NXZone *zone) {
    NXStream		*physical = NXOpenMemory (buffer, length, NX_READONLY);
    NXTypedStream	*stream = NXOpenTypedStream (physical, NX_READONLY);
    id			object;
    
    if (!stream)
      NX_RAISE (TYPEDSTREAM_CALLER_ERROR, "NXOpenTypedStream: null stream", 0);
    NXSetTypedStreamZone(stream, zone);
    object = NXReadObject (stream);
    NXCloseTypedStream (stream);
    NXCloseMemory(physical, NX_SAVEBUFFER);
    return object;
    };

void NXFreeObjectBuffer (char *buffer, int length) {
    (void) vm_deallocate (task_self(), (vm_address_t) buffer, length);
    };


/***********************************************************************
 *	Dealing with versions
 **********************************************************************/

int NXSystemVersion (NXTypedStream *stream) {
    TypedStream	*s = (TypedStream *) stream;
    return s->systemVersion;
    };

int NXTypedStreamClassVersion (NXTypedStream *stream, const char *className) {
    _ClassVersion	pseudo;
    _ClassVersion	*original;
    TypedStream		*s = (TypedStream *) stream;
    checkRead (s);
    pseudo.className = className;
    original = (_ClassVersion *) NXHashGet (s->classVersions, &pseudo);
    if (!original) {
        /* Hack to allow the renaming of the appkit classes to NXFoo's */
	if (*className && (*className == 'N') 
	    && *(className + 1) && (*(className + 1) == 'X')
	    && *(className + 2)) {
	   pseudo.className = className + 2;
	   original = (_ClassVersion *) NXHashGet (s->classVersions, &pseudo);
	}
    }
    return original ? original->version : -1;
    };

BOOL NXTypedStreamIsSwapped (NXTypedStream *stream) {
   TypedStream		*s = (TypedStream*) stream;
   return s->coder->swap;
}

#if 0

static void NXPrintTypes (NXTypedStream *stream, NXStream *output);

static char *SkipClass (NXTypedStream *stream)
{
  TypedStream *s = (TypedStream *) stream;
  char ch;
  
  checkRead (s);
  switch (ch = NXGetc (s->coder->physical))
    {
    case NULLLABEL:
      return "Nil";
      
    case NEWLABEL:
      {
	char *className = _NXDecodeSharedString (s->coder);
	int version = _NXDecodeInt (s->coder);
        int label = inc_ptrCounter (s->coder);
	
	PTRS (s->coder->ptrs)[label] = className;
	SkipClass (s);
	return className;
      }
      
    default:
      {
        int label = Bias (FinishDecodeInt (s->coder, ch));
	char *className = PTRS (s->coder->ptrs)[label];
	
	return className;
      }
    }
}

static void NXPrintClass (NXTypedStream *stream, NXStream *output)
{
  NXPrintf (output, "(%s)", SkipClass (stream));
}


static void InternalPrintObject (TypedStream *s, NXStream *output)
{
  char ch;
  
  switch (ch = NXGetc (s->coder->physical))
    {
    case NULLLABEL:
      NXPrintf (output, "nil");
      return;
    
    case NEWLABEL:
      {
        int label = inc_ptrCounter (s->coder);
	char *className = SkipClass (s);
	
	NXPrintf (output, "(%s#%d = ", className, label);
	PTRS (s->coder->ptrs)[label] = className;
	if (s->streamerVersion == FIRSTSTREAMERVERSION)
	  _NXDecodeInt (s->coder);
	while ((ch = NXGetc (s->coder->physical)) != EOOLABEL)
	  {
	    if (ch == NEWLABEL)
	      {
		NXUngetc (s->coder->physical);
		NXPrintTypes (s, output);
	      }
	    else
	      NXPrintf (output, "?");
	  }
	NXPrintf (output, ")");
	return;
      }
    default:
      {
        int label = Bias (FinishDecodeInt (s->coder, ch));
	const char *className = PTRS (s->coder->ptrs)[label];
	
	NXPrintf (output, "(%s#d)", className, label);
	return;
      }
    }
}


// Print one object described by "type".
// Returns the unread portion of "type".

static const char *
PrintValue (NXTypedStream *stream, const char *type, NXStream *output)
{
  TypedStream *s = (TypedStream *) stream;
  char c = *type++;
  
  switch (c)
    {
    case 'c':
      NXPrintf (output, "%d", _NXDecodeChar (s->coder));
      break;
      
    case 'C':
      NXPrintf (output, "%u", _NXDecodeChar (s->coder));
      break;
      
    case 's':
      NXPrintf (output, "%d", _NXDecodeShort (s->coder));
      break;
      
    case 'S':
      NXPrintf (output, "%u", _NXDecodeShort (s->coder));
      break;
      
    case 'i':
    case 'l':
      NXPrintf (output, "%d", _NXDecodeInt (s->coder));
      break;
      
    case 'I':
    case 'L':
      NXPrintf (output, "%u", _NXDecodeInt (s->coder));
      break;
      
    case 'f':
      NXPrintf (output, "%g", _NXDecodeFloat (s->coder));
      break;
      
    case 'd':
      NXPrintf (output, "%g", _NXDecodeDouble (s->coder));
      break;
      
    case '@':
      InternalPrintObject (s, output);
      break;
      
    case '*':
      NXPrintf (output, "\"%s\"", _NXDecodeString (s->coder, s->objectZone));
      break;
      
    case '%':
      NXPrintf (output, "\"%s\"", _NXDecodeUniqueString (s->coder));
      break;
      
    case ':':
      NXPrintf (output, "\"%s\"", _NXDecodeSharedString (s->coder));
      break;
      
    case '#':
      NXPrintClass (s, output);
      break;
      
    case '[':
      {
	unsigned int size, align;
	unsigned int i, count = 0;
	const char *elementType;
	
	while ('0' <= *type && *type <= '9')
	  count = 10 * count + (*type++ - '0');
	
	elementType = type;
	type = SizeOfType (elementType, &size, &align);
	
	NXPrintf (output, "[");
	for (i = 0; i < count; i++)
	  {
	    PrintValue (stream, elementType, output);
	    if (i + 1 < count)
	      NXPrintf (output, ", ");
	  }
	NXPrintf (output, "]");
	
	c = *type++;
	if (c != ']')
	  typeDescriptorError (c, "missing ']' in type descriptor");
	
	break;
      }
      
    case '{':
      {
        unsigned int offset = 0;
	
	type = skipAggregateName (type);
	
	NXPrintf (output, "{");
	while (*type != '}')
	  {
	    unsigned int size, align;
	    
	    SizeOfType (type, &size, &align);
	    
	    offset = roundUp (offset, align);
	    
	    type = PrintValue (stream, type, output);
	    
	    offset += size;
	    
	    if (*type != '}')
	      NXPrintf (output, ", ");
	  }
	NXPrintf (output, "}");
	
	type++;
	break;
      }
      
    case '(':
      {
	unsigned int i, size, align;
	
	type = skipAggregateName (type);
	
	type = SizeOfType (type - 1, &size, &align);
	
	NXPrintf (output, "(");
	for (i = 0; i < size; i++)
	  {
	    PrintValue (stream, "C", output);
	    if (i + 1 < size)
	      NXPrintf (output, ", ");
	  }
	NXPrintf (output, ")");
	
	break;
      }
      
    case '!': 
      break;
      
    default:
      typeDescriptorError (c, "unknown type descriptor");
    }
  
  return type;
}

static void NXPrintTypes (NXTypedStream *stream, NXStream *output)
{
  TypedStream *s = (TypedStream *) stream;
  const char *readType;
  
  checkRead (s);
  readType = _NXDecodeSharedString (s->coder);
  
  while (*readType)
    {
      readType = PrintValue (stream, readType, output);
      if (*readType)
	NXPrintf (output, ", ");
    }
}

void main (int argc, char *argv[])
{
  int i;
  NXStream *output = NXOpenFile (fileno (stdout), NX_WRITEONLY);
  
  for (i = 1; i < argc; i++)
    {
      NXTypedStream *stream = NXOpenTypedStreamForFile (argv[i], NX_READONLY);
      
      NXPrintTypes (stream, output);
    }
}

#endif
#endif /* KERNEL */