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
#import <mach/mach.h>
#import <stuff/bool.h>

/*
 * The default paths have $HOME prepended to them so the first path is
 * $HOME/Library/Frameworks or $HOME/lib .
 */
#ifdef __SLICK__
#define DEFAULT_FALLBACK_FRAMEWORK_PATH \
	"/Library/Frameworks:/LocalLibrary/Frameworks:/NextLibrary/Frameworks"
#else
#define DEFAULT_FALLBACK_FRAMEWORK_PATH \
	"/Library/Frameworks:/Local/Library/Frameworks:/Network/Library/Frameworks:/System/Library/Frameworks"
#endif
#define DEFAULT_FALLBACK_LIBRARY_PATH "/lib:/usr/local/lib:/lib:/usr/lib"

extern struct host_basic_info host_basic_info;
extern enum bool executable_bind_at_load;
extern char *home;
extern char *dyld_framework_path;
extern char *dyld_fallback_framework_path;
extern char *default_fallback_framework_path;
extern char *dyld_library_path;
extern char *dyld_fallback_library_path;
extern char *default_fallback_library_path;
extern char *dyld_insert_libraries;
extern enum bool dyld_print_libraries;
extern enum bool dyld_mem_protect;
extern enum bool dyld_ebadexec_only;
extern enum bool dyld_bind_at_launch;
extern unsigned long dyld_prebind_debug;
extern unsigned long dyld_sample_debug;
extern unsigned long dyld_image_vmaddr_slide;

extern enum bool profile_server;
extern enum bool prebinding;
extern enum bool launched;
extern enum bool executable_prebound;

extern void protect_data_segment(
    void);
extern void unprotect_data_segment(
    void);

#ifdef DYLD_PROFILING
extern void profiling_exit(
    int status);
extern void dyld_monoutput(
    void);
#endif
