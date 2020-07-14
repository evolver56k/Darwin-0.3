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
 * KeyMap.m - Generic keymap string parser and keycode translator.
 *
 * HISTORY
 * 19 June 1992    Mike Paquette at NeXT
 *      Created. 
 * 5  Aug 1993	  Erik Kay at NeXT
 *	minor API cleanup
 * 11 Nov 1993	  Erik Kay at NeXT
 *	fix to allow prevent long sequences from overflowing the event queue
 */

#import <machkit/NXLock.h>
#import <bsd/dev/event.h>
#import <driverkit/KeyMap.h>

@implementation KeyMap: Object


/*
 * Common KeyMap initialization
 */
- initFromKeyMapping	:(const unsigned char *)mapping
		length	:(int)len
		canFree	:(BOOL)canFree
{
	[super init];
	
	if ( keyMappingLock == nil )
	    keyMappingLock = [NXLock new];
	
	if ( [self setKeyMapping:mapping length:len canFree:canFree] == nil )
		return [self free];
	return self;
}

/*
 * Load a new keymapping.  Returns nil if the map is invalid.
 */
- setKeyMapping	:(const unsigned char *)mapping
		length	:(int)len
		canFree	:(BOOL)canFree
{
	NXParsedKeyMapping newMapping;

	if ([self _parseKeyMapping:mapping length:len into:&newMapping] == nil)
		return nil;

	[keyMappingLock lock];
	if (	curMapping.mapping != (unsigned char *)NULL
	    &&	canFreeMapping == YES )
	{
		IOFree( curMapping.mapping, curMapping.mappingLen );
	}
	bcopy( &newMapping, &curMapping, sizeof curMapping );
	canFreeMapping = canFree;
	[keyMappingLock unlock];
	return self;
}

- (const unsigned char *)keyMapping:(int *)len
{
	if ( len != (int *)0 )
		*len = curMapping.mappingLen;
	return (const unsigned char *)curMapping.mapping;
}

- (int)keyMappingLength
{
	return curMapping.mappingLen;
}

- free
{
	id lock;
	[keyMappingLock lock];
	lock = keyMappingLock;
	keyMappingLock = nil;
	if (	curMapping.mapping != (unsigned char *)NULL
	    &&	canFreeMapping == YES )
	{
		IOFree( curMapping.mapping, curMapping.mappingLen );
		curMapping.mapping = (unsigned char *)NULL;
	}
	[lock unlock];
	[lock free];
	return [super free];
}

- setDelegate:(id)newDelegate
{
	if ( [newDelegate conformsTo:@protocol(KeyMapDelegate)] )
		delegate = newDelegate;
	else
		IOLog( "KeyMap setDelegate: new delegate [%s] does not "
			"implement KeyMapDelegate protocol.\n",
			object_getClassName(newDelegate) );
	return self;
}

- delegate
{
	return delegate;
}

//
// Perform the mapping of 'key' moving in the specified direction
// into events.
//
// On entry, the _kbdListLock should be held.
//
- doKeyboardEvent:(unsigned)key
	direction:(BOOL)down
	keyBits	:(kbdBitVector)keyBits
{
    unsigned char thisBits;

    [keyMappingLock lock];
    if ( curMapping.mapping == (unsigned char *)NULL )
    {
	[keyMappingLock unlock];
	return self;		// No map set up yet
    }
    /* Do mod bit update and char generation in useful order */
    thisBits = curMapping.keyBits[key];
    if ( down == YES )
    {
	EVK_KEYDOWN( key, keyBits );
	if (thisBits & NX_MODMASK)
		[self _doModCalc:key keyBits:keyBits];
	if (thisBits & NX_CHARGENMASK)
		[self _doCharGen:key direction:down];
    }
    else
    {
	EVK_KEYUP( key, keyBits );
	if (thisBits & NX_CHARGENMASK)
		[self _doCharGen:key direction:down];
	if (thisBits & NX_MODMASK)
		[self _doModCalc:key keyBits:keyBits];
    }
    [keyMappingLock unlock];

    return self;
}


