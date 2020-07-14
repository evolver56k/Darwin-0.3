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
	Copyright (c) 1998-1999 Apple Computer, Inc.
	All Rights Reserved.
	
	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
	The copyright notice above does not evidence any actual or
	intended publication of such source code.
	
	Change History:
	 9-Feb-1999 Don Brady		Fix UnicodeToMacRoman to handle a terminating decomposed char.
	06-Feb-1999 Clark Warner	Hacked up ConvertCStringtoUTF8 to workaround the fact that the 
				        Japanese converter can't handle certain MacOS Roman Chracters.
	18-Jan-1999 Don Brady		Fix name-mangling of Japanese names.
	07-Jan-1999 Don Brady		Add max unicode chars parameter to MacRomanToUnicode.
    16-Dec-1998	Don Brady	Dump NSString and use CF code instead. Added UTF-8 Conversion routines.
    30-Nov-1998	Don Brady	Add UnicodeToMacRoman routine to handle decomposed Unicode.
    20-Nov-1998	Don Brady	New file.
*/
#import <sys/types.h>
#import <stdio.h>
#import <string.h>
#import "vdi_translation.h"
#import "JapaneseConverter.h"

#import "vol.h"

#define TOGGLE_SLASH_COLON 0

#define kMaxUnicodeChars 256

enum {
	kMinFileExtensionChars = 1,		// does not include dot
	kMaxFileExtensionChars = 5		// does not include dot
};

// Note:	'µ' has two Unicode representations 0x00B5 (micro sign) and 0x03BC (greek)
//			'Æ' has two Unicode representations 0x2206 (increment) and 0x0394 (greek)
#define	IsSpecialUnicodeChar(c)		( (c) == 0x00B5 || (c) == 0x03BC || (c) == 0x03C0 || (c) == 0x2206 || (c) == 0x0394 )


static void MacRomanToUnicode(const u_char* cString, u_long maxChars, u_long *unicodeChars,
							  UniChar* unicodeString);

static int UnicodeToMacRoman (u_long unicodeChars, const UniChar* unicodeString,
								u_long maxBytes, u_char* cString);

static void GetFilenameExtension(u_long length, const UniChar* unicodeStr, char* extStr);

typedef unsigned long	UCS4;
typedef unsigned short	UCS2;
typedef unsigned short	UTF16;
typedef unsigned char	UTF8;

typedef enum {
	ok, 				/* conversion successful */
	sourceExhausted,	/* partial character in source, but hit end */
	targetExhausted		/* insuff. room in target for conversion */
} ConversionResult;

static ConversionResult	ConvertUTF16toUTF8 (
		UTF16** sourceStart, const UTF16* sourceEnd, 
		UTF8** targetStart, const UTF8* targetEnd);
		
static ConversionResult	ConvertUTF8toUTF16 (
		UTF8** sourceStart, UTF8* sourceEnd, 
		UTF16** targetStart, const UTF16* targetEnd);

/* ================================================================ */



void
ConvertCStringToUTF8 (const char * string, u_long textEncoding, int bufferSize, char* buffer)
{
	if (textEncoding == kTextEncodingUTF8) {
		bcopy(string, buffer, MIN(strlen(string), bufferSize-1) + 1);
	} else {
		UniChar uniStr[kMaxUnicodeChars];
		UInt32 unicodeChars;
		UInt32 processedChars;
		UTF16* sourceStart;
		UTF16* sourceEnd;
		UTF8* targetStart;
		UTF8* targetEnd;
		

		/* first convert to Unicode */

		switch (textEncoding) {

		case kTextEncodingMacRoman:
			/* XXX need to pass max unicode chars */
			MacRomanToUnicode(string, kMaxUnicodeChars, &unicodeChars, uniStr);
			break;

		case kTextEncodingMacJapanese:
			if (processedChars = __CFFromMacJapanese( kCFStringEncodingUseCanonical,
										  string,
										  strlen(string),
										  uniStr,
										  kMaxUnicodeChars,
										  &unicodeChars ) < strlen(string))
				{
				/*  This is a workaround because the Japanese Unicode converter can't handle
				    certain MacOS Roman characters and stops translating.  In some cases that
				    can lead to false matches as we match only a partial string.  If the 
				    Japanese converters can't match the string we'll retranslate as MacOS Roman
				    and try again.  In the GetCatalogInfo case we'd be doing that anyway.  I think
				    it is safe in the other cases.  The worst that can happen is matching a Roman
				    file inadvertantly when a user somehow inputs an improper ShiftJis string on
				    a Japanese system.  That's likely to be rare.				  */

                        	MacRomanToUnicode(string, kMaxUnicodeChars, &unicodeChars, uniStr);
				}
			break;

		default:
			uniStr[0] = '?';
			unicodeChars = 1;
		};	

		/* now convert it to UTF-8 */

		sourceStart = (UTF16*) uniStr;
		sourceEnd = sourceStart + unicodeChars;
		targetStart = (UTF8*) buffer;
		targetEnd = targetStart + bufferSize - 1;

		(void) ConvertUTF16toUTF8 (&sourceStart, sourceEnd, &targetStart, targetEnd);
		
		buffer[targetStart - (UTF8*) buffer] = '\0';	/* add null termination */
	}
}


