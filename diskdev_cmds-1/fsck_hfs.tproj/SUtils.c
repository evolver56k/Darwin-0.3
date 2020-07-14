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
	File:		SUtils.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(DSH)	Deric Horn

	Change History (most recent first):

	  <HFS8>	 12/2/97	DSH		Add IsFileSharingOn routine
	  <HFS7>	 11/4/97	DSH		Misc ApplScratch changes
	  <HFS6>	10/21/97	DSH		InvalidateCalculatedVolumeBitMap()
	  <HFS5>	 8/18/97	DSH		DebugStr to M_DebugStr
	  <HFS4>	 5/21/97	DSH		Added UsersAreConnected()
	  <HFS3>	 3/27/97	DSH		Removing DebugStrs
	  <HFS2>	 3/17/97	DSH		ProcessInfoRec needs fields initialized
*/



#include "ScavDefs.h"
#include "Prototypes.h"

#include "DFALowMem.h"

#ifdef INVESTIGATE

/*------------------------------------------------------------------------------

Routine:	ValidDrive

Function: 	Given a DQE address, return a boolean that determines whether
			or not a drive is to be considered for scavenging.  This is 
			primarily used to filter out weird things like AppleShare volumes
			and non-hfs partitions.
			
Input:		Arg 1	- DQE pointer

Output:		D0.L -	0 if drive not to be used
					1 otherwise			
------------------------------------------------------------------------------*/

Boolean ValidDrive ( DrvQElPtr dqPtr )
{
	DCtlHandle dce;
	
	if (dqPtr->dQFSID != kMac_FSID)		// does it have HFS File System ID?
		return( false );
	
	dce = GetDCtlEntry(dqPtr->dQRefNum);
		
	return ( ((**dce).dCtlFlags & Read_Enable_Mask) != 0 );		// is device readable (block device)?
}


//
//	Set one of our ApplScratch LowMems to use as a global pointer in
//	inconvinient times.
//
void	SetApplScratchValue( short whichLong, UInt32 value )	//	[0, 1, 2]
{
	UInt32	*scratchP;
	
	scratchP = (UInt32 *)LMGetApplScratch();
	scratchP[whichLong] = value;			//	set the scratch ptr to contain our value.
	
	LMSetApplScratch( (Ptr)scratchP );
}

//
//	Retrieve one of our ApplScratch LowMems.
//
UInt32	GetApplScratchValue( short whichLong )
{
	return( ((UInt32 *) ( LMGetApplScratch() ))[whichLong] );
}

#endif

//
//	Get one of our DFA replacement LowMem values.  The LM Get/Set routines
//	are redirected through this bit of code for accessing LowMems to be maintained
//	by DFA.  We keep them as an array pointed to by a pointer stored in one of our
//	ApplScratch longs.
//
UInt32	GetDFALowMem( SInt16 whichLowMem )
{
	UInt32	*dfaLowMemArray;
	
	dfaLowMemArray = (UInt32 *)GetApplScratchValue( kDFALowMemIndex );
	
	if ( dfaLowMemArray != 0 )
		return( dfaLowMemArray[ whichLowMem ] );
	else
		return( 0 );
}


//
//	Set one of our DFA replacement LowMem values.
//
void	SetDFALowMem( SInt16 whichLowMem, UInt32 value )
{
	UInt32	*dfaLowMemArray;
	
	dfaLowMemArray = (UInt32 *)GetApplScratchValue( kDFALowMemIndex );
	
	dfaLowMemArray[ whichLowMem ] = value;
}


//
//	Set one of our DFA replacement LowMem values.
//
UInt32	GetDFAStage( void )
{
	UInt32	stage;
	
	if ( AccessDFALowMem() == false )
		stage	= kHFSStage;
	else
		stage	= GetDFALowMem( kDFAStage );
		
	return( stage );
}

void	SetDFAStage( UInt32 stage )
{
	SetDFALowMem( kDFAStage, stage );
}


//	DFA Globals accesor routine
UInt32	GetDFAGlobals( void )
{
	return( GetDFALowMem( kDFAGlobals ) );
}

#ifdef INVESTIGATE