//
// Support goop for parseKeyMapping.  These routines are
// used to walk through the keymapping string.  The string
// may be composed of bytes or shorts.  If using shorts, it
// MUST always be aligned to use short boundries.
//
typedef struct _NewMappingData {
    unsigned const char *bp;
    unsigned const char *endPtr;
    int shorts;
} NewMappingData;

static unsigned int NextNum(NewMappingData *nmd)
{
    if (nmd->bp >= nmd->endPtr)
	return(0);
    if (nmd->shorts)
	return(*((unsigned short *)nmd->bp)++);
    else
	return(*((unsigned char *)nmd->bp)++);
}

//
// perform the actual parsing operation on a keymap.
// Returns self, or nil on failure.
//
- _parseKeyMapping:(const unsigned char *)mapping
	length:(int)mappingLen
	into:(NXParsedKeyMapping *)newMapping
{
	NewMappingData nmd;
	int i, j, k, l, n;
	unsigned int m;
	int keyMask, numMods;
	int maxSeqNum = -1;
 
	/* Initialize the new map. */
	bzero( newMapping, sizeof (NXParsedKeyMapping) );
	newMapping->maxMod = -1;
	newMapping->numDefs = -1;
	newMapping->numSeqs = -1;

	nmd.endPtr = mapping + mappingLen;
	nmd.bp = mapping;
	nmd.shorts = 1;		// First value, the size, is always a short

	/* Start filling it in with the new data */
	newMapping->mapping = (unsigned char *)mapping;
	newMapping->mappingLen = mappingLen;
	newMapping->shorts = nmd.shorts = NextNum(&nmd);

	/* Walk through the modifier definitions */
	numMods = NextNum(&nmd);
	for(i=0; i<numMods; i++)
	{
	    /* Get bit number */
	    if ((j = NextNum(&nmd)) >= NX_NUMMODIFIERS)
		return nil;

	    /* Check maxMod */
	    if (j > newMapping->maxMod)
		newMapping->maxMod = j;

	    /* record position of this def */
	    newMapping->modDefs[j] = (unsigned char *)nmd.bp;

	    /* Loop through each key assigned to this bit */
	    for(k=0,n = NextNum(&nmd);k<n;k++)
	    {
		/* Check that key code is valid */
		if ((l = NextNum(&nmd)) >= NX_NUMKEYCODES)
		    return nil;
		/* Make sure the key's not already assigned */
		if (newMapping->keyBits[l] & NX_MODMASK)
			return nil;
		/* Set bit for modifier and which one */
		newMapping->keyBits[l] |= NX_MODMASK | (j & NX_WHICHMODMASK);
	    }
	}

	/* Walk through each key definition */
	newMapping->numDefs = NextNum(&nmd);
	n = newMapping->numDefs;
	for( i=0; i < NX_NUMKEYCODES; i++)
	{
	    if (i < n)
	    {
		newMapping->keyDefs[i] = (unsigned char *)nmd.bp;
		if ((keyMask = NextNum(&nmd)) != (nmd.shorts ? 0xFFFF: 0x00FF))
		{
		    /* Set char gen bit for this guy: not a no-op */
		    newMapping->keyBits[i] |= NX_CHARGENMASK;
		    /* Check key defs to find max sequence number */
		    for(j=0, k=1; j<=newMapping->maxMod; j++, keyMask>>=1)
		    {
			    if (keyMask & 0x01)
				k*= 2;
		    }
		    for(j=0; j<k; j++)
		    {
			m = NextNum(&nmd);
			l = NextNum(&nmd);
			if (m == (nmd.shorts ? 0xFFFF: 0x00FF))
			    if (((int)l) > maxSeqNum)
				maxSeqNum = l;	/* Update expected # of seqs */
		    }
		}
		else /* unused code within active range */
		    newMapping->keyDefs[i] = NULL;
	    }
	    else /* Unused code past active range */
	    {
		newMapping->keyDefs[i] = NULL;
	    }
	}
	/* Walk through sequence defs */
	newMapping->numSeqs = NextNum(&nmd);
	/* If the map calls more sequences than are declared, bail out */
	if (newMapping->numSeqs <= maxSeqNum)
	    return nil;

	/* Walk past all sequences */
	for(i = 0; i < newMapping->numSeqs; i++)
	{
	    newMapping->seqDefs[i] = (unsigned char *)nmd.bp;
	    /* Walk thru entries in a seq. */
	    for(j=0, l=NextNum(&nmd); j<l; j++)
	    {
		NextNum(&nmd);
		NextNum(&nmd);
	    }
	}
	/* Install Special device keys.  These override default values. */
	numMods = NextNum(&nmd);	/* Zero on old style keymaps */
	if ( numMods > NX_NUMSPECIALKEYS )
	    return nil;
	if ( numMods )
	{
	    for ( i = 0; i < NX_NUMSPECIALKEYS; ++i )
		newMapping->specialKeys[i] = NX_NOSPECIALKEY;
	    for ( i = 0; i < numMods; ++i )
	    {
		j = NextNum(&nmd);	/* Which modifier key? */
		l = NextNum(&nmd);	/* Scancode for modifier key */
		if ( j >= NX_NUMSPECIALKEYS )
		    return nil;
		newMapping->specialKeys[j] = l;
	    }
	}
	else  /* No special keys defs implies an old style keymap */
	{
		return nil;	/* Old style keymaps are guaranteed to do */
				/* the wrong thing on ADB keyboards */
	}
	/* Install bits for Special device keys */
	for(i=0; i<NX_NUM_SCANNED_SPECIALKEYS; i++)
	{
	    if ( newMapping->specialKeys[i] != NX_NOSPECIALKEY )
	    {
		newMapping->keyBits[newMapping->specialKeys[i]] |=
		    (NX_CHARGENMASK | NX_SPECIALKEYMASK);
	    }
	}
    
	return self;
}


