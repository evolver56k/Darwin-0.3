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
/* 	Copyright (c) 1992-96 NeXT Software, Inc.  All rights reserved. 
 *
 * IOFrameBufferDisplay.h - Standard frame buffer display driver class.
 *
 *
 * HISTORY
 * 24 Oct 95	Rakesh Dubey
 *      Added mode change feature. 
 * 01 Sep 92	Joe Pasqua
 *      Created. 
 */

/* Notes:
 * This module defines an abstract superclass for "standard" (linear)
 * framebuffers.
 */

#ifndef __IOFRAMEBUFFER_H__
#define __IOFRAMEBUFFER_H__

#import <driverkit/IODisplay.h>
#import <driverkit/ppc/IOMacOSTypes.h>

typedef UInt32	IOFBSelect;
typedef SInt32	IOFBIndex;
typedef UInt32	IOFBBitsPerPixel;
typedef SInt32	IOFBDisplayModeID;
typedef UInt32  IOFBStandardTimingID;

struct ModeData {
    IOFBDisplayModeID	modeID;
    IOFBIndex		firstDepth;
    IOFBIndex		numDepths;
};

@interface IOFramebuffer:IODisplay
{
@private
    void *priv;
    /* Mapping tables used in cursor drawing to 5-5-5 displays. */
    unsigned char *_bm34To35SampleTable;
    unsigned char *_bm35To34SampleTable;

    /* Mapping tables used in cursor drawing to 8-bit RGB displays. */
    unsigned int *_bm256To38SampleTable;
    unsigned char *_bm38To256SampleTable;

    /* Used for dynamic mode changes */
    int			_pendingDisplayMode;
   
    IOFBDisplayModeID	currentDisplayModeID; 
    IOFBIndex		currentDepthIndex;
    BOOL		currentMono;

    IOFBDisplayModeID	pendingDisplayModeID; 
    IOFBIndex		pendingDepthIndex;
    BOOL		pendingMono;

    IOBitsPerPixel 	cursorShmemBitsPerPixel;
    IOBitsPerPixel 	cursorBitsPerPixel;

    UInt32		numRanges;
    IORange		userAccessRanges[ 10 ]; // first is framebuffer, then 
						// those requested by accelerator
    IOByteCount		framebufferOffset;	// from range to active pixels

    BOOL		opened;
    BOOL		clutValid;

    IOItemCount		numModes;
    IOItemCount		numConfigs;
    struct ModeData   *	modeTable;

    id			smartDisplay;

    /* Reserved for future expansion. */
    int 		_IOFramebuffer_reserved[8];
}


/* IODevice methods reimplemented by this class. */

- setTransferTable:(const unsigned int *)table count:(int)count;

/* 'IOScreenEvents' protocol methods reimplemented by this class. */

- hideCursor: (int)token;

- moveCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;

- showCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;

/* NOTE: Subclasses must override setBrightness and implement appropriately. */
- setBrightness:(int)level token:(int)t;

- (BOOL)setPendingDisplayMode:(int)configIndex;

- (BOOL)_commitToPendingMode;

#define IO_R_UNDEFINED_MODE		(-750)		// no such mode
#define IO_R_FAILED_TO_SET_MODE		(-751)		// mode change failed

// For NeXT logical palette to real palette conversion, an additional
//   256 bytes can be added to the STDFB_BM38_TO_BM256_MAP table
#define STDFB_BM38_TO_256_WITH_LOGICAL_SIZE			\
	(STDFB_BM38_TO_BM256_MAP_SIZE + (256/sizeof(int)))

- (UInt32) tempFlags;

////////////////////////////////////////////////////////////////////////


//// Modes


- (IOReturn)
    getDisplayModeByIndex:(IOFBIndex)modeIndex displayMode:(IOFBDisplayModeID *)displayModeID;

struct IOFBDisplayModeInformation {
    IOVersion			version;
    IOFBDisplayModeID		displayModeID;
    IOFBIndex			maxDepthIndex;
    UInt32			nominalWidth;
    UInt32			nominalHeight;
    IOFixed			refreshRate;
};
typedef struct IOFBDisplayModeInformation IOFBDisplayModeInformation;
enum { IOFBDisplayModeInfoVersion = 1 };

- (IOReturn)
    getDisplayModeInformation:(IOFBDisplayModeID)modeID info:(IOFBDisplayModeInformation *)info; 

enum {
    kIOFBFixedRGBCLUTPixelType	= 1,
    kIOFBRGBCLUTPixelType,
    kIOFBDirectRGBPixelType,
    kIOFBDirectBGRPixelType,
    kIOFBOneIsBlackPixelType,
    kIOFBOneIsWhitePixelType,
    kIOFBVGA_zz,		// VGA mode zz ?
};

