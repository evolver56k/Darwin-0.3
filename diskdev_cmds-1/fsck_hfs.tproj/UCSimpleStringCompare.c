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
	File:		UCSimpleStringCompare.c

	Contains:	xxx put contents here xxx

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(djb)	Don Brady
		(msd)	Mark Day

	Change History (most recent first):

	   <CS1>	 4/24/97	djb		first checked in
	  <HFS2>	 2/27/97	msd		Update to latest version from John Jenkins. Added a #define,
									USE_GLOBAL_DATA; when true, the tables are included in their
									normal C form; when false, functions are called to get the table
									addresses. Changed two const int globals to be enum's.
	  <HFS1>	 2/13/97	msd		first checked in
*/

/* UnicodeStringCompare.c */

#if TARGET_OS_MAC
#include <Types.h>
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */

#include "UCStringComparePriv.h"

#ifndef USE_GLOBAL_DATA
#define USE_GLOBAL_DATA 1
#endif

#if USE_GLOBAL_DATA
	//#include "UCSimpleStringCompareTables.h"
	#include "UCStringCompareData.h"
#else
	//
	//	The tables were actually linked in as PROC's.  We declare them as functions
	//	so we can take their addresses.  Then, we'll cast those to pointers of the
	//	correct type.
	//
	UniChar *			Get_gLowerCaseTable(void);
	unsigned char *		Get_gCompositionClass(void);
	unsigned short *	Get_gDecompositionIndices(void);
	UniChar *			Get_gDecompositions(void);
	
//	extern void gDecompositionIndices(void);
//	extern void gDecompositions(void);
//	extern void gCompositionClass(void);
//	extern void gLowerCaseTable(void);
#endif

/* Prototypes */
//#pragma mark -Prototypes-
UniChar	MakeUniCharLowerCase( UniChar theChar, UniChar* lowerCaseTable );
UniChar	*GetCharExpansion( UniChar *theChar, ItemCount *expansionLength, unsigned short* expansionIndices, UniChar* expansionTable );
int		GetCombiningClass( UniChar theChar, unsigned char* classTable );
UCCollateResult ReorderAndCompare( UniChar* charsSeen1, UniChar* charsSeen2, int combiningCharsBuffered, int bufferPos, unsigned char* compositionClassTable );

#ifdef INVESTIGATE

/*
//--------------------------------------------------------------------------------------------------
//	Routine:	MakeUniCharLowerCase
//	(internal routine)
//--------------------------------------------------------------------------------------------------
*/

static /* inline */ UniChar MakeUniCharLowerCase( UniChar theChar, UniChar* lowerCaseTable )
{
	UniChar returnValue = theChar;
	int index = ( theChar >> 8 );
	if ( lowerCaseTable[ index ] != 0xFFFF ) {
		index = ( lowerCaseTable[ index ] ) * 256 + ( theChar & 0x00FF );
		if ( lowerCaseTable[ index ] != 0xFFFF )
			returnValue = lowerCaseTable[ index ];
		}
	return returnValue;
} /* MakeUniCharLowerCase */





/*
//--------------------------------------------------------------------------------------------------
//	Routine:	GetCharExpansion
//	(internal routine)
//  Handles Latin-1 only for now.
//--------------------------------------------------------------------------------------------------
*/

static /* inline */ UniChar *GetCharExpansion( UniChar *theChar, ItemCount *expansionLength, unsigned short* expansionIndices, UniChar* expansionTable )
{

	UniChar* returnValue = theChar;
	int upperByte = ( *theChar >> 8 );
	UniChar decompositionIndex = expansionIndices[ upperByte ];
	*expansionLength = 1;
	
	if ( decompositionIndex & 0x8000 ) {
		decompositionIndex &= 0x7FFF;
		}
	else {
		int indexIndex = 256 * decompositionIndex + ( *theChar & 0x00FF );
		decompositionIndex = expansionIndices[ indexIndex ];
		}
	
	if ( expansionTable[ decompositionIndex ] != 0xFFFF ) {
		returnValue = expansionTable + decompositionIndex + 1;
		*expansionLength = expansionTable[ decompositionIndex ];
		}
	return returnValue;
}	/*	GetCharExpansion	*/	




static	/*	inline	*/	int	GetCombiningClass( UniChar theChar, unsigned char* compositionClass )
{
	int returnValue = 0;
	int index = ( theChar >> 8 );
	if ( compositionClass[ index ] != 0xFF ) {
		index = ( compositionClass[ index ] ) * 256 + ( theChar & 0x00FF );
		returnValue = compositionClass[ index ];
		}
	return returnValue;
}


