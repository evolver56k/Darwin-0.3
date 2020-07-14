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
/*	NXString.h
	Copyright 1991, 1992, 1993 NeXT, Inc. All rights reserved.
*/
	
#ifndef _OBJC_NXSTRING_H_
#define _OBJC_NXSTRING_H_

#import "Object.h"
#import <limits.h>
#import "hashtable.h"
#import "typedstream.h"
#import "unichar.h"
#import <objc/zone.h>
#import "NXCharacterSet.h"

#define NX_MAX_STRING_LENGTH	(INT_MAX-1)
#define NX_STRING_NOT_FOUND	NX_MAX_STRING_LENGTH
#define NX_HASH_STRING_LENGTH	63


/* Byte returned when the contents of the string is requested as bytes and a character cannot be represented as bytes.
*/
#define NX_UNREPRESENTABLE_CHARACTER 0x0ff


/* NXRange is more general purpose than NXString and can be used in other situations. NOTE: Instead of accessing the .loc and .len fields of an NXRange directly, please use the NX_LENGTH and NX_LOCATION macros provided below. The names of these fields will be changed when NXString becomes public in a future release.
*/
typedef struct _NXRange {
    unsigned int loc;	// Location. USE THE NX_LOCATION() MACRO WHEN ACCESSING THIS FIELD!
    unsigned int len;	// Length of range. USE THE NX_LENGTH() MACRO WHEN ACCESSING THIS FIELD!
} NXRange;

/* Use these macros when accessing fields of an NXRange. The field names will be changed in a future release.
*/
#define NX_LENGTH(r) (r).len
#define NX_LOCATION(r) (r).loc


/* NXComparisonResult is used for ordered comparision results; it's meant to be more general than NXString. If the first argument to the comparision (the receiving object in a message call or the left argument in a function call) is greater than the second, NX_OrderedDescending is returned.  If it is smaller, NX_OrderedAscending is returned. Examples:
  [@"ZZTop" compare:@"ACDC"] returns NX_OrderedDescending
  compareInts (1, 7) returns NX_OrderedAscending
*/
typedef enum _NXComparisonResult {NX_OrderedAscending = -1, NX_OrderedSame, NX_OrderedDescending} NXComparisonResult;


/* Flags passed to compare & find: With a zero mask passed in, the comparisions are case sensitive, strings are assumed not to be normalized (ie, floating and non floating diacritics are both used)
*/
#define NX_CASE_INSENSITIVE		1
#define NX_FLOATING_DIACRITICS		2	// Useful for already normalized strings
#define NX_BACKWARDS_SEARCH		4	// Search backwards in the range (only with find)


/* Functions to provide subclassers with basic find and compare functions
*/
extern NXComparisonResult NXCompareCharacters (const unichar *first, const unichar *second, unsigned firstLen, unsigned secondLen, unsigned flagMask, void *table);

extern NXRange NXFindCharacters (const unichar *findStr, const unichar *inStr, unsigned findStrLen, unsigned inStrLen, unsigned flagMask, void *table);


/* The following function will hash the specified character string upto NX_HASH_STRING_LENGTH characters. Note that the provided length should be actual length of the string in question, although it's enough for the buffer to point to the first NX_HASH_STRING_LENGTH characters... ??? Need to document the hash function for subclassers.
*/
extern unsigned NXHashCharacters(const unichar *buffer, unsigned length);



/* Abstract superclasses
*/
@interface NXString : Object


/* initFromCharactersNoCopy:length: is the designated initializer for NXString. It is called by all other init methods in NXString. This method does nothing except call [super init]. (It can't do anything else, it doesn't know the implementation!). 

ALL SUBCLASSERS OF NXSTRING NEED TO IMPLEMENT THIS METHOD and have it call the version in NXString. If you need to provide a different designated initializer for your subclass, then you should still implement this method and have it call your new initializer, which should in turn class this method in NXString.

The chars argument points to an array of unichars. The array is not zero-terminated (as is the case with most C-strings); the length argument determines the number of unichars.

The receiver can simply reference the buffer, and free it when it no longer needs it. If the receiver cannot reference the buffer but has to create its own, then it should free the supplied buffer before returning from this method. When this method is called, the caller is guaranteeing that the supplied buffer will not go away and the receiver is free to do whatever it wants with it.
*/
- initFromCharactersNoCopy:(unichar *)chars length:(unsigned int)length;

