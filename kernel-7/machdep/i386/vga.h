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

/*****************************************************************************

    vga.h
    Header file for VGA Device Driver

******************************************************************************/

#import PACKAGE_SPECS
#import ENVIRONMENT
#import MOUSEKEYBOARD

// VGA mode selection is done here for now.

#define VGA_MODE MODE_VGA_AT_12
#define VGA_WIDTH	640	/* Pixels wide (visible pixels) */
#define VGA_HEIGHT	480	/* Pixels high (visible scanlines)*/
#define VGAFLUSH(bounds) vga_at_mode12_bpp2_to_bpp4(bounds)

#if 0
#define VGA_MODE MODE_VGA_TS_29
#define VGA_WIDTH	800	/* Pixels wide (visible pixels) */
#define VGA_HEIGHT	600	/* Pixels high (visible scanlines)*/
#define VGAFLUSH(bounds) vga_ts_mode29_bpp2_to_bpp4(bounds)
#endif

#if 0
#define VGA_MODE MODE_VGA_TS_37n
#define VGA_WIDTH	1024	/* Pixels wide (visible pixels) */
#define VGA_HEIGHT	768	/* Pixels high (visible scanlines)*/
#define VGAFLUSH(bounds) vga_ts_mode37n_bpp2_to_bpp4(bounds)
#endif

#define VGA_ROMID	0	/* Unique ROM id for product (dummy value) */
#define VGA_ROWBYTES	(VGA_WIDTH / 4)	/* Row bytes to start of next line (not used) */

#define VGA_SAMPLES_PER_CHAN	0	/* LUT entries per channel */

/* Private state information cached in the NXSDevice priv field. */

typedef struct _VGA_PrivInfo_ {
    unsigned int *vgaVirtAddr;
    uint bytes_per_scanline;
    int monitorType;
} VGA_PrivInfo;
#define SHMEMPTR(device)  ((VGAShmem_t *)device->shmemPtr)
#define VGASETSEMA(device)	(ev_lock(&(SHMEMPTR(device)->cursorSema)))
#define VGACLEARSEMA(device)	(ev_unlock(&(SHMEMPTR(device)->cursorSema)))
#define VGAPRIVDRIVER(driver)  ((VGA_PrivInfo *)(driver->priv))
#define VGAPRIVDEVICE(device)  ((VGA_PrivInfo *)(device->driver->priv))

/* structure for registering graphics package drivers */

typedef struct package_init_t {
    struct package_init_t *next;
    int package_id;
    void *(*func)(NXDevice *dev, int package_id, void *data);
} package_init_t;


/*****************************************************************************
	Forward Declarations (ANSI C Prototypes)
******************************************************************************/

/**** vga ****/

extern int  VGAStart(NXDriver *);
extern void VGAInitScreen(NXDevice *);
extern void VGARegisterScreen(NXDevice *device);

/**** vgacursor ****/

extern void VGASetCursor(NXDevice *d, LocalBitmap *lbm, Point hotSpot,
	    int frame, int *waitCursorFlag);
extern void VGADisplayCursor2(NXDevice *);
extern void VGARemoveCursor2(NXDevice *);
extern void VGAHideCursor(NXDevice *device);
extern void VGAShowCursor(NXDevice *device);
extern void VGARevealCursor(NXDevice *device);
extern void VGAObscureCursor(NXDevice *device);
extern void VGAShieldCursor(NXDevice *device, Bounds *shieldRect);
extern void VGAUnshieldCursor(NXDevice *device);

/**** generic ****/

extern void VGAGenDrvComposite(CompositeOperation *cop, Bounds *dstBounds);
extern void VGAGenDrvFreeWindow(NXBag *bag, int termflag);
extern void VGAGenDrvMark(NXBag *bag, int channel, MarkRec *mrec, Bounds *markBds,
		       Bounds *bpBds);
extern void VGAGenDrvMoveWindow(NXBag *bag, short dx, short dy, Bounds *old,
			     Bounds *new);
extern void VGAGenDrvNewAlpha(NXBag *bag);
extern void VGAGenDrvNewWindow(NXBag *bag, Bounds *bounds, int windowType,
			    int depth, int local);
extern void VGAGenDrvPromoteWindow(NXBag *bag, Bounds *bounds, int newDepth,
				int windowType, DevPoint phase);
extern int VGAGenDrvWindowSize(NXBag *bag);



/*****************************************************************************
	Globals
******************************************************************************/
