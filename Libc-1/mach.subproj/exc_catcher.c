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
 * catch_exception_raise will be implemented by user programs
 * This implementation is provided to resolve the reference in
 * exc_server().
 */

#include <mach/boolean.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach-o/dyld.h>

__private_extern__ kern_return_t internal_catch_exception_raise (
	port_t exception_port, 
	port_t thread, 
	port_t task, 
	int exception, 
	int code, int subcode)
{
    static int checkForFunction = 0;
    /* This will be non-zero if the user has defined this function */
    static kern_return_t (*catch_exception_raise_func)(port_t, port_t, port_t, int, int, int);
    if (checkForFunction == 0) {
        checkForFunction = 1;
        _dyld_lookup_and_bind("_catch_exception_raise", (unsigned long *)&catch_exception_raise_func, (void **)0);
    }
    if (catch_exception_raise_func == 0) {
        /* The user hasn't defined catch_exception_raise in their binary */
        abort();
    }
    return (*catch_exception_raise_func)(exception_port, thread, task, exception, code, subcode);
}