#undef NEXTNUM
#define NEXTNUM() (shorts ?	*((short *)mapping)++ : \
				*((unsigned char *)mapping)++ )
//
// Look up in the keymapping each key associated with the modifier bit.
// Look in the device state to see if that key is down.
// Return 1 if a key for modifier 'bit' is down.  Return 0 if none is down
//
static inline int IsModifierDown(NXParsedKeyMapping *curMapping,
			 	kbdBitVector keyBits,
				int bit )
{
    int i, n;
    char *mapping;
    unsigned key;
    short shorts = curMapping->shorts;

    if ( (mapping = curMapping->modDefs[bit]) != 0 ) {
	for(i=0, n=NEXTNUM(); i<n; i++)
	{
	    key = NEXTNUM();
	    if ( EVK_IS_KEYDOWN(key, keyBits) )
		return 1;
	}
    }
    return 0;
}

- (void)_calcModBit	:(int)bit
		keyBits	:(kbdBitVector)keyBits
{
	int		bitMask,i,n;
	unsigned	myFlags;

	bitMask = 1<<(bit+16);

	/* Initially clear bit, as if key-up */
	myFlags = [delegate deviceFlags] & (~bitMask);
	/* Set bit if any associated keys are down */
	if ( IsModifierDown( &curMapping, keyBits, bit ) )
		myFlags |= bitMask;

	/* If this was the shift bit... */
	if (bit == NX_MODIFIERKEY_SHIFT)
	{
	    /*
	     * And CMD is already down, and we don't have a Caps Lock key,
	     * and we don't have a NX_KEYTYPE_CAPS_LOCK special key...
	     * We check both the CAPS LOCK modifier and the CAPS LOCK
	     * special key since only one or the other might be in use.
	     * We use the CAPS LOCK special key by itself for keyboards on
	     * which the CAPS LOCK key does not lock.
	     */
	    if (   (myFlags & NX_COMMANDMASK) != 0
		&& !curMapping.modDefs[NX_MODIFIERKEY_ALPHALOCK]
		&& (curMapping.specialKeys[NX_KEYTYPE_CAPS_LOCK] == 
		    NX_NOSPECIALKEY) )
	    {
		/*
		 * On the SHIFT key down, clear the active char key if any,
		 * and on SHIFT key up, if no char key has been pressed,
		 * toggle the state of the Alpha Lock. This lets us do upper-
		 * case command keys without toggling Alpha Lock.
		 */
		if ( myFlags & bitMask ) // SHIFT key down event
		    [delegate setCharKeyActive:NO];
		else if ( [delegate charKeyActive] == NO )
		    [delegate setAlphaLock:([delegate alphaLock] == NO)];
	    }
	    /* Set NX_ALPHASHIFTMASK based on alphaLock OR shift active */
	    myFlags = (myFlags & ~NX_ALPHASHIFTMASK)
			| (([delegate alphaLock]==YES)<<16)
			| ((myFlags & NX_SHIFTMASK)>>1);
	}
	else if ( bit == NX_MODIFIERKEY_ALPHALOCK ) /* Caps Lock key */
	    [delegate setAlphaLock:(myFlags & NX_ALPHASHIFTMASK) ? YES : NO];

	[delegate setDeviceFlags:myFlags];
}