//
//	Calculates the maximum safe available TempMem for us to use
//
SInt32	CalculateSafePhysicalTempMem( void )
{
	#define	kProcessManagerSlop	512*1024	//	leave 512K for process manager
	OSErr				err;
	SInt32				availableMemory;
	ProcessSerialNumber	psn;
	ProcessInfoRec		processInfo;
	Size				dummy, maxTempMem;

	err = Gestalt( gestaltPhysicalRAMSize, &availableMemory );
	if( err != noErr )
	{
		M_DebugStr("\p Gestalt gestaltPhysicalRAMSize FAILED");
		goto BAIL;
	}

	availableMemory -= ((SInt32)(LMGetSysZone())->bkLim);

	err = GetCurrentProcess( &psn );
	if( err != noErr )
	{
		M_DebugStr("\p GetCurrentProcess FAILED");
		goto BAIL;
	}
	
	processInfo.processName		= nil;
	processInfo.processAppSpec	= nil;
	
	err = GetProcessInformation( &psn, &processInfo );
	if( err != noErr )
	{
		M_DebugStr("\p GetProcessInformation FAILED");
		goto BAIL;
	}

	availableMemory -= processInfo.processSize;
	maxTempMem = TempMaxMem( &dummy );
	
	//	return the lesser of the two, and leave some slop for safety
	if ( availableMemory < maxTempMem )
		return( availableMemory - kProcessManagerSlop );
	else
		return( maxTempMem - kProcessManagerSlop );

BAIL:
	M_DebugStr("\p CalculateSafePhysicalTempMem FAILED");
	return( 64*1024 );
}

#endif

/*------------------------------------------------------------------------------

Routine:	RcdError

Function:	Record errors detetected by scavenging operation.
			
Input:		GPtr		-	pointer to scavenger global area.
			ErrCode		-	error code

Output:		None			
------------------------------------------------------------------------------*/

void RcdError( SGlobPtr	GPtr, OSErr errorCode)
{
	GPtr->ErrCode = errorCode;
	
	WriteError ( errorCode, GPtr->TarID, GPtr->TarBlock );	//	log to summary window
}


/*------------------------------------------------------------------------------

Routine:	GetVBlk	(Get Volume Block)

Function:	Gets a specified logical block from the target volume.  The block
			is obtained through the cache.
			
			Note:  The cache buffer is released right away leaving the buffer
			unlocked.
			
Input:		GPtr		-	pointer to scavenger global area.
			BlkNum		-	logical block number
			BufPtr		-	pointer to cache buffer containing block

Output:		GetVBlk	-	function result:
								0 =	no error
								n =	error
------------------------------------------------------------------------------*/

int GetVBlk( SGlobPtr GPtr, UInt32 BlkNum, void **BufPtr)
{
	OSErr		result;
	
	result = GetBlock_FSGlue( gbDefault, BlkNum, (Ptr *)BufPtr, GPtr->calculatedVCB->vcbVRefNum, GPtr->calculatedVCB );
	if (result != 0)
	{
		result = IntError( GPtr,result );
		return( result );							//	could't get it
	}	
 
	result = RelBlock_FSGlue( *BufPtr, 0 );			//	release it, to unlock buffer

	return( noErr );								//	all is fine
}


/*------------------------------------------------------------------------------

Routine:	AllocExt (Allocate Extent)

Function:	Allocates an extent in the Scavengers volume bit map.
			
Input:		GPtr		-	pointer to scavenger global area.
			startBlock	-	starting allocation block number.
			blockCount	-	number of allocation blocks.

Output:		AllocExt	-	function result:			
								0 = no error
								n = error
30Mar91		KSCT	Fixing a bug which used short for an unsigned short.
------------------------------------------------------------------------------*/