#if 0
typedef IODisplayInfo IOFBPixelInformation;
#else
// maybe later
struct IOFBPixelInformation {
    IOVersion			version;
    IOOptionBits		flags;
    IOByteCount			rowBytes;
    UInt32			width;
    UInt32			height;
    IOFixed			refreshRate;
    IOFBIndex			pageCount;
    IOFBIndex			pixelType;
    IOItemCount			bitsPerPixel;
    UInt32			channelMasks[ 5 ];
};
typedef struct IOFBPixelInformation IOFBPixelInformation;
#endif
enum { IOFBPixelInfoVersion = 1 };

- (IOReturn)
    getPixelInformationForDisplayMode:(IOFBDisplayModeID)mode andDepthIndex:(IOFBIndex)depthIndex
    	pixelInfo:(IOFBPixelInformation *)info;

struct IOFBTimingInformation {
    IOVersion			version;
    IOFBStandardTimingID	standardTimingID;	// appleTimingXXX
    // rest from EDID defn
    UInt32			pixelClock;		// Hertz
    UInt32			horizontalActive;	// pixels
    UInt32			horizontalBlanking;	// pixels
    UInt32			horizontalBorder;	// pixels
    UInt32			horizontalSyncOffset;	// pixels
    UInt32			horizontalSyncWidth;	// pixels
    UInt32			verticalActive;		// lines
    UInt32			verticalBlanking;	// lines
    UInt32			verticalBorder;		// lines
    UInt32			verticalSyncOffset;	// lines
    UInt32			verticalSyncWidth;	// lines
    IOOptionBits		timingflags;			// b0-7 from EDID
};
typedef struct IOFBTimingInformation IOFBTimingInformation;
enum { kIOFBTimingInfoVersion = 1 };

enum {
    kDisplayModeValidFlag	= 1,
    kDisplayModeSafeFlag	= 2,
    kDisplayModeDefaultFlag	= 4
};

- (IOReturn)
    getDisplayModeTiming:(IOFBIndex)connectIndex mode:(IOFBDisplayModeID)modeID
		timingInfo:(IOFBTimingInformation *)info connectFlags:(UInt32 *)flags;

#if 0
 - (IOReturn)
    addDisplayModeTiming:(IOFBTimingInformation *)timingInfo;
#endif

- (IOReturn)
    setDisplayMode:(IOFBDisplayModeID)modeID depth:(IOFBIndex)depthIndex page:(IOFBIndex)pageIndex;

- (IOReturn)
    setStartupMode:(IOFBDisplayModeID)modeID depth:(IOFBIndex)depthIndex;
- (IOReturn)
    getStartupMode:(IOFBDisplayModeID *)modeID depth:(IOFBIndex *)depthIndex;

// IOFBConfiguration flags
enum {
    kFBLuminanceMapped		= 0x00000001
};
struct IOFBConfiguration {
    IOVersion			version;
    IOFBDisplayModeID		displayMode;
    IOFBIndex			depth;
    IOFBIndex			page;
    IOOptionBits		flags;
    IOPhysicalAddress		physicalFramebuffer;
    IOVirtualAddress		mappedFramebuffer;		// kernel virtual, may be nil if unmapped
};
typedef struct IOFBConfiguration IOFBConfiguration;
enum { kIOFBConfigVersion = 1 };

- (IOReturn)
    getConfiguration:(IOFBConfiguration *)config;
- (IOReturn)
    open;


// extra info to select modes, like
enum{
    kIOFBStandardMode		= 'std ',		// "normal" - minimal resources, most offscreen
    kIOFBPagedMode		= 'page',		// page flipping possible
};

- (IOReturn)
    getAttributeForMode:(IOFBDisplayModeID) modeID andDepth:(IOFBIndex)depthIndex
    				attribute:(IOFBSelect)attr value:(UInt32 *)value;

//// Apertures

// IOFBApertureInformation type
enum {
    kIOFBApertureTypeMask		= 0x000000ff,
    kIOFBUnknown			= 0x00000000,
    kIOFBControlRegisters,
    kIOFBGraphicsRegisters,
    kIOFBGraphicsMemory,
    kIOFBMemory,

    kIOFBLittleEndian			= 0x00010000,
    kIOFBInterceptorProtected		= 0x00020000,
};

struct IOFBApertureInformation {
    IOVersion			version;
    IOPhysicalAddress		physicalAddress;
    IOVirtualAddress		mappedAddress;			// kernel virtual, may be nil if unmapped
    IOByteCount			length;
    IOCacheMode			cacheMode;
    IOOptionBits		type;
};
typedef struct IOFBApertureInformation IOFBApertureInformation;

