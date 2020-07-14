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
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 */

#import "libsaio.h"
#if TEST
#import "testmodule/test.h"
#endif

int
loadModule(char *moduleName, moduleEntry_t *pointer)
{
    int fd, ret, size;
    char buf[128], *addr;
    
    sprintf(buf, "/usr/standalone/i386/%s", moduleName);
    fd = open(buf, 0);
    if (fd < 0)
	return -1;
    addr = 0;
//printf("Loading module %s\n",moduleName);
    ret = loadprog(0, fd, 0, (entry_t *)pointer, &addr, &size);
//printf("Loaded module at %x\n", addr);
    close(fd);
    return ret;
}

#if TEST
moduleEntry_t test_table_pointer;
void testModules(void)
{
    loadModule("Test.module", &test_table_pointer);
    printf("Running test module (pausing 5 seconds first)\n");
    sleep(5);
    test_init("Initialize test module.\n");
    test_start("Start module test.\n");
    test_end("End module test.\n");
}
#endif