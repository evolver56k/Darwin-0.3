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
	Copyright (c) 1998 Apple Computer, Inc.
	All Rights Reserved.
	
	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
	The copyright notice above does not evidence any actual or
	intended publication of such source code.
	
	Change History:
    20-Nov-1998	Don Brady	New file.
*/

void ConvertCStringToUTF8(const char* string, u_long textEncoding, int bufferSize, char* buffer);

void ConvertStringToUTF8(int stringLength, const void* string, u_long textEncoding,
						 int bufferSize, char* buffer);

void ConvertUTF8ToCString(const char* utf8str, u_long to_encoding, u_long cnid, int bufferSize, char* buffer);

