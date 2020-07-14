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
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * EIDEInspector.m
 *
 * Driver inspector.
 */

#import "EIDEInspector.h"
#import "localization.h"
#import "IdeShared.h"

#define MYNAME		"EIDEInspector"
#define NIB_TYPE	"nib"

#define SET_LOCAL_TITLES	1

@implementation EIDEInspector

/*
 * Find and load our nib, put a localized title atop the connector
 * box, and init buttons.
 */
- init
{
	char 	buffer[MAXPATHLEN];

	myBundle = [NXBundle bundleForClass:[self class]];

	[super init];

	if (![myBundle getPath:buffer forResource:MYNAME ofType:NIB_TYPE]) {
		[self free];
		return nil;
	}
	if (![NXApp loadNibFile:buffer owner:self withNames:NO]) {
		[self free];
		return nil;
	}

	/*
	 * Connect the target/action of the override PopUps to methods
	 * defined in this class.
	 */
	[[popUpMasterDual target] setTarget:(id)self];
	[[popUpSlaveDual target] setTarget:(id)self];
	[[popUpMasterSingle target] setTarget:(id)self];
	[[popUpSlaveSingle target] setTarget:(id)self];
	[[popUpMasterSec target] setTarget:(id)self];
	[[popUpSlaveSec target] setTarget:(id)self];	

	[[popUpMasterDual target] setAction:@selector(selectMasterOverride:)];
	[[popUpSlaveDual target] setAction:@selector(selectSlaveOverride:)];
	[[popUpMasterSingle target] setAction:@selector(selectMasterOverride:)];
	[[popUpSlaveSingle target] setAction:@selector(selectSlaveOverride:)];	
	[[popUpMasterSec target] setAction:@selector(selectMasterOverrideSec:)];
	[[popUpSlaveSec target] setAction:@selector(selectSlaveOverrideSec:)];
	
	return self;
}

/*
 * Get current values of the buttons from the existing
 * config table. If the current table has no entry for specified
 * key, the associated button will be disabled.
 */
- (void)_initButton : button   key : (const char *)key
{
	const char *value;
	int ival;

	value = [table valueForStringKey:key];
	if (value == NULL) {
			[button setState:0];
			[button setEnabled:0];
			return;
	}
	else if (strcmp(value, "Yes") == 0) {
			ival = 1;
	}
	else {
			ival = 0;
	}
	[button setState:ival];
}

/*
 * Set the state of the DMA enable buttons from the mask value in the
 * config table. If the key is NULL, then the button shall be disabled.
 */
#define DMA_FIELDS_MASK		0xffffff00

- (void)_initDMAButton:button key:(const char *)key
{
	const char *value;
	unsigned int mask;

	if ((key == NULL) || ((value = [table valueForStringKey:key]) == NULL)) {
		[button setState:0];
		[button setEnabled:NO];
		return;
	}
	
	[button setEnabled:YES];
	mask = strtoul(value, NULL, 16);
	if ((mask & DMA_FIELDS_MASK) == 0)
		[button setState:NO];
	else
		[button setState:YES];
}

- (const char *)_getKeyForDMAButton:button
{
	char *key;

	if (button == enableDMAMaster)
		key = MODES_MASK_MASTER;
	else if (button == enableDMASlave)
		key = MODES_MASK_SLAVE;
	else if (button == enableDMAMasterPrimary)
		key = MODES_MASK_MASTER;
	else if (button == enableDMASlavePrimary)
		key = MODES_MASK_SLAVE;
	else if (button == enableDMAMasterSecondary)
		key = MODES_MASK_MASTER_SEC;
	else if (button == enableDMASlaveSecondary)
		key = MODES_MASK_SLAVE_SEC;
	else
		key = NULL;

	return (key);
}

- _initPopUp:popUp key:(const char *)key choices:(unsigned int)choices
{
	int i;
	int overrideIndex;
	const char *overrideString = [table valueForStringKey:key];
	id	popUpList = [popUp target];

	// Remove all cells before populating the PopUpList
	for (i = [popUpList count] - 1; i >= 0; i--) {
		[popUpList removeItemAt:i];
	}

	// Populate the PopUp
	for (i = 0; i < choices; i++) {	
		[popUpList addItem:OVERRIDE_STRING(overrideTable[i], myBundle)];
		[[[popUpList itemList] cellAt:i :0] setTag:i];
	}

	// Read the current configuration from the table
	overrideIndex = OVERRIDE_AUTO;
	if (overrideString) {
		for (i = 0; i < choices; i++) {
			if (strcmp(overrideTable[i], overrideString) == 0) {
				overrideIndex = i;
				break;
			}
		}
		if (overrideIndex == OVERRIDE_TABLE_SIZE)
			overrideIndex = OVERRIDE_AUTO;
	}
	
	// Make a selection
	[[popUpList itemList] selectCellWithTag:overrideIndex];
	[popUp setTitle:[popUpList selectedItem]];

	return self;
}

