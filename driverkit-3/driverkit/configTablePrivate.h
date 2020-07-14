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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * configTablePrivate.h - private defintions for configTable mechanism.
 *
 * HISTORY
 * 28-Jan-93    Doug Mitchell at NeXT
 *      Created.
 */

/*
 * Max size fo config data array, in bytes.
 */
#define IO_CONFIG_DATA_SIZE		4096

/*
 * Location of driver and system config table bundles.
 */ 
#define IO_CONFIG_DIR 	"/usr/Devices/"

/*
 * File names and extensions.
 */
#define IO_BUNDLE_EXTENSION		".config"
#define IO_TABLE_EXTENSION		".table"
#define IO_DEFAULT_TABLE_FILENAME	"Default.table"
#define IO_INSPECTOR_FILENAME		"Inspector.nib"
#define IO_SYSTEM_CONFIG_FILE 	"/usr/Devices/System.config/Instance0.table"
#define IO_SYSTEM_CONFIG_DIR 	"/usr/Devices/System.config/"
#define IO_BINARY_EXTENSION		"_reloc"

