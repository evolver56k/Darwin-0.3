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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 *	ev_keymap.h
 *	Defines the structure used for parsing keymappings.  These structures
 *	and definitions are used by event sources in the kernel and by
 *	applications and utilities which manipulate keymaps.
 *	
 * HISTORY
 * 02-Jun-1992    Mike Paquette at NeXT
 *      Created. 
 */
#import <bsd/dev/ev_types.h>	/* Kernel keymap, keyboard info */

#define	NX_NUMKEYCODES	128	/* Highest key code is 0x7f */
#define NX_NUMSEQUENCES	128	/* Maximum possible number of sequences */
#define	NX_NUMMODIFIERS	16	/* Maximum number of modifier bits */
#define	NX_BYTE_CODES	0	/* If first short 0, all are bytes (else shorts) */

#define	NX_WHICHMODMASK	0x0f 	/* bits out of keyBits for bucky bits */
#define	NX_MODMASK	0x10	/* Bit out of keyBits indicates modifier bit */
#define	NX_CHARGENMASK	0x20	/* bit out of keyBits for char gen */
#define	NX_SPECIALKEYMASK 0x40	/* bit out of keyBits for specialty key */
#define	NX_KEYSTATEMASK	0x80	/* OBSOLETE - DO NOT USE IN NEW DESIGNS */

/*
 * Special keys currently known to and understood by the system.
 * If new specialty keys are invented, extend this list as appropriate.
 * The presence of these keys in a particular implementation is not
 * guaranteed.
 */
#define NX_NOSPECIALKEY			0xFFFF
#define NX_KEYTYPE_SOUND_UP		0	/* Processed by kernel */
#define NX_KEYTYPE_SOUND_DOWN		1	/* Processed by kernel */
#define NX_KEYTYPE_BRIGHTNESS_UP	2	/* Processed by kernel */
#define NX_KEYTYPE_BRIGHTNESS_DOWN	3	/* Processed by kernel */
#define NX_KEYTYPE_CAPS_LOCK		4	/* Processed by kernel */
#define NX_KEYTYPE_HELP			5
#define NX_POWER_KEY			6
#define NX_UP_ARROW_KEY			7
#define NX_DOWN_ARROW_KEY		8
#define	NX_NUMSPECIALKEYS		9 /* Maximum number of special keys */
// on the 68k, the power key is handled specially... we don't want it scanned
#ifdef m68k
#define NX_NUM_SCANNED_SPECIALKEYS	5 /* First 5 special keys are */
					  /* actively scanned in kernel */
#else
#define NX_NUM_SCANNED_SPECIALKEYS	7 /* First 7 special keys are */
					  /* actively scanned in kernel */
#endif

/* Modifier key indices into modDefs[] */
#define NX_MODIFIERKEY_ALPHALOCK	0
#define NX_MODIFIERKEY_SHIFT		1
#define NX_MODIFIERKEY_CONTROL		2
#define NX_MODIFIERKEY_ALTERNATE	3
#define NX_MODIFIERKEY_COMMAND		4
#define NX_MODIFIERKEY_NUMERICPAD	5
#define NX_MODIFIERKEY_HELP		6

typedef struct _NXParsedKeyMapping_ {
 	/* If nonzero, all numbers are shorts; if zero, all numbers are bytes*/
	short	shorts;
	
	/*
	 *  For each keycode, low order bit says if the key
	 *  generates characters.
	 *  High order bit says if the key is assigned to a modifier bit.
	 *  The second to low order bit gives the current state of the key.
	 */
	char	keyBits[NX_NUMKEYCODES];
	
	/* Bit number of highest numbered modifier bit */
	int			maxMod;
	
	/* Pointers to where the list of keys for each modifiers bit begins,
	 * or NULL.
	 */
	unsigned char *modDefs[NX_NUMMODIFIERS];
	
	/* Key code of highest key deinfed to generate characters */
	int			numDefs;
	
	/* Pointer into the keyMapping where this key's definitions begin */
	unsigned char *keyDefs[NX_NUMKEYCODES];
	
	/* number of sequence definitions */
	int			numSeqs;
	
	/* pointers to sequences */
	unsigned char *seqDefs[NX_NUMSEQUENCES];
	
	/* Special key definitions */
	int			numSpecialKeys;
	
	/* Special key values, or 0xFFFF if none */
	unsigned short specialKeys[NX_NUMSPECIALKEYS];
	
	/* Pointer to the original keymapping string */	
	unsigned char *mapping;
	
	/* Length of the original string */
	int	mappingLen;	
} NXParsedKeyMapping;


