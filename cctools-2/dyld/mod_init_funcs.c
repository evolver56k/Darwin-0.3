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
#import <stdlib.h>
#import <string.h>
#import <mach/mach.h>
#import <mach-o/loader.h>
#import <mach-o/nlist.h>
#import <mach-o/reloc.h>
#import <mach-o/dyld_debug.h>

#import "images.h"
#import "mod_init_funcs.h"
#import "errors.h"
#import "lock.h"

/*
 * The calls to the module initialization routines start off delayed so that
 * the initializtion in the runtime start off is done.  When that is done the
 * runtime startoff calls call_module_initializers(TRUE) and then sets
 * delay_mod_init to FALSE. 
 */
static enum bool delay_mod_init = TRUE;

static enum bool call_module_initializers_for_objects(
    enum bool make_delayed_calls,
    enum bool bind_now);

/*
 * call_module_initializers() calls the module initialization routines for
 * modules that are in the registered state and sets them to the linked state
 * when make_delayed_calls is FALSE.  When make_delayed_calls is TRUE this makes
 * the calls to the module initialization routines that have been delayed and
 * sets delay_mod_init to FALSE when done.
 */
void
call_module_initializers(
enum bool make_delayed_calls,
enum bool bind_now)
{
    unsigned long i, j, k;
    enum link_state link_state;
    struct library_images *q;
    struct segment_command *linkedit_segment;
    struct dysymtab_command *dyst;
    struct dylib_module *dylib_modules;
    unsigned long addr, *init;
    void (*func)(void);

	/*
	 * First for the object_images modules that are in the registered state
	 * call their module initialization routines.  Because objects can be
	 * unloaded and we have to release the lock to call the functions
	 * we need to repeatedly pass through the data structures for the loaded
	 * objects after each call until we get through cleanly.
	 */
	while(call_module_initializers_for_objects(
		make_delayed_calls, bind_now) == TRUE)
	    ;

	/*
	 * Next for the library modules that are in the registered state call
	 * their module initialization routines.  Note libraries can't be
	 * unloaded.
	 */
	init = NULL;
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		linkedit_segment = q->images[i].image.linkedit_segment;
		dyst = q->images[i].image.dyst;
		dylib_modules = (struct dylib_module *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->modtaboff -
		     linkedit_segment->fileoff);
		/*
		 * First set the address of the section with the module
		 * initialization pointers in the image.
		 */
		if(q->images[i].image.init != NULL)
		    init = (unsigned long *)(q->images[i].image.init->addr +
					     q->images[i].image.vmaddr_slide);
		for(j = 0; j < dyst->nmodtab; j++){
		    /* skip modules that that not in the registered state */
		    link_state = q->images[i].modules[j];

		    if(link_state != REGISTERING &&
		       (make_delayed_calls == FALSE ||
			(link_state != LINKED && link_state != FULLY_LINKED)))
			continue;

		    if(link_state == REGISTERING)
			q->images[i].modules[j] = INITIALIZING;

		    if(delay_mod_init == FALSE || make_delayed_calls == TRUE){
			for(k = 0; k < (dylib_modules[j].ninit & 0xffff); k++){
			    addr = init[(dylib_modules[j].iinit & 0xffff) + k];
			    func = (void(*)(void))addr;
			    release_lock();
			    func();
			    set_lock();
			}
		    }

		    if(q->images[i].modules[j] == INITIALIZING){
			if(bind_now == FALSE)
			    q->images[i].modules[j] = LINKED;
			else
			    q->images[i].modules[j] = FULLY_LINKED;
		    }
		}
	    }
	    q = q->next_images;
	}while(q != NULL);

	if(make_delayed_calls == TRUE)
	    delay_mod_init = FALSE;
}

/*
 * call_module_initializers_for_objects() goes through the list of objects and
 * finds an object that is in the registered state and then calles it's module
 * initilization functions for at most one object image.  If a functions are
 * call TRUE is returned.  If no functions are called (because all object
 * images have had their module initilization functions called) then FALSE is
 * returned.
 */
static
enum bool
call_module_initializers_for_objects(
enum bool make_delayed_calls,
enum bool bind_now)
{
    unsigned long i, j, n;
    enum link_state link_state;
    struct object_images *p;
    unsigned long slide_value, addr;
    void (*func)(void);

	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		/* skip modules that that not in the registered state */
		link_state = p->images[i].module;

		if(link_state != REGISTERING &&
		   (make_delayed_calls == FALSE ||
		    (link_state != LINKED && link_state != FULLY_LINKED)))
		    continue;

		if(link_state == REGISTERING){
		    p->images[i].module = INITIALIZING;
		    if(p->images[i].image.init == NULL){
			if(bind_now == FALSE)
			    p->images[i].module = LINKED;
			else
			    p->images[i].module = FULLY_LINKED;
			continue;
		    }
		}

		if(p->images[i].image.init == NULL)
		    continue;

		if(delay_mod_init == FALSE || make_delayed_calls == TRUE){
		    slide_value = p->images[i].image.vmaddr_slide;
		    n = p->images[i].image.init->size / sizeof(unsigned long);
		    for(j = 0; j < n; j++){
			addr = *((long *)
			     (p->images[i].image.init->addr + slide_value) + j);
			func = (void(*)(void))addr;
			release_lock();
			func();
			set_lock();
		    }
		}
		if(p->images[i].module == INITIALIZING){
		    if(bind_now == FALSE)
			p->images[i].module = LINKED;
		    else
			p->images[i].module = FULLY_LINKED;
		}

		if(make_delayed_calls == FALSE)
		    return(TRUE);
	    }
	    p = p->next_images;
	}while(p != NULL);
	return(FALSE);
}

/*
 * call_module_terminator_for_object() is part of the sort term work to make
 * module termination functions work for NSUnLinkModule() ONLY.
 *
 * It is passed a pointer to an object image and calls the module termination
 * functions if it has any.
 */
void
call_module_terminator_for_object(
struct object_image *object_image)
{
    unsigned long i, n;
    unsigned long slide_value, addr;
    void (*func)(void);

	if(object_image->image.term == NULL)
	    return;

	slide_value = object_image->image.vmaddr_slide;
	n = object_image->image.term->size / sizeof(unsigned long);
	for(i = 0; i < n; i++){
	    addr = *((long *)
		(object_image->image.term->addr + slide_value) + i);
	    func = (void(*)(void))addr;
	    release_lock();
	    func();
	    set_lock();
	}
}
