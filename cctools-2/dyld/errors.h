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
#import <stdarg.h>
#import <mach/mach.h>
#import <mach-o/nlist.h>

#import "images.h"
#if defined(__MWERKS__) || (defined(__MACH30__) && !defined(EBADEXEC))
#define EBADEXEC	85      /* Bad executable */
#define EBADARCH        86      /* Bad CPU type in executable */
#define ESHLIBVERS      87      /* Shared library version mismatch */
#define EBADMACHO       88      /* Malformed Macho file */
#endif

extern enum bool dyld_error_print;

enum dyld_exit_failure_base { DYLD_EXIT_FAILURE_BASE = 60 };

/*
 * The link_edit_error_class values are passed to the user's link edit error
 * handler and also added to the DYLD_EXIT_FAILURE_BASE as the value used for
 * exit().  The other dyld_exit_* types are only used with exit values.
 */
enum link_edit_error_class {
    DYLD_FILE_ACCESS,	/* 0 */
    DYLD_FILE_FORMAT,	/* 1 */
    DYLD_MACH_RESOURCE,	/* 2 */
    DYLD_UNIX_RESOURCE,	/* 3 */
    DYLD_OTHER_ERROR,	/* 4 */
    DYLD_WARNING	/* 5 */
};
enum dyld_exit_multiply_defined {
    DYLD_EXIT_MULTIPLY_DEFINED = 6
};
enum dyld_exit_undefined {
    DYLD_EXIT_UNDEFINED = 7
};
enum dyld_dead_lock {
    DYLD_DEAD_LOCK = 8
};

enum dyld_other_error_numbers {
    DYLD_RELOCATION,
    DYLD_LAZY_BIND,
    DYLD_INDR_LOOP
};

/*
 * These are the pointers to the user's three error handler functions.
 */
extern void (*user_undefined_handler)(
    const char *symbol_name);
extern module_state * (*user_multiple_handler)(
    struct nlist *symbol,
    module_state *old_module,
    module_state *new_module);
extern void (*user_linkEdit_handler)(
    enum link_edit_error_class error_class,
    int error_number,
    const char *file_name,
    const char *error_string);

extern void check_and_report_undefineds(
    void);

extern void multiply_defined_error(
    char *symbol_name,
    struct image *new_image,
    struct nlist *new_symbol,
    module_state *new_module,
    char *new_library_name,
    char *new_module_name,
    struct image *prev_image,
    struct nlist *prev_symbol,
    module_state *prev_module,
    char *prev_library_name,
    char *prev_module_name);

extern void unlinked_lazy_pointer_handler(
    void);

extern void dead_lock_error(
    void);

extern void link_edit_error(
    enum link_edit_error_class error_class,
    int error_number,
    char *file_name);

extern void reset_error_string(
    void);

extern void error(
    const char *format,
    ...) __attribute__ ((format (printf, 1, 2)));

extern void system_error(
    int errnum,
    const char *format,
    ...) __attribute__ ((format (printf, 2, 3)));

extern void mach_error(
    kern_return_t r,
    char *format,
    ...) __attribute__ ((format (printf, 2, 3)));

extern void set_error_string(
    const char *format,
    ...) __attribute__ ((format (printf, 1, 2)));

extern void vset_error_string(
    const char *format,
    va_list ap);

extern void add_error_string(
    const char *format,
    ...) __attribute__ ((format (printf, 1, 2)));

extern void vadd_error_string(
    const char *format,
    va_list ap);

extern void print(
    const char *format,
    ...) __attribute__ ((format (printf, 1, 2)));