/* init creates an empty string
*/
- init;

/* Create a string from the provided characters. By default this method makes a copy of the provided buffer and calls initFromCharactersNoCopy:length:.
*/
- initFromCharacters:(const unichar *)chars length:(unsigned)len;

/* The next two deal with zero-terminated or counted C-strings. The one with the length will not stop at the null character but rather get the full number of bytes.
*/
- initFromCString:(const char *)zeroTerminatedBytes;
- initFromCString:(const char *)bytes length:(unsigned)len;

/* The next two methods create instances from other strings.
*/
- initFromString:string;
- initFromString:string range:(NXRange)range;

/* Create a string from the printf-like format string and arguments. The format string is a string object.
*/
- initFromFormat:(NXString *)format, ...;
- initFromFormat:(NXString *)format withArgList:(va_list)argList;

/* Create a string either a character in the specified set is reached or len characters have been read. set might be nil, in which case the len argument determines the string length. You can also set len to NX_MAX_STRING_LENGTH. This method might raise on a stream error.
*/
- initFromStream:(NXStream *)stream untilOneOf:(NXCharacterSet *)set maxLength:(unsigned)length;
- initFromCStringStream:(NXStream *)stream untilOneOf:(NXCharacterSet *)set maxLength:(unsigned)length;




/* The following two methods are unimplemented in the abstract NXString and will raise errors.
*/
- (unsigned int)length;			
- (unichar)characterAt:(unsigned)pos;

/* Most subclassers should probably also implement this method for any kind of reasonable performance.
*/
- (void)getCharacters:(unichar *)buffer range:(NXRange)range;

/* Cover on getCharacters:range:
*/
- (void)getCharacters:(unichar *)buffer;




/* getCString:... methods return the C string representation of the string contents. CStrings are assumed to be NeXTSTEP encoded, unless the system is EUC, in which case all CStrings are EUC. Thus the cStringLength method should be used in determining the lengths of CStrings, and the number of bytes in the CString might be more than the number of characters in the string.

In all three of the methods below, the CString is copied into the supplied buffer, with a terminating \0. getCString: (without any other arguments) will copy cStringLength+1 bytes into the supplied buffer. getCString:maxLength: will copy MIN(cStringLength,max)+1 bytes. The +1 is for the terminating \0. Thus this method should be given a buffer containing at least max+1 bytes.

The last method takes a range, which specifies the range of Unicode characters to be converted. The leftover return argument, which is optional (specify NULL if you don't care),  returns the range still remaining to be converted. This would happen if the number of bytes resulting from converting the specified range of unichars exceeded the available space in the specified buffer. 
*/
- (void)getCString:(char *)buffer;
- (void)getCString:(char *)buffer maxLength:(unsigned)max;
- (void)getCString:(char *)buffer maxLength:(unsigned)max range:(NXRange)range remainingRange:(NXRange *)leftover;

/* Returns the number of bytes required to hold the C-String representation.
*/
- (unsigned int)cStringLength;

/* Convenience method to allocate and return C style null-terminated strings. Caller needs to free these.
*/
- (char *)cStringCopy;
- (NXAtom)uniqueCStringCopy;




/* In the below methods (compare & find), not clear what the table is yet: Without the table argument (or table == NULL), it'll do comparision in default unicode world, whatever that means, augmented by the user's environment (system language and perhaps app language). The table argument might even be a stack of tables.
*/
- (NXComparisonResult)compare:obj;
- (NXComparisonResult)compare:obj mask:(unsigned int)options table:(void *)table;