/*
//--------------------------------------------------------------------------------------------------
//	Routine:	UCSimpleCompareStrings
//  Doesn't do canonical ordering of diacritics.
//--------------------------------------------------------------------------------------------------
*/


typedef struct {
			UniChar*	fCharacters;
			ItemCount	fLength;
			} TCharacterExpansion;
			

//	In theory, a properly written string comparison function would deal with two
//		situations:  indefinitely long "chains" of decomposition, and indefinitely
//		long strings of combining characters which interact typographically
//	In practice, we've hard-coded in the assumption that decomposition will only
//		recurse so far, and that strings of combining characters are only so long
//	In each case, we've tried to make the hard-coded limits longer than is likely
//		ever to actually occur, but we do so reluctantly, remembering the days when
//		nobody would ever want more than 640 Kb RAM

//	NOTA BENE!!!	Unicode's official decomposition tables are NOT maximal
//		The tables we use here have been produced by massaging Unicode's data
//		We're therefore able to set our stack depth for recursive decomposition to 2
//

enum {
	kStackDepth = 2,		//	How many recursive levels of decomposition we can handle
	kBufferSize = 16		//	How many combining characters in a row we can handle; this is overkill
};


extern UCCollateResult UCSimpleCompareStrings (
	const UniChar str1[], ItemCount length1,
	const UniChar str2[], ItemCount length2,
	UCCollateOptions options )
{

	UniChar*			lowerCaseTable;
	unsigned char*		compositionClassTable;
	unsigned short*		decompositionIndices;
	UniChar*			decompositions;

	Boolean						foldCase, doExpansion;
	Boolean						equivalent;
	UCCollateResult				order;

	//	These two buffers hold our expansions of precomposed characters
	//	Since Unicode's decomposition tables are *not* maximal, we have 
	//		to implement a stack to decompose more than once
	
	TCharacterExpansion			expansion1[ kStackDepth ];
	TCharacterExpansion			expansion2[ kStackDepth ];
	int 						depth1;
	int							depth2;
	
	//	These two buffers hold the characters we've seen
	//	We use them as a place to store characters so we can
	//		destructively reorder them when we do canonical
	//		ordering of combining characters
	
	UniChar	charsSeen1[ kBufferSize ];
	UniChar	charsSeen2[ kBufferSize ];
	int		bufferPos = -1;
	
	//	These variables are used to do canonical reordering of combining characters
	
	int	combiningClass1;
	int combiningClass2;

	int	combiningCharsBuffered = 0;
	
	//
	//	IMPORTANT STUFF!!!!	Get the tables we use
	//
	
#if USE_GLOBAL_DATA
	lowerCaseTable = gLowerCaseTable;
	compositionClassTable = gCompositionClass;
	decompositionIndices = gDecompositionIndices;
	decompositions = gDecompositions;
#else
	lowerCaseTable = Get_gLowerCaseTable();
	compositionClassTable = Get_gCompositionClass();
	decompositionIndices = Get_gDecompositionIndices();
	decompositions = Get_gDecompositions();
#endif

	/* Handle some simple cases first */	
	if (length1 == 0 && length2 == 0) return 0;
	
	order = 0;
	if ( length1 == 0 && length2 > 0 )
		order = -1;
	else if ( length1 > 0 && length2 == 0 )
		order = 1;

	if ( order != 0 ) return order;
		
	/* Here we have nonzero content in both strings. */
	
	foldCase = ( (options & kUCCollateCaseSensitiveMask) == 0 );
	doExpansion = ( (options & kUCCollateComposeSensitiveMask) == 0 );

	equivalent = true;
	combiningCharsBuffered = 0;
	expansion1[ 0 ].fCharacters = (UniChar*) str1;
	expansion1[ 0 ].fLength = length1;
	expansion2[ 0 ].fCharacters = (UniChar*) str2;
	expansion2[ 0 ].fLength = length2;
	depth1 = depth2 = 0;
	
	while ( equivalent && ( expansion1[ 0 ].fLength > 0 && expansion2[ 0 ].fLength > 0 ) ) {
	
		UniChar uc1, uc2;
		int stackPopped;

		//	
		//	Step one, get the next character from each string (decompose as needed)
		//
	
		if ( doExpansion ) {
			ItemCount n;
			UniChar *unexpansion;
			UniChar	*expansion;
			//	Push any expansions onto the stack
			do {
				if ( depth1 >= kStackDepth-1 ) 
					break;
				unexpansion = expansion1[ depth1 ].fCharacters;
				expansion = GetCharExpansion( unexpansion, &n, decompositionIndices, decompositions );
				if ( expansion != unexpansion ) {
					( expansion1[ depth1 ].fCharacters )++;
					expansion1[ ++depth1 ].fCharacters = expansion;
					expansion1[ depth1 ].fLength = n;
					}
				}
			while ( unexpansion != expansion );
			do {
				if ( depth2 >= kStackDepth-1 )
					break;
				unexpansion = expansion2[ depth2 ].fCharacters;
				expansion = GetCharExpansion( unexpansion, &n, decompositionIndices, decompositions );
				if ( expansion != unexpansion ) {
					( expansion2[ depth2 ].fCharacters )++;
					expansion2[ ++depth2 ].fCharacters = expansion;
					expansion2[ depth2 ].fLength = n;
					}
				}
			while ( unexpansion != expansion );
			}

		//	Position our buffer index to hold the characters
		
		bufferPos = ( bufferPos + 1 ) % kBufferSize;

		charsSeen1[ bufferPos ] = uc1 = *( expansion1[ depth1 ].fCharacters++ );
		charsSeen2[ bufferPos ] = uc2 = *( expansion2[ depth2 ].fCharacters++ );
		
		//	
		//	Step two, prepare to get the next character from each string (popping the decomposition stack as needed)
		//
			
		do {
			stackPopped = false;
			expansion1[ depth1 ].fLength -= 1;
			if ( depth1 > 0 && expansion1[ depth1 ].fLength == 0 ) {
				depth1--;
				stackPopped = true;
				}
			}
		while ( stackPopped );
		do {
			stackPopped = false;
			expansion2[ depth2 ].fLength -= 1;
			if ( depth2 > 0 && expansion2[ depth2 ].fLength == 0 ) {
				depth2--;
				stackPopped = true;
				}
			}
		while ( stackPopped );
		
		//
		//	Step three, compare the characters
		//
		//	If characters match exactly, then go on to next character immediately without
		//	doing any extra work.
		//
		//	If they don't match exactly, do the hard stuff
		//	

		if ( foldCase && uc1 != uc2 ) {
			charsSeen1[ bufferPos ] = uc1 = MakeUniCharLowerCase( uc1, lowerCaseTable );
			charsSeen2[ bufferPos ] = uc2 = MakeUniCharLowerCase( uc2, lowerCaseTable );
			}
		
		if ( combiningCharsBuffered > 0 ) {

			combiningClass1 = GetCombiningClass( uc1, compositionClassTable );
			combiningClass2 = GetCombiningClass( uc2, compositionClassTable );

			//	In this case, we don't care if the current characters match or not
			//		We have to see if we're through buffering, and if we are, 
			//		reorder and compare the results

			if ( combiningClass1 == 0 || combiningClass2 == 0 ) {
				//	We're through buffering
				if ( combiningCharsBuffered == 1 ) {
					//	The easy case; backtrack one and we're done
					bufferPos = ( bufferPos + kBufferSize - 1 ) % kBufferSize;
					uc1 = charsSeen1[ bufferPos ];
					uc2 = charsSeen2[ bufferPos ];
					equivalent = false;
					order = (uc1 > uc2) ? 1 : -1;
					}
				else if ( ( order = ReorderAndCompare( charsSeen1, charsSeen2, combiningCharsBuffered, bufferPos, compositionClassTable ) ) != 0 ) {
					equivalent = false;
					}
				combiningCharsBuffered = 0;
				}
			else combiningCharsBuffered++;
			}
		else if ( uc1 != uc2 ) {

			combiningClass1 = GetCombiningClass( uc1, compositionClassTable );
			combiningClass2 = GetCombiningClass( uc2, compositionClassTable );

			if ( combiningClass1 == 0 || combiningClass2 == 0 ) {
				equivalent = false;
				order = (uc1 > uc2) ? 1 : -1;
				}
			else combiningCharsBuffered = 1;
			}
			
		}	/*	while	*/

	//	If we're buffering comining characters, handle them--
	
	if ( combiningCharsBuffered > 0 ) {
		if ( combiningCharsBuffered == 1 ) {
			UniChar uc1, uc2;
			//	The easy case; backtrack one and we're done
			bufferPos = ( bufferPos + kBufferSize - 1 ) % kBufferSize;
			uc1 = charsSeen1[ bufferPos ];
			uc2 = charsSeen2[ bufferPos ];
			equivalent = false;
			order = (uc1 > uc2) ? 1 : -1;
			}
		else if ( ( order = ReorderAndCompare( charsSeen1, charsSeen2, combiningCharsBuffered+1, bufferPos, compositionClassTable ) ) != 0 ) {
			equivalent = false;
			}
		}

	/* Take care of different length strings */
	if ( equivalent && ( expansion1[ 0 ].fLength != expansion2[ 0 ].fLength ) ) {
		if ( expansion1[ 0 ].fLength == 0 )
			order = -1;
		else if ( expansion2[ 0 ].fLength == 0 )
			order = 1;
	}
	
	return order;
} /* UCSimpleCompareStrings */