OSErr AllocExt( SGlobPtr GPtr, const UInt32 startBlock, const UInt32 blockCount)
{
	UInt32				i;
	UInt32				currentAllocationBlock;
	unsigned char		mask;
	Ptr					byteP;
	UInt16				bitPos;
	UInt32				startBlockInBuffer;
	UInt32				blockCountInBuffer;
	
	VolumeBitMapHeader	*volumeBitMap				= GPtr->volumeBitMapPtr;
	UInt32				blocksPerBuffer				= volumeBitMap->bufferSize * 8;	//	number of allocation blocks represented in each buffer
	UInt32				bufferStartAllocationBlock	= volumeBitMap->currentBuffer * blocksPerBuffer;
	UInt32				bufferEndAllocationBlock	= ((volumeBitMap->currentBuffer+1) * blocksPerBuffer) - 1;


	//	Since we have to alternate BitMap buffers, make sure this one is in the current buffer
	if ( (startBlock > bufferEndAllocationBlock) || (startBlock+blockCount < bufferStartAllocationBlock) )
		return( noErr );

	//
	//	first make sure the extent doesn't overlap
	//

	//	Find the offset into the current buffer in which the extent starts
	startBlockInBuffer = startBlock;
	blockCountInBuffer = blockCount;
	
	if ( startBlock < bufferStartAllocationBlock )
	{
		startBlockInBuffer = bufferStartAllocationBlock;
		blockCountInBuffer = blockCountInBuffer - ( bufferStartAllocationBlock - startBlockInBuffer );
	}
	if ( startBlock+blockCount > bufferEndAllocationBlock )
	{
		blockCountInBuffer = blockCountInBuffer - ( startBlock + blockCount - bufferEndAllocationBlock );
	}
	
	startBlockInBuffer %= blocksPerBuffer;							//	calculate the offset into the buffer
	currentAllocationBlock = startBlockInBuffer;

	byteP = volumeBitMap->buffer + (startBlockInBuffer >> 3);		//	ptr to starting byte
	
	for ( i = 1 ; i <= blockCountInBuffer ; i++ )
	{
		bitPos = (currentAllocationBlock++ % 8);
		mask = (0x80 >> bitPos);
		if ((*byteP & mask) != 0)
		{
			RcdError( GPtr, E_OvlExt );
			return( E_OvlExt );										//	overlapped extents
		}
		if (bitPos == 7)
			byteP++;
	}

	//
	//	now do the real allocation
	//
	currentAllocationBlock = startBlockInBuffer;
	byteP = volumeBitMap->buffer + (startBlockInBuffer >> 3);	/* ptr to starting byte */
	
	for ( i = 1; i <= blockCountInBuffer; i++ )
	{
		bitPos = (currentAllocationBlock++ % 8);
		mask = (0x80 >> bitPos);
		*byteP = *byteP | mask;
		if (bitPos == 7)
			byteP++;
	}
//	MountCheck() calculates the freeBlocks for us
	GPtr->calculatedVCB->freeBlocks -= blockCountInBuffer;	/* adjust free block count */
	volumeBitMap->bitMapRecord[volumeBitMap->currentBuffer].count += blockCountInBuffer;
	
	return( noErr );								/* all done */
	
}	/* end AllocExt */


/*------------------------------------------------------------------------------

Routine:	IntError

Function:	Records an internal Scavenger error.
			
Input:		GPtr		-	pointer to scavenger global area.
			ErrCode		-	internal error code

Output:		IntError	-	function result:			
								(E_IntErr for now)
------------------------------------------------------------------------------*/

int IntError( SGlobPtr GPtr, OSErr errorCode )
{
	GPtr->RepLevel = Unrepairable;
	
	if ( errorCode == ioErr )				//	Cast I/O errors as read errors
		errorCode	= R_RdErr;
		
	if( (errorCode == R_RdErr) || (errorCode == R_WrErr) )
	{
		GPtr->ErrCode	= GPtr->volumeErrorCode;
		GPtr->IntErr	= 0;
		return( errorCode );		
	}
	else
	{
		GPtr->ErrCode	= R_IntErr;
		GPtr->IntErr	= errorCode;
		return( R_IntErr );
	}
	
}	//	End of IntError


/*------------------------------------------------------------------------------

Routine:	AllocBTN (Allocate BTree Node)

Function:	Allocates an BTree node in a Scavenger BTree bit map.
			
Input:		GPtr		-	pointer to scavenger global area.
			StABN		-	starting allocation block number.
			NmABlks		-	number of allocation blocks.

Output:		AllocBTN	-	function result:			
								0 = no error
								n = error
------------------------------------------------------------------------------*/

