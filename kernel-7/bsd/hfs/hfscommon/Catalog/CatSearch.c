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
	File:		CatSearch.c

	Contains:	CatSearch (fast search of all files on a volume)

	Version:	System 7.5.x (Tempo?)

	Copyright:	© 1996-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contact:		Don Brady

		Technology:			HFS+

	Writers:

		(JL)	Jim Luther
		(RNP)	Roger Pantos
		(DSH)	Deric Horn
		(djb)	Don Brady
		(msd)	Mark Day

	Change History (most recent first):
	  <Rhap>	03/31/98	djb		Sync up with final HFSVolumes.h header file.

	  		 	4/14/98		DSH		Now also compile some compare routines for Rhapsody for use in searchfs().
	  <CS18>	11/3/97		JL		#2001483 - Return WDCBRecPtr from CatSearchC.
	  <CS17>	10/19/97	msd		Bug 1684586. GetCatInfo and SetCatInfo use only contentModDate.
	  <CS16>	10/17/97	msd		Conditionalize DebugStrs.
	  <CS15>	10/10/97	msd		Bug 1683670. InitCatSearch was calling the wrong routine to
									flush the volume; only the user files were getting flushed.
									Added casts to Ptr for first parameter to UprText.
	  <CS14>	 9/27/97	RNP		Replace UprString() with UprText().
	  <CS13>	  9/9/97	msd		The PLstrcmp routine we link against has a bug with one
									character strings, so just call our own PascalBinaryCompare
									instead.
	  <CS12>	  9/4/97	msd		In CatalogNodeData, change attributeModDate to modifyDate.
	  <CS11>	 7/28/97	msd		Use fast new B-Tree scanner.
	  <CS10>	 7/23/97	msd		Radar #1668914. CatSearch should compare the file's backup date
									with the search info's backup date range, not the create date
									range. The mod date compare had the same problem.
	   <CS9>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS8>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	   <CS7>	 6/24/97	djb		Include "CatalogInternal.h"
	   <CS6>	 6/20/97	msd		Use contentModDate and attributeModDate fields instead of
									modifyDate. Use CatalogNodeData as common record format. Got rid
									of NEW_CATSEARCH define by removing old code.
	   <CS5>	 6/17/97	msd		Bug #1661443. Initialize cat position to parent dirID =
									fsRtParID. When restoring a cat position, special case that
									parent ID since there is no such thread record. If ioSearchTime
									is -1, assume they meant infinite, and use 0 instead. The call
									to ConvertUnicodeToHFSName in CheckCriteria was getting
									parameters from the wrong places.
		 <4>	 6/13/97	djb		Change PrepareOutputName calls to ConvertUnicodeToHFSName calls.
	   <CS3>	  6/9/97	msd		Convert HFS Plus dates from UTC to local time in MakeCatDirRec
									and MakeCatFilRec.
	   <CS2>	 5/16/97	msd		Fix some string type conversion problems in InsertMatch().
	   <CS1>	 4/24/97	djb		first checked in
	 <HFS22>	 4/16/97	djb		Always use new B-tree code.
	 <HFS21>	 4/11/97	DSH		use extended VCB fields catalogRefNum, and extentsRefNum.
	 <HFS20>	 3/27/97	djb		Unicode string lengths are now passed as a byte count.
	 <HFS19>	 2/19/97	djb		Get in sync with new B-tree definitions...
	 <HFS18>	  2/6/97	msd		When instrumentation is active, log a complete FSSpec when a
									match is inserted into the buffer.
	 <HFS17>	  2/5/97	msd		On HFS+ disks, CheckCriteria needs to test against the HFS-style
									copy of the catalog record rather than the HFS variants of the
									CatalogRecord of the actual catalog data.
	 <HFS16>	 1/29/97	msd		InitCatSearch needs to preset the BTree position by searching
									for a node before the directory record of the root so that the
									root will be searched, too.
	 <HFS15>	 1/29/97	msd		CheckCriteria was ignoring the fsSBNegate bit when testing the
									ioDirMask bit in the file attributes. Made sure that thread
									records never match.
	 <HFS14>	 1/17/97	msd		Add trivial instrumentation.
	 <HFS13>	 1/16/97	msd		When using the new BTree manager, the old btcWCount field is
									always zero, so use numUpdateNodes instead. However,
									numUpdateNodes can be zero so use numUpdateNodes+1 instead; this
									still allows the caller to set the writeCount in the
									CatPositionRec to zero to indicate a new search.
	 <HFS12>	  1/9/97	djb		Get in sync with new HFSVolumesPriv.i..
	 <HFS11>	  1/9/97	msd		The CatPosition was not being updated during or after the call.
									Also, InitCatSearch was going back to the root directory instead
									of the parent directory specified in the CatPosition.
									InitCatSearch should not use GetCatalogNode; it should use
									SearchBTreeRecord instead since we want to be able to find
									thread records and move relative to them.
	 <HFS10>	  1/3/97	djb		Integrated latest HFSVolumesPriv.h changes.
	  <HFS9>	12/23/96	msd		Oops. Re-enable timers.
	  <HFS8>	12/23/96	msd		Update to use new BTree interfaces. Conditionalized the code via
									the NEW_CATSEARCH symbol. The code should compile both ways, but
									should be cleaned up later. There is a DebugStr in case the
									BTree returns an unexpected error. Performance is noticably
									worse than the prior implementation.
	  <HFS7>	12/19/96	DSH		All refs to VCB are now refs to ExtendedVCB
	  <HFS6>	12/18/96	msd		Use PrepareOutputName when copying a found item's name into
									FSSpec array.
	  <HFS5>	12/12/96	msd		Use the Unicode wrapper functions.
	  <HFS4>	12/10/96	msd		First version of HFS+ changes complete. The HFS+ catalog data is
									converted to a CatalogRecord before comparison. Copied code from
									Catalog.c for dealing with the Unicode names.
	  <HFS3>	12/10/96	msd		Begin HFS+ changes.  Check PRAGMA_LOAD_SUPPORTED before loading
									precompiled headers.
	  <HFS2>	12/10/96	msd		Add support for precompiled headers.
	  <HFS1>	 12/9/96	msd		first checked in