/*
//--------------------------------------------------------------------------------------------------
//	Routine:	UCSimpleEqualStrings
//  Just calls through to UCSimpleCompareStrings for now.
//--------------------------------------------------------------------------------------------------
*/
Boolean UCSimpleEqualStrings (
	const UniChar str1[], ItemCount length1,
	const UniChar str2[], ItemCount length2,
	UCCollateOptions options )
{
	return ( UCSimpleCompareStrings( str1, length1, str2, length2, options ) == 0 );
}


/*
//--------------------------------------------------------------------------------------------------
//	Routine:	UCSimpleFindString
//  The dumb implementation that shifts by one character each try. We expect src to
//	be fairly short, so KMP or Boyer-Moore would be overkill.
//	
//	@@@ This search keeps going longer than it needs to: it searches even when the
//	remaining src string is shorter than sub. This is a bit tricky to figure out, though,
//	so I haven't put the code in. You need to compute the expanded lengths of both src
//  and sub and stop searching when you looked at the difference.
//	This requires examining all of src an extra time, so the overall savings may not be
//	worth it, especially if the patterns tend to be short. @@@
//--------------------------------------------------------------------------------------------------
*/
Boolean UCSimpleFindString (
	const UniChar src[], ItemCount srcLength,
	const UniChar sub[], ItemCount subLength,
	UCCollateOptions options )
{
	Boolean						foldCase, doExpansion;
	Boolean						matched;
	UniChar				*srcPtr, *srcExp, *subPtr, *subExp;
	ItemCount					srcExpLen, subExpLen;
	UniChar						firstSubChar, firstSubCharFolded;
	UniChar				*lowerCaseTable;
	unsigned short*		decompositionIndices;
	UniChar*			decompositions;
	
	//
	//	IMPORTANT STUFF!!!!	Get the tables we use
	//
	
#if USE_GLOBAL_DATA
	lowerCaseTable = gLowerCaseTable;
	decompositionIndices = gDecompositionIndices;
	decompositions = gDecompositions;	
#else
	lowerCaseTable = Get_gLowerCaseTable();
	decompositionIndices = Get_gDecompositionIndices();
	decompositions = Get_gDecompositions();	
#endif

	/* A couple of degenerate cases: */
	if ( srcLength == 0 ) return false;
	if ( subLength == 0 ) return true;
	
	/* Here we have nonzero content in both strings. */
	
	foldCase = ( (options & kUCCollateCaseSensitiveMask) == 0 );
	doExpansion = ( (options & kUCCollateComposeSensitiveMask) == 0 );
	
	/* Pull out the first char of sub and set up the rest or the string */
	subPtr = (UniChar*) &sub[0];
	if ( doExpansion )
	{
		subExp = GetCharExpansion( subPtr++, &subExpLen, decompositionIndices, decompositions );
		firstSubChar = *subExp++; subExpLen--;
		if ( subExpLen == 0 ) subLength--;
	}
	else
	{
		firstSubChar = *subPtr++;
		subExpLen = 0;
	}
	if ( foldCase ) firstSubCharFolded = MakeUniCharLowerCase( firstSubChar, lowerCaseTable );
	
	srcPtr = (UniChar*) &src[0];
	srcExpLen = 0;
	
	matched = false;
	while ( !matched && srcLength > 0 )
	{
		UniChar		srcChar;
	
		/* See if the first character of the pattern matches */
		if ( doExpansion )
		{
			if ( srcExpLen == 0 )
				srcExp = GetCharExpansion( srcPtr++, &srcExpLen, decompositionIndices, decompositions );
			srcChar = *srcExp++; srcExpLen--;
			if ( srcExpLen == 0 ) srcLength--;
		}
		else
		{
			srcChar = *srcPtr++; srcLength--;
		}
	
		matched = (srcChar == firstSubChar);
		if ( !matched && foldCase )
		{
			srcChar = MakeUniCharLowerCase( srcChar, lowerCaseTable );
			matched = (srcChar == firstSubCharFolded);
		}
		
		/* If the first character matched, go into a loop that checks the rest of sub */
		if ( matched )
		{
			UniChar		*srcPtrToCheck = srcPtr, *srcExpToCheck = srcExp;
			ItemCount	srcLenToCheck = srcLength, srcExpLenToCheck = srcExpLen;
			UniChar		*subPtrToCheck = subPtr, *subExpToCheck = subExp;
			ItemCount		subLenToCheck = subLength, subExpLenToCheck = subExpLen;
			
			while ( matched && (srcLenToCheck > 0 && subLenToCheck > 0) )
			{
				UniChar		srcChar, subChar;
				
				if ( doExpansion )
				{
					if ( srcExpLenToCheck == 0 )
						srcExpToCheck = GetCharExpansion( srcPtrToCheck++, &srcExpLenToCheck, decompositionIndices, decompositions );
					if ( subExpLenToCheck == 0 )
						subExpToCheck = GetCharExpansion( subPtrToCheck++, &subExpLenToCheck, decompositionIndices, decompositions );
					
					srcChar = *srcExpToCheck++; srcExpLenToCheck--;
					if ( srcExpLenToCheck == 0 ) srcLenToCheck--;

					subChar = *subExpToCheck++;	subExpLenToCheck--;
					if ( subExpLenToCheck == 0 ) subLenToCheck--;
				}
				else
				{
					srcChar = *srcPtrToCheck++;
					subChar = *subPtrToCheck++;
				}
				
				matched = (srcChar == subChar);
				if ( !matched && foldCase )
				{
					srcChar = MakeUniCharLowerCase( srcChar, lowerCaseTable );
					subChar = MakeUniCharLowerCase( subChar, lowerCaseTable );
					matched = (srcChar == subChar);
				}
			} /* loop over each character in sub */
			
			if ( matched && srcLenToCheck == 0 && srcExpLenToCheck == 0 )
			{
				/* Handle a partial match at the tail */
				matched = ( subLenToCheck == 0 && subExpLenToCheck == 0 );
				
				/* Bail 'cause you're not going to find a match after this */
				if ( !matched ) srcLength = 0;
			}
		}
	} /* loop over each character in src */

	return matched;
} /* UCSimpleFindString */