int AllocBTN( SGlobPtr GPtr, SInt16 fileRefNum, UInt32 nodeNumber )
{
	UInt16				bitPos;
	unsigned char		mask;
	char				*byteP;
	FCB					*calculatedFCB;
	BTreeControlBlock	*calculatedBTCB;

	//	set up

	calculatedFCB = GetFileControlBlock( fileRefNum );
	calculatedBTCB = (BTreeControlBlock *)calculatedFCB->fcbBTCBPtr;

	//	allocate the node 

	byteP = ( (BTreeExtensionsRec*)calculatedBTCB->refCon)->BTCBMPtr + (nodeNumber / 8 );	//	ptr to starting byte
	bitPos = nodeNumber % 8;						//	bit offset
	mask = ( 0x80 >> bitPos );
	if ( (*byteP & mask) != 0 )
	{	
		RcdError( GPtr, E_OvlNode );
		return( E_OvlNode );					//	node already allocated
	}
	*byteP = *byteP | mask;						//	allocate it
	calculatedBTCB->freeNodes--;		//	decrement free count
	
	return( noErr );
}

#ifdef INVESTIGATE

/*------------------------------------------------------------------------------

Routine:	GetFBlk	(Get File Block)

Function:	Gets a specified file block from the target volume.  The block
			is obtained through the cache.
			
			Note:  The cache buffer is released right away leaving the buffer
			unlocked.
			
Input:		GPtr		-	pointer to scavenger global area.
			fileRefNum	-	file refnum
			BlkNum		-	file block number
			BufPtr		-	pointer to cache buffer containing block

Output:		GetFBlk	-	function result:
								0 =	no error
								n =	error
------------------------------------------------------------------------------*/

OSErr GetFBlk( SGlobPtr GPtr, SInt16 fileRefNum, SInt32 blockNumber, void **bufferH )
{
	OSErr			result;
	
	result = GetBlock_FSGlue( gbDefault, blockNumber, (Ptr *)bufferH, fileRefNum, GPtr->calculatedVCB );
	
	if ( result != noErr )
	{
		result = IntError( GPtr, result );				 //	could't get it
		return( result );
	}	
 
 	//€€	perhaps we should copy block to a buffer before releasing, nahhh
	result = RelBlock_FSGlue( *bufferH, 0 );				//	release it, to unlock buffer

	return( noErr );									//	all is fine
}

#endif

OSErr	GetBTreeHeader( SGlobPtr GPtr, SInt16 fileRefNum, HeaderRec **header )
{
	OSErr				err;
//	BTreeControlBlock	*btcb;
	
//	btcb = (BTreeControlBlock *)GetFileControlBlock((fileRefNum))->fcbBTCBPtr;
	
	GPtr->TarBlock = kHeaderNodeNum;					/* set target node number */

	err = GetBlock_FSGlue( gbReleaseMask, kHeaderNodeNum, (Ptr *)header, fileRefNum, GPtr->calculatedVCB );
	
	//	Validate Node Size
	switch ( (**header).nodeSize )								// node size == 512*2^n
	{
		case   512:
		case  1024:
		case  2048:
		case  4096:
		case  8192:
		case 16384:
		case 32768:		break;
		default:		RcdError( GPtr, E_InvalidNodeSize );
						return( E_InvalidNodeSize );			//	bad tree depth
	}

	return( err );
}


/*------------------------------------------------------------------------------

Routine:	Alloc[Minor/Major]RepairOrder

Function:	Allocate a repair order node and link into the 'GPtr->RepairXxxxxP" list.
			These are descriptions of minor/major repairs that need to be performed;
			they are compiled during verification, and executed during minor/major repair.

Input:		GPtr	- scavenger globals
			n		- number of extra bytes needed, in addition to standard node size.

Output:		Ptr to node, or NULL if out of memory or other error.
------------------------------------------------------------------------------*/

RepairOrderPtr AllocMinorRepairOrder( SGlobPtr GPtr, int n )						/* #extra bytes needed */
{
	RepairOrderPtr	p;						/* the node we allocate */
	
	n += sizeof(RepairOrder);				/* add in size of basic node */
	
	p = (RepairOrderPtr) NewPtr(n);			/* get the node */
	
	if (p != NULL )							/* if we got one... */
	{
		p->link = GPtr->MinorRepairsP;		/* then link into list of repairs */
		GPtr->MinorRepairsP = p;
	}
	
	if ( GPtr->RepLevel == No_RepairNeeded )
		GPtr->RepLevel = Minor_Repair;

	return( p );							/* return ptr to node */
}