//
// Perform flag state update and generate flags changed events for this key.
//
- (void)_doModCalc:(int)key keyBits:(kbdBitVector)keyBits
{
    int thisBits;
    thisBits = curMapping.keyBits[key];
    if (thisBits & NX_MODMASK)
    {
	[self _calcModBit:(thisBits & NX_WHICHMODMASK)
		keyBits:keyBits];
	/* The driver generates flags-changed events only when there is
	   no key-down or key-up event generated */
	if (!(thisBits & NX_CHARGENMASK))
	{
		/* Post the flags-changed event */
		[delegate keyboardEvent:NX_FLAGSCHANGED
			flags:[delegate eventFlags]
			keyCode:key
			charCode:0
			charSet:0
			originalCharCode:0
			originalCharSet:0];
	}
	else	/* Update, but don't generate an event */
		[delegate updateEventFlags:[delegate eventFlags]];
    }
}


#undef NEXTNUM
#define NEXTNUM() (shorts ?	*((short *)mapping)++ : \
				*((unsigned char *)mapping)++ )

//
// Perform character event generation for this key
//
- (void)_doCharGen:(int)keyCode
		direction:(BOOL)down
{
    int	i, j, n, eventType, adjust, thisMask, shorts, modifiers, origflags;
    unsigned charSet, origCharSet;
    unsigned charCode, origCharCode;
    unsigned char *mapping;
    unsigned eventFlags;

    [delegate setCharKeyActive:YES];	// A character generating key is active

    eventType = (down == YES) ? NX_KEYDOWN : NX_KEYUP;
    eventFlags = [delegate eventFlags];
    modifiers = eventFlags >> 16;	// machine independent mod bits

    /* Get this key's key mapping */
    shorts = curMapping.shorts;
    mapping = curMapping.keyDefs[keyCode];

    if ( mapping )
    {
	/* Build offset for this key */
	thisMask = NEXTNUM();
	if (thisMask && modifiers)
	{
	    adjust = (shorts ? sizeof(short) : sizeof(char))*2;
	    for( i = 0; i <= curMapping.maxMod; ++i)
	    {
		if (thisMask & 0x01)
		{
		    if (modifiers & 0x01)
			mapping += adjust;
		    adjust *= 2;
		}
		thisMask >>= 1;
		modifiers >>= 1;
	    }
	}
	charSet = NEXTNUM();
	charCode = NEXTNUM();
	
	/* construct "unmodified" character */
	mapping = curMapping.keyDefs[keyCode];
	modifiers = (eventFlags & (NX_ALPHASHIFTMASK | NX_SHIFTMASK)) >> 16;

	thisMask = NEXTNUM();
	if (thisMask && modifiers)
	{
	    adjust = (shorts ? sizeof(short) : sizeof(char)) * 2;
	    for ( i = 0; i <= curMapping.maxMod; ++i)
	    {
		if (thisMask & 0x01)
		{
		    if (modifiers & 0x01)
			mapping += adjust;
		    adjust *= 2;
		}
		thisMask >>= 1;
		modifiers >>= 1;
	    }
	}
	origCharSet = NEXTNUM();
	origCharCode = NEXTNUM();
	
	if (charSet == (shorts ? 0xFFFF : 0x00FF))
	{
	    // Process as a character sequence
	    // charCode holds the sequence number
	    mapping = curMapping.seqDefs[charCode];
	    
	    origflags = eventFlags;
	    for(i=0,n=NEXTNUM();i<n;i++)
	    {
		// every 10 characters, give the windowserver a chance to
		// actually read the events out of the event queue
		if ((i % 10) == 9)
		    (void) thread_block();
		if ( (charSet = NEXTNUM()) == 0xFF ) /* metakey */
		{
		    if ( down == YES )	/* down or repeat */
		    {
			eventFlags |= (1 << (NEXTNUM() + 16));
			[delegate keyboardEvent:NX_FLAGSCHANGED
				flags:[delegate deviceFlags]
				keyCode:keyCode
				charCode:0
				charSet:0
				originalCharCode:0
				originalCharSet:0];
		    }
		    else
			NEXTNUM();	/* Skip over value */
		}
		else
		{
		    charCode = NEXTNUM();
		    [delegate	keyboardEvent:eventType
				flags:eventFlags
				keyCode:keyCode
				charCode:charCode
				charSet:charSet
				originalCharCode:charCode
				originalCharSet:charSet];
		}
	    }
	    /* Done with macro.  Restore the flags if needed. */
	    if ( eventFlags != origflags )
	    {
		[delegate keyboardEvent:NX_FLAGSCHANGED
			flags:[delegate deviceFlags]
			keyCode:keyCode
			charCode:0
			charSet:0
			originalCharCode:0
			originalCharSet:0];
		eventFlags = origflags;
	    }
	}
	else	/* A simple character generating key */
	{
	    [delegate	keyboardEvent:eventType
			flags:eventFlags
			keyCode:keyCode
			charCode:charCode
			charSet:charSet
			originalCharCode:origCharCode
			originalCharSet:origCharSet];
	}
    } /* if (mapping) */
    
    /*
     * Check for a device control key: note that they always have CHARGEN
     * bit set
     */
    if (curMapping.keyBits[keyCode] & NX_SPECIALKEYMASK)
    {
	for(i=0; i<NX_NUM_SCANNED_SPECIALKEYS; i++)
	{
	    if ( keyCode == curMapping.specialKeys[i] )
	    {
		[delegate keyboardSpecialEvent:eventType
				flags:eventFlags
				keyCode:keyCode
				specialty:i];
		/*
		 * Special keys hack for letting an arbitrary (non-locking)
		 * key act as a CAPS-LOCK key.  If a special CAPS LOCK key
		 * is designated, and there is no key designated for the 
		 * AlphaLock function, then we'll let the special key toggle
		 * the AlphaLock state.
		 */
		if (   i == NX_KEYTYPE_CAPS_LOCK
		    && !curMapping.modDefs[NX_MODIFIERKEY_ALPHALOCK]
		    && down == YES )
		{
		    unsigned myFlags = [delegate deviceFlags];
		    BOOL alphaLock = ([delegate alphaLock] == NO);
		    // Set delegate's alphaLock state
		    [delegate setAlphaLock:alphaLock];
		    // Update the delegate's flags
		    if ( alphaLock )
		    	myFlags |= NX_ALPHASHIFTMASK;
		    else
		        myFlags &= ~NX_ALPHASHIFTMASK;
		    [delegate setDeviceFlags:myFlags];
		    [delegate keyboardEvent:NX_FLAGSCHANGED
			    flags:myFlags
			    keyCode:keyCode
			    charCode:0
			    charSet:0
			    originalCharCode:0
			    originalCharSet:0];
		} 
		break;
	    }
	}
    }
}

@end