void
ConvertUTF8ToCString(const char* utf8str, u_long to_encoding, u_long cnid, int bufferSize, char* buffer)
{
	if (to_encoding == kTextEncodingUTF8) {
		bcopy(utf8str, buffer, MIN(strlen(utf8str), bufferSize-1) + 1);
	} else {
		UniChar uniStr[kMaxUnicodeChars];
		UInt32 unicodeChars;
		UInt32 srcCharsUsed;
		ConversionResult result;
		UTF8* sourceStart;
		UTF8* sourceEnd;
		UTF16* targetStart;
		UTF16* targetEnd;
		UInt32 usedByteLen = 0;

		/* convert to Unicode first */
		sourceStart = (UTF8*) utf8str;
		sourceEnd = sourceStart + strlen(utf8str);
		targetStart = (UTF16*) uniStr;
		targetEnd = targetStart + kMaxUnicodeChars;

		result = ConvertUTF8toUTF16 (&sourceStart, sourceEnd, &targetStart, targetEnd);

		/* XXX need to handle targetExhausted and sourceExhausted errors */
	
		unicodeChars = (targetStart - uniStr);
	
		switch (to_encoding) {

		case kTextEncodingMacJapanese:
			srcCharsUsed = __CFToMacJapanese(kCFStringEncodingUseCanonical,
											 uniStr,
											 unicodeChars,
											 (UInt8*)buffer,
											 bufferSize - 1,
											 &usedByteLen);
			if (srcCharsUsed == unicodeChars)
				result = 0;
			else
				result = -1;	/* not all the chars were used so mangle the name */
			buffer[usedByteLen] = '\0';
			break;

		case kTextEncodingMacRoman:
		/* fall through */
		default:
			result = UnicodeToMacRoman(unicodeChars, uniStr, bufferSize, buffer);
			/* XXX need to handle targetExhausted and unmappable char errors */
			break;
		};

		// If name was too long or some characters were unrepresentable...
		// we need to mangle the name so that the file can be found later
		if (result) {
			UInt8		fileIDStr[16];		// file ID as a string
			UInt8		extStr[8];			// dot extension as a string
			UInt32		sizeLimit;
			UInt32		nameLength;
			UInt32		prefixLength;

			sizeLimit = bufferSize - 1;

			sprintf(fileIDStr, "#%lX", cnid);

			//	Get a filename extension (unless its a volume name)
			if (cnid > 2)
				GetFilenameExtension(unicodeChars, uniStr, extStr);
			else
				extStr[0] = '\0';	// volumes don't have extensions

			// calculate free space for filename prefix
			sizeLimit -= strlen(extStr) + strlen(fileIDStr);
			nameLength = strlen(buffer);

			//	Generate the prefix part of the name (before extension or File ID string).
			//	Use the string we already have, shortening it if needed.
			if (nameLength > sizeLimit)
				prefixLength = sizeLimit;
			else
				prefixLength = nameLength - strlen(extStr);	// remove extension from short name

			/* 
			 * we only want whole Shift-JIS characters, so do a
			 * constrained conversion to get the prefix
			 */
			if (to_encoding == kTextEncodingMacJapanese) {
				(void) __CFToMacJapanese(kCFStringEncodingUseCanonical,
										 uniStr,
										 unicodeChars,
										 (UInt8*)buffer,
										 prefixLength,
										 &prefixLength);
			}

			buffer[prefixLength] = '\0';

			strcat(buffer, fileIDStr);
			strcat(buffer, extStr);
		}
	}
}