*/

/*
Public routines:
	CatSearchC
					Searches the entire volume for files/folders matching given
					criteria.  It does this by reading every record in the Catalog
					BTree; all non-index records are compared to the given criteria.
					And files/folders that match have their FSSpec's returned
					in a caller-supplied buffer.  The caller may also provide a
					buffer to use when reading the BTree; otherwise, a default
					buffer within the File Manager is used.
	
	VerifyCatSearchPB
					Validates the parameters in a CatSearch parameter block to
					make sure the values are mutually consistent and in range.

Internal routines:
	ExtOffLineCheck
					Make sure the given volume is both online and HFS (or HFS+).
					If not, an appropriate error is returned.

	StartScan		Initialize state for scan and return first record.
	EndScan			Cleanup state and return hint
	ScanNextRecord	Get the next record in the scan.
*/

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
        #include <Timer.h>
        #include <TextUtils.h>
        #include <LowMemPriv.h>
        #include <PLStringFuncs.h>
        #else
        #include "../headers/system/MacOSStubs.h"
        #endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */



#include "../headers/HFSBtreesPriv.h"
#include "../headers/HFSVolumes.h"
#include "../headers/FileMgrInternal.h"
#include "../headers/system/HFSUnicodeWrappers.h"
#include "../headers/BTreesInternal.h"
#include "../headers/BTreesPrivate.h"
#include "../headers/BTreeScanner.h"
#include "../headers/CatalogPrivate.h"

#if HFSInstrumentation
	#include "../headers/system/Instrumentation.h"
#endif


#ifndef CATSEARCH_DEBUG
#define CATSEARCH_DEBUG 1
#endif

#define BTREE_CB_TYPE				BTreeControlBlock
#define BTREE_WRITE_COUNT(btcb)		(btcb->numUpdateNodes+1)

extern OSErr VerifyCatSearchPB( const CSParam *pb, UInt8 *outAttributes, UInt8 *outFlags );
extern OSErr CatSearchC( CSParam *pb, WDCBRecPtr *wdcb );
extern OSErr ExtOffLineCheck( ExtendedVCB *vcb );

extern OSErr CheckExternalFileSystem( ExtendedVCB *vcb );
extern OSErr DeturmineVolume3( VolumeParam *iopb, short *nameLength, Boolean *nameSpecified, ExtendedVCB **vcb, WDCBRec **wdcb, unsigned char **pathName );
extern OSErr FlushVolume( ExtendedVCB *vcb );
extern	OSErr	FlushVolumeBuffers( ExtendedVCB *vcb );
extern OSErr ChkNode_glue( BTNodeDescriptor *node, BTREE_CB_TYPE *btcb );
extern void LocRec_glue( BTNodeDescriptor *node, BTREE_CB_TYPE *btcb, UInt32 recordNumber,
						 Ptr *key, Ptr *data, UInt32 *recordSize );

//
//€€ These need to be moved to an interface file
//
enum {
	kBTreePrevious	= -1,
	kBTreeCurrent	= 0,
	kBTreeNext		= 1
	
};

enum {
	//
	//	Masks for bits in the file attributes
	//
//	kFileLockedMask		= 1,
	kMaskBatAttributes	= ~(ioDirMask | kHFSFileLockedMask),
	
	//
	//	Masks for search bits
	//
	kMaskNotSearchInfo2	= ~(fsSBPartialName | fsSBFullName | fsSBNegate),
	kMaskLengthBits		= (fsSBFlLgLen | fsSBFlPyLen | fsSBFlRLgLen | fsSBFlRPyLen),

	//
	//	Masks for flags
	//
	kIncludeDirs		= 1,
	kIncludeFiles		= 2,
	kIncludeNames		= 4,
	
	kMinimumBufferSize	= 512
};


struct CatPosition {
	UInt32		writeCount;			//	The BTree's write count (to see if the catalogwriteCount
									//	changed since the last search).  If 0, the rest
									//	of the record is invalid, start from beginning.
	UInt32		nextNode;			//	node number to resume search
	UInt32		nextRecord;			//	record number to resume search
	UInt32		recordsFound;		//	number of leaf records seen so far
};
typedef struct CatPosition CatPosition;


typedef BTScanState ScanState;


struct CatSearchState {
	TMTask			timer;					//	MUST be first field!
	CSParam			*pb;
	Str31			targetName;				//	Upper case version of name being matched against
	FSSpec			*nextMatch;
	long			searchBits;
	CatalogKey		*currentKey;			//	Pointer to key within current record
	CatalogRecord	*currentData;			//	Pointer to data within current record
#if HFSInstrumentation
	InstTraceClassRef trace;
	InstEventTag	eventTag;
	InstDataDescriptorRef traceDescriptor;
#endif
	short			vRefNum;				//	Volume reference of volume being searched
	Boolean			isHFS;					//	True if volume is HFS
	UInt8			attributes;				//	desired file attributes to match against
	UInt8			flags;
	Boolean			timerCompleted;			//	Set to true by timer's completion routine
};
typedef struct CatSearchState CatSearchState;

