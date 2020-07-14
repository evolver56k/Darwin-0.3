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
#if NS_TARGET_MAJOR < 5
/*
 * This is what an entry in the shared library initialization section is.
 * These entries are produced in their own section by the program mkshlib(l)
 * and when a program that uses something from the a shared library that needs
 * initialization then it forces loading the object file in the host shared
 * library object file which has the initialization tables for that entry which
 * then becomes part of the file that uses the shared library.
 */
struct shlib_init {
    long value;		/* the value to be stored at the address */
    long *address;	/* the address to store the value */
};
/*
 * These two symbols bound the shared library initialization section.  The first
 * is in the the shared library initialization section.  The second is the
 * following that section.  These symbols are defined with the asm's at the
 * end of the file.
 */
extern struct shlib_init _fvmlib_init0, _fvmlib_init1;

/*
 * _init_shlibs() does the shared library initialization.  It basicly runs over
 * the shared library initialization section and processes each entry.
 */
void
_init_shlibs()
{
    struct shlib_init *p;

	p = &(_fvmlib_init0);
	while(p < &(_fvmlib_init1)){
	    *(p->address) = p->value;
	    p++;
	}
}

asm("	    .fvmlib_init0 		");
asm("	    .globl __fvmlib_init0	");
asm("    __fvmlib_init0:		");
asm("	    .fvmlib_init1		");
asm("	    .globl __fvmlib_init1	");
asm("    __fvmlib_init1:		");

#endif /* NS_TARGET_MAJOR < 5 */