//
// Get filename extension (if any)
//
static void
GetFilenameExtension( u_long length, const UniChar* unicodeStr, char* extStr )
{
	UInt32	i;
	UniChar	c;
	UInt16	extChars;			// number of extension characters (excluding the dot)
	UInt16	maxExtChars;
	Boolean	foundExtension;

	extStr[0] = '\0';		// assume there's no extension

	if ( length < 3 )
		return;					// sorry, "x.y" is smallest possible extension	
	
	if ( length < (kMaxFileExtensionChars + 2) )
		maxExtChars = length - 2;	// we need at least one prefix character and dot
	else
		maxExtChars = kMaxFileExtensionChars;

	i = length;
	extChars = 0;
	foundExtension = 0;

	while ( extChars <= maxExtChars )
	{
		c = unicodeStr[--i];

		if ( c == (UniChar) '.' )		// look for leading dot
		{
			if ( extChars > 0 )			// cannot end with a dot
				foundExtension = 1;
			break;
		}

		if ( (c >= 0x20 && c <= 0x7F) || IsSpecialUnicodeChar(c) )
			++extChars;
		else
			break;
	}
	
	// if we found one then copy it
	if ( foundExtension )
	{
		UInt8 *extStrPtr = extStr;
		const UniChar *unicodeStrPtr = &unicodeStr[i];	// point to dot char
		
		for ( i = 0; i <= extChars; ++i )
		{
			c = *(unicodeStrPtr++);
			
			// map any special characters
			switch (c)
			{
				case 0x00B5:			// micro sign
				case 0x03BC:			// greek mu
					c = (UniChar) 'µ';
					break;

				case 0x03C0:			// greek pi
					c = (UniChar) '¹';
					break;

				case 0x2206:			// increment sign
				case 0x0394:			// greek capital delta
					c = (UniChar) 'Æ';
					break;
			}

			*(extStrPtr++) = (UInt8) c;		// copy/convert to ascii
		}
		
		*(extStrPtr) = '\0';		// terminate string
	}

} // end GetFilenameExtension


UniChar	gHiBitBaseUnicode[128] =
{
	/* 0x80 */	0x0041, 0x0041, 0x0043, 0x0045, 0x004e, 0x004f, 0x0055, 0x0061, 
	/* 0x88 */	0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0063, 0x0065, 0x0065, 
	/* 0x90 */	0x0065, 0x0065, 0x0069, 0x0069, 0x0069, 0x0069, 0x006e, 0x006f, 
	/* 0x98 */	0x006f, 0x006f, 0x006f, 0x006f, 0x0075, 0x0075, 0x0075, 0x0075, 
	/* 0xa0 */	0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
	/* 0xa8 */	0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
	/* 0xb0 */	0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
	/* 0xb8 */	0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 
	/* 0xc0 */	0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
	/* 0xc8 */	0x00bb, 0x2026, 0x00a0, 0x0041, 0x0041, 0x004f, 0x0152, 0x0153, 
	/* 0xd0 */	0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
	/* 0xd8 */	0x0079, 0x0059, 0x2044, 0x00a4, 0x2039, 0x203a, 0xfb01, 0xfb02, 
	/* 0xe0 */	0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x0041, 0x0045, 0x0041, 
	/* 0xe8 */	0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004f, 0x004f, 
	/* 0xf0 */	0xf8ff, 0x004f, 0x0055, 0x0055, 0x0055, 0x0131, 0x02c6, 0x02dc, 
	/* 0xf8 */	0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
};

