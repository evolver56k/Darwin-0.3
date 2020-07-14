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

/* Sym8xxMisc.m created by russb2 on Sat 30-May-1998 */

#import "Sym8xxController.h"

@implementation Sym8xxController
@end

/*
 * Miscellaneous IO worker routines
 */

u_int32_t Sym8xxReadRegs( volatile u_int8_t *chipRegs, u_int32_t regOffset, u_int32_t regSize )
{
    if ( regSize == 1 )
    {
        return chipRegs[regOffset];
    }
    if ( regSize == 2 )
    {
        return EndianSwap16( *(volatile u_int16_t *)&chipRegs[regOffset] );
    }
    else if (regSize == 4 )
    {
        return EndianSwap32( *(volatile u_int32_t *)&chipRegs[regOffset] );
    }
    else
    {
        kprintf("SCSI(SymBios875): Sym8xxReadRegs incorrect regSize\n\r" );
        return 0;
    } 
}

void Sym8xxWriteRegs( volatile u_int8_t *chipRegs, u_int32_t regOffset, u_int32_t regSize, u_int32_t regValue )
{
    if ( regSize == 1 )
    {
        chipRegs[regOffset] = regValue;
    }
    else if ( regSize == 2 )
    {
        volatile u_int16_t *p = (volatile u_int16_t *)&chipRegs[regOffset];
        *p = EndianSwap16( regValue );
    }
    else if ( regSize == 4 )
    {
        volatile u_int32_t *p = (volatile u_int32_t *)&chipRegs[regOffset];
        *p = EndianSwap32( regValue );
    }
    else
    {
        kprintf("SCSI(SymBios875): Sym8xxWriteRegs incorrect regSize\n\r" );
    }
    eieio();
}
