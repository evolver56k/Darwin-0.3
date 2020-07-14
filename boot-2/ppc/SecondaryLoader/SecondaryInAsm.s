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
			STRING		ASIS

			ifndef	DEBUGLEVEL
DEBUGLEVEL	equ			0
			endif

			ifndef	DEBUG_SHOWPROGRESS
DEBUG_SHOWPROGRESS equ	1
			endif

			include		'SecondaryLoaderStructs.a'
			include		'BootBlocksPriv.a'
			include		'SupportMacros.s'

kBlockSizeLog2	equ		9			; Size of a disk block for HFS
kBlockSize		equ		1 << kBlockSizeLog2

kPageSizeLog2	equ		12			; Size of a page of memory
kPageSize		equ		1 << kPageSizeLog2

; NOTE stack size must be < 32768 to permit use of LI instruction (no sign extension)
kStackSize		equ		31 * 1024	; Size of stack we allocate for our use and for tertiary loader
kRedZoneSize	equ		224			; Size of PowerPC stack "red zone"

kMaxExtents		equ		1024		; Maximum supported number of extents in a file

kMDBBlockNumber	equ		2			; Block number (zero origin) of MDB in an HFS volume
kCatalogFileID	equ		4			; File ID of catalog B*Tree "file"

kLoadFileNameLength equ	16			; Maximum size of load file name

MDBblessedDirID	equ		MDB.finderInfo+0x14		; Offset in MDB for blessed Mac OS 8 blessed dirID

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Static register assignments
rSP				equ		r1			; Stack pointer
;(dup) rTOC		equ		r2			; Pointer to our initialized data

; The nonvolatile registers we always restore upon entry to one of our routines (in
; case the caller has munged them).
rBSS			equ		r31			; Pointer to our "BSS" data
firstNonVolatileReg equ	r31			; Starting register number to save in our stack frames


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Record which defines the symbols for offsets from rBSS for our uninitialized data
BSS			RECORD	0
blockBuf:	ds.b	kBlockSize		; Keep this first so we know it doesn't cross a page boundary
mdbBuf:		ds.b	kBlockSize		; Keep this first so we know it doesn't cross a page boundary
			align	2

; The Exts arrays consist of a longword count followed by an array of that many Extent records
extentsExts: ds.b	4+(3 * Extent.sizeof)			; Extents array for extents B*Tree file
catalogExts: ds.b	4+(kMaxExtents * Extent.sizeof)	; Extents array for catalog B*Tree file
loadExts:	 ds.b	4+(kMaxExtents * Extent.sizeof)	; Extents array for extents of file we are loading

bootIH:		ds.l	1				; Instance handle for boot disk
nPartitions: ds.l	1				; Number of partitions on boot disk
defaultPartitionNumber:
			ds.l	1				; Specified default partition # we try first specially
loadBase:	ds.l	1				; Base address at which we load the next stage image
loadSize:	ds.l	1				; Size in bytes of loaded next stage image

sizeof		equ		$				; Size of this record
			ENDR
			
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		csect	[PR]
SecondaryLoaderMain:
			mflr	r0				; Remember LR to return to Open Firmware with
			bl		NextCode		; Get into LR the address of our data to keep in rTOC

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TOCbase		equ		$

; Transition vectors for PatchVector
TryPartition:		dc.l	.TryPartition-TOCbase, 0
ReadPartitionBlocks: dc.l	.ReadPartitionBlocks-TOCbase, 0
ReadRawBlock:		dc.l	.ReadRawBlock-TOCbase, 0
GetFileExtents:		dc.l	.GetFileExtents-TOCbase, 0
ReadHFSNode:		dc.l	.ReadHFSNode-TOCbase, 0
CompareLoweredStrings: dc.l	.CompareLoweredStrings-TOCbase, 0
_moveeq:			dc.l	._moveeq-TOCbase, 0
FindLeafNode:		dc.l	.FindLeafNode-TOCbase, 0
FileIDCompare:		dc.l	.FileIDCompare-TOCbase, 0

; Array of transition vectors which can be patched out by extensions.  This array and the
; transition vectors to which it refers are automagically relocated to be absolute pointers
; during initialization by adding rTOC to each.
PatchVector:
			dc.l	TryPartition-TOCbase
			dc.l	ReadPartitionBlocks-TOCbase
			dc.l	ReadRawBlock-TOCbase
			dc.l	GetFileExtents-TOCbase
			dc.l	ReadHFSNode-TOCbase
			dc.l	CompareLoweredStrings-TOCbase
			dc.l	_moveeq-TOCbase
			dc.l	FindLeafNode-TOCbase
			dc.l	FileIDCompare-TOCbase
			dc.l	0				; PatchVector's terminating NULL

returnLR:	dc.l	0				; LR value to return to Open Firmware's initial call to us

saveCI:		ds.l	1				; Save slot for Open Firmware Client Interface callback function ptr
saveBSS:	ds.l	1				; Save slot for our BSS data pointer

blessedDirID: ds.l	1				; Directory ID of blessed system folder

ciArgs		ds.l	10				; Space for client interface callback argument block

loadFileName: ds.b	kLoadFileNameLength		; Pascal counted string name of file to load as next stage