#if TARGET_OS_MAC
static OSErr InitCatSearch( ExtendedVCB *volume, CSParam *pb, ScanState *scanState );
static void UpdateCatPosition( const ExtendedVCB *volume, CatPosition *endingPosition, const ScanState *scanState );
static void InsertMatch(CatSearchState *searchState);
static Boolean CheckCriteria(ExtendedVCB *vcb, const CatSearchState *searchState);
Boolean CompareMasked(const UInt32 *thisValue, const UInt32 *compareData,
							 const UInt32 *compareMask, UInt32 count);
Boolean ComparePartialName(ConstStr31Param thisName, ConstStr31Param substring);
Boolean CompareFullName(ConstStr31Param thisName, ConstStr31Param testName);
#endif	/* TARGET_OS_MAC */
Boolean CompareRange(UInt32 val, UInt32 low, UInt32 high);
#define CompareRange(val, low, high)	(val >= low && val <= high)

extern void CSTimerGlue(void);
extern void CatSearchTimerComplete(CatSearchState *state);

UInt32 Low32BitsPinned(UInt64 x);
#define Low32BitsPinned(x)	(x.hi ? 0xFFFFFFFF : x.lo)



/*
;__________________________________________________________________________________
;
; Routine:		CMCatSearch
;
; Function: 	Locates files by search criteria
;
; Input:		A0.L  -  parameter block pointer
;
; Register Usage (during call):
;	A0.L  -  scratch									D0.L - scratch
;	A1.L  -  scratch									D1.L - scratch
;	A2.L  -  scratch									D2.L - scratch
;	A3.L  -  pointer to CatSearch State Record (CSSR)	D3.L - scratch
;	A4.L  -  pointer to PScan State Record (PSR)		D4.L - scratch
;	A5.L  -  pointer to user's param block              D5.L - ioSearchBits
;	sp   -  standard file system stack
;														D7.L - ioActMatchCount
;
; Output:		D0.W  -  result code (noErr, extFSErr, ioErr, nsvErr, catChanged, paramErr)
;	A0.L, A1.L - trash
;	D0.L, D1.L, D2.L - trash
;__________________________________________________________________________________
*/
#if TARGET_OS_MAC
OSErr CatSearchC( CSParam *pb, WDCBRecPtr *wdcb )
{
	OSErr			err;
	ExtendedVCB		*vcb;
	short			volNameLength;
	Boolean			volNameSpecified;
	unsigned char	*pathname;
	UInt32			catalogRecordSize;
	ScanState		scanState;
	CatSearchState	searchState;
	Boolean			timerInstalled = false;
#if HFSInstrumentation
	FSVarsRec			*fsVars = (FSVarsRec *) LMGetFSMVars();
#endif


#if HFSInstrumentation
	searchState.trace = (InstTraceClassRef) fsVars->later[0];
	searchState.traceDescriptor = (InstDataDescriptorRef) fsVars->later[1];
	searchState.eventTag = InstCreateEventTag();
	InstLogTraceEvent( searchState.trace, searchState.eventTag, kInstStartEvent);
#endif

	//
	//	Start a timer for the call
	//
	searchState.timerCompleted = false;

	if (pb->ioSearchTime != 0 && pb->ioSearchTime != -1) {
		searchState.timer.tmAddr = (TimerUPP) CSTimerGlue;
		InsTime((QElem *) &searchState.timer);
		PrimeTime((QElem *) &searchState.timer, pb->ioSearchTime);
		timerInstalled = true;
	}
	
	//
	//	Find the volume specified
	//
	err = DeturmineVolume3((VolumeParam *) pb, &volNameLength, &volNameSpecified, &vcb, wdcb, &pathname);
	if (err != noErr) goto ErrorExit;

	//
	//	Make sure the volume is ours, and online.
	//
	err = ExtOffLineCheck(vcb);
	if (err != noErr) goto ErrorExit;

	//
	//	Set up some information about the volume
	//
	searchState.vRefNum = vcb->vcbVRefNum;
	if (vcb->vcbSigWord == kHFSSigWord)
		searchState.isHFS = true;
	else
		searchState.isHFS = false;
	
	//
	//	Clear ioActMatchCount
	//
	pb->ioActMatchCount = 0;
	
	//
	//	Verify the parameter block
	//
	err = VerifyCatSearchPB(pb, &searchState.attributes, &searchState.flags);
	if (err != noErr) goto ErrorExit;

	//
	//	If no matches requested, exit early
	//
	if (pb->ioReqMatchCount == 0) {
		err = noErr;
		goto ErrorExit;
	}
	
	searchState.pb = pb;
	searchState.searchBits = pb->ioSearchBits;
	searchState.nextMatch = pb->ioMatchPtr;
	
	//
	//	If a name is part of the criteria, prepare it for matching.  (We make it upper
	//	case and remove diacriticals.)
	//
	if (searchState.flags & kIncludeNames) {
		StringPtr name;
		name = pb->ioSearchInfo1->hFileInfo.ioNamePtr;
		//	If caller's name is longer than HFS names can be, then
		//	we can optimize.
		if (name[0] > 31) {
			//	Nothing can possibly match the given name.  If the caller asked
			//	us to negate the search, then we need to return everything.
			if (searchState.searchBits & fsSBNegate) {
				searchState.searchBits = 0;		//	causes everything to match
			}
			else {
				err = eofErr;					// nothing left to match
				goto ErrorExit;
			}
		}
		else {
			//
			//	Make a copy of the supplied name and convert it to upper case
			//	for later comparison.
			//
			//€€ There is a bug in the assembly CatSearch: it calls _FSUprString which
			//€€ is really just _UpperString.  It passes the length of the string in D0.
			//€€ According to the interfaces, the second parameter (in D0) is a boolean
			//€€ to determine whether it should be diacritical sensitive (i.e. whether it
			//€€ should leave diacriticals vs. stripping them).  Apparently, this always
			//€€ evaluates to false (testing the high bit of the word?).
			//
			//€€ I disagree with the above analysis. The $A054 trap, which _FSUprString is 
			//€€ aliased to, is actually UprStringTrap(), implemented in :SuperMario:OS:SysUtil.a. 
			//€€ It does in fact take a ptr to text in A0 and len in D0.   UpperString(), 
			//€€ on the other hand, is glue in OSTrapsGlue.a which munges 
			//€€ the input parameters (including the diac boolean) and then turns around and calls
			//€€ _UprString, with A0 pointing to text and D0 holding length. Same as assembly CatSearch.
			//€€ So _UprString (or _UpperString) != UpperString(), sadly enough.
			//€€ - RNP.
			//
			// Note: we still do this even for HFS+, since we want to compare the names that
			// would actually be returned in the same way as HFS.  That is name comparisons
			// for HFS and HFS+ should behave identically if they returned the same name via
			// GetCatInfo, even though HFS+ may be truncating or otherwise mangling the name
			// to fit in 31 characters; we want to compare against the mangled name.
			//
			BlockMoveData(name, searchState.targetName, sizeof(Str31));
			UprText( (Ptr)(searchState.targetName + 1), searchState.targetName[ 0]);
		}
	}

	//
	//	Set up for the search.
	//
	err = InitCatSearch(vcb, pb, &scanState);
	#if DEBUG_BUILD
		if (err > 0) DebugStr("\pCatSearch: Internal Error");
	#endif
	
	//
	//	Now loop over the records
	//
	while (err == noErr) {
        err = BTScanNextRecord(&scanState, searchState.timerCompleted, (void**)&searchState.currentKey, (void**)&searchState.currentData, &catalogRecordSize);
		if (err == noErr) {
			//	Check for a match
			if (CheckCriteria(vcb, &searchState)) {					//	Found a match?
				err = BTScanCacheNode(&scanState);					//		Make sure current node is cached
				InsertMatch(&searchState);							//		Return in to caller
				if (++pb->ioActMatchCount == pb->ioReqMatchCount)	//		Got enough matches yet?
					break;
			}
		}
	}
	
	//
	//	See if we've scanned the entire Catalog BTree.  If so, return eofErr.
	//	If we exited because of a timeout, then it's not an error.
	//
	if (err == btNotFound || err == fsBTEndOfIterationErr)
		err = eofErr;
	if (err == fsBTTimeOutErr)
		err = noErr;
	#if DEBUG_BUILD
		if (err > 0) DebugStr("\pCatSearch: Internal Error");
	#endif
	//
	//	Update caller's CatPosition so they can resume the search
	//
	UpdateCatPosition(vcb, (CatPosition *) &pb->ioCatPosition, &scanState);

	//
	//	If we get here, we never actually started a scan, so all we have left
	//	is to remove the timer (if we installed one) and return the error code.
	//
ErrorExit:
	//
	//	Stop timer
	//
	if (timerInstalled)
		RmvTime((QElem *) &searchState.timer);
	
#if HFSInstrumentation
	InstLogTraceEvent( searchState.trace, searchState.eventTag, kInstEndEvent);
#endif

	return err;
}

