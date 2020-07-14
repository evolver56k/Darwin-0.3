/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *      File:   libc/i386/gen/bzero.c
 *      Author: Bruce Martin, NeXT Computer, Inc.
 *
 *      This file contains machine dependent code for the bzero function
 *      on NeXT i386-based products.  Currently tuned for the i486.
 *
 * HISTORY
 * 26-Aug-92  Bruce Martin (Bruce_Martin@NeXT.COM)
 *      Created.
 */

#include <string.h>

#undef bzero

void bzero (void *b, size_t length)
{
    memset((void *) b, 0, (size_t) length);
}