UniChar	gHiBitCombUnicode[128] =
{
	/* 0x80 */	0x0308, 0x030a, 0x0327, 0x0301, 0x0303, 0x0308, 0x0308, 0x0301, 
	/* 0x88 */	0x0300, 0x0302, 0x0308, 0x0303, 0x030a, 0x0327, 0x0301, 0x0300, 
	/* 0x90 */	0x0302, 0x0308, 0x0301, 0x0300, 0x0302, 0x0308, 0x0303, 0x0301, 
	/* 0x98 */	0x0300, 0x0302, 0x0308, 0x0303, 0x0301, 0x0300, 0x0302, 0x0308, 
	/* 0xa0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	/* 0xa8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xb0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xb8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xc0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xc8 */	0x0000, 0x0000, 0x0000, 0x0300, 0x0303, 0x0303, 0x0000, 0x0000, 
	/* 0xd0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xd8 */	0x0308, 0x0308, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xe0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0302, 0x0302, 0x0301, 
	/* 0xe8 */	0x0308, 0x0300, 0x0301, 0x0302, 0x0308, 0x0300, 0x0301, 0x0302, 
	/* 0xf0 */	0x0000, 0x0300, 0x0301, 0x0302, 0x0300, 0x0000, 0x0000, 0x0000, 
	/* 0xf8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};


static void
MacRomanToUnicode (const u_char* cString, u_long maxChars, u_long *unicodeChars, UniChar* unicodeString)
{
	const u_char *s;
	UniChar *u;
	u_long srcChars, dstChars;
	u_char c;

	s = cString;
	u = unicodeString;
	dstChars = 0;
	srcChars = strlen(cString);

	while ((dstChars < maxChars) && srcChars--) {
		c = *(s++);

		if ( (char) c >= 0 ) {		// make sure its seven bit ascii
			*(u++) = (UniChar) c;		//  pad high byte with zero
			++dstChars;
		}
		else {	/* its a hi bit character */
			UniChar uc;

			c &= 0x7F;
			*(u++) = uc = gHiBitBaseUnicode[c];
			++dstChars;
			
			/*
			 * if the unicode character (uc) is an alpha char
			 * then we have an additional combining character
			 */
			if ((uc <= (UniChar) 'z') && (uc >= (UniChar) 'A')) {
				if (dstChars >= maxChars)
					break;
				*(u++) = gHiBitCombUnicode[c];
				++dstChars;
			}
		}
	}

	*unicodeChars = dstChars;
}



/* 0x00A0 - 0x00FF = Latin 1 Supplement (30 total) */
u_char gLatin1Table[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x00A0 */	0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4,  '?', 0xA4, 0xAC, 0xA9, 0xBB, 0xC7, 0xC2,  '?', 0xA8, 0xF8,
	/* 0x00B0 */	0xA1, 0XB1,  '?',  '?', 0xAB, 0xB5, 0xA6, 0xe1, 0xFC,  '?', 0xBC, 0xC8,  '?',  '?',  '?', 0xC0,
	/* 0x00C0 */	 '?',  '?',  '?',  '?',  '?',  '?', 0xAE,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x00D0 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xAF,  '?',  '?',  '?',  '?',  '?',  '?', 0xA7,
	/* 0x00E0 */	 '?',  '?',  '?',  '?',  '?',  '?', 0xBE,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x00F0 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xD6, 0xBF,  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


/* 0x02C0 - 0x02DF = Spacing Modifiers (8 total) */
u_char gSpaceModsTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x02C0 */	 '?',  '?',  '?',  '?',  '?',  '?', 0xF6, 0xFF,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x02D0 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xF9, 0xFA, 0xFB, 0xFE, 0xF7, 0xFD,  '?',  '?'
};


/* 0x20xx = General Punctuation (16 total) */
u_char gPunctTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x2000 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2010 */	 '?',  '?',  '?', 0xd0, 0xd1,  '?',  '?',  '?', 0xd4, 0xd5, 0xe2,  '?', 0xd2, 0xd3, 0xe3,  '?',
	/* 0x2020 */	0xa0, 0xe0, 0xa5,  '?',  '?',  '?', 0xc9,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2030 */	0xe4,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xdc, 0xdd,  '?',  '?',  '?',  '?',  '?',
	/* 0x2040 */	 '?',  '?',  '?',  '?', 0xda,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