/* Looks for the specified string within the specified range. If flagMask contains NX_BACKWARDS_SEARCH, search starts from the end of the range. The comparision never leaves the provided range. Returns the range where the string is found. If not found, loc is set to NX_STRING_NOT_FOUND and length is set to 0 (but either one by itself is enough to indicate failure).
*/
- (NXRange)findString:(NXString *)string;
- (NXRange)findString:(NXString *)string range:(NXRange)range mask:(unsigned int)options table:(void *)table;


/* Finds a member of the set in the string. Returns the location of the first member found, or NX_STRING_NOT_FOUND. The flags NX_CASE_INSENSITIVE and NX_FLOATING_DIACRITICS are ignored. ??? Not clear if table is needed in these cases.
*/
- (unsigned)findOneOf:(NXCharacterSet *)set;
- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)range mask:(unsigned int)options table:(void *)table;


/* Finds a character in the string. Returns the location or NX_STRING_NOT_FOUND. ??? Not clear if table is needed in these cases.
*/
- (unsigned)findCharacter:(unichar)ch;
- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange mask:(unsigned int)options table:(void *)table;




/* Used for creating a buffer suitable for keeping the string belonging to the receiving object in. Default in NXString creates a buffer in NXDefaultMallocZone(); subclasses might wish to override this to create buffers in special zones, or the object's own zone. If nChars == 0, NULL is returned.
*/ 
- (unichar *)allocateCharacterBuffer:(unsigned)nChars;



/* isEqual: compares strings for exact equality. ??? Some thought needs to go into what this means in the Unicode world. Whatever it means, NXUniquedStrings should be uniqued on that basis.
*/
- (BOOL)isEqual:otherString;
- (unsigned)hash;



/* Ways to get other types of strings. NXString will give back generic mutable/immutable copies by default. Subclasses may wish to override these methods to return more fine-tuned copies.
*/
- immutableCopyFromZone:(NXZone *)zone;
- mutableCopyFromZone:(NXZone *)zone;

/* Gives back an NXString created from the characters in the specified range. By default NXString creates a generic copy; subclassers may wish to override this method to return instances of other types of objects.
*/
- (NXString *)copySubstring:(NXRange)range fromZone:(NXZone *)zone;

/* Covers for the above three.
*/
- mutableCopy;
- immutableCopy;
- (NXString *)copySubstring:(NXRange)range; 



/* Write the string object out to a stream. writeToStream: will write out the characters. writeBytesToStream: will write only bytes (NeXTSTEP (or EUC) encoding).
*/
- (void)writeToStream:(NXStream *)stream;
- (void)writeCStringToStream:(NXStream *)stream;



/* Does some stuff.
*/
+ initialize;

@end


@interface NXMutableString : NXString

- (void)replaceCharactersInRange:(NXRange)range withString:(NXString *)string;
- (void)insertString:(NXString *)string at:(unsigned)loc;
- (void)appendString:(NXString *)string;
- (void)deleteCharactersInRange:(NXRange)range;

- immutableCopyFromZone:(NXZone *)zone;
- mutableCopyFromZone:(NXZone *)zone;

@end



/* NXSimpleReadOnlyString is a terribly simple minded readonly string which can act as a superclass for some different classes of immutable strings... It exports its instance variables for subclass usage. The code in NXSimpleReadOnlyString assumes that the characters instance variable points to the actual characters and _length stores the length of the unichars in that buffer. If this is not the case in a given subclass, then that class shouldn't be a subclass of NXSimpleReadOnlyString.
*/
@interface NXSimpleReadOnlyString : NXString
{
@protected
    unichar *characters;
    unsigned int _length;
}

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len;
- (unichar)characterAt:(unsigned)loc;
- (void)getCharacters:(unichar *)buffer range:(NXRange)range;
- (void)getCString:(char *)buffer maxLength:(unsigned)max range:(NXRange)range remainingRange:(NXRange *)leftover;
- (NXComparisonResult)compare:obj mask:(unsigned int)options table:(void *)table;
- (NXRange)findString:(NXString *)findStr range:(NXRange)fRange mask:(unsigned int)options table:(void *)table;
- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange mask:(unsigned int)options  table:(void *)table;
- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)fRange mask:(unsigned int)options table:(void *)table;
- (unsigned)hash;
- (unsigned)length;
- (NXString *)copySubstring:(NXRange)range fromZone:(NXZone *)zone;
- copyFromZone:(NXZone *)zone;
- write:(NXTypedStream *)s;
- read:(NXTypedStream *)s;
- free;