#endif

/*
;__________________________________________________________________________________
;
; Routine:		VerifyCSPB
;
; Function: 	Verify that a param block contains legal values
;
; Input:		A0.L  -  parameter block pointer
;
; Register Usage (during call):
;	A0.L  -  pointer to user's param block              D0.L - scratch
;	A1.L  -  pointer to ioSearchInfo1							D1.L - scratch
;														D2.B - copy of user's ioFlAttrib mask
;														D3.L - cssr.flags byte
;	A4.L  -  pointer to ioSearchInfo2
;	A5.L  -  pointer to user's param block              D5.L - ioSearchBits
;
; Output:
;	d0.W  -  result code (noErr, paramErr)
;	d2.b  -  copy of user's ioFlAttrib mask, ioDirFlg masked
;	d3.b  -  cssr.flags with inclFiles, inclDirs and inclNames set correctly
;
; Here are the parameter checking rules for PBCatSearch:						<1.3>
; The labels at the left also appear in the code which checks that rule.
;
; You get a paramErr if
; P1	ioMBufPtr is nil
; P2	ioSearchInfo1 is nil
; P3	(ioSearchBits includes not(fsSBPartialName or fsSBFullName or fsSBNegate)) and ioSearchInfo2 is nil;
;		i.e. A selected search bit requires the presence of ioSearchInfo2
;
; Then these,
; ioSearchBits rules: You get a paramErr if
; P4	fsSBPartialName
;		and (ioSearchInfo1->ioNamePtr = (nil or empty string))
; P5	fsSBFullName (same as above)
; P6	fsSBFlAttrib and some bit other than 0 (locked) or 4 (directory) returns paramErr
; P7	search includes directories and
;		(ioSearchBits includes fsSBFlAttrib) and (ioSearchInfo2^.ioFlAttrib bit 0 = 1)
;		return paramErr
;		(i.e. "locked directory" makes no sense)
; P8	fsSBDrNmFls and files returns paramErr
; P9	fsSBFlLgLen and directories returns paramErr
; P10	fsSBFlPyLen (same as above)
; P11	fsSBFlRLgLen (same as above)
; P12	fsSBFlRPyLen (same as above)
;
; A search includes (files only) if
;	ioSearchBits includes fsSBFlAttrib
;			- and -
;	ioSearchInfo1^.ioFlAttrib bit 4 = 0 		i.e. explicitly looking for non-directories
;			- and -
;	ioSearchInfo2^.ioFlAttrib bit 4 = 1
;
; A search includes (directories only) if
;	ioSearchBits includes fsSBFlAttrib
;			- and -
;	ioSearchInfo1^.ioFlAttrib bit 4 = 1 		i.e. explicitly looking for directories
;			- and -
;	ioSearchInfo2^.ioFlAttrib bit 4 = 1
;
; A search includes both if
;	ioSearchBits includes fsSBFlAttrib
;			- and -
;	ioSearchInfo2^.ioFlAttrib bit 4 = 0 		i.e. don't care about the directory bit
;
;			- OR -
;	ioSearchBits doesn't include fsSBFlAttrib   i.e. don't care about attributes at all
;__________________________________________________________________________________
*/