/* 0x2200 = Mathematical Operators (11 total) */
u_char gMathTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x2200 */	 '?',  '?', 0xb6,  '?',  '?',  '?', 0xc6,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xb8,
	/* 0x2210 */	 '?', 0xb7,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xc3,  '?',  '?',  '?', 0xb0,  '?',
	/* 0x2220 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xba,  '?',  '?',  '?',  '?',
	/* 0x2230 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2240 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xc5,  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2250 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2260 */	0xad,  '?',  '?',  '?', 0xb2, 0xb3,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


/* */
u_char gReverseCombTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x40 */		0xDA, 0x40, 0xDA, 0xDA, 0xDA, 0x56, 0xDA, 0xDA, 0xDA, 0x6C, 0xDA, 0xDA, 0xDA, 0xDA, 0x82, 0x98,
	/* 0x50 */		0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xAE, 0xDA, 0xDA, 0xDA, 0xC4, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA,
	/* 0x60 */		0xDA, 0x4B, 0xDA, 0xDA, 0xDA, 0x61, 0xDA, 0xDA, 0xDA, 0x77, 0xDA, 0xDA, 0xDA, 0xDA, 0x8D, 0xA3,
	/* 0x70 */		0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xB9, 0xDA, 0xDA, 0xDA, 0xCF, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA,

	/* Combining Diacritical Marks (0x0300 - 0x030A) */

	/*                0     1     2     3     4     5     6     7     8     9     A  */
	/*  'A'   */
	/* 0x0300 */	0xCB, 0xE7, 0xE5, 0xCC,  '?',  '?',  '?',  '?', 0x80,  '?', 0x81,

	/*  'a'   */
	/* 0x0300 */	0x88, 0x87, 0x89, 0x8B,  '?',  '?',  '?',  '?', 0x8A,  '?', 0x8C,

	/*  'E'   */
	/* 0x0300 */	0xE9, 0x83, 0xE6,  '?',  '?',  '?',  '?',  '?', 0xE8,  '?',  '?',

	/*  'e'   */
	/* 0x0300 */	0x8F, 0x8E, 0x90,  '?',  '?',  '?',  '?',  '?', 0x91,  '?',  '?',

	/*  'I'   */
	/* 0x0300 */	0xED, 0xEA, 0xEB,  '?',  '?',  '?',  '?',  '?', 0xEC,  '?',  '?',

	/*  'i'   */
	/* 0x0300 */	0x93, 0x92, 0x94,  '?',  '?',  '?',  '?',  '?', 0x95,  '?',  '?',

	/*  'N'   */
	/* 0x0300 */	 '?',  '?',  '?', 0x84,  '?',  '?',  '?',  '?',  '?',  '?',  '?',

	/*  'n'   */
	/* 0x0300 */	 '?',  '?',  '?', 0x96,  '?',  '?',  '?',  '?',  '?',  '?',  '?',

	/*  'O'   */
	/* 0x0300 */	0xF1, 0xEE, 0xEF, 0xCD,  '?',  '?',  '?',  '?', 0x85,  '?',  '?',

	/*  'o'   */
	/* 0x0300 */	0x98, 0x97, 0x99, 0x9B,  '?',  '?',  '?',  '?', 0x9A,  '?',  '?',

	/*  'U'   */
	/* 0x0300 */	0xF4, 0xF2, 0xF3,  '?',  '?',  '?',  '?',  '?', 0x86,  '?',  '?',
	
	/*  'u'   */
	/* 0x0300 */	0x9D, 0x9C, 0x9E,  '?',  '?',  '?',  '?',  '?', 0x9F,  '?',  '?',

	/*  'Y'   */
	/* 0x0300 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xD9,  '?',  '?',

	/*  'y'   */
	/* 0x0300 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xD8,  '?',  '?',

	/*  else  */
	/* 0x0300 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


static int
UnicodeToMacRoman (u_long unicodeChars, const UniChar* unicodeString,
					u_long maxBytes, u_char* cString)
{
	register u_char *cp;
	register const UniChar *up;
	register UniChar c;
	UniChar			mask;
	int				inputChars;
	int				outputChars;
	int				maxChars;
	int				result = 0;
	u_char			lsb;
	u_char			prevChar;
	u_char			mc;

	mask = (UniChar) 0xFF80;
	cp = cString;
	up = unicodeString;
	maxChars = maxBytes - 1;
	inputChars = unicodeChars;
	outputChars = prevChar = 0;
	
	while (inputChars) {
		c = *(up++);
		lsb = (u_char) c;

		/*
		 * If its not 7-bit ascii, then we need to map it
		 */
		if ( c & mask ) {
			mc = '?';
			switch (c & 0xFF00) {
				case 0x0000:
					if (lsb >= 0xA0)
						mc = gLatin1Table[lsb - 0xA0];
					break;

				case 0x0200:
					if (lsb >= 0xC0 && lsb <= 0xDF)
						mc = gSpaceModsTable[lsb - 0xC0];
					break;

				case 0x2000:
					if (lsb <= 0x4F)
						mc = gPunctTable[lsb];
					break;

				case 0x2200:
					if (lsb <= 0x6F)
						mc = gMathTable[lsb];
					break;

				case 0x0300:
					if (c <= 0x030A) {
						if (prevChar >= 'A' && prevChar < 'z') {
							mc = gReverseCombTable[gReverseCombTable[prevChar - 0x40] + lsb];
							--cp;	/* backup over base char */
							--outputChars;
						}
					}
					else {
						switch (c) {
							case 0x0327:	/* combining cedilla */
								if (prevChar == 'C')
									mc = 0x82;
								else if (prevChar == 'c')
									mc = 0x8D;
								else
									break;
								--cp;	/* backup over base char */
								--outputChars;
								break;

							case 0x03A9: mc = 0xBD; break;	/* omega */

							case 0x03C0: mc = 0xB9; break;	/* pi */
						}
					}
					break;
					
				default:
					switch (c) {
						case 0x0131: mc = 0xf5; break;	/* dotless i */

						case 0x0152: mc = 0xce; break;	/* OE */

						case 0x0153: mc = 0xcf; break;	/* oe */

						case 0x0192: mc = 0xc4; break;	/* Ä */

						case 0x2122: mc = 0xaa; break;	/* TM */

						case 0x25ca: mc = 0xd7; break;	/* diamond */

						case 0xf8ff: mc = 0xf0; break;	/* apple logo */

						case 0xfb01: mc = 0xde; break;	/* fi */

						case 0xfb02: mc = 0xdf; break;	/* fl */
					}
			} /* end switch (c & 0xFF00) */
			
			/*
			 * If we have an unmapped character then we need to mangle the name...
			 */
			if (mc == '?')
				result = -2;

			prevChar = 0;
			lsb = mc;

		} /* end if (c & mask) */
		else {
			prevChar = lsb;
		}

		if (outputChars >= maxChars)
			break;

		*(cp++) = lsb;
		++outputChars;
		--inputChars;

	} /* end while */
	
	cString[outputChars] = '\0';
	
	if (inputChars > 0)
		result = -1;	/* ran out of room! */

	return result;
}