void	InvalidateCalculatedVolumeBitMap( SGlobPtr GPtr )
{
	short				bufferNumber;
	VolumeBitMapHeader	*volumeBitMap	= GPtr->volumeBitMapPtr;

	volumeBitMap->currentBuffer	= -1;
	
	for ( bufferNumber=0 ; bufferNumber<volumeBitMap->numberOfBuffers ; bufferNumber++ )
	{
		volumeBitMap->bitMapRecord[bufferNumber].processed	= false;
	}
}

#ifdef INVESTIGATE

/*------------------------------------------------------------------------------

Routine:	UsersAreConnected

Function:	This functions uses the ServerDispatch trap (0xA094) to check to
			see if users are connected.  The SCGetUserNameRec selector is used to 
			iterate through all connected users to get info about them and will 
			return an error (fnfErr) when there are no more users.  This also works
			to find out if there are any connected users by asking for the first
			user which is what we are doing here.  If file sharing is not on, the
			ServerDispatch call will return paramErr telling us that there can't be
			any connected users.

Output:		Boolean, true if users are connected
------------------------------------------------------------------------------*/
static OSErr ServerDispatch( void *pb )
{
	if ( GetOSTrapAddress( 0xa094 ) == GetToolTrapAddress(_Unimplemented) ) 
		return ( paramErr );
	else 
		return CallA094( 0, pb );
}

Boolean	UsersAreConnected( void )
{
	Boolean 		usersAreConnected = false;
	ParamBlockRec 	pb;
	OSErr			err;
	
	// server dispatch code
	pb.cntrlParam.csCode 		= 19;		//	Server dispatch codes
	
	// words 0 & 1 or the csParam are the id of the user to get.
	// use 0 to get the first user
	pb.cntrlParam.csParam[0] 	= 0;
	pb.cntrlParam.csParam[1] 	= 0;
	
	// set namePtr to NULL since we don't care about retrieving the name
	pb.cntrlParam.ioNamePtr		= NULL;

	err = ServerDispatch( &pb );
	if ( err == noErr )
		usersAreConnected		= true;
	
	return ( usersAreConnected );
}


// routines to see if file sharing is enabled.  Converted from Inside Macintosh, Files volume

/*------------------------------------------------------------------------------
Routine:	VolIsSharable
Function:	determines if a volume is sharable
Input:		vRefNum:  the volume to check
Output:		true if sharable, false otherwise
------------------------------------------------------------------------------*/

static Boolean VolIsSharable( short vRefNum )
{
	HParamBlockRec			myHPB;
	GetVolParmsInfoBuffer	myInfoBuffer;
	
	myHPB.ioParam.ioNamePtr		= nil;
	myHPB.ioParam.ioVRefNum		= vRefNum;
	myHPB.ioParam.ioBuffer		= (Ptr) &myInfoBuffer;
	myHPB.ioParam.ioReqCount	= sizeof( myInfoBuffer );
	
	if ( PBHGetVolParms (&myHPB, false) == noErr )
		if ( myInfoBuffer.vMAttrib & (1 << bHasPersonalAccessPrivileges) )
			return( true );
		else
			return( false );
	else
		return( false );
}

/*------------------------------------------------------------------------------
Routine:	FileSharingIsOn
Function:	returns true if at least one mounted volume is shared
Input:		none
Output:		true if sharing is on, false otherwise
------------------------------------------------------------------------------*/

Boolean FileSharingIsOn()
{
	HParamBlockRec	myHPB;
	OSErr			err;
	Boolean			fileSharingIsOn	= false;
	SInt16			volIndex		= 1;
	
	do
	{
		myHPB.volumeParam.ioNamePtr		= nil;
		myHPB.volumeParam.ioVolIndex	= volIndex++;
		
		err = PBHGetVInfo( &myHPB, false );
		
		if ( err == noErr )
			fileSharingIsOn = VolIsSharable( myHPB.volumeParam.ioVRefNum );
	}
	while ( (err == noErr) && (fileSharingIsOn == false) );
	
	return( fileSharingIsOn );
}

#endif