OSErr VerifyCatSearchPB(
	const CSParam	*pb,				// user's parameter block
	UInt8			*outAttributes,		// ioFlAttrib, with ioDirFlg cleared
	UInt8			*outFlags)			// cssr.flags with inclFiles, inclDirs and inclNames set correctly
{
	SInt8		attributes = 0;				// local copy of output outAttributes
	UInt8		flags;					// local copy of output outFlags
	long		searchBits;				// copy of pb->ioSearchBits
	CInfoPBPtr	searchInfo1;			// copy of pb->ioSearchInfo1
	CInfoPBPtr	searchInfo2;			// copy of pb->ioSearchInfo2
	
	//
	//	P1: User must supply a buffer to return the found FSSpec's
	//
	if (pb->ioMatchPtr == NULL)
		goto ParamErrExit;
	
	//
	//	P2: Must supply values to compare against (ioSearchInfo1)
	//
	searchInfo1 = pb->ioSearchInfo1;
	searchInfo2 = pb->ioSearchInfo2;
	if (searchInfo1 == NULL)
		goto ParamErrExit;

	searchBits = pb->ioSearchBits;
	
	//
	//	P3: See if we need any end-of-range or mask values (in ioSearchInfo2).
	//	If so, the pointer must be non-NULL.
	//
	if (((searchBits & kMaskNotSearchInfo2) != 0) && (searchInfo2 == NULL))
		goto ParamErrExit;

	flags = 0;
	
	//
	//	P4-5: If we're searching by name (partial or full), then check the name passed.
	//
	if (searchBits & (fsSBPartialName | fsSBFullName)) {
		StringPtr name = searchInfo1->hFileInfo.ioNamePtr;
		if (name == NULL || name[0] == 0)
			goto ParamErrExit;
		flags |= kIncludeNames;
	}
	
	//
	//	Determine whether we're looking for files, folders, or both.
	//
	if (searchBits & fsSBFlAttrib) {
		attributes = searchInfo2->hFileInfo.ioFlAttrib;		// get mask for attributes
		if (attributes & kMaskBatAttributes)	// P6: only locked and directory bits are allowed
			goto ParamErrExit;
		if (attributes & ioDirMask)				// do they care whether object is a file or folder?
			if (searchInfo1->hFileInfo.ioFlAttrib & ioDirMask)
				flags |= kIncludeDirs;			// wanted folders only
			else
				flags |= kIncludeFiles;			// wanted files only
		else
			flags |= (kIncludeDirs | kIncludeFiles);	// caller didn't care about files vs. folders
	}
	else
		flags |= (kIncludeDirs | kIncludeFiles);
	
	attributes &= ~ioDirMask;			// clear ioDirMask since it isn't actually on disk

	//
	//	P7: If they cared about file attributes, and search includes folders, then
	//	ioLockFlag must not be set (since it is meaningless for folders).
	//
	if ((flags & kIncludeDirs) && (searchBits & fsSBFlAttrib) && (attributes & kHFSFileLockedMask))
		goto ParamErrExit;

	//
	//	P8: If search includes files, then # of files per folder must not be set.
	//
	if ((searchBits & fsSBDrNmFls) && (flags & kIncludeFiles))
		goto ParamErrExit;

	//
	//	P9-P12: If search includes directories, then none of the fork length bits
	//	must be set.
	//
	if ((flags & kIncludeDirs) && (searchBits & kMaskLengthBits))
		goto ParamErrExit;

	//
	//	When we get here, all the checks have passed.  Return the output flags
	//	and file attributes.
	//
	*outAttributes = attributes;
	*outFlags = flags;
	return noErr;

	//
	//	If we get here, there was a problem with one of the parameters.
	//
ParamErrExit:
	return paramErr;
}



