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
#import <mach-o/loader.h>
#import <stuff/bool.h>
#import <sys/types.h>

enum link_state {
    UNLINKED,		/* the starting point for UNLINKED modules */
    BEING_LINKED,	/* moduled selected to be link into the program */
    RELOCATED,		/* moduled relocated dyld can now use the module */
    REGISTERING,	/* functions registered for modules being link called */
    INITIALIZING,	/* module initializers being called */
    LINKED,		/* module initializers called user can now use module */
    FULLY_LINKED,	/* module fully linked (all lazy symbols resolved) */

    PREBOUND_UNLINKED,	/* the module is prebound image but unlinked after */
			/*  the program was launch. */

    BEING_UNLINKED,	/* not yet used.  TODO unlinking */
    REPLACED,		/* not yet used.  TODO replacing */

    UNUSED		/* a module handle that is now unused */
};

struct image {
    char *name;			/* Image name for reporting errors. */
    unsigned long vmaddr_slide; /* The amount the vmaddresses are slid in the */
				/*  image from the staticly link addresses. */
    unsigned long vmaddr_size;  /* The size of the vm this image uses */
    unsigned long seg1addr;	/* The address of the first segment */
    enum bool prebound;		/* Link states set from prebound state */
    struct mach_header *mh;	/* The mach header of the image. */
    struct symtab_command *st;	/* The symbol table command for the image. */
    struct dysymtab_command	/* The dynamic symbol table command for the */
	*dyst;			/*  image. */
    struct segment_command	/* The link edit segment command for the */
	*linkedit_segment;	/*  image. */
    enum bool			/* The image has relocations in read-only */
	change_protect_on_reloc;/*  segments and protection needs to change. */
    enum bool			/* The image has relocations for instructions */
	cache_sync_on_reloc;	/*  and the i cache needs to sync with d cache*/
    struct section *init;	/* The mod init section */
    struct section *term;	/* The mod term section */
    enum bool registered;	/* The functions registered for add images */
				/*  have been called */
    enum bool private;		/* global symbols are not used for linking */
#ifdef __ppc__
    unsigned long 		/* the image's dyld_stub_binding_helper */
	dyld_stub_binding_helper; /* address */
#endif
};

/*
 * This is really an enum link_state.  Originally there was a module structure
 * that had an enum link_state field.  Because the minimum structure aligment 
 * is more than one-byte aligned this wasted space.  Since this is one of the
 * few allocated and written data structures of dyld it is important it is as
 * small as reasonable.  It needs to be addressable so using less than a byte
 * is not acceptable.
 */
typedef char module_state;

struct object_image {
    struct image image;
    module_state module;
};

struct library_image {
    struct image image;
    unsigned long nmodules;
    module_state *modules;
    struct dylib_command *dlid;
    enum bool dependent_libraries_loaded;
    dev_t dev;
    ino_t ino;
};

enum nobject_images { NOBJECT_IMAGES = 10 };
struct object_images {
    unsigned long nimages;
    struct object_image images[NOBJECT_IMAGES];
    struct object_images *next_images;
};
extern struct object_images object_images;

enum nlibrary_images { NLIBRARY_IMAGES = 10 };
struct library_images {
    unsigned long nimages;
    struct library_image images[NLIBRARY_IMAGES];
    struct library_images *next_images;
};
extern struct library_images library_images;

extern void (*dyld_monaddition)(char *lowpc, char *highpc);

extern void load_executable_image(
    char *name,
    struct mach_header *mh_execute,
    unsigned long *entry_point);

extern void load_dependent_libraries(
    void);

extern enum bool load_library_image(
    struct dylib_command *dl,
    char *dylib_name);

extern struct object_image *map_bundle_image(
    char *name,
    char *object_addr,
    unsigned long object_size);

extern void unload_bundle_image(
    struct object_image *object_image,
    enum bool keepMemoryMapped,
    enum bool reset_lazy_references);

extern void shared_pcsample_buffer(
    char *name,
    struct section *s,
    unsigned long slide_value);

extern enum bool set_images_to_prebound(
    void);

extern void undo_prebound_images(
    void);

extern void try_to_use_prebound_libraries(
    void);

extern char *executables_name;

extern char *save_string(
    char *name);