; Broken system folder icon in 1 bit/pixel format (we expand this to 8 bit when we need it)
deathIcon:	dc.l	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x07F00000
			dc.l	0x08080000,0x10040000,0x20020000,0x7FFF3FFE,0x80024001,0x80048001,0x80048001,0x803DF801
			dc.l	0x80450401,0x805DF401,0x80529401,0x80549401,0x80525401,0x80549401,0x80525401,0x805EF401
			dc.l	0x80450401,0x80428401,0x8042F401,0x80450401,0x803DF801,0x803DF801,0x800A0001,0xFFFDFFFF
			dc.l	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x07F00000
			dc.l	0x0FF80000,0x1FFC0000,0x3FFE0000,0x7FFF3FFE,0xFFFE7FFF,0xFFFCFFFF,0xFFFCFFFF,0xFFFDFFFF
			dc.l	0xFFFDFFFF,0xFFFDFFFF,0xFFFEFFFF,0xFFFCFFFF,0xFFFE7FFF,0xFFFCFFFF,0xFFFE7FFF,0xFFFEFFFF
			dc.l	0xFFFDFFFF,0xFFFEFFFF,0xFFFEFFFF,0xFFFDFFFF,0xFFFDFFFF,0xFFFDFFFF,0xFFFBFFFF,0xFFFDFFFF

; Collect all strings together so we can keep all misaligned allocations in one chunk
; to avoid wastage.
sInterpret:	dc.b	'interpret', 0

sApple_HFS:	dc.b	'Apple_HFS', 0

sAllocZeroed:	; ( bytes-to-allocate -- allocated-pointer )
			dc.b	'dup alloc-mem ?dup if swap erase else drop 0 then', 0

sGetBootIH:		; ( -- bootIH )
			dc.b	'" /macosboot" open-dev to my-self '		; Open our special boot 'device'
			dc.b	'" dev" get-my-property if " No /macosboot dev property" OOPS then '
			dc.b	'decode-string 2- open-dev -rot 2drop '		; Open the raw device we were loaded from
			dc.b	'dup value bootIH', 0						; Leave instance handle on stack AND in value word bootIH