- setTable:(NXStringTable *)instance
{
	char *str;

	[super setTable:instance];
	[self setAccessoryView:boundingBox];
	
	/*
	 * Initialize the isBusMaster and isDualChannel variables.
	 */
	isDMACapable = NO;
	isDualChannel = NO;
	
	// Cheesy, but assume the driver supports DMA modes if the
	// controller is PCI based.
	str = (char *)[table valueForStringKey:BUS_TYPE];
	if ((str != NULL) && (strcmp(str, "PCI") == 0)) {
		isDMACapable = YES;
	}

	// Hack to detect Dual EIDE case.
	str = (char *)[table valueForStringKey:"Class Names"];
	if ((str != NULL) && (strstr(str, "Dual")))
		isDualChannel = YES;

	if (!isDMACapable) {
		[self _initDMAButton:enableDMAMaster key:NULL];
		[self _initDMAButton:enableDMASlave key:NULL];
		[self _initDMAButton:enableDMAMasterPrimary key:NULL];
		[self _initDMAButton:enableDMASlavePrimary key:NULL];
		[self _initDMAButton:enableDMAMasterSecondary key:NULL];
		[self _initDMAButton:enableDMASlaveSecondary key:NULL];
	}
	else {
		[self _initDMAButton:enableDMAMaster
			key:[self _getKeyForDMAButton:enableDMAMaster]];
		[self _initDMAButton:enableDMASlave
			key:[self _getKeyForDMAButton:enableDMASlave]];
		[self _initDMAButton:enableDMAMasterPrimary
			key:[self _getKeyForDMAButton:enableDMAMasterPrimary]];
		[self _initDMAButton:enableDMASlavePrimary
			key:[self _getKeyForDMAButton:enableDMASlavePrimary]];
		[self _initDMAButton:enableDMAMasterSecondary 
			key:[self _getKeyForDMAButton:enableDMAMasterSecondary]];
		[self _initDMAButton:enableDMASlaveSecondary 
			key:[self _getKeyForDMAButton:enableDMASlaveSecondary]];	
	}

	[self _initButton:multipleSectors key:MULTIPLE_SECTORS_ENABLE];
	[self _initPopUp:popUpMasterDual key:IDE_MASTER_KEY
		choices:(OVERRIDE_TABLE_SIZE - 1)];
	[self _initPopUp:popUpSlaveDual key:IDE_SLAVE_KEY
		choices:OVERRIDE_TABLE_SIZE];
	[self _initPopUp:popUpMasterSingle key:IDE_MASTER_KEY
		choices:(OVERRIDE_TABLE_SIZE - 1)];
	[self _initPopUp:popUpSlaveSingle key:IDE_SLAVE_KEY
		choices:OVERRIDE_TABLE_SIZE];
	[self _initPopUp:popUpMasterSec key:IDE_MASTER_KEY_SEC
		choices:(OVERRIDE_TABLE_SIZE - 1)];
	[self _initPopUp:popUpSlaveSec key:IDE_SLAVE_KEY_SEC
		choices:OVERRIDE_TABLE_SIZE];
		
#if	SET_LOCAL_TITLES
	[optionsBox setTitle:OPTION_BOX_STRING(myBundle)];
	[multipleSectors setTitle:MULTIPLE_SECTORS_STRING(myBundle)];
	[overrideButton setTitle:OPTIONS_BUTTON_STRING(myBundle)];

	[titleMasterDual setStringValue:MASTER_STRING(myBundle)];
	[titleSlaveDual setStringValue:SLAVE_STRING(myBundle)];
	[titleMasterSingle setStringValue:MASTER_STRING(myBundle)];
	[titleSlaveSingle setStringValue:SLAVE_STRING(myBundle)];
	[titleMasterSec setStringValue:MASTER_STRING(myBundle)];
	[titleSlaveSec setStringValue:SLAVE_STRING(myBundle)];
	
	[boxPriDual setTitle:PRI_CHANNEL_STRING(myBundle)];
	[boxSecDual setTitle:SEC_CHANNEL_STRING(myBundle)];
	[boxSingle setTitle:SINGLE_CHANNEL_STRING(myBundle)];
	[panelDualChannel setTitle:OPTIONS_BUTTON_STRING(myBundle)];
	[panelSingleChannel setTitle:OPTIONS_BUTTON_STRING(myBundle)];
#endif	SET_LOCAL_TITLES
	
	return self;
}

- selectOverrides:sender
{
	id panel;
	
	if (isDualChannel)
		panel = panelDualChannel;
	else
		panel = panelSingleChannel;

    [panel center];
    [panel makeKeyAndOrderFront:self];
	[NXApp runModalFor:panel];
	[panel orderOut:self];

    return self;
}

- ok:sender
{
    [NXApp stopModal:1];
    return self;
}

- setDMAMode:sender
{
	const char *key = [self _getKeyForDMAButton:sender];
	
	if (key) {
		char *value;
		if ([sender state] == NO)
			value = "0x000000ff";
		else
			value = "0xffffffff";
		[table insertKey:key value:value];
	}
	return self;
}

- multipleSectors:sender
{
	char *str;

	if ([sender state])
		str = "Yes";
	else
		str = "No";

	[table insertKey:MULTIPLE_SECTORS_ENABLE value:str];
	return self;
}

- selectMasterOverride:sender
{
    int i = [[sender selectedCell] tag];
    [table insertKey:IDE_MASTER_KEY value:(char *)overrideTable[i]];
    return self;
}

- selectSlaveOverride:sender
{
    int i = [[sender selectedCell] tag];
    [table insertKey:IDE_SLAVE_KEY value:(char *)overrideTable[i]];
    return self;
}

- selectMasterOverrideSec:sender
{
    int i = [[sender selectedCell] tag];
    [table insertKey:IDE_MASTER_KEY_SEC value:(char *)overrideTable[i]];
    return self;
}

- selectSlaveOverrideSec:sender
{	
    int i = [[sender selectedCell] tag];
    [table insertKey:IDE_SLAVE_KEY_SEC value:(char *)overrideTable[i]];
    return self;
}

@end