@end


/* NXReadOnlyString and NXReadWriteString are ref-counted concrete string classes... Designated initializer for these classes is initFromCharactersNoCopy:length:freeWhenDone:.
*/
@interface NXReadOnlyString : NXSimpleReadOnlyString
{
    struct __realStringFlags {
	unsigned int notCopied:1;
	unsigned int unused:15;
	unsigned int refs:16;
    } _flags;
}

+ alloc;
+ allocFromZone:(NXZone *)zone;
- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len;
- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len freeWhenDone:(BOOL)flag;
- (unichar *)allocateCharacterBuffer:(unsigned)nChars;
- read:(NXTypedStream *)stream;
- write:(NXTypedStream *)stream;
- finishUnarchiving;
- copyFromZone:(NXZone *)zone;
- (NXString *)copySubstring:(NXRange)range fromZone:(NXZone *)zone;
- mutableCopyFromZone:(NXZone *)zone;
- free;

@end

/* NXReadOnlySubstrings are useful as substrings on NXReadOnlyStrings. There's no need to explicitly create NXReadOnlySubstrings; using -copySubstring on NXReadOnlyString does the right thing.
*/
@interface NXReadOnlySubstring : NXReadOnlyString
{
    NXReadOnlyString *_referenceString;
}

- free;

@end

@interface NXReadWriteString : NXMutableString
{
    NXReadOnlyString *actualString;    
}

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len;
- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len freeWhenDone:(BOOL)flag;
- (unsigned)length;
- (unichar)characterAt:(unsigned)loc;
- (void)getCharacters:(unichar *)buffer range:(NXRange)range;
- (void)getCString:(char *)buffer maxLength:(unsigned)max range:(NXRange)range remainingRange:(NXRange *)leftover;
- (NXComparisonResult)compare:obj mask:(unsigned int)options table:(void *)table;
- (NXRange)findString:(NXString *)findStr range:(NXRange)fRange mask:(unsigned int)options table:(void *)table;
- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange mask:(unsigned int)options  table:(void *)table;
- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)fRange mask:(unsigned int)options table:(void *)table;
- (unichar *)allocateCharacterBuffer:(unsigned)nChars;
- (void)replaceCharactersInRange:(NXRange)range withString:(NXString *)string;
- free;
- (unsigned)hash;
- read:(NXTypedStream *)stream;
- write:(NXTypedStream *)stream;
- finishUnarchiving;
- copyFromZone:(NXZone *)zone;
- immutableCopyFromZone:(NXZone *)zone;

@end


@interface NXUniquedString : NXSimpleReadOnlyString

/* newFromString: and initFromCharactersNoCopy:length:freeWhenDone: are designated initializers for NXUniquedString.  Because of the nature of uniqued strings, the  new... methods are preferable to the init... methods.
*/
+ newFromString:(NXString *)string;
+ newFromCharacters:(const unichar *)chars length:(unsigned)length;
- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len;	// Might free self

+ alloc;
+ allocFromZone:(NXZone *)zone;
- copyFromZone:(NXZone *)zone;		// Returns self
- free;					// Doesn't do anything.
- finishUnarchiving;
- (BOOL)isEqual:string;		// Fast if both strings are NXUniquedString

@end


/* NXConstantStrings are strings created by the compiler with the @" " construct.  Upon encountering @" ", the compiler will take the characters between the quotes and create an NXConstantString instance.  Instances of NXConstantString cannot be created at runtime.
*/
@interface NXConstantString : NXSimpleReadOnlyString

