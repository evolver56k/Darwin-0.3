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

#import "ret1.h"

@interface SuperTest : MsgTest

-(char ) ret_char:(char *)retRef; 
-(uchar_t ) ret_uchar_t:(uchar_t *)retRef; 
-(short ) ret_short:(short *)retRef; 
-(ushort_t ) ret_ushort_t:(ushort_t *)retRef; 
-(int ) ret_int:(int *)retRef; 
-(unsigned ) ret_unsigned:(unsigned *)retRef; 
-(long ) ret_long:(long *)retRef; 
-(ulong_t ) ret_ulong_t:(ulong_t *)retRef; 
-(float ) ret_float:(float *)retRef; 
-(double ) ret_double:(double *)retRef; 
-(id ) ret_id:(id *)retRef; 
-(STR ) ret_STR:(STR *)retRef; 
-(S_BITS_16_t ) ret_S_BITS_16_t:(S_BITS_16_t *)retRef; 
-(S_BITS_32_t ) ret_S_BITS_32_t:(S_BITS_32_t *)retRef; 
-(S_BITS_64_t ) ret_S_BITS_64_t:(S_BITS_64_t *)retRef; 
-(S_BITS_BIG_t ) ret_S_BITS_BIG_t:(S_BITS_BIG_t *)retRef; 
-(U_BITS_16_t ) ret_U_BITS_16_t:(U_BITS_16_t *)retRef; 
-(U_BITS_32_t ) ret_U_BITS_32_t:(U_BITS_32_t *)retRef; 
-(U_BITS_64_t ) ret_U_BITS_64_t:(U_BITS_64_t *)retRef; 
-(U_BITS_BIG_t ) ret_U_BITS_BIG_t:(U_BITS_BIG_t *)retRef; 
-(E_BITS_8_t ) ret_E_BITS_8_t:(E_BITS_8_t *)retRef; 
-(E_BITS_16_t ) ret_E_BITS_16_t:(E_BITS_16_t *)retRef; 
-(E_BITS_32_t ) ret_E_BITS_32_t:(E_BITS_32_t *)retRef; 

@end
