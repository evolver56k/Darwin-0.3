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
/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#define DEBUGOFF 1

#include <mach/mach.h>
#include <mach/cthreads.h>
#include <mach/boolean.h>

#include <sys/types.h>

#ifdef WIN32
#include <winnt-pdo.h>
#include <winsock.h>  // For the struct timeval definition!
#else
#include <sys/time.h>
#endif WIN32

#include "netmsg.h"
#include "uid.h"

#ifdef NeXT_PDO

extern void srand (unsigned int seed);
extern int rand (void);
#define random() rand()

#else

extern long	random();
extern char	*initstate();

#define STATE_SIZE	256
static char		random_state[STATE_SIZE];

#endif WIN32

extern int gettimeofday ();


/*
 * uid_init
 *
 * Initialises random number generator in order to produce unique identifiers.
 *
 */
EXPORT boolean_t uid_init()
{
    struct timeval tp;

    (void)gettimeofday(&tp, NULL);

#ifdef NeXT_PDO
    srand((int)tp.tv_sec);
#else
    (void)initstate((unsigned int)tp.tv_usec, (char *)random_state, STATE_SIZE);
#endif
    
    RETURN(TRUE);
}


/*
 * uid_get_new_uid
 *
 * Returns a new unique identifier from the random number generator.
 *
 */
EXPORT long uid_get_new_uid()
{
    long new_uid;
    new_uid = (long)random();
    RETURN(new_uid);
}