- (IOReturn)
    getApertureInformationByIndex:(IOFBIndex)apertureIndex
    	apertureInfo:(IOFBApertureInformation *)apertureInfo;

//// CLUTs

struct IOFBColorEntry {
    UInt16			value;
    UInt16			red;
    UInt16			green;
    UInt16			blue;
};
typedef struct IOFBColorEntry IOFBColorEntry;

// setClut options (masks)
enum {
    kSetCLUTByValue		= 0x00000001,		// else at firstIndex
    kSetCLUTAtVBL		= 0x00000002,		// else immediate
    kSetCLUTWithLuminance	= 0x00000004,		// else RGB
    kSetCLUTWithAllGray		= 0x00000008		// ignore table
};

- (IOReturn)
    setCLUT:(IOFBColorEntry *) colors index:(UInt32)index numEntries:(UInt32)numEntries 
    	brightness:(IOFixed)brightness options:(IOOptionBits)options;

//// Gammas

- (IOReturn)
    setGammaTable:(UInt32) channelCount dataCount:(UInt32)dataCount dataWidth:(UInt32)dataWidth
    	data:(void *)data;

//// HW Cursors

// All straight from Marconi

enum {
    kTransparentEncoding 	= 0,
    kInvertingEncoding
};

enum {
    kTransparentEncodingShift	= (kTransparentEncoding << 1),
    kTransparentEncodedPixel	= (0x01 << kTransparentEncodingShift),

    kInvertingEncodingShift	= (kInvertingEncoding << 1),
    kInvertingEncodedPixel	= (0x01 << kInvertingEncodingShift),
};

enum {
    kHardwareCursorDescriptorMajorVersion	= 0x0001,
    kHardwareCursorDescriptorMinorVersion	= 0x0000
};

typedef UInt32 * UInt32Ptr;

struct HardwareCursorDescriptorRec {
    UInt16		majorVersion;
    UInt16		minorVersion;
    UInt32		height;
    UInt32		width;
    UInt32		bitDepth;
    UInt32		maskBitDepth;
    UInt32		numColors;
    UInt32Ptr		colorEncodings;
    UInt32		flags;
    UInt32		supportedSpecialEncodings;
    UInt32		specialEncodings[16];
};
typedef struct HardwareCursorDescriptorRec HardwareCursorDescriptorRec;

enum {
    kHardwareCursorInfoMajorVersion		= 0x0001,
    kHardwareCursorInfoMinorVersion		= 0x0000
};

struct HardwareCursorInfoRec {
    UInt16		majorVersion;
    UInt16		minorVersion;
    UInt32		cursorHeight;
    UInt32		cursorWidth;
    IOFBColorEntry  *	colorMap;				// nil or big enough for hardware's max colors
    UInt8	    *	hardwareCursor;
    UInt32		reserved[6];
};
typedef struct HardwareCursorInfoRec HardwareCursorInfoRec;

// class code to convert ie. VSLPrepareCursor...
+ (IOReturn)
    convertCursorImage:(void *)cursorImage hwDescription:(HardwareCursorDescriptorRec *)hwDescrip
			cursorResult:(HardwareCursorInfoRec *)cursorResult;

- (IOReturn)
    setCursorImage:(void *)cursorImage;
- (IOReturn)
    setCursorState:(SInt32)x y:(SInt32)y visible:(Boolean)visible;

//// Interrupts

enum {
    kVBLInterruptServiceType		= 'vbl ',
    kHBLInterruptServiceType		= 'hbl ',
    kFrameInterruptServiceType		= 'fram',
    kConnectInterruptServiceType	= 'dci '
};

enum {
    kDisabledInterruptState		= 0,
    kEnabledInterruptState		= 1
};

typedef (IOFBInterruptProc)( UInt32 refcon );

// this is driven in the opposite direction to ndrv's ie. the base class registers a proc with
// the driver, and controls int generation with setInterruptState. Clients ask for serviceType.

- (IOReturn)
    registerForInterruptType:(IOFBSelect)interruptType proc:(IOFBInterruptProc)proc refcon:(UInt32)refcon;
- (IOReturn)
    setInterruptState:(IOFBSelect)interruptType state:(UInt32)state;

//// State save

#if 0
- (IOReturn)
    saveState:(void *)storage storageSize:(IOByteCount *)size;		// storage can be nil to find size
- (IOReturn)
    restoreState:(void *)storage storageSize:(IOByteCount)size;
#endif

//// Controller attributes

enum {
    kIOFBPowerAttribute		= 'powr'
};

- (IOReturn)
    setAttribute:(IOFBSelect)attribute value:(UInt32)value;
- (IOReturn)
    getAttribute:(IOFBSelect)attribute value:(UInt32 *)value;


//// Private calls