/* ================================================================ */

#define halfShift				10
#define halfBase				0x0010000UL
#define halfMask				0x3FFUL
#define kSurrogateHighStart		0xD800UL
#define kSurrogateHighEnd		0xDBFFUL
#define kSurrogateLowStart		0xDC00UL
#define kSurrogateLowEnd		0xDFFFUL

#define kReplacementCharacter	0x0000FFFDUL
#define kMaximumUCS2			0x0000FFFFUL
#define kMaximumUTF16			0x0010FFFFUL
#define kMaximumUCS4			0x7FFFFFFFUL

/* ================================================================ */

UCS4 offsetsFromUTF8[6] =	{0x00000000UL, 0x00003080UL, 0x000E2080UL, 
					 	 	 0x03C82080UL, 0xFA082080UL, 0x82082080UL};
char bytesFromUTF8[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5};

UTF8 firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

/*
 * Colons vs. Slash
 *
 * The VFS layer uses a "/" as a pathname separator but HFS disks
 * use a ":".  So when converting from UTF-8, ":" characters need
 * to be changed to "/" so that colons don't end up on HFS disks.
 * Likewise when converting into UTF-8, "/" characters need to be
 * changed to ":" so that a "/" in a filename is not returned 
 * through the VFS layer.
 *
 * We do not need to worry about full-width slash or colons since
 * their respective representations outside of Unicode are never
 * the 7-bit versions (0x2f or 0x3a).
 */