sReadBlocks:	;  ( buf 512-byte-block# blk-count -- )
			dc.b	'hex '							; Hex is bliss
			dc.b	'2 pick 200 erase '				; Clear buffer first so failures result in "safe junk"
			dc.b	'200 * '						; Convert blk-count to a byte count
			dc.b	'swap 200 um* '					; ( buf byte-count seek.lo seek.hi )
			dc.b	'" seek" bootIH $CM drop '		; Seek to specified 512-byte block
			dc.b	'" read" bootIH $CM drop', 0	; This can't fail.  Right.

sParseBootFile:	; ( -- partition-number )
			dc.b	'hex boot-file parse-2int nip', 0	; Grab part before the "," as a hex number


			if	DEBUG_SHOWPROGRESS
sShowWhatWeLoad:	; ( loadFileStr15 partNum -- )
			dc.b	'." Loading from part# " . 1+ dup 1- c@ type cr', 0
			endif

sInterpretError:
			dc.b	'err in interpret', 0

sGetLoadBase:
			dc.b	'load-base', 0

sAllocationFailure:
			dc.b	'alloc-mem failed', 0

sReadFailure:
			dc.b	'read failed', 0

sNoDir:		dc.b	'no dir', 0

sNextStageReturned:
			dc.b	'tertiary returned', 0

sDisplayError:						; ( c-string-message -- )
			dc.b	'begin dup c@ ?dup while emit 1+ repeat drop cr', 0

sNoBootablePartitions:
			dc.b	'No bootable partition', 0

sCopyright:	dc.b	'Copyright(c)1996-97 Apple Computer Inc. All rights reserved.', 0
			align	2

dataSize:	equ		$

; RECORDs defining INTERPRET Client Interface callback parameter blocks for various values
; of nArgs (N) and nReturns (M).  Naming convention is Interpret_N_M.
rInterpret_1_0	RECORD	0						; INTERPRET callback CI args block ( x -- )
service:	ds.l	1							; "interpret"
nArgs:		ds.l	1							; 2
nReturns:	ds.l	1							; 1
forth:		ds.l	1							; FORTH code to interpret ( x -- )
arg1:		ds.l	1							; Parameter "x" to FORTH code
catchResult: ds.l	1							; CATCH result for interpreted code
			ENDR

rInterpret_1_1	RECORD	0						; INTERPRET callback CI args block ( x -- y )
service:	ds.l	1							; "interpret"
nArgs:		ds.l	1							; 2
nReturns:	ds.l	1							; 2
forth:		ds.l	1							; FORTH code to interpret ( x -- y )
arg1:		ds.l	1							; Parameter "x" to FORTH code
catchResult: ds.l	1							; CATCH result for interpreted code
return1:	ds.l	1							; Returned value "y"
			ENDR

rInterpret_0_1	RECORD	0						; INTERPRET callback CI args block ( -- y )
service:	ds.l	1							; "interpret"
nArgs:		ds.l	1							; 1
nReturns:	ds.l	1							; 2
forth:		ds.l	1							; FORTH code to interpret ( -- y )
catchResult: ds.l	1							; CATCH result for interpreted code
return1:	ds.l	1							; Returned value "y"
			ENDR

rInterpret_2_0	RECORD	0						; INTERPRET callback CI args block ( y z -- )
service:	ds.l	1							; "interpret"
nArgs:		ds.l	1							; 3
nReturns:	ds.l	1							; 1
forth:		ds.l	1							; FORTH code to interpret ( y z -- )
arg1:		ds.l	1							; Parameter "y" to FORTH code
arg2:		ds.l	1							; Parameter "z" to FORTH code
catchResult: ds.l	1							; CATCH result for interpreted code
			ENDR

rInterpret_3_0	RECORD	0						; INTERPRET callback CI args block ( x y z -- )
service:	ds.l	1							; "interpret"
nArgs:		ds.l	1							; 4
nReturns:	ds.l	1							; 1
forth:		ds.l	1							; FORTH code to interpret ( x y z -- )
arg1:		ds.l	1							; Parameter "x" to FORTH code
arg2:		ds.l	1							; Parameter "y" to FORTH code
arg3:		ds.l	1							; Parameter "z" to FORTH code
catchResult: ds.l	1							; CATCH result for interpreted code
			ENDR

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Define registers we use as nonvolatile temporaries herein
SecondaryLoaderTemporaries	RECORD	firstNonVolatileReg-1, DECR
rPartNo			ds.b	1
firstRegToSave	equ		$-1
				ENDR

NextCode:	mflr	rTOC						; Make rTOC point to our constant data
			stw		r0, returnLR-TOCbase(rTOC)	; Save previous LR to get back to O/F if we fail
			stw		r5, saveCI(rTOC)			; Save pointer to client interface callback function

	; Relocate the entries in PatchVector and the associated transition vectors so they
	; contain absolute addresses (i.e., offset by rTOC).
			la		r3, PatchVector-TOCbase-4(rTOC)		; Start with base of PatchVector-4 (so we can use LWZU)

@loop:		lwzu	r4, 4(r3)					; Get next PatchVector element
			cmplwi	r4, 0						; Reached trailing NULL pointer?
			beq		@done						; Bail from loop when done

			addi	r4, r4, rTOC				; Offset value in PatchVector[k] by rTOC
			stw		r4, 0(r3)					; Store back result

		; Relocate transition vector to which this entry (r4) points.
			lwz		r5, 0(r4)					; Get PC field of tvector
			addi	r5, r5, rTOC				; Offset to absolute address
			stw		r5, 0(r4)					; Store result
			stw		rTOC, 4(r4)					; Store TOC field of tvector

			b		@loop						; Loop for all entries in PatchVector

@done:
	; Call Client Interface to allocate some zeroed memory for our BSS data
		WITH	rInterpret_1_1
GetBSSMemory:
			la		r3, ciArgs-TOCbase(rTOC)	; Point at interpret args
			la		r0, sInterpret-TOCbase(rTOC)	; Get a pointer to our "interpret" string
			stw		r0, service(r3)
			li		r0, 2						; Setup nArgs and nReturns
			stw		r0, nArgs(r3)
			stw		r0, nReturns(r3)
			li		r0, BSS.sizeof+(2 * kPageSize)	; Get size of BSS to alloc+space for page alignment
			stw		r0, arg1(r3)
			la		r0, sAllocZeroed-TOCbase(rTOC)	; Point at FORTH to execute
			stw		r0, forth(r3)
			lwz		r0, saveCI(rTOC)			; Get client interface callback function pointer
			mtctr	r0							; Point at client interface callback function
			bctrl								; Call O/F client interface

			la		r0, sAllocationFailure-TOCbase(rTOC)		; Point at our error message
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not
			lwz		r3, ciArgs-TOCbase+catchResult(rTOC)		 ; Check CATCH-RESULT too
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not

			lwz		rBSS, ciArgs-TOCbase+return1(rTOC)	; Get base of allocated space
			addi	rBSS, rBSS, kPageSize-1	; Setup to round up to next page boundary
			clrrwi	rBSS, rBSS, kPageSizeLog2	; Clear low bits to get down to boundary
			
	; Call Client Interface again to allocate the stack we will use and then pass on to the
	; loaded program
GetStackMemory:
			la		r3, ciArgs-TOCbase(rTOC)	; Point at interpret args
			li		r0, kStackSize				; Get size of stack to allocate
			stw		r0, arg1(r3)
			lwz		r0, saveCI(rTOC)			; Get client interface callback function pointer
			mtctr	r0							; Point at client interface callback function
			bctrl								; Call O/F client interface

			la		r0, sAllocationFailure-TOCbase(rTOC)		; Point at our error message
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not
			lwz		r3, ciArgs-TOCbase+catchResult(rTOC)		 ; Check CATCH-RESULT too
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not

			lwz		rSP, ciArgs-TOCbase+return1(rTOC)	; Get base of allocated stack space
			addi	rSP, rSP, kStackSize-kRedZoneSize		; Point at top of this region
		ENDWITH		; rInterpret_1_1
		
		WITH	BSS, SecondaryLoaderTemporaries		; From this point on BSS symbols are OkeeDokee
	
	; Determine the value of the load-base NVRAM configuration variable and save it around
GetLoadBase:
			la			r3, sGetLoadBase-TOCbase(rTOC)	; Get FORTH string pointer
			bl			.Interpret_0_1				; Get load-base
			stw			r3, loadBase(rBSS)			; Save load-base address

	; Open the /macosboot pseudo-device and get its "dev" property to get the path
	; to the disk device we're loading from.  Open that device and save the resulting
	; instance handle in bootIH(rBSS).
			la		r3, sGetBootIH-TOCbase(rTOC)	; Point at FORTH to execute
			bl		.Interpret_0_1					; Open raw disk device
			stw		r3, bootIH(rBSS)				; Save instance handle for raw disk

	; Use Open Firmware driver to get the first partition map entry to find out the count of
	; partitions on the disk.  Save the resulting count in nPartitions(rBSS).
			li		r3, 1							; Get disk raw block number to read
			bl		.ReadRawBlock
			lwz		r0, Partition.pmMapBlkCnt+blockBuf(rBSS)		; Get partition map block count
			stw		r0, nPartitions(rBSS)			; Save around for various loop limits

	; Parse the boot-file environment variable to retrieve the partition number we're supposed
	; to boot from on this disk.
			la		r3, sParseBootFile-TOCbase(rTOC)	; Point at FORTH to execute
			bl		.Interpret_0_1				; Parse boot-file
			mr		rPartNo, r3					; Get result into our loop counter reg

			cmpwi	rPartNo, 0					; Is it nonzero?
			beq		@beginAlgorithmicProbe		; Branch if not valid

			lwz		r0, nPartitions(rBSS)		; Get maximum to check for validity
			cmplw	rPartNo, r0					; Too high?
			bgt		@beginAlgorithmicProbe		; Branch if greater than maximum partition map block #
			
		; Partition number in rPartNo is valid, so remember it in defaultPartitionNumber
			stw		rPartNo, defaultPartitionNumber(rBSS)	; Remember default so we don't try it twice

		; Try specified default partition number to see if she flies...
			mr		r3, rPartNo					; Get default partition number as first to try
			
			li		rPartNo, 1					; Starting loop counter for algorithmic probe in case default doesn't work
			b		@partitionLoopEntry			; Branch into loop for default as first one to try

; We get here only if the default partition number is invalid.  Begin the laborious
; process of looping through all HFS partitions on the disk to try booting each in turn.
; We deliberately avoid trying the default partition again.
@beginAlgorithmicProbe:
			li		rPartNo, 1					; Starting loop counter for algorithmic probe since default doesn't work

@loop:		lwz		r0, defaultPartitionNumber(rBSS)	; Get default, which we have already tried, to avoid doing it again
			cmplw	r0, rPartNo					; Is this the default one (which we skip now)?
			beq		@next						; Skip if yes

			mr		r3, rPartNo					; Get partition # as disk block # to read partition map entry

; We jump here into the middle of the loop with r3 already set to the default partition number but with
; rPartNo set up for the algorithmic loop if the default fails.
@partitionLoopEntry:

			bl		.ReadRawBlock				; Read partition map entry for this partition

			lwz		r3, Partition.pmPartStatus+blockBuf(rBSS)		; Get partition's status flags
			andis.	r0, r3, 0x8000				; Bit #31 on? (i.e., is partition bootable?)
			beq		@next						; Skip if not
			
			andi.	r3, r3, 0x37				; Are all partition validity flags on?
			cmplwi	r3, 0x37
			bne		@next						; Skip if not
			
			la		r3, sApple_HFS-TOCbase(rTOC)	; Get "Apple_HFS" partition type string pointer
			la		r4, Partition.pmParType+blockBuf(rBSS)	; Point at this partition's type field
			bl		.CompareLoweredStrings		; Compare to see if it's the right kind
			bne		@next						; Skip if not

			mr		r3, rPartNo					; Get partition # to try
			bl		.TryPartition				; Otherwise, give it a whirl...

@next:		lwz		r3, nPartitions(rBSS)		; Get maximum loop count
			addi	rPartNo, rPartNo, 1			; Increment loop counter
			cmplw	rPartNo, r3					; Check for loop count overflow
			bgt		DropWithExhaustion			; Branch out of loop if no more to do and FAIL
			b		@loop						; Loop for more
		ENDWITH		; BSS
		ENDWITH		; SecondaryLoaderTemporaries

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; void TryPartition (int partNo)
;
; Check out partition partNo and try to load the next stage from the partition if it is HFS.
; Since this routine is accessed via a patchable vector, we must take care to use the stack
; frame right and conform to PowerPC ABI register conventions.  Particularly, the non-volatile
; registers (r > 12) must be preserved herein -- they must be loaded from their save slots if used.
_try:
		StartStackFrame	TryPartition

; Define registers we use as nonvolatile temporaries herein
TryPartitionTemporaries	RECORD	firstNonVolatileReg-1, DECR
rPartNo			ds.b	1
firstRegToSave	equ		$-1
				ENDR

		EndStackFrame	TryPartition, TryPartitionTemporaries.firstRegToSave

		Entry	TryPartition
		WITH	BSS, TryPartitionTemporaries, MDB
			lwz		rBSS, saveBSS(rTOC)			; Set up our BSS register pointer
			mr		rPartNo, r3					; Remember partition number we're working on
		
	; Grab the volume's Master Directory Block into mdbBuf(rBSS)
			la		r4, mdbBuf(rBSS)			; Buffer to read MDB into
			li		r5, kMDBBlockNumber			; Block number where we'll find MDB
			li		r6, 1						; Number of blocks to read
		; void ReadPartitionBlocks (int partitionNumber, void *buffer, int blockNumber, int nBlocks)
			bl		.ReadPartitionBlocks		; Get get MDB

	; Copy Extents B*Tree extent list in our extentsExts buffer
			li		r3, 3						; Set maximum entry count
			stw		r3, BSS.extentsExts(rBSS)
			la		r3, BSS.extentsExts+4(rBSS)	; Get destination pointer
			la		r4, MDB.extentsExtents+mdbBuf(rBSS)	; Get source pointer
			li		r5, 12						; Get count of bytes to copy
			bl		._moveeq					; Copy from MDB to our BSS buffer

	; Remember the blessed system folder directory ID so comparison functions can use it
			lwz		r0, MDBblessedDirID+mdbBuf(rBSS)		; Get directory ID from MDB
			stw		r0, blessedDirID-TOCbase(rTOC)	; Store it for posterity

	; Get list of extents for the Catalog B*Tree file
			li		r3, kCatalogFileID			; Get file ID of catalog file to look up in
			la		r4, catalogExts(rBSS)		; Get extents list to copy to
			bl		.GetFileExtents				; Find all fragmentation extents of catalog file

	; Get the volume's boot blocks into blockBuf(rBSS) and save the load file's name
	; in loadFileName(rTOC).
			mr		r3, rPartNo					; Get partition number we're working on
			la		r4, blockBuf(rBSS)			; Buffer to read boot block into
			li		r5, 0						; Block number where we'll find boot blocks
			li		r6, 1						; Number of blocks to read
		; void ReadPartitionBlocks (int partitionNumber, void *buffer, int blockNumber, int nBlocks)
			bl		.ReadPartitionBlocks		; Get get first boot block

			la		r3, loadFileName-TOCbase(rTOC)		; Get destination
			la		r4, BootBlocks.coplandLoaderFileName+blockBuf(rBSS)
			li		r5, kLoadFileNameLength		; Maximum size of name
			bl		._moveeq					; Copy filename string

		if	DEBUG_SHOWPROGRESS		; Debugging output
		WITH	rInterpret_2_0
			la		r3, ciArgs-TOCbase(rTOC)	; Point at interpret args
			la		r0, sInterpret-TOCbase(rTOC)	; Get a pointer to our "interpret" string
			stw		r0, service(r3)
			li		r0, 3						; Setup nArgs
			stw		r0, nArgs(r3)
			li		r0, 1						; Setup nReturns
			stw		r0, nReturns(r3)
			la		r0, loadFileName-TOCbase(rTOC)	; Point at filename buffer
			stw		r0, arg1(r3)
			stw		rPartNo, arg2(r3)			; Store partition number we're loading from
			la		r0, sShowWhatWeLoad-TOCbase(rTOC)	; Point at FORTH to execute
			stw		r0, forth(r3)				; showWhatWeLoad ( loadFileStr15 partNum -- )
			lwz		r0, saveCI(rTOC)			; Get client interface callback function pointer
			mtctr	r0							; Point at client interface callback function
			bctrl								; Call O/F client interface to read partition block
		ENDWITH		; rInterpret_2_0
		endif
		
	; Find the catalog B*Tree leaf node containing the first record for the blessed system dir ID
			la		r3, catalogExts(rBSS)		; Get address of extents array to search through
			la		r4, .FileIDCompare-TOCbase(rTOC)	; Get address of routine to call to compare keys
			bl		.FindLeafNode				; Search for first leaf node containing blessed dir ID
			la		r0, sNoDir-TOCbase(rTOC)	; Pessimistically get error string pointer
			cmplwi	r3, 0						; Failure?
			beq		@exit						; Bail if yes


	; Successful exit path.  We get here when we get a valid partition loaded.  Setup parameters to the code we
	; just loaded and jump to it to get the ball rolling really fast!  The stack is already set up in rSP.
	; We actually CAN fail even at this point.  The next stage can return to us, in which case we simply
	; return to continue the partition walk where we left off...
	;
	; The calling convention is:
	;
	; void entryPoint (CICell diskIHandle,
	;					int partitionNumber,
	;					int (*clientInterfacePtr) (CICell *args),
	;					CICell loadBase,
	;					CICell loadSize)

		; Set up next stage's parameters
			lwz		r3, bootIH(rBSS)
			mr		r4, rPartNo
			lwz		r5, saveCI(rTOC)
			lwz		r6, loadBase(rBSS)
			mtctr	r6							; Base is also the entry point
			lwz		r7, loadSize(rBSS)
			stw		rTOC, savedTOC(rSP)			; Save TOC since this is effectively a cross-TOC call
			bctrl								; Off to it -- return only on dire error

		; If next stage returns, it means it failed, so we return to our looping
		; attempting to find a bootable partition on our boot disk bootIH.
			lwz		rTOC, savedTOC(rSP)			; Restore our saved TOC
			la		r0, sNextStageReturned(rTOC)	; Get error message string pointer

@exit:
		WITH	rInterpret_1_0
			la		r3, ciArgs-TOCbase(rTOC)	; Point at interpret args
			stw		r0, arg1(r3)				; Save pointer to our message string
			la		r0, sInterpret-TOCbase(rTOC)	; Get a pointer to our "interpret" string
			stw		r0, service(r3)
			li		r0, 2						; Setup nArgs
			stw		r0, nArgs(r3)
			li		r0, 1						; Setup nReturns
			stw		r0, nReturns(r3)
			la		r0, sDisplayError-TOCbase(rTOC)	; Point at FORTH to execute
			stw		r0, forth(r3)
			lwz		r0, saveCI(rTOC)
			mtctr	r0							; Point at client interface callback function
			bctrl								; Call O/F back to display message and return
		ENDWITH		; rInterpret_1_0
		
		ENDWITH		; BSS
		ENDWITH		; TryPartitionTemporaries
		ENDWITH		; MDB
		Exit	TryPartition
			

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; void _moveeq (void *destP, void *sourceP, unsigned long byteCount)
;
# @(#)28	1.23	R2/inc/sys/asdef.s, bos, bos320 6/24/91 10:56:29
#
# COMPONENT_NAME: (CMDAS) Assembler and Macroprocessor 
#
# FUNCTIONS: 
#
# ORIGINS: 3, 27
#
# (C) COPYRIGHT International Business Machines Corp. 1985, 1990
# All Rights Reserved
# Licensed Materials - Property of IBM
#
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
#
# XCOFF version
#
#	FUNCTIONS: _moveeq
#
#	NAME: _moveeq
#
#	FUNCTION: Equal length character string move
#
#	EXECUTION ENVIRONMENT:
#	Standard register usage and linkage convention.
#	Registers used r0,r3-r12
#	Condition registers used: 0,1
#	No stack requirements.
#
#	NOTES:
#	The source string represented by R5 and R6 is moved to the target string
#	area represented by R3 and R6.
#
#	The strings may be on any address boundary and may be of any length from 0
#	through (2**31)-1 inclusive.  The strings may overlap.  The move is performed
#	in a nondestructive manner (backwards if necessary).
#
#	The addresses are treated as unsigned quantities,
#	i.e., max is 2**32-1.
#
#	The lengths are treated as unsigned quantities,
#	i.e., max is 2**31-1.
#
#	Use of this logic assumes string lengths <= 2**31-1 - Signed arithmetic
#
#	RETURN VALUE DESCRIPTION: Target string modified.
#
#
# The PL.8 entry description is ENTRY(INTEGER VALUE, CHAR(*)) NOSI.
#
#	Calling sequence: _moveeq
#	R3	Address of target string
#	R4	Address of source string
#	R5	Length of source string
#

	EntryNoStack	_moveeq

		srwi.	r0,r5,5			# Number of 32-byte chunks to move (L/32),
								# CR0 = short/long move switch.
		bgt		0,movelong		# Branch if > 32 bytes to move.
#
# Short move (0 to 32 bytes).
#
		mtxer	r5				# Set move length (presuming short).
		lswx	r5,r0,r4		# Get source string (kills R5 - R12).
		stswx	r5,r0,r3		# Store it.
		blr						# Return.
#
# Here we check if we have overlapping strings.
# overlap if: source < target && target < source + length
#		r4	<	r3	&&	r3	<	r4	+	r5
#
movelong:
		cmplw	cr1,r4,r3		# CR1 = forward/backward switch.
		bge		cr1,nobackward	# skip if r4 >= r3
		addc	r10,r4,r5		# r10 = r4 + r5 (source + length)
		cmplw	cr1,r3,r10		# Test if long fwd move might destruct
nobackward:
		mtctr	r0				# CTR = num chunks to move.
		li		r0,32			#
		mtxer	r0				# XER = 32 (move length per iteration).
		rlwinm.	r0,r5,0,0x1f	# R0 = remainder length.
		stw		r3,24(r1)		# save target address for return
		subc	r3,r3,r4		# R3 = targ addr - source addr.
		blt		cr1,backward	# B If A(source) < A(target) logically.

forward:
		lswx	r5,r0,r4		# Get 32 bytes of source.
		stswx	r5,r3,r4		# Store it.
		addic	r4,r4,32		# Increment source address.
		bdnz	forward			# Decr count, Br if chunk(s) left to do.

		mtxer	r0				# XER = remainder length.
		lswx	r5,r0,r4		# Get the remainder string.
		stswx	r5,r3,r4		# Store it.
		lwz		r3,24(r1)		# restore target start address for return
		blr						# Return.

backward:
		addc	r4,r4,r5		# R4 = source addr + len

loopb:
		subic	r4,r4,32		# Decrement source address.
		lswx	r5,r0,r4		# Get 32 bytes of source.
		stswx	r5,r3,r4		# Store it.
		bdnz	loopb			# Decr count, Br if chunk(s) left to do.

		subc	r4,r4,r0		# Decrement source address.
		mtxer	r0				# XER = remainder length.
		lswx	r5,r0,r4		# Get the remainder string.
		stswx	r5,r3,r4		# Store it.
		lwz		r3,24(1)		# restore target start address for return

	ExitNoStack		_moveeq


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CICell Interpret_0_1 (char *forthToInterpret)
; Interpret the specified forth code and return one value from the stack.  The forth code should
; have a signature of ( -- return-value ).
_interpret:
		StartStackFrame		Interpret_0_1
		EndStackFrame		Interpret_0_1

		Entry	Interpret_0_1
		WITH	rInterpret_0_1
			stw		r3, ciArgs-TOCbase+forth(rTOC)	; Save FORTH pointer
			la		r3, ciArgs-TOCbase(rTOC)		; Point at interpret args
			la		r0, sInterpret-TOCbase(rTOC)	; Get a pointer to our "interpret" string
			stw		r0, service(r3)
			li		r0, 1						; Setup nArgs
			stw		r0, nArgs(r3)
			li		r0, 2						; Setup nReturns
			stw		r0, nReturns(r3)
			lwz		r0, saveCI(rTOC)			; Get client interface callback function pointer
			mtctr	r0							; Point at client interface callback function
			bctrl								; Call O/F client interface to get load-base

			la		r0, sInterpretError-TOCbase(rTOC)	; Point at our error message
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not
			lwz		r3, ciArgs-TOCbase+catchResult(rTOC)	; Check CATCH-RESULT too
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not

			lwz		r3, ciArgs-TOCbase+return1(rTOC)	; Get returned instance handle
		ENDWITH		; rInterpret_0_1
		Exit	Interpret_0_1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; int CompareLoweredStrings (char *s1, char *s2)
; Returns ZERO (and EQL in CR0) if two NUL-terminated strings s1 and s2 are equal when
; compared without regard to USASCII case.

_compare:
		EntryNoStack	CompareLoweredStrings
			addi	r3, r3, -1					; Back up one for each pointer so we can use LBZU
			addi	r4, r4, -1

@loop:		lbzu	r5, 1(r3)					; Fetch S1 char
			cmplwi	r5, 0+'A'					; Is it alphabetic?
			blt		@s1OK
			cmplwi	r5, 'Z'
			bgt		@s1OK
			addi	r5, r5, 'a'-'A'				; Convert to lowercase

@s1OK:		lbzu	r6, 1(r4)					; Fetch S2 char
			cmplwi	r6, 'A'						; Is it alphabetic?
			blt		@s2OK
			cmplwi	r6, 'Z'
			bgt		@s2OK
			addi	r6, r6, 'a'-'A'				; Is uppercase alphabetic; convert to lowercase

@s2OK:		cmplw	r5, r6						; Are characters equal?
			bne		@exit						; Branch if not
		
			cmplwi	r5, 0						; Are they both NUL?
			bne		@loop						; Branch for more characters if not

			li		r3, 0						; Strings are EQUAL!
@exit:		cmplwi	r3, 0						; Set CR0 to reflect our return value
		ExitNoStack		CompareLoweredStrings


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; int FileIDCompare (void *r1)
; Compare catalog B*Tree record r1 looking at the parent ID
; part of the key.  This is used to follow the index links to find the
; left-most leaf node containing the blessedDirID(rTOC).  This is the first
; record in the sequence of records for the blessed folder directory.
; Returns ZERO (and EQL in CR0) if r1's parent ID field is
; equal to blessedDirID(rTOC) and the length of the following filename string is zero.
; otherwise returns negative if r1's parent ID is less than blessedDirID(rTOC), and
; positive if r1's parent ID is greater than blessedDirID(rTOC).

_fileIDCompare:
		EntryNoStack	FileIDCompare
			mr		r6, r3						; Save record pointer for later finer comparisons if needed
			lwz		r4, 2(r3)					; Get parent ID from key record
			lwz		r5, blessedDirID-TOCbase(rTOC)	; Get directory ID we're looking for
			sub.	r3, r4, r5					; Compare
			bne		@exit						; If not zero, just return negative or positive result already in R3
			
			lbz		r3, 6(r6)					; Get key's string length byte
			cmplwi	r3, 0						; Set CR0 to match
@exit:	ExitNoStack		FileIDCompare


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UInt32 FindLeafNode (void *extentsBufferP, int (*compareFunction) (void *key1, void *key2))
;	Searches through the specified extents list (which must represent the extents of an HFS
;	B*Tree file looking for the first leaf node in the tree for which compareFunction returns
;	a MATCH value.  The node number of the leaf node (or zero if none is found) is returned
;	in the lower 16 bits of R3 and # of records in that node (or zero if none is found) in the
;	upper 16 bits of R3.  The contents of the matching leaf node block is left in blockBuf(rBSS).
_findLeaf:
		StartStackFrame	FindLeafNode
		EndStackFrame	FindLeafNode

		Entry	FindLeafNode
		Exit	FindLeafNode


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; void GetFileExtents (UInt32 fileID, void *extentsBufferP)
; Retrieves into buffer to which extentsBufferP points a counted array of file
; Extents records for the file with file ID fileID, where the first longword of
; extentsBufferP is the count of subsequent entries.
;	Errors are treated as FATAL.
_getExtents:
		StartStackFrame	GetFileExtents
		EndStackFrame	GetFileExtents

		Entry	GetFileExtents
		Exit	GetFileExtents


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; int ReadHFSNode (UInt32 nodeNumber, void *extentsBufferP)
; Retrieves node number nodeNumber from the file whose disk extents are described by
; extentsBufferP.  Returns number of records in the resulting canonical HFS node.
;	Errors are treated as FATAL.
_readNode:
		StartStackFrame	ReadHFSNode
		EndStackFrame	ReadHFSNode

		Entry	ReadHFSNode
		Exit	ReadHFSNode


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; void ReadPartitionBlocks (int partitionNumber, void *buffer, int blockNumber, int nBlocks)
;	Reads specified number of disk 512-byte blocks from specified partition (1 origin).
;	If partitionNumber is ZERO we treat the blockNumber as a raw disk block number rather
;	than relative to any partition.
;	Errors are treated as FATAL.
_readPartBlks:
		StartStackFrame	ReadPartitionBlocks

; Define registers we use as nonvolatile temporaries herein
ReadPartitionBlocksTemporaries	RECORD	firstNonVolatileReg-1, DECR
rBuffer			ds.b	1
rBlockNumber	ds.b	1
rNBlocks		ds.b	1
firstRegToSave	equ		$-1
				ENDR

		EndStackFrame	ReadPartitionBlocks, ReadPartitionBlocksTemporaries.firstRegToSave

		Entry	ReadPartitionBlocks
		WITH	BSS, ReadPartitionBlocksTemporaries
			lwz		rBSS, saveBSS(rTOC)			; Set up our BSS register pointer
		
		; Copy our input parameters into nonvolatile registers (leave partitionNumber in R3)
			mr		rBuffer, r4
			mr		rBlockNumber, r5
			mr		rNBlocks, r6
			
			cmplwi	r3, 0						; Is partition number 0
			beq		@noPartitionOffset			; Branch if yes so we read RAW DISK blocks instead of partitioned blocks

			bl		.ReadRawBlock				; Get partition # block into blockBuf(rBSS)

			lwz		r3, Partition.pmPyPartStart+blockBuf(rBSS)	; Get block offset of partition
			add		rBlockNumber, rBlockNumber, r3		; Offset partition relative block # to absolute raw block #

@noPartitionOffset:
		WITH	rInterpret_3_0
			la		r3, ciArgs-TOCbase(rTOC)	; Point at interpret args
			la		r0, sInterpret-TOCbase(rTOC)	; Get a pointer to our "interpret" string
			stw		r0, service(r3)
			li		r0, 4						; Setup nArgs
			stw		r0, nArgs(r3)
			li		r0, 1						; Setup nReturns
			stw		r0, nReturns(r3)
			stw		rBuffer, arg1(r3)			; Set buffer base to read into
			stw		rBlockNumber, arg2(r3)		; Set raw block # to read
			stw		rNBlocks, arg3(r3)			; Set # blocks to read
			la		r0, sReadBlocks-TOCbase(rTOC)	; Point at FORTH to execute
			stw		r0, forth(r3)				; readBlocks ( buf block# nBlocks -- )
			lwz		r0, saveCI(rTOC)			; Get client interface callback function pointer
			mtctr	r0							; Point at client interface callback function
			bctrl								; Call O/F client interface to read partition block

			la		r0, sReadFailure-TOCbase(rTOC)		; Point at our error message
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not
			lwz		r3, ciArgs-TOCbase+catchResult(rTOC)		 ; Check CATCH-RESULT too
			cmpwi	r3, 0						; Success?
			bne		Bail						; Branch if not
		ENDWITH		; rInterpret_3_0
		ENDWITH		; BSS
		ENDWITH		; ReadPartitionBlocksTemporaries
		Exit	ReadPartitionBlocks


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; void ReadRawBlock (int blockNumber)
;	Reads specified (blockNumber) raw disk block into blockBuf(rBSS).  Errors are treated as FATAL.
_readRawBlk:
		StartStackFrame	ReadRawBlock
		EndStackFrame	ReadRawBlock, firstNonVolatileReg

		Entry	ReadRawBlock
		WITH	BSS
			lwz		rBSS, saveBSS(rTOC)			; Set up our BSS register pointer
			mr		r5, r3						; Get partition # as block # to read
			li		r3, 0						; Indicate we want not a partition relative read but whole disk
			la		r4, blockBuf(rBSS)			; Point at our block buffer
			li		r6, 1						; Get count of blocks to read
			bl		.ReadPartitionBlocks		; Recurse to get raw disk block into blockBuf(rBSS)
		ENDWITH		; BSS
		Exit		ReadRawBlock


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
DropWithExhaustion:
			la		r0, sNoBootablePartitions(rTOC)	; Get error message string
			b		Bail

		WITH	rInterpret_1_0
; FATAL ERROR EXIT PATH, called with error message Cstring pointer in R0.  Assumes rTOC is set up.  Never returns.
Bail:		la		r3, ciArgs-TOCbase(rTOC)	; Point at interpret args
			stw		r0, arg1(r3)				; Save pointer to our message string
			la		r0, sInterpret-TOCbase(rTOC)	; Get a pointer to our "interpret" string
			stw		r0, service(r3)
			li		r0, 2						; Setup nArgs
			stw		r0, nArgs(r3)
			li		r0, 1						; Setup nReturns
			stw		r0, nReturns(r3)
			la		r0, sDisplayError-TOCbase(rTOC)		; Point at FORTH to execute
			stw		r0, forth(r3)
			lwz		r0, saveCI(rTOC)
			mtctr	r0							; Point at client interface callback function
			lwz		r0, returnLR-TOCbase(rTOC)	; Retrieve LR to return to Open Firmware
			mtlr	r0
			bctr								; Call O/F back to display message and return
		ENDWITH		; rInterpret_1_0
			

	if 0
		; Pointer Glue
		lwz      r0,0(r12)
		stw      RTOC,20(SP)
		mtctr    r0
		lwz      RTOC,4(r12)
		bctr
	endif

			end