//
//	Copy the FSSpec for the current catalog node into the caller's match buffer
//
#if	TARGET_OS_MAC
static void InsertMatch(CatSearchState *searchState)
{
	CatalogKey	*current;
	FSSpec		*match;
	
	//	Get the key for the item we just found, and the place where we should
	//	store the item's FSSpec
	current = searchState->currentKey;
	match = searchState->nextMatch;
	
	//	Copy vRefNum
	match->vRefNum = searchState->vRefNum;

	if (searchState->isHFS) {
		//	Copy parentDirID
		match->parID = current->hfs.parentID;
		
		//	Copy the name as a PString
		BlockMoveData(current->hfs.nodeName, &match->name[0], StrLength(current->hfs.nodeName) + 1);
	}
	else {
		TextEncoding		encoding;
		HFSCatalogNodeID	cnid;

		cnid = searchState->currentData->hfsPlusFile.fileID;				// note: cnid at same offset for folders
		encoding = searchState->currentData->hfsPlusFile.textEncoding;	// note: textEncoding at same offset for folders

		//	Copy parentDirID
		match->parID = current->hfsPlus.parentID;
		
		//	Copy the name as a PString
		(void) ConvertUnicodeToHFSName( &current->hfsPlus.nodeName, encoding, cnid, &match->name[0]);	
	}
	
#if HFSInstrumentation
//	InstLogTraceEvent( searchState->trace, searchState->eventTag, kInstMiddleEvent);
	{
		struct {
			long	vRefNum;
			long	parID;
			char	name[32];
		} x;
		unsigned	len;
		
		x.vRefNum = match->vRefNum;
		x.parID = match->parID;
		
		len = match->name[0];
		BlockMoveData(&match->name[1], &x.name[0], len);
		x.name[len] = 0;
				
		InstLogTraceEventWithDataStructure( searchState->trace, searchState->eventTag, kInstMiddleEvent,
											searchState->traceDescriptor, (UInt8 *) &x, sizeof(x));
	}
#endif

	//	Point to next FSSpec in caller's buffer
	searchState->nextMatch++;
}
#endif	/* TARGET_OS_MAC */



