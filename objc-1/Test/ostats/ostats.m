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
#include <stdio.h>
#include <mach.h>
#include <nlist.h>

#include "ObjcProcess.h"

void
main(
     int argc, 
     char **argv
     )
{
	kern_return_t ret;
	task_t task;
	struct mach_header **mhdrs;

	if (argc != 2) {
		fprintf(stderr, "usage: %s pid\n", argv[0]);
		exit(1);
	}
	ret = task_by_unix_pid(task_self(), atoi(argv[1]), &task);
	if (ret != KERN_SUCCESS) {
		fprintf(stderr, "task_by_unix_pid: %s\n", 
			mach_error_string(ret));
		exit(1);
	}
	ret = task_suspend(task);
	if (ret != KERN_SUCCESS) {
		fprintf(stderr, "task_suspend: %s\n", 
			mach_error_string(ret));
		exit(1);
	}

	{
	id anApp = [ObjcProcess newFromTask:task];
	
	}

	task_resume(task);
	exit(0);
} 
