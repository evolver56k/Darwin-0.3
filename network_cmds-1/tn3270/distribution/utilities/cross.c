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
        This program is, essentially, a cross product generator.  It takes
        an input file which is said to consist of a number of lines, and
        expands each line.  A line like
                (a,b)(c,d)
        will be expanded to lines like
                ac
                ad
                bc
                bd
        (without regard for the ORDER of the output; ie: the lines can appear
        in any order).

        Parenthesis can be nested, so
                (a,b)(c(d,e),f)
        will produce
                acd
                ace
                af
                bcd
                bce
                bf
 */


#include <stdio.h>

char leftParen,                         /* The left parenthesis character */
        rightParen;                     /* The right parenthesis character */


/* Finds next occurrence of 'character' at this level of nesting.
        Returns 0 if no such character found.
 */

char *
ThisLevel(string, character)
char *string, character;
{
        int level;                      /* Level 0 is OUR level */

        level = 0;

        while (*string != '\0') {
                if (*string == leftParen)
                        level++;
                else if (*string == rightParen) {
                        level--;
                        if (level < 0)
                                return(0);
                }
                if ((level == 0) && (*string == character))
                                return(string);
                string++;
        }