//
//	Determine whether the current catalog node matches the caller's criteria.  Return
//	true if the object matches.
//
#if	TARGET_OS_MAC
static Boolean CheckCriteria(ExtendedVCB *vcb, const CatSearchState *searchState)
{
	CatalogKey		*key;
	Boolean			matched;
	long			searchBits;
	CInfoPBRec		*searchInfo1;
	CInfoPBRec		*searchInfo2;
	CatalogNodeData	catData;
	
	key = searchState->currentKey;
	
	//	Change the catalog record data into a single common form
	catData.nodeType = 0;		//	mark this record as not in use
	CopyCatalogNodeData(vcb, searchState->currentData, &catData);
	
	switch (searchState->currentData->recordType) {
		case kHFSFolderThreadRecord:
			return false;		// Never match a thread record!
			break;
		case kHFSPlusFolderThreadRecord:
			return false;		// Never match a thread record!
			break;

		case kHFSFolderRecord:
		case kHFSFileRecord:
		case kHFSPlusFolderRecord:
		case kHFSPlusFileRecord:
			break;

		default:
			return false;			// Not a file or folder record, so can't search it
	}
	
	searchBits = searchState->searchBits;
	matched = true;					//	Assume we got a match

	//	See if this type of object (file vs. folder) is one of the types they're trying
	//	to match.
	if (catData.nodeType == kCatalogFolderNode && (searchState->flags & kIncludeDirs) == 0 ) {
		//	Found a folder, but they don't want folders.
		matched = false;
		goto TestDone;
	}
	if (catData.nodeType == kCatalogFileNode && (searchState->flags & kIncludeFiles) == 0 ) {
		//	Found a file, but they don't want files.
		matched = false;
		goto TestDone;
	}

	searchInfo1 = searchState->pb->ioSearchInfo1;
	searchInfo2 = searchState->pb->ioSearchInfo2;
	
	//
	//	First, attempt to match the name -- either partial or complete.
	//	If the volume is HFS+, then we need to make a Str31 version of the name
	//	to use for comparison.
	//
	if (searchBits & (fsSBPartialName | fsSBFullName)) {
		Str31			keyName;
		ConstStr31Param	namePtr;

		//	Make a pointer to a PString version of the name
		if (searchState->isHFS)
			namePtr = &key->hfs.nodeName[0];
		else {
			TextEncoding	encoding;
			
			encoding = searchState->currentData->hfsPlusFile.textEncoding;	// same offset for folders

			(void) ConvertUnicodeToHFSName(&key->hfsPlus.nodeName, encoding, catData.nodeID, keyName);
			namePtr = keyName;
		}

		//	Check for a partial name match
		if (searchBits & fsSBPartialName) {
			matched = ComparePartialName(namePtr, searchState->targetName);
			// Exit if no match, or we no there's nothing more to compare
			if (matched == false || searchBits == fsSBPartialName)
				goto TestDone;
		}
		
		//	Check for a full name match
		if (searchBits & fsSBFullName) {
			matched = CompareFullName(namePtr, searchState->targetName);
			// Exit if no match, or we no there's nothing more to compare
			if (matched == false || searchBits == fsSBFullName)
				goto TestDone;
		}
	}
	
	//
	//	parent ID
	//
	if (searchBits & fsSBFlParID) {
		HFSCatalogNodeID	parentID;
		
		//	Note: ioFlParID and ioDrParID have the same offset in search info, so no
		//	need to test the object type here.  However, we do need to test for HFS
		//	vs. HFSPlus, since their key structures are different.
		
		if (searchState->isHFS)
			parentID = key->hfs.parentID;
		else
			parentID = key->hfsPlus.parentID;

		matched = CompareRange(parentID, searchInfo1->hFileInfo.ioFlParID,
								searchInfo2->hFileInfo.ioFlParID);
		if (matched == false) goto TestDone;
	}

	//
	//	File attributes
	//
	if (searchBits & fsSBFlAttrib) {
		UInt32	temp;
		
		temp = (catData.nodeFlags ^ searchInfo1->hFileInfo.ioFlAttrib) & searchState->attributes;
		if (temp != 0) {
			matched = false;
			goto TestDone;
		}
	}
	
	//
	//	Number of files in a directory
	//
	if (searchBits & fsSBDrNmFls) {
		matched = CompareRange(catData.valence, searchInfo1->dirInfo.ioDrNmFls,
								searchInfo2->dirInfo.ioDrNmFls);
		if (matched == false) goto TestDone;
	}
	
	//
	//	Finder Info
	//
	if (searchBits & fsSBFlFndrInfo) {
		UInt32 *thisValue;
		
		thisValue = (UInt32 *) &catData.finderInfo[0];

		//	Note: ioFlFndrInfo and ioDrUsrWds have the same offset in search info, so
		//	no need to test the object type here.
		matched = CompareMasked(thisValue, (UInt32 *) &searchInfo1->hFileInfo.ioFlFndrInfo,
								(UInt32 *) &searchInfo2->hFileInfo.ioFlFndrInfo,
								sizeof(FInfo)/sizeof(UInt32));

		if (matched == false) goto TestDone;
	}
	
	//
	//	Extended Finder Info
	//
	if (searchBits & fsSBFlXFndrInfo) {
		UInt32 *thisValue;
		
		thisValue = (UInt32 *) &catData.extFinderInfo[0];

		//	Note: ioFlXFndrInfo and ioDrFndrInfo have the same offset in search info, so
		//	no need to test the object type here.
		matched = CompareMasked(thisValue, (UInt32 *) &searchInfo1->hFileInfo.ioFlXFndrInfo,
								(UInt32 *) &searchInfo2->hFileInfo.ioFlXFndrInfo,
								sizeof(FXInfo)/sizeof(UInt32));

		if (matched == false) goto TestDone;
	}
	
	//
	//	File logical length (data fork)
	//
	if (searchBits & fsSBFlLgLen) {
		matched = CompareRange(catData.dataLogicalSize, searchInfo1->hFileInfo.ioFlLgLen,
								searchInfo2->hFileInfo.ioFlLgLen);
		if (matched == false) goto TestDone;
	}
	
	//
	//	File physical length (data fork)
	//
	if (searchBits & fsSBFlPyLen) {
		matched = CompareRange(catData.dataPhysicalSize, searchInfo1->hFileInfo.ioFlPyLen,
								searchInfo2->hFileInfo.ioFlPyLen);
		if (matched == false) goto TestDone;
	}
	
	//
	//	File logical length (resource fork)
	//
	if (searchBits & fsSBFlRLgLen) {
		matched = CompareRange(catData.rsrcLogicalSize, searchInfo1->hFileInfo.ioFlRLgLen,
								searchInfo2->hFileInfo.ioFlRLgLen);
		if (matched == false) goto TestDone;
	}
	
	//
	//	File physical length (resource fork)
	//
	if (searchBits & fsSBFlRPyLen) {
		matched = CompareRange(catData.rsrcPhysicalSize, searchInfo1->hFileInfo.ioFlRPyLen,
								searchInfo2->hFileInfo.ioFlRPyLen);
		if (matched == false) goto TestDone;
	}
	
	//
	//	Create date
	//
	if (searchBits & fsSBFlCrDat) {
		matched = CompareRange(catData.createDate, searchInfo1->hFileInfo.ioFlCrDat,
								searchInfo2->hFileInfo.ioFlCrDat);
		if (matched == false) goto TestDone;
	}
	
	//
	//	Mod date
	//
	if (searchBits & fsSBFlMdDat) {
		matched = CompareRange(catData.contentModDate, searchInfo1->hFileInfo.ioFlMdDat,
								searchInfo2->hFileInfo.ioFlMdDat);
		if (matched == false) goto TestDone;
	}
	
	//
	//	Backup date
	//
	if (searchBits & fsSBFlBkDat) {
		matched = CompareRange(catData.backupDate, searchInfo1->hFileInfo.ioFlBkDat,
								searchInfo2->hFileInfo.ioFlBkDat);
	}

TestDone:
	//
	//	Finally, determine whether we need to negate the sense of the match (i.e. find
	//	all objects that DON'T match).
	//
	if (searchBits & fsSBNegate) {
		matched = !matched;
	}
	
	return matched;
}
#endif	/* TARGET_OS_MAC */



Boolean CompareMasked(const UInt32 *thisValue, const UInt32 *compareData,
							 const UInt32 *compareMask, UInt32 count)
{
	Boolean	matched;
	UInt32	i;
	
	matched = true;			//	Assume it will all match
	
	for (i=0; i<count; i++) {
		if (((*thisValue++ ^ *compareData++) & *compareMask++) != 0) {
			matched = false;
			break;
		}
	}
	
	return matched;
}