UCCollateResult 
ReorderAndCompare( UniChar* charsSeen1, UniChar* charsSeen2, int count, int bufferPos, unsigned char* compositionClassTable )
//	We assume count > 1
{

	UniChar s1[ kBufferSize ];
	UniChar s2[ kBufferSize ];
	int i;
	int j;
	UCCollateResult returnValue = 0;
	UniChar*	t1 = charsSeen1;
	UniChar*	t2 = charsSeen2;

	//	First, let's roll our buffers around into a more useable position

	count %= kBufferSize;
	j = ( bufferPos + kBufferSize - count + 1 ) % kBufferSize;
		//	j is the index in the charsSeen buffers where the buffer "starts"
		//	(the buffers are set up as wrapping around and around and around...)
		//	if j is 0, then they start at index 0, and so we don't need to 
		//		unwrap them
	if ( j != 0 ) {
		for ( i = 0; i < count; i++, j = ( j+1 ) % kBufferSize ) {
			s1[ i ] = charsSeen1[ j ];
			s2[ i ] = charsSeen2[ j ];
			}
		t1 = s1;
		t2 = s2;
		}

	//	Second, sort them by their combination classes
	//		(We use a bubble sort 'cuz it's easy to write and performance isn't a big issue in this
	//		low-runner function.)

	for ( i = 1; i < count; i++ )
		for ( j = 0; j < count - i; j++ ) {
			UniChar temp;
			int class1 = GetCombiningClass( t1[ j ], compositionClassTable );
			int class2 = GetCombiningClass( t1[ j+1 ], compositionClassTable );
			if ( class1 != 0 && class2 != 0 && class1 > class2 ) {
				temp = t1[ j ];
				t1[ j ] = t1[ j+1 ];
				t1[ j+1 ] = temp;
				}
			class1 = GetCombiningClass( t2[ j ], compositionClassTable );
			class2 = GetCombiningClass( t2[ j+1 ], compositionClassTable );
			if ( class1 != 0 && class2 != 0 && class1 > class2 ) {
				temp = t2[ j ];
				t2[ j ] = t2[ j+1 ];
				t2[ j+1 ] = temp;
				}
			}
		
	//	Third, compare the entire string
	
	for ( i = 0; returnValue == 0 && i < count; i++ )
		if ( t1[ i ] != t2[ i ] ) 
			returnValue = ( t1[ i ] > t2[ i ] ) ? 1 : -1;
	
	return returnValue;
	
}

#endif