- (IOReturn)
    privateCall:(IOFBSelect)callClass select:(IOFBSelect)select
    		paramSize:(UInt32)paramSize params:(void *)params
		resultSize:(UInt32 *)resultSize results:(void *)results;

//// Connections

// senseTypes
enum {
    kIOFBDDCSense		= 0x00000001,
    kIOFBAppleSense		= 0x00000002,
    kIOFBSimpleSense		= 0x00000004
};

struct IOFBConnectionInfo {
    IOVersion		version;
    IOOptionBits	senseType;
    IOOptionBits	flags;
};
typedef struct IOFBConnectionInfo IOFBConnectionInfo;

- (IOReturn)
    getConnections:(IOFBIndex)connectIndex 
		connectInfo:(IOFBConnectionInfo *)info;

enum {
    kEnableConnectionAttribute		= 'enab',
    kSyncControlConnectionAttribute	= 'sync'
};

enum {
    kIOFBHSyncDisable		= 0x00000001,
    kIOFBVSyncDisable		= 0x00000002,
    kIOFBCSyncDisable		= 0x00000004,
    kIOFBTriStateSyncs		= 0x00000080,
    kIOFBSyncOnBlue		= 0x00000008,
    kIOFBSyncOnGreen		= 0x00000010,
    kIOFBSyncOnRed		= 0x00000020
};

- (IOReturn)
    setConnectionAttribute:(IOFBIndex)connectIndex attribute:(IOFBSelect)attribute value:(UInt32)value;
- (IOReturn)
    getConnectionAttribute:(IOFBIndex)connectIndex attribute:(IOFBSelect)attribute value:(UInt32 *)value;

//// Private
- (IOReturn) makeConfigList;
- (IOReturn) setupForCurrentConfig;

@end	/* IOFramebuffer */

@protocol IOFBAppleSense

struct IOFBAppleSenseInfo {
// ??	    UInt32		standardDisplayType;
    UInt8		primarySense;
    UInt8		extendedSense;
};
typedef struct IOFBAppleSenseInfo IOFBAppleSenseInfo;

- (IOReturn)
    getAppleSense:(IOFBIndex)connectIndex info:(IOFBAppleSenseInfo *)info;

@end	/* IOFBAppleSense */

@protocol IOFBSimpleSense

enum {
    kIOFBNoConnectSense		= 0,	// definitely none
    kIOFBPossibleSense		= 1,	// no sensing hw
    kIOFBUnknownSense		= 2,	// present but unknown type
    kIOFBMonochromeSense	= 3,
    kIOFBColorSense		= 4,
    kIOFBSTNPanelSense		= 6,
    kIOFBTFTPanelSense		= 7
};

- (IOReturn)
    getSimpleSense:(IOFBIndex)connectIndex senseType:(UInt32 *)senseType;

@end	/* IOFBSimpleSense */

@protocol IOFBLowLevelDDCSense

enum {
    kIOFBDDCLow			= 0,
    kIOFBDDCHigh		= 1,
    kIOFBDDCTristate		= 2
};

- (void)
    setDDCClock:(IOFBIndex)connectIndex value:(UInt32)value;
- (void)
    setDDCData:(IOFBIndex)connectIndex value:(UInt32)value;
- (UInt32)
    readDDCClock:(IOFBIndex)connectIndex;
- (UInt32)
    readDDCData:(IOFBIndex)connectIndex;
- (IOReturn)
    makeDDCRaster;

@end	/* IOFBLowLevelDDCSense */

@protocol IOFBHighLevelDDCSense

enum {
    kIOFBDDCBlockSize               = 128
};

/* ddcBlockType constants*/
enum {
    kIOFBDDCBlockTypeEDID           = 0                             /* EDID block type. */
};

/* ddcFlags constants*/
enum {
    kIOFBDDCForceReadBit            = 0,                            	/* Force a new read of the EDID. */
    kIOFBDDCForceReadMask           = (1 << kIOFBDDCForceReadBit)       /* Mask for kddcForceReadBit. */
};


- (BOOL)
    hasDDCConnect:(IOFBIndex)connectIndex;
- (IOReturn)
    getDDCBlock:(IOFBIndex)connectIndex blockNumber:(UInt32)num blockType:(OSType)type
	options:(IOOptionBits)options data:(UInt8 *)data length:(IOByteCount *)length;

@end	/* IOFBHighLevelDDCSense */

@protocol IOFBGammaTableList

// necessary for ndrv specific gammas, accessed by display
- (IOReturn)
    getGammaTableByIndex:
	(UInt32 *)channelCount dataCount:(UInt32 *)dataCount
    	dataWidth:(UInt32 *)dataWidth data:(void **)data;		

@end

#endif	/* __IOFRAMEBUFFER_H__ */