#if TARGET_OS_MAC
Boolean CompareFullName(ConstStr31Param thisName, ConstStr31Param testName)
{
	Str31	tempName;

	//
	//	Make a quick check first.  If the lengths differ, they can't possibly match.
	//
	if (thisName[0] != testName[0])
		return false;

	//
	//	Now make an upper case copy of thisName.
	//
	//€€ Would it be better to skip making upper case versions of the names
	//€€ and just call EqualString(thisName, testName, false, false)?  I think it
	//€€ should give the same result.
	//
	BlockMoveData(thisName, tempName, sizeof(Str31));
	UprText( (Ptr)(tempName + 1), tempName[0]);
	
	//
	//	Compare tempName and testName; they must be identical PStrings to match.
	//
	return PascalBinaryCompare(tempName, testName);	
}
#endif	/* TARGET_OS_MAC */



#if TARGET_OS_MAC
Boolean ComparePartialName(ConstStr31Param thisName, ConstStr31Param substring)
{
	Boolean	matched;
	Str31	tempName;

	//
	//	Make a quick check first.  If the substring is longer than this object's name,
	//	it can't possibly match.
	//
	if (substring[0] > thisName[0])
		return false;

	//
	//	Now make an upper case copy of thisName.
	//
	//€€ Would it be better to skip making upper case versions of the names
	//€€ and just call EqualString(thisName, testName, false, false)?  I think it
	//€€ should give the same result.
	//
	BlockMoveData(thisName, tempName, sizeof(Str31));
	UprText( (Ptr)(tempName + 1), tempName[0]);
	
	//
	//	Now see if substring is a substring of tempName.
	//
	matched = (PLstrstr(tempName, substring) != NULL);
	
	return matched;
}
#endif	/* TARGET_OS_MAC */



//€€ Same function appears in Volumes.c as CheckVolumeOffLine(), but the code is slightly different
//€€ we should delete one copy.
#if TARGET_OS_MAC
OSErr ExtOffLineCheck( ExtendedVCB *vcb )
{
	OSErr	err;
	
	err = CheckExternalFileSystem(vcb);
	if (err == noErr) {
		if (vcb->vcbDrvNum != 0)
			LMSetReqstVol((Ptr) vcb);
		else
			err = volOffLinErr;
	}
	
	return err;
}
#endif


void CatSearchTimerComplete(CatSearchState *state)
{
	state->timerCompleted = true;
}

#if TARGET_OS_MAC
static OSErr InitCatSearch( ExtendedVCB *volume, CSParam *pb, ScanState *scanState )
{
	OSErr			err;
	FCB				*fcb;
	BTREE_CB_TYPE	*btcb;
	CatPosition		*startingPosition;	
	void			*buffer;
	UInt32			bufferSize;
	FSVarsRec		*fsVars;
	
	//
	//	Set up pointers to the catalog's FCB and BTCB
	//	
	fcb = GetFileControlBlock(volume->catalogRefNum);
	btcb = (BTREE_CB_TYPE *) fcb->fcbBTCBPtr;
	
	//
	//	Flush all files on this volume so the BTrees will be up-to-date (since we only
	//	read the BTree on disk, not what might be in the control blocks).
	//
	LMSetFlushOnly(0xFF);
	err = FlushVolume(volume);
	if (err != noErr) goto ErrorExit;
	
	//
	//	Make sure the volume control files (especially the Catalog B-tree) gets flushed,
	//	too, or else we'll be reading stale catalog data.
	//
	err = FlushVolumeBuffers(volume);
	if (err != noErr) goto ErrorExit;

	//
	//	Use our internal optimization buffer unless the caller supplied a suitable one.
	//
	fsVars = (FSVarsRec *) LMGetFSMVars();
	buffer		= pb->ioOptBuffer;
	bufferSize	= pb->ioOptBufSize;
	if (buffer == NULL || bufferSize < fsVars->gAttributesBufferSize) {
		buffer		= fsVars->gAttributesBuffer;
		bufferSize	= fsVars->gAttributesBufferSize;
	}

	//
	//	Set up the scan state to start a new search, or resume a previous one.
	//
	startingPosition = (CatPosition *) &pb->ioCatPosition;
	if (startingPosition->writeCount == 0) {
		//
		//	Starting a new search
		//
		err = BTScanInitialize(fcb, 0, 0, 0, buffer, bufferSize, scanState);
	}
	else {
		//
		//	Resuming a previous search.  Make sure the catalog hasn't changed
		//	since the previous search.
		//
		if (startingPosition->writeCount != BTREE_WRITE_COUNT(btcb)) {
			startingPosition->writeCount = BTREE_WRITE_COUNT(btcb);		// return the new write count
			err = catChangedErr;
			goto ErrorExit;
		}
		
		err = BTScanInitialize(fcb, startingPosition->nextNode, startingPosition->nextRecord, startingPosition->recordsFound,
								buffer, bufferSize, scanState);
	}

ErrorExit:
	return err;
}
#endif	/* TARGET_OS_MAC */

#if TARGET_OS_MAC
static void UpdateCatPosition( const ExtendedVCB *volume, CatPosition *endingPosition, const ScanState *scanState )
{
	FCB				*fcb;
	BTREE_CB_TYPE	*btcb;

	//
	//	Set up pointers to the catalog's FCB and BTCB
	//	
	fcb = GetFileControlBlock(volume->catalogRefNum);
	btcb = (BTREE_CB_TYPE *) fcb->fcbBTCBPtr;
	
	//
	//	Update the caller's CatPosition for the next time
	//
	endingPosition->writeCount	= BTREE_WRITE_COUNT(btcb);
	BTScanTerminate(scanState, &endingPosition->nextNode, &endingPosition->nextRecord, &endingPosition->recordsFound);
}
#endif	/* TARGET_OS_MAC */