/* ================================================================ */
static ConversionResult
ConvertUTF16toUTF8 ( UTF16** sourceStart, const UTF16* sourceEnd, 
					 UTF8** targetStart, const UTF8* targetEnd )
{
	ConversionResult result = ok;
	register UTF16* source = *sourceStart;
	register UTF8* target = *targetStart;
	while (source < sourceEnd) {
		register UCS4 ch;
		register unsigned short bytesToWrite = 0;
		register const UCS4 byteMask = 0xBF;
		register const UCS4 byteMark = 0x80; 
#if TOGGLE_SLASH_COLON
		register const UCS4 slash = '/';
#endif
		ch = *source++;

#if TOGGLE_SLASH_COLON
		if (ch == slash) {
			ch = ':';	/* VFS doesn't like slash */
		} else if (ch >= kSurrogateHighStart && ch <= kSurrogateHighEnd
				&& source < sourceEnd) {
#else
		if (ch >= kSurrogateHighStart && ch <= kSurrogateHighEnd
				&& source < sourceEnd) {
#endif
			register UCS4 ch2 = *source;
			if (ch2 >= kSurrogateLowStart && ch2 <= kSurrogateLowEnd) {
				ch = ((ch - kSurrogateHighStart) << halfShift)
					+ (ch2 - kSurrogateLowStart) + halfBase;
				++source;
			};
		};
		if (ch < 0x80) {				bytesToWrite = 1;
		} else if (ch < 0x800) {		bytesToWrite = 2;
		} else if (ch < 0x10000) {		bytesToWrite = 3;
		} else if (ch < 0x200000) {		bytesToWrite = 4;
		} else if (ch < 0x4000000) {	bytesToWrite = 5;
		} else if (ch <= kMaximumUCS4){	bytesToWrite = 6;
		} else {						bytesToWrite = 2;
										ch = kReplacementCharacter;
		}; /* I wish there were a smart way to avoid this conditional */
		
		target += bytesToWrite;
		if (target > targetEnd) {
			target -= bytesToWrite; result = targetExhausted; break;
		};
		switch (bytesToWrite) {	/* note: code falls through cases! */
			case 6:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 5:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 4:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 3:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 2:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 1:	*--target =  ch | firstByteMark[bytesToWrite];
		};
		target += bytesToWrite;
	};
	*sourceStart = source;
	*targetStart = target;
	return result;
};

/* ================================================================ */

static ConversionResult
ConvertUTF8toUTF16 ( UTF8** sourceStart, UTF8* sourceEnd, 
					 UTF16** targetStart, const UTF16* targetEnd )
{
	ConversionResult result = ok;
	register UTF8* source = *sourceStart;
	register UTF16* target = *targetStart;
	while (source < sourceEnd) {
		register UCS4 ch = 0;
#if TOGGLE_SLASH_COLON
		register const UCS4 colon = ':';
#endif
		register unsigned short extraBytesToWrite = bytesFromUTF8[*source];
		if (source + extraBytesToWrite > sourceEnd) {
			result = sourceExhausted; break;
		};
		switch(extraBytesToWrite) {	/* note: code falls through cases! */
			case 5:	ch += *source++; ch <<= 6;
			case 4:	ch += *source++; ch <<= 6;
			case 3:	ch += *source++; ch <<= 6;
			case 2:	ch += *source++; ch <<= 6;
			case 1:	ch += *source++; ch <<= 6;
			case 0:	ch += *source++;
		};
		ch -= offsetsFromUTF8[extraBytesToWrite];

		if (target >= targetEnd) {
			result = targetExhausted; break;
		};
		if (ch <= kMaximumUCS2) {
#if TOGGLE_SLASH_COLON
			if (ch == colon) {
				ch = '/';	/* HFS doesn't like colons */
			}
#endif
			*target++ = ch;
		} else if (ch > kMaximumUTF16) {
			*target++ = kReplacementCharacter;
		} else {
			if (target + 1 >= targetEnd) {
				result = targetExhausted; break;
			};
			ch -= halfBase;
			*target++ = (ch >> halfShift) + kSurrogateHighStart;
			*target++ = (ch & halfMask) + kSurrogateLowStart;
		};
	};
	*sourceStart = source;
	*targetStart = target;
	return result;
};