- initFromCharactersNoCopy:(unichar *)chars length:(unsigned)len; // Raises
- copyFromZone:(NXZone *)zone;		// Returns self
- free;					// Doesn't do anything.
- finishUnarchiving;			// Returns an NXReadOnlyString

@end

/* Errors that can be raised by NXString
*/
#define NX_STRING_ERROR_BASE 8500

typedef enum _NXStringErrors {
    NXStringBoundsError = NX_STRING_ERROR_BASE, 
    NXStringInvalidArgumentError, 
    NXStringInternalError
} NXStringErrors;



#ifndef DONT_USE_OLD_NXSTRING_NAMES

/* Obsolete 2.x names which should not be used. These are here for easy conversion of 2.x and 3.0 apps only. When the time comes for conversion, you might want to define DONT_USE_OLD_NXSTRING_NAMES in your cc line (with -DDONT_USE_OLD_NXSTRING_NAMES). This will assure none of this stuff gets included, and you will gets errors and warnings from the compiler, allowing you to fix them all.
*/
#define NXSTRING_ERROR_BASE		NX_STRING_ERROR_BASE
#define NX_STRINGMAXLEN			NX_MAX_STRING_LENGTH
#define NX_STRINGNOTFOUND		NX_STRING_NOT_FOUND
#define NX_STRINGHASHLEN		NX_HASH_STRING_LENGTH
#define NX_BADBYTE			NX_UNREPRESENTABLE_CHARACTER
#define NX_STRINGCASEINSENSITIVE	NX_CASE_INSENSITIVE
#define NX_STRINGFLOATINGDIACRITICS	NX_FLOATING_DIACRITICS
#define NXStringRange			NXRange
#define _NXStringRange			_NXRange
#define NXComparisionResult		NXComparisonResult
#define NX_STRINGBACKWARDS		NX_BACKWARDS_SEARCH

@interface NXString(NXObsoleteMethods)
- initFromStream:(NXStream *)stream uptoLength:(unsigned)length orUntilOneOf:(NXCharacterSet *)set;
- initFromByteStream:(NXStream *)stream uptoLength:(unsigned)length orUntilOneOf:(NXCharacterSet *)set;
- (void)getCString:(char *)buffer range:(NXRange)range;
- (void)getCString:(char *)buffer length:(unsigned)bytes;
- (void)getCString:(char *)buffer length:(unsigned)bytes range:(NXRange)range;
- (NXComparisonResult)compare:(NXString *)string mask:(unsigned int)options;
- (NXComparisonResult)compare:(NXString *)string mask:(unsigned int)options usingTable:(void *)table;
- (NXRange)find:(NXString *)string;
- (NXRange)find:(NXString *)string range:(NXRange)range;
- (NXRange)find:(NXString *)string mask:(unsigned int)options;
- (NXRange)find:(NXString *)string range:(NXRange)range mask:(unsigned int)options usingTable:(void *)table;
- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)range;
- (unsigned)findOneOf:(NXCharacterSet *)set mask:(unsigned int)options;
- (unsigned)findOneOf:(NXCharacterSet *)set range:(NXRange)range mask:(unsigned int)options usingTable:(void *)table;
- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange;
- (unsigned)findCharacter:(unichar)ch mask:(unsigned int)options;
- (unsigned)findCharacter:(unichar)ch range:(NXRange)fRange mask:(unsigned int)options usingTable:(void *)table;
- (unichar *)createCharacterBuffer:(unsigned)nChars;
- (void)writeBytesToStream:(NXStream *)stream;
@end

@interface NXMutableString(NXObsoleteMethods)
- (void)replaceRange:(NXRange)range with:(NXString *)string;
- (void)replaceWith:(NXString *)string;
- (void)insert:(NXString *)string at:(unsigned)loc;
- (void)append:(NXString *)string;
- (void)deleteRange:(NXRange)range;
@end

#endif

#endif /* _OBJC_NXSTRING_H_ */
