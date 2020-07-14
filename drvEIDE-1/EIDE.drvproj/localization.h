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

#define HOST_IORDY_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Host IORDY", NULL, \
	"The title of the buttom which enables host IORDY support.")

#define MULTIPLE_SECTORS_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Multiple Sectors", NULL, \
	"The title of the button which enables multiple sector transfers.")

#define OPTION_BOX_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Enhanced IDE Options", NULL, \
	"The title of the Enhanced IDE Options box.")

#define MASTER_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Master", NULL, \
	"The title of the Master pop-up list")

#define SLAVE_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Slave", NULL, \
	"The title of the Slave pop-up list")

#define OPTIONS_BUTTON_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Advanced settings...", NULL, \
	"The title of the advanced options button")

#define OVERRIDE_STRING(override, bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, override, NULL, override)

#define PRI_CHANNEL_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Primary IDE Channel", NULL, \
	"The title of the primary channel bounding box")
	
#define SEC_CHANNEL_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"Secondary IDE Channel", NULL, \
	"The title of the secondary channel bounding box")

#define SINGLE_CHANNEL_STRING(bundle) \
	NXLocalStringFromTableInBundle(NULL, bundle, \
	"IDE Channel", NULL, \
	"The title of the single channel bounding box")
