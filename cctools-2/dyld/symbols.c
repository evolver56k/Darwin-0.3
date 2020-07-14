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
#import <stdio.h>
#import <stdlib.h>
#import <mach/mach.h>
#import "stuff/openstep_mach.h"
#import <mach-o/loader.h>
#import <mach-o/nlist.h>

#import "stuff/bool.h"
#import "images.h"
#import "symbols.h"
#import "allocate.h"
#import "errors.h"
#import "reloc.h"
#import "register_funcs.h"
#import "mod_init_funcs.h"
#import "lock.h"
#import "dyld_init.h"

/*
 * The head of the undefined list, the list of symbols being linked and a free
 * list to hold symbol list structures not currently being used.
 * These are circular lists so they can be searched from start to end and so
 * new items can be put on the end.  These three structures never have their
 * symbol, module or image filled in but they only serve as the heads and tails
 * of their lists.
 */
struct symbol_list undefined_list = {
    NULL, NULL, NULL, &undefined_list, &undefined_list
};
static struct symbol_list being_linked_list = {
    NULL, NULL, NULL, &being_linked_list, &being_linked_list
};
static struct symbol_list free_list = {
    NULL, NULL, NULL, &free_list, &free_list
};
/*
 * The structures for the symbol list are allocated in blocks and placed on the
 * free list and then handed out.  When they ar no longer needed they are placed
 * on the free list to use again.  This is data structure is the largest one in
 * dyld.  As such great care is taken to dirty as little as possible.
 */
enum nsymbol_lists { NSYMBOL_LISTS = 100 };
struct symbol_block {
    enum bool initialized;
    struct symbol_list symbols[NSYMBOL_LISTS];
    struct symbol_block *next;
};
enum nsymbol_blocks { NSYMBOL_BLOCKS = 35 };
/*
 * The data is allocated in it's own section so it can go at the end of the
 * data segment.  So that the blocks never touched are never dirtied.
 */
extern struct symbol_block symbol_blocks[NSYMBOL_BLOCKS];
static unsigned long symbol_blocks_used = 0;

static void initialize_symbol_block(
    struct symbol_block *symbol_block);
static struct symbol_list *new_symbol_list(
    void);
static void add_to_undefined_list(
    char *name,
    struct nlist *symbol,
    struct image *image);
static void add_to_being_linked_list(
    char *name,
    struct nlist *symbol,
    struct image *image);

static int nlist_bsearch(
    const char *symbol_name,
    const struct nlist *symbol);
static char *nlist_bsearch_strings;
static int toc_bsearch(
    const char *symbol_name,
    const struct dylib_table_of_contents *toc);
static struct nlist *toc_bsearch_symbols;
static char *toc_bsearch_strings;

static char * look_for_reference(
    char *symbol_name,
    struct nlist **referenced_symbol,
    module_state **referenced_module,
    struct image **referenced_image,
    struct library_image **referenced_library_image,
    enum bool ignore_lazy_references);

static void relocate_symbol_pointers(
    unsigned long symbol_index,
    unsigned long *indirect_symtab,
    struct image *image,
    unsigned long value,
    enum bool only_lazy_pointers);

static enum bool check_libraries_for_definition_and_refernce(
    char *symbol_name);

/*
 * setup_initial_undefined_list() builds the initial list of non-lazy symbol
 * references based on the executable's symbols.
 */
void
setup_initial_undefined_list(
enum bool all_symbols)
{
    unsigned long i;
    char *symbol_name;
    struct nlist *symbols;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct image *image;

	/*
	 * The executable is the first object on the object_image list.
	 */
	linkedit_segment = object_images.images[0].image.linkedit_segment;
	st = object_images.images[0].image.st;
	dyst = object_images.images[0].image.dyst;
	if(linkedit_segment == NULL || st == NULL || dyst == NULL)
	    return;
	/* the vmaddr_slide of an executable is always 0, no need to add it */
	symbols = (struct nlist *)
	    (linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	image = &object_images.images[0].image;

	for(i = dyst->iundefsym; i < dyst->iundefsym + dyst->nundefsym; i++){
	    if(executable_bind_at_load == TRUE ||
	       all_symbols == TRUE ||
	       (symbols[i].n_desc & REFERENCE_TYPE) ==
	        REFERENCE_FLAG_UNDEFINED_NON_LAZY){
		symbol_name = (char *)
		    (image->linkedit_segment->vmaddr +
		     image->st->stroff -
		     image->linkedit_segment->fileoff) +
		    symbols[i].n_un.n_strx;
		add_to_undefined_list(symbol_name, symbols + i, image);
	    }
	}
}

/*
 * initialize_symbol_block() puts the specified symbol_block on free_list.
 */
static
void
initialize_symbol_block(
struct symbol_block *symbol_block)
{
    unsigned long i;
    struct symbol_list *undefineds;

	undefineds = symbol_block->symbols;

	free_list.next = &undefineds[0];
	undefineds[0].prev = &free_list;
	undefineds[0].next = &undefineds[1];
	undefineds[0].symbol = NULL;
	undefineds[0].image = NULL;
	for(i = 1 ; i < NSYMBOL_LISTS - 1 ; i++){
	    undefineds[i].prev  = &undefineds[i-1];
	    undefineds[i].next  = &undefineds[i+1];
	    undefineds[i].symbol = NULL;
	    undefineds[i].image = NULL;
	}
	free_list.prev = &undefineds[i];
	undefineds[i].prev = &undefineds[i-1];
	undefineds[i].next = &free_list;
	undefineds[i].symbol = NULL;
	undefineds[i].image = NULL;

	symbol_block->initialized = TRUE;
}

/*
 * new_symbol_list() takes a symbol list item off the free list and returns it.
 * If there are no more items on the free list it allocates a new block of them
 * links them on to the free list and returns one.
 */
static
struct symbol_list *
new_symbol_list(
void)
{
    struct symbol_block *p, *q;
    struct symbol_list *new;

	if(free_list.next == &free_list){
	    if(symbol_blocks[0].initialized == FALSE){
		symbol_blocks_used++;
		q = symbol_blocks;
	    }
	    else{
		for(p = symbol_blocks; p->next != NULL; p = p->next)
		    ;
		if(symbol_blocks_used < NSYMBOL_BLOCKS){
		    q = symbol_blocks + symbol_blocks_used;
		    symbol_blocks_used++;
		}
		else{
		    q = allocate(sizeof(struct symbol_block));
		}
		p->next = q;
		q->next = NULL;
	    }
	    initialize_symbol_block(q);
	}
	/* take the first one off the free list */
	new = free_list.next;
	new->next->prev = &free_list;
	free_list.next = new->next;

	return(new);
}

/*
 * add_to_undefined_list() adds an item to the list of undefined symbols.
 */
static
void
add_to_undefined_list(
char *name,
struct nlist *symbol,
struct image *image)
{
    struct symbol_list *undefined;
    struct symbol_list *new;

	for(undefined = undefined_list.next;
	    undefined != &undefined_list;
	    undefined = undefined->next){
	    if(undefined->name == name)
		return;
	}

	/* get a new symbol list entry */
	new = new_symbol_list();

	/* fill in the pointers for the undefined symbol */
	new->name = name;
	new->symbol = symbol;
	new->image = image;

	/* put this at the end of the undefined list */
	new->prev = undefined_list.prev;
	new->next = &undefined_list;
	undefined_list.prev->next = new;
	undefined_list.prev = new;
}

/*
 * add_to_being_linked_list() adds an item to the list of being linked symbols.
 */
static
void
add_to_being_linked_list(
char *name,
struct nlist *symbol,
struct image *image)
{
    struct symbol_list *new;

	/* get a new symbol list entry */
	new = new_symbol_list();

	/* fill in the pointers for the being_linked symbol */
	new->name = name;
	new->symbol = symbol;
	new->image = image;

	/* put this at the end of the being_linked list */
	new->prev = being_linked_list.prev;
	new->next = &being_linked_list;
	being_linked_list.prev->next = new;
	being_linked_list.prev = new;
}

/*
 * clear_being_linked_list() clear off the items on the list of being linked
 * symbols and returns then to the free list.
 */
void
clear_being_linked_list(
void)
{
    struct symbol_list *old, *old_next;

	for(old = being_linked_list.next;
	    old != &being_linked_list;
	    /* no increment expression */){

	    /* take off the list */
	    old->next->prev = &being_linked_list;
	    old_next = old->next;

	    /* put this at the end of the free_list list */
	    old->prev = free_list.prev;
	    old->next = &free_list;
	    free_list.prev->next = old;
	    free_list.prev = old;

	    /* clear the pointers */
	    old->name = NULL;
	    old->symbol = NULL;
	    old->image = NULL;
	    old = old_next;
	}

	being_linked_list.name = NULL;
	being_linked_list.symbol = NULL;
	being_linked_list.image = NULL;
	being_linked_list.prev = &being_linked_list;
	being_linked_list.next = &being_linked_list;
}

/*
 * clear_undefined_list() clear off the items on the list of undefined
 * symbols and returns then to the free list.
 */
void
clear_undefined_list(
void)
{
    struct symbol_list *old, *old_next;

	for(old = undefined_list.next;
	    old != &undefined_list;
	    /* no increment expression */){

	    /* take off the list */
	    old->next->prev = &undefined_list;
	    old_next = old->next;

	    /* put this at the end of the free_list list */
	    old->prev = free_list.prev;
	    old->next = &free_list;
	    free_list.prev->next = old;
	    free_list.prev = old;

	    /* clear the pointers */
	    old->name = NULL;
	    old->symbol = NULL;
	    old->image = NULL;
	    old = old_next;
	}

	undefined_list.name = NULL;
	undefined_list.symbol = NULL;
	undefined_list.image = NULL;
	undefined_list.prev = &undefined_list;
	undefined_list.next = &undefined_list;
}

/*
 * resolve undefined symbols by searching symbol table and linking library
 * modules as needed.  bind_now is TRUE only when fully binding an image,
 * the normal case when lazy binding is to occur it is FALSE.  When trying to
 * launch with prebound libraries launching_with_prebound_libraries is TRUE and
 * multiply defined symbols do not cause an error but cause this routine to
 * routine to return FALSE otherwise it returns TRUE.
 */
enum bool
resolve_undefineds(
enum bool bind_now,
enum bool launching_with_prebound_libraries)
{
    struct symbol_list *undefined, *next_undefined, *defined;
    struct nlist *symbol;
    module_state *module;
    struct image *image;
    enum link_state link_state;
    struct library_image *library_image;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct dylib_module *dylib_modules;
    struct relocation_info *relocs;
    struct nlist *symbols;
    char *strings;
    unsigned long module_index;
    enum bool r;
    unsigned long j;
    struct load_command *lc;
    struct segment_command *sg;

	for(undefined = undefined_list.next;
	    undefined != &undefined_list;
	    /* no increment expression */){

	    lookup_symbol(undefined->name, &symbol, &module, &image,
			  &library_image, NO_INDR_LOOP);
	    if(symbol != NULL){
		/*
		 * The symbol was found so remove it from the undefined_list
		 * and add it to the being_linked_list so that any symbol
		 * pointers for this symbol can be set to this symbol's value.
		 */
		/* take this off the undefined list */
		next_undefined = undefined->next;
		undefined->prev->next = undefined->next;
		undefined->next->prev = undefined->prev;
		defined = undefined;
		undefined = next_undefined;

		/* fill in the pointers for the defined symbol */
		defined->name = (char *)
		    (image->vmaddr_slide +
		     image->linkedit_segment->vmaddr +
		     image->st->stroff -
		     image->linkedit_segment->fileoff) +
		    symbol->n_un.n_strx;
		defined->symbol = symbol;
		defined->image = image;

		/* put this at the end of the being_linked list */
		defined->prev = being_linked_list.prev;
		defined->next = &being_linked_list;
		being_linked_list.prev->next = defined;
		being_linked_list.prev = defined;
	
		/*
		 * If the module that defineds this symbol is not linked or
		 * being linked then link the module.
		 */
		link_state = *module;
		/* TODO: figure out what to do with REPLACED modules */
		if(link_state == PREBOUND_UNLINKED){
		    linkedit_segment = image->linkedit_segment;
		    st = image->st;
		    dyst = image->dyst;
		    relocs = (struct relocation_info *)
			(image->vmaddr_slide +
			 linkedit_segment->vmaddr +
			 dyst->extreloff -
			 linkedit_segment->fileoff);
		    symbols = (struct nlist *)
			(image->vmaddr_slide +
			 linkedit_segment->vmaddr +
			 st->symoff -
			 linkedit_segment->fileoff);
		    strings = (char *)
			(image->vmaddr_slide +
			 linkedit_segment->vmaddr +
			 st->stroff -
			 linkedit_segment->fileoff);
		    dylib_modules = (struct dylib_module *)
			(image->vmaddr_slide +
			 linkedit_segment->vmaddr +
			 dyst->modtaboff -
			 linkedit_segment->fileoff);
		    module_index = module - library_image->modules;

		    /*
		     * If the image has relocations in read-only segments and
		     * the protection needs to change change it.
		     */
		    if(image->change_protect_on_reloc){
			lc = (struct load_command *)((char *)image->mh +
				sizeof(struct mach_header));
			for(j = 0; j < image->mh->ncmds; j++){
			    switch(lc->cmd){
			    case LC_SEGMENT:
				sg = (struct segment_command *)lc;
				if((r = vm_protect(mach_task_self(),sg->vmaddr +
				    image->vmaddr_slide, (vm_size_t)sg->vmsize,
				    FALSE, sg->maxprot)) != KERN_SUCCESS){
				    mach_error(r, "can't set vm_protection on "
					"segment: %.16s for object: %s",
					sg->segname, image->name);
				    link_edit_error(DYLD_MACH_RESOURCE, r,
					image->name);
				}
				break;
			    }
			    lc = (struct load_command *)
				 ((char *)lc + lc->cmdsize);
			}
		    }

		    undo_external_relocation(
			TRUE, /* undo_prebinding */
			image,
			relocs + dylib_modules[module_index].iextrel,
			dylib_modules[module_index].nextrel,
			symbols,
			strings,
			image->name,
			strings + dylib_modules[module_index].module_name);
		    *module = UNLINKED;

		    /*
		     * If the image has relocations in read-only segments and
		     * the protection was changed change it back.
		     */
		    if(image->change_protect_on_reloc){
			lc = (struct load_command *)((char *)image->mh +
				sizeof(struct mach_header));
			for(j = 0; j < image->mh->ncmds; j++){
			    switch(lc->cmd){
			    case LC_SEGMENT:
				sg = (struct segment_command *)lc;
				if((r = vm_protect(mach_task_self(),sg->vmaddr +
				    image->vmaddr_slide,
				    (vm_size_t)sg->vmsize, FALSE,
				    sg->initprot)) != KERN_SUCCESS){
				    mach_error(r, "can't set vm_protection on "
					"segment: %.16s for object: %s",
					sg->segname, image->name);
				    link_edit_error(DYLD_MACH_RESOURCE, r,
					image->name);
				}
				break;
			    }
			    lc = (struct load_command *)
				 ((char *)lc + lc->cmdsize);
			}
		    }

		    /* link this library module */
		    (void)link_library_module(library_image, image, module,
			    bind_now, launching_with_prebound_libraries);
		}
		else if(link_state == UNLINKED){
		    /* link this library module */
		    r = link_library_module(library_image, image, module,
			    bind_now, launching_with_prebound_libraries);
		    if(launching_with_prebound_libraries == TRUE && r == FALSE)
			return(FALSE);
		}
		else if(bind_now == TRUE &&
			library_image != NULL &&
			link_state == LINKED){
		    /* cause this library module to be fully linked */
		    (void)link_library_module(library_image, image, module,
			    bind_now, launching_with_prebound_libraries);
		}
		if(undefined == &undefined_list &&
		   undefined->next != &undefined_list)
		    undefined = undefined->next;
	    }
	    else{
		undefined = undefined->next;
	    }
	}
	return(TRUE);
}

/*
 * link_library_module() links in the specified library module. It checks the
 * module for symbols that are already defined and reports multiply defined
 * errors.  Then it adds it's non-lazy undefined symbols to the undefined
 * list. bind_now is TRUE only when fully binding a library module, the normal
 * case when lazy binding is to occur it is FALSE.  When trying to launch with
 * prebound libraries launching_with_prebound_libraries is TRUE and multiply
 * defined symbols do not cause an error but cause this routine to routine to
 * return FALSE otherwise it returns TRUE.
 */
enum bool
link_library_module(
struct library_image *library_image,
struct image *image,
module_state *module,
enum bool bind_now,
enum bool launching_with_prebound_libraries)
{
    unsigned long i, j;
    char *symbol_name;

    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols;
    char *strings;
    struct dylib_module *dylib_modules, *dylib_module;
    unsigned long module_index;
    enum link_state link_state;

    struct nlist *prev_symbol;
    module_state *prev_module;
    struct image *prev_image;
    struct library_image *prev_library_image;
    enum link_state prev_link_state;
    struct segment_command *prev_linkedit_segment;
    struct symtab_command *prev_st;
    struct dysymtab_command *prev_dyst;
    struct nlist *prev_symbols;
    char *prev_strings, *prev_library_name, *prev_module_name;
    struct dylib_module *prev_dylib_modules, *prev_dylib_module;
    unsigned long prev_module_index;

    struct dylib_reference *dylib_references;
    struct nlist *ref_symbol;
    module_state *ref_module;
    struct image *ref_image;
    struct library_image *ref_library_image;
    enum link_state ref_link_state;
    enum bool found;
    struct symbol_list *being_linked;

	linkedit_segment = image->linkedit_segment;
	st = image->st;
	dyst = image->dyst;
	symbols = (struct nlist *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	strings = (char *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->stroff -
	     linkedit_segment->fileoff);
	dylib_modules = (struct dylib_module *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     dyst->modtaboff -
	     linkedit_segment->fileoff);
	module_index = module - library_image->modules;
	dylib_module = dylib_modules + module_index;

	/*
	 * For each defined symbol check to see if it is not defined in a module
	 * that is already linked (or being linked).
	 */
	for(i = dylib_module->iextdefsym;
	    i < dylib_module->iextdefsym + dylib_module->nextdefsym;
	    i++){
	    symbol_name = strings + symbols[i].n_un.n_strx;
	    lookup_symbol(symbol_name, &prev_symbol, &prev_module, &prev_image,
			  &prev_library_image, NO_INDR_LOOP);
	    /*
	     * If we are fully binding a module now that was previously only
	     * bound for non-lazy symbols the symbol maybe found in this module
	     * which is not an error.
	     */
	    if(prev_symbol != NULL && module != prev_module){
		prev_link_state = *prev_module;

		if(prev_link_state == BEING_LINKED ||
		   prev_link_state == RELOCATED ||
		   prev_link_state == REGISTERING ||
		   prev_link_state == INITIALIZING ||
		   prev_link_state == LINKED  ||
		   prev_link_state == FULLY_LINKED){

		    if(launching_with_prebound_libraries == TRUE){
			if(dyld_prebind_debug != 0 && prebinding == TRUE)
			    print("dyld: %s: trying to use prebound libraries "
				   "failed due to multiply defined symbols\n",
				   executables_name);
			prebinding = FALSE;
			return(FALSE);
		    }

		    prev_linkedit_segment = prev_image->linkedit_segment;
		    prev_st = prev_image->st;
		    prev_dyst = prev_image->dyst;
		    prev_symbols = (struct nlist *)
			(prev_image->vmaddr_slide +
			 prev_linkedit_segment->vmaddr +
			 prev_st->symoff -
			 prev_linkedit_segment->fileoff);
		    prev_strings = (char *)
			(prev_image->vmaddr_slide +
			 prev_linkedit_segment->vmaddr +
			 prev_st->stroff -
			 prev_linkedit_segment->fileoff);
		    prev_dylib_modules = (struct dylib_module *)
			(prev_image->vmaddr_slide +
			 prev_linkedit_segment->vmaddr +
			 prev_dyst->modtaboff -
			 prev_linkedit_segment->fileoff);
		    if(prev_library_image != NULL){
			prev_module_index = prev_module -
					    prev_library_image->modules;
			prev_dylib_module = prev_dylib_modules +
					    prev_module_index;
			prev_library_name = prev_image->name;
			prev_module_name = prev_strings +
					   prev_dylib_module->module_name;
		    }
		    else{
			prev_library_name = NULL;
			prev_module_name = prev_image->name;
		    }
		    multiply_defined_error(symbol_name,
					   image,
					   symbols + i,
					   module,
					   image->name,
					   strings + dylib_module->module_name,
					   prev_image,
					   prev_symbol,
					   prev_module,
					   prev_library_name,
					   prev_module_name);
		}
	    }
	}

	/*
	 * For each reference to a non-lazy undefined symbol look it up to see
	 * if it is defined in an already linked (or being linked) module.
	 *     If it is then check the list of symbols being linked and put it
	 *     on that list if it is not already there.
	 *
	 *     If it is not then add it to the undefined list.
	 */
	dylib_references = (struct dylib_reference *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     dyst->extrefsymoff -
	     linkedit_segment->fileoff);
	for(i = dylib_module->irefsym;
	    i < dylib_module->irefsym + dylib_module->nrefsym;
	    i++){
	    if(dylib_references[i].flags == REFERENCE_FLAG_UNDEFINED_NON_LAZY ||
	       ((executable_bind_at_load == TRUE || bind_now == TRUE) && 
	        dylib_references[i].flags == REFERENCE_FLAG_UNDEFINED_LAZY)){
		symbol_name = strings +
			      symbols[dylib_references[i].isym].n_un.n_strx;
		lookup_symbol(symbol_name, &ref_symbol, &ref_module, &ref_image,
			      &ref_library_image, NO_INDR_LOOP);
		if(ref_symbol != NULL){
		    ref_link_state = *ref_module;

		    if(ref_link_state == BEING_LINKED ||
		       ref_link_state == RELOCATED ||
		       ref_link_state == REGISTERING ||
		       ref_link_state == INITIALIZING ||
		       ref_link_state == LINKED ||
		       ref_link_state == FULLY_LINKED){

			/* check being_linked_list of symbols */
			found = FALSE;
			for(being_linked = being_linked_list.next;
			    being_linked != &being_linked_list;
			    being_linked = being_linked->next){
			    if(being_linked->symbol == ref_symbol){
				found = TRUE;
				break;
			    }
			}
			if(found == FALSE)
			    add_to_being_linked_list(symbol_name, ref_symbol,
						     ref_image);
			if(bind_now == TRUE &&
			   ref_library_image != NULL &&
			   ref_link_state == LINKED)
			    add_to_undefined_list(symbol_name, ref_symbol,
						     ref_image);
		    }
		    else{
			add_to_undefined_list(symbol_name,ref_symbol,ref_image);
		    }
		}
		else{
		    add_to_undefined_list(symbol_name, symbols +
					  dylib_references[i].isym, image);
		}
	    }
	    else{
		/*
		 * If this is a reference to a private extern make sure the
		 * module that defineds it is linked and if not set cause it
		 * to be linked.  References to private externs in a library
		 * only are resolved to symbols in the same library and modules
		 * in a library that have private externs can't have any global
		 * symbols (this is done by the static link editor).  The reason
		 * this is done at all is so that module initialization
		 * functions are called and functions registered for modules
		 * being linked are called.
		 */
		if(dylib_references[i].flags ==
				   REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY ||
		   dylib_references[i].flags ==
				   REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY){
		    for(j = 0; j < dyst->nmodtab; j++){
			if(dylib_references[i].isym >=
			       dylib_modules[j].ilocalsym &&
			   dylib_references[i].isym <
			       dylib_modules[j].ilocalsym +
				   dylib_modules[j].nlocalsym)
			    break;
		    }
		    if(j < dyst->nmodtab){
			ref_link_state = library_image->modules[j];
			if(ref_link_state == UNLINKED)
			    library_image->modules[j] = BEING_LINKED;
		    }
		}
	    }
	}

	/*
	 * Finally change this module's linked state.  If we are not fully
	 * binding marked this as being linked.  If we are fully binding and
	 * if the module is not linked again set it to being linked otherwise
	 * set it to fully linked.
	 */
	if(bind_now == FALSE)
	    *module = BEING_LINKED;
	else{
	    link_state = *module;

	    if(link_state != LINKED)
		*module = BEING_LINKED;
	    else
		*module = FULLY_LINKED;
	}
	return(TRUE);
}

/*
 * link_object_module() links in the specified object module. It checks the
 * module for symbols that are already defined and reports multiply defined
 * errors.  Then it adds it's non-lazy undefined symbols to the undefined
 * list.  And if bind_now is true it also adds lazy undefined symbols to the
 * undefined symbols to the undefined list.  If the module is private then
 * the multiply symbols are not checked for.
 */
void
link_object_module(
struct object_image *object_image,
enum bool bind_now)
{
    unsigned long i;
    char *symbol_name;

    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols;
    char *strings;
    enum link_state link_state;

    struct nlist *prev_symbol;
    module_state *prev_module;
    struct image *prev_image;
    struct library_image *prev_library_image;
    enum link_state prev_link_state;
    struct segment_command *prev_linkedit_segment;
    struct symtab_command *prev_st;
    struct dysymtab_command *prev_dyst;
    struct nlist *prev_symbols;
    char *prev_strings, *prev_library_name, *prev_module_name;
    struct dylib_module *prev_dylib_modules, *prev_dylib_module;
    unsigned long prev_module_index;

    struct nlist *ref_symbol;
    module_state *ref_module;
    struct image *ref_image;
    struct library_image *ref_library_image;
    enum link_state ref_link_state;
    enum bool found;
    struct symbol_list *being_linked;

	/*
	 * For each defined symbol check to see if it is not defined in a module
	 * that is already linked (or being linked).
	 */
	linkedit_segment = object_image->image.linkedit_segment;
	st = object_image->image.st;
	dyst = object_image->image.dyst;
	symbols = (struct nlist *)
	    (object_image->image.vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	strings = (char *)
	    (object_image->image.vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->stroff -
	     linkedit_segment->fileoff);

	/*
	 * If this object file image is private skip checking for multiply
	 * defined symbols and move on to added undefined symbols.
	 */
	if(object_image->image.private == TRUE)
	    goto add_undefineds;

	for(i = dyst->iextdefsym; i < dyst->iextdefsym + dyst->nextdefsym; i++){
	    symbol_name = strings + symbols[i].n_un.n_strx;
	    lookup_symbol(symbol_name, &prev_symbol, &prev_module, &prev_image,
			  &prev_library_image, NO_INDR_LOOP);
	    /*
	     * If we are fully binding a module now that was previously only
	     * bound for non-lazy symbols the symbol maybe found in this module
	     * which is not an error.
	     */
	    if(prev_symbol != NULL && &object_image->module != prev_module){
		prev_link_state = *prev_module;

		if(prev_link_state == BEING_LINKED ||
		   prev_link_state == RELOCATED ||
		   prev_link_state == REGISTERING ||
		   prev_link_state == INITIALIZING ||
		   prev_link_state == LINKED ||
		   prev_link_state == FULLY_LINKED){

		    prev_linkedit_segment = prev_image->linkedit_segment;
		    prev_st = prev_image->st;
		    prev_dyst = prev_image->dyst;
		    prev_symbols = (struct nlist *)
			(prev_image->vmaddr_slide +
			 prev_linkedit_segment->vmaddr +
			 prev_st->symoff -
			 prev_linkedit_segment->fileoff);
		    prev_strings = (char *)
			(prev_image->vmaddr_slide +
			 prev_linkedit_segment->vmaddr +
			 prev_st->stroff -
			 prev_linkedit_segment->fileoff);
		    prev_dylib_modules = (struct dylib_module *)
			(prev_image->vmaddr_slide +
			 prev_linkedit_segment->vmaddr +
			 prev_dyst->modtaboff -
			 prev_linkedit_segment->fileoff);
		    if(prev_library_image != NULL){
			prev_module_index = prev_module -
					    prev_library_image->modules;
			prev_dylib_module = prev_dylib_modules +
					    prev_module_index;
			prev_library_name = prev_image->name;
			prev_module_name = prev_strings +
					   prev_dylib_module->module_name;
		    }
		    else{
			prev_library_name = NULL;
			prev_module_name = prev_image->name;
		    }
		    multiply_defined_error(symbol_name,
					   &(object_image->image),
					   symbols + i,
					   &object_image->module,
					   NULL,
					   object_image->image.name,
					   prev_image,
					   prev_symbol,
					   prev_module,
					   prev_library_name,
					   prev_module_name);
		}
	    }
	}

add_undefineds:
	/*
	 * For each reference to a non-lazy undefined symbol look it up to see
	 * if it is defined in an already linked (or being linked) module.
	 *     If it is then check the list of symbols being linked and put it
	 *     on that list if it is not already there.
	 *
	 *     If it is not then add it to the undefined list.
	 */
	for(i = dyst->iundefsym; i < dyst->iundefsym + dyst->nundefsym; i++){
	    if(bind_now == TRUE || executable_bind_at_load == TRUE ||
	       (symbols[i].n_desc & REFERENCE_TYPE) ==
	        REFERENCE_FLAG_UNDEFINED_NON_LAZY){

		symbol_name = strings + symbols[i].n_un.n_strx;

		lookup_symbol(symbol_name, &ref_symbol, &ref_module, &ref_image,
			      &ref_library_image, NO_INDR_LOOP);
		if(ref_symbol != NULL){
		    ref_link_state = *ref_module;

		    if(ref_link_state == BEING_LINKED ||
		       ref_link_state == RELOCATED ||
		       ref_link_state == REGISTERING ||
		       ref_link_state == INITIALIZING ||
		       ref_link_state == LINKED ||
		       ref_link_state == FULLY_LINKED){

			/* check being_linked_list of symbols */
			found = FALSE;
			for(being_linked = being_linked_list.next;
			    being_linked != &being_linked_list;
			    being_linked = being_linked->next){
			    if(being_linked->symbol == ref_symbol){
				found = TRUE;
				break;
			    }
			}
			if(found == FALSE)
			    add_to_being_linked_list(symbol_name, ref_symbol,
						     ref_image);
		    }
		    else{
			add_to_undefined_list(symbol_name,ref_symbol,ref_image);
		    }
		}
		else{
		    add_to_undefined_list(symbol_name, symbols + i,
					  &object_image->image);
		}
	    }
	}

	/*
	 * Finally change this module's linked state.  If we are not fully
	 * binding marked this as being linked.  If we are fully binding and
	 * if the module is not linked again set it to being linked otherwise
	 * set it to fully linked.
	 */
	if(bind_now == FALSE)
	    object_image->module = BEING_LINKED;
	else{
	    link_state = object_image->module;

	    if(link_state != LINKED)
		object_image->module = BEING_LINKED;
	    else
		object_image->module = FULLY_LINKED;
	}
}

/*
 * unlink_object_module() create undefined symbols if any of the defined symbols
 * in the object_image are being referenced.  Also reset any lazy pointers for
 * defined symbols in the object_image if reset_lazy_references is TRUE.
 */
void
unlink_object_module(
struct object_image *object_image,
enum bool reset_lazy_references)
{
    unsigned long i;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols, *symbol;
    char *strings, *symbol_name;

    char *reference_name;
    struct nlist *referenced_symbol;
    module_state *referenced_module;
    struct image *referenced_image;
    struct library_image *referenced_library_image;

/*
printf("In unlink_object_module() image being unlinked: %s\n", object_image->image.name);
*/
	linkedit_segment = object_image->image.linkedit_segment;
	st = object_image->image.st;
	dyst = object_image->image.dyst;
	/*
	 * Object images could be loaded that do not have the proper
	 * link edit information.
	 */
	if(linkedit_segment == NULL || st == NULL || dyst == NULL)
	    return;

	symbols = (struct nlist *)
	    (object_image->image.vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	strings = (char *)
	    (object_image->image.vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->stroff -
	     linkedit_segment->fileoff);

	/*
	 * Walk through the list of defined symbols in this module and see
	 * if anything is referencing a defined symbol.  If so add this to
	 * the undefined symbol list.
	 */
	for(i = 0; i < dyst->nextdefsym; i++){
	    symbol = symbols + dyst->iextdefsym + i;
	    if(symbol->n_un.n_strx == 0)
		continue;
	    symbol_name = strings + symbol->n_un.n_strx;
/*
printf("defined symbol: %s\n", symbol_name);
*/

	    reference_name = look_for_reference(symbol_name, &referenced_symbol,
    				&referenced_module, &referenced_image,
				&referenced_library_image,
				reset_lazy_references);

	    if(reference_name != NULL){
/*
printf("found reference %s in referenced_image->name %s\n calling add_to_undefined_list()\n", reference_name, referenced_image->name);
*/
		add_to_undefined_list(reference_name, referenced_symbol,
				      referenced_image);
	    }

	    /*
	     * If the reset_lazy_references option is TRUE then reset the
	     * lazy pointers in all images for this symbol.
	     */
	    if(reset_lazy_references == TRUE){
		change_symbol_pointers_in_images(symbol_name,
				(unsigned long)&unlinked_lazy_pointer_handler,
/*
				0x3584,
*/
				TRUE /* only_lazy_pointers */);
	    }
	}
}

/*
 * lookup_symbol() looks up the symbol_name and sets pointers to the defintion
 * symbol, module and image.  If the symbol is not found the pointers are set
 * to NULL.  The last argument, indr_loop, should be NULL on initial calls to
 * lookup_symbol().
 */
void
lookup_symbol(
char *symbol_name,
struct nlist **defined_symbol,	/* the defined symbol */
module_state **defined_module,	/* the module the symbol is in */
struct image **defined_image,	/* the image the module is in */
struct library_image **defined_library_image,
struct indr_loop_list *indr_loop)
{
    unsigned long i;
    struct object_images *p;
    struct library_images *q;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols, *symbol;
    struct dylib_table_of_contents *tocs, *toc;
    struct indr_loop_list new_indr_loop, *loop, *l;
    char *strings, *module_name;
    struct dylib_module *dylib_modules, *dylib_module;
    unsigned long module_index;
    enum link_state link_state;

	/*
	 * First look in object_images for a definition of symbol_name.
	 */
	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		/* If this object file image is currently unused skip it */
		link_state = p->images[i].module;
		if(link_state == UNUSED)
		    continue;

		/* If the image is private skip it */
		if(p->images[i].image.private == TRUE)
		    continue;

		/* TODO: skip modules that that are replaced */

		linkedit_segment = p->images[i].image.linkedit_segment;
		st = p->images[i].image.st;
		dyst = p->images[i].image.dyst;
		/*
		 * Object images could be loaded that do not have the proper
		 * link edit information.
		 */
		if(linkedit_segment == NULL || st == NULL || dyst == NULL)
		    continue;
		symbols = (struct nlist *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		nlist_bsearch_strings = (char *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
		symbol = bsearch(symbol_name, symbols + dyst->iextdefsym,
			   dyst->nextdefsym, sizeof(struct nlist),
			   (int (*)(const void *, const void *))nlist_bsearch);
		if(symbol != NULL &&
		   (symbol->n_desc & N_DESC_DISCARDED) == 0){
		    if((symbol->n_type & N_TYPE) == N_INDR &&
			indr_loop != NO_INDR_LOOP){
			for(loop = indr_loop; loop != NULL; loop = loop->next){
			    if(loop->defined_symbol == symbol &&
			       loop->defined_module == &(p->images[i].module) &&
			       loop->defined_image == &(p->images[i].image) &&
			       loop->defined_library_image == NULL){
				error("indirect symbol loop:");
				for(l = indr_loop; l != NULL; l = l->next){
				    linkedit_segment =
					l->defined_image->linkedit_segment;
				    st = l->defined_image->st;
				    strings = (char *)
					(l->defined_image->vmaddr_slide +
					 linkedit_segment->vmaddr +
					 st->stroff -
					 linkedit_segment->fileoff);
				    if(l->defined_library_image != NULL){
					dyst = l->defined_image->dyst;
					dylib_modules =
					    (struct dylib_module *)
					    (l->defined_image->vmaddr_slide +
					    linkedit_segment->vmaddr +
					    dyst->modtaboff -
					    linkedit_segment->fileoff);
					module_index = l->defined_module -
					    l->defined_library_image->modules;
					dylib_module = dylib_modules +
						       module_index;
					module_name = strings +
					    dylib_module->module_name;
					add_error_string("%s(%s) definition of "
					    "%s as indirect for %s\n", 
					    l->defined_image->name, module_name,
					    strings +
						l->defined_symbol->n_un.n_strx,
					    strings +
						l->defined_symbol->n_value);
				    }
				    else{
					add_error_string("%s definition of "
					    "%s as indirect for %s\n", 
					    l->defined_image->name,
					    strings +
						l->defined_symbol->n_un.n_strx,
					    strings +
						l->defined_symbol->n_value);
				    }
				    if(l == loop)
					break;
				}
				link_edit_error(DYLD_OTHER_ERROR,
						DYLD_INDR_LOOP, NULL);
				*defined_symbol = NULL;
				*defined_module = NULL;
				*defined_image = NULL;
				*defined_library_image = NULL;
				return;
			    }
			}
			new_indr_loop.defined_symbol = symbol;
			new_indr_loop.defined_module = &(p->images[i].module);
			new_indr_loop.defined_image = &(p->images[i].image);
			new_indr_loop.defined_library_image = NULL;
			new_indr_loop.next = indr_loop;
			symbol_name = nlist_bsearch_strings +
				      symbol->n_value;
			lookup_symbol(symbol_name, defined_symbol,
				      defined_module, defined_image,
				      defined_library_image, &new_indr_loop);
			return;
		    }
		    else{
			*defined_symbol = symbol;
			*defined_module = &(p->images[i].module);
			*defined_image = &(p->images[i].image);
			*defined_library_image = NULL;
			return;
		    }
		}
	    }
	    p = p->next_images;
	}while(p != NULL);

	/*
	 * Next look in the library_images for a definition of symbol_name.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		linkedit_segment = q->images[i].image.linkedit_segment;
		st = q->images[i].image.st;
		dyst = q->images[i].image.dyst;

		tocs = (struct dylib_table_of_contents *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->tocoff -
		     linkedit_segment->fileoff);
		toc_bsearch_symbols = (struct nlist *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		toc_bsearch_strings = (char *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
		toc = bsearch(symbol_name, tocs, dyst->ntoc,
			      sizeof(struct dylib_table_of_contents),
			      (int (*)(const void *, const void *))toc_bsearch);
		/* TODO: skip modules that that are replaced */
		if(toc != NULL &&
		   (toc_bsearch_symbols[toc->symbol_index].n_desc &
		    N_DESC_DISCARDED) == 0){
		    symbol = toc_bsearch_symbols + toc->symbol_index;
		    if((symbol->n_type & N_TYPE) == N_INDR &&
			indr_loop != NO_INDR_LOOP){
			for(loop = indr_loop; loop != NULL; loop = loop->next){
			    if(loop->defined_symbol ==
				toc_bsearch_symbols + toc->symbol_index &&
			       loop->defined_module ==
				q->images[i].modules + toc->module_index &&
			       loop->defined_image == &(q->images[i].image) &&
			       loop->defined_library_image == &(q->images[i])){
				error("indirect symbol loop:");
				for(l = indr_loop; l != NULL; l = l->next){
				    linkedit_segment =
					l->defined_image->linkedit_segment;
				    st = l->defined_image->st;
				    strings = (char *)
					(l->defined_image->vmaddr_slide +
					 linkedit_segment->vmaddr +
					 st->stroff -
					 linkedit_segment->fileoff);
				    if(l->defined_library_image != NULL){
					dyst = l->defined_image->dyst;
					dylib_modules =
					    (struct dylib_module *)
					    (l->defined_image->vmaddr_slide +
					    linkedit_segment->vmaddr +
					    dyst->modtaboff -
					    linkedit_segment->fileoff);
					module_index = l->defined_module -
					    l->defined_library_image->modules;
					dylib_module = dylib_modules +
						       module_index;
					module_name = strings +
					    dylib_module->module_name;
					add_error_string("%s(%s) definition of "
					    "%s as indirect for %s\n", 
					    l->defined_image->name, module_name,
					    strings +
						l->defined_symbol->n_un.n_strx,
					    strings +
						l->defined_symbol->n_value);
				    }
				    else{
					add_error_string("%s definition of "
					    "%s as indirect for %s\n", 
					    l->defined_image->name,
					    strings +
						l->defined_symbol->n_un.n_strx,
					    strings +
						l->defined_symbol->n_value);
				    }
				    if(l == loop)
					break;
				}
				link_edit_error(DYLD_OTHER_ERROR,
						DYLD_INDR_LOOP, NULL);
				*defined_symbol = NULL;
				*defined_module = NULL;
				*defined_image = NULL;
				*defined_library_image = NULL;
				return;
			    }
			}
			new_indr_loop.defined_symbol =
			    toc_bsearch_symbols + toc->symbol_index;
			new_indr_loop.defined_module =
			    q->images[i].modules + toc->module_index;
			new_indr_loop.defined_image = &(q->images[i].image);
			new_indr_loop.defined_library_image = &(q->images[i]);
			new_indr_loop.next = indr_loop;
			symbol_name = toc_bsearch_strings +
				      symbol->n_value;
			lookup_symbol(symbol_name, defined_symbol,
				      defined_module, defined_image,
				      defined_library_image, &new_indr_loop);
			return;
		    }
		    else{
			*defined_symbol =
			    toc_bsearch_symbols + toc->symbol_index;
			*defined_module =
			    q->images[i].modules + toc->module_index;
			*defined_image = &(q->images[i].image);
			*defined_library_image = &(q->images[i]);
			return;
		    }
		}
	    }
	    q = q->next_images;
	}while(q != NULL);

	*defined_symbol = NULL;
	*defined_module = NULL;
	*defined_image = NULL;
	*defined_library_image = NULL;
}

/*
 * lookup_symbol_in_object_image() returns a pointer to the nlist structure in
 * this object_file image if it is defined.  Otherwise NULL.
 */
struct nlist *
lookup_symbol_in_object_image(
char *symbol_name,
struct object_image *object_image)
{
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols, *symbol;

	linkedit_segment = object_image->image.linkedit_segment;
	st = object_image->image.st;
	dyst = object_image->image.dyst;
	/*
	 * Object images could be loaded that do not have the proper
	 * link edit information.
	 */
	if(linkedit_segment == NULL || st == NULL || dyst == NULL)
	    return(NULL);
	symbols = (struct nlist *)
	    (object_image->image.vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	nlist_bsearch_strings = (char *)
	    (object_image->image.vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->stroff -
	     linkedit_segment->fileoff);
	symbol = bsearch(symbol_name, symbols + dyst->iextdefsym,
		   dyst->nextdefsym, sizeof(struct nlist),
		   (int (*)(const void *, const void *))nlist_bsearch);
	/*
	 * If we find a symbol that was descarded:
	 * (symbol->n_desc & N_DESC_DISCARDED) != 0
	 * by a multiply defined handler return it anyway.
	 * As the caller is looking it up in a specified module.
	 *
	 * TODO: What to do about INDR symbols.
	 */
	return(symbol);
}

/*
 * look_for_reference() looks up the symbol_name and sets pointers to the
 * reference symbol, module and image.  If no reference is found the pointers
 * are set to NULL.  If it a reference is found then the symbol_name in the
 * referencing image is returned else NULL.  If ignore_lazy_references is
 * TRUE then it returns a non-lazy reference or NULL if there are only
 * lazy references.
 */
static
char *
look_for_reference(
char *symbol_name,
struct nlist **referenced_symbol,	/* the referenced symbol */
module_state **referenced_module,	/* the module the symbol is in */
struct image **referenced_image,	/* the image the module is in */
struct library_image **referenced_library_image,
enum bool ignore_lazy_references)
{
    unsigned long i, j, k, isym;
    struct object_images *p;
    struct library_images *q;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols, *symbol;
    struct dylib_table_of_contents *tocs, *toc;
    struct dylib_module *dylib_modules;
    struct dylib_reference *dylib_references;
    char *reference_name;
    enum link_state link_state;

	/*
	 * First look in object_images for a reference of symbol_name.
	 */
	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		/* If this object file image is currently unused skip it */
		link_state = p->images[i].module;
		if(link_state == UNUSED)
		    continue;
		/* TODO: skip modules that that are replaced */
		linkedit_segment = p->images[i].image.linkedit_segment;
		st = p->images[i].image.st;
		dyst = p->images[i].image.dyst;
		/*
		 * Object images could be loaded that do not have the proper
		 * link edit information.
		 */
		if(linkedit_segment == NULL || st == NULL || dyst == NULL)
		    continue;
		symbols = (struct nlist *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		nlist_bsearch_strings = (char *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
		symbol = bsearch(symbol_name, symbols + dyst->iundefsym,
			   dyst->nundefsym, sizeof(struct nlist),
			   (int (*)(const void *, const void *))nlist_bsearch);
		if(symbol != NULL){
		    if(ignore_lazy_references == TRUE &&
		       (symbol->n_desc & REFERENCE_TYPE) ==
			REFERENCE_FLAG_UNDEFINED_LAZY)
			continue;
/*
printf("a reference to %s was found in %s\n",
symbol_name,
p->images[i].image.name);
*/
		    *referenced_symbol = symbol;
		    *referenced_module = &(p->images[i].module);
		    *referenced_image = &(p->images[i].image);
		    *referenced_library_image = NULL;
		    reference_name = nlist_bsearch_strings +
				     symbol->n_un.n_strx;
		    return(reference_name);
		}
	    }
	    p = p->next_images;
	}while(p != NULL);

/*
printf("looking in library_images for a reference of: %s\n", symbol_name);
*/
	/*
	 * Next look in the library_images for a reference of symbol_name.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		linkedit_segment = q->images[i].image.linkedit_segment;
		st = q->images[i].image.st;
		dyst = q->images[i].image.dyst;

		/*
		 * First find the symbol either in the table of contents
		 * (for defined symbols) or in the undefined symbols.
		 */
		tocs = (struct dylib_table_of_contents *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->tocoff -
		     linkedit_segment->fileoff);
		toc_bsearch_symbols = (struct nlist *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		toc_bsearch_strings = (char *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
/*
if(strcmp("libx.dylib", q->images[i].image.name) == 0)
printf("looking in toc of libx.dylib for %s\n",  symbol_name);
*/
		toc = bsearch(symbol_name, tocs, dyst->ntoc,
			      sizeof(struct dylib_table_of_contents),
			      (int (*)(const void *, const void *))toc_bsearch);
		if(toc != NULL){
		    /* TODO: could have N_INDR in the defined symbols */
		    symbol = toc_bsearch_symbols + toc->symbol_index;
		    isym = toc->symbol_index;
		}
		else{
/*
if(strcmp("libx.dylib", q->images[i].image.name) == 0)
printf("looking in undefs of libx.dylib for %s\n",  symbol_name);
*/
		    nlist_bsearch_strings = toc_bsearch_strings;
		    symbol = bsearch(symbol_name,
				     toc_bsearch_symbols + dyst->iundefsym,
				     dyst->nundefsym, sizeof(struct nlist),
			    (int (*)(const void *, const void *))nlist_bsearch);
		    if(symbol == NULL)
			continue;
		    isym = symbol - toc_bsearch_symbols;
		}
/*
printf("the symbol was found in %s\n", q->images[i].image.name);
*/
		reference_name = nlist_bsearch_strings + symbol->n_un.n_strx;
		dylib_modules = (struct dylib_module *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->modtaboff -
		     linkedit_segment->fileoff);
		dylib_references = (struct dylib_reference *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->extrefsymoff -
		     linkedit_segment->fileoff);
		/*
		 * Walk the module table and look at modules that are linked in.
		 */
		for(j = 0; j < dyst->nmodtab; j++){
		    if(q->images[i].modules[j] == BEING_LINKED ||
		       q->images[i].modules[j] == RELOCATED ||
		       q->images[i].modules[j] == REGISTERING ||
		       q->images[i].modules[j] == INITIALIZING ||
		       q->images[i].modules[j] == LINKED  ||
		       q->images[i].modules[j] == FULLY_LINKED){
/*
printf("module %ld is linked in\n",j);
*/
			/*
			 * The module is linked in look at the reference table
			 * for this module to see if we are making a reference
			 * from the module to this symbol.
			 */
			for(k = dylib_modules[j].irefsym;
			    k < dylib_modules[j].irefsym +
				dylib_modules[j].nrefsym;
			    k++){
			    if(dylib_references[k].isym == isym){
/*
printf("module %ld is has a reference\n",j);
*/
				if(dylib_references[k].flags ==
				   REFERENCE_FLAG_UNDEFINED_NON_LAZY ||
				   (dylib_references[k].flags ==
				    REFERENCE_FLAG_UNDEFINED_LAZY &&
				    ignore_lazy_references == FALSE)){
/*
printf("a reference to %swas found in %s(%s)\n", symbol_name,
q->images[i].image.name,
nlist_bsearch_strings + dylib_modules[j].module_name);
*/
					/* a reference is found return it */
					*referenced_symbol = symbol;
					*referenced_module =
					    q->images[i].modules + j;
					*referenced_image =
					    &(q->images[i].image);
					*referenced_library_image =
					    &(q->images[i]);
					return(reference_name);
				}
			    }
			}
		    }
		}
	    }
	    q = q->next_images;
	}while(q != NULL);

	*referenced_symbol = NULL;
	*referenced_module = NULL;
	*referenced_image = NULL;
	*referenced_library_image = NULL;
	return(NULL);
}

/*
 * nlist_bsearch() is used to search the symbol table of an object image that
 * is sorted by symbol name.  nlist_bsearch_strings must be set to a pointer to
 * the string table for the specified symbol table.
 */
static
int
nlist_bsearch(
const char *symbol_name,
const struct nlist *symbol)
{
	return(strcmp(symbol_name,
		      nlist_bsearch_strings + symbol->n_un.n_strx));
}

/*
 * toc_bsearch() is used to search the table of contents a library image that
 * is sorted by symbol name. toc_bsearch_strings and toc_bsearch_symbols must be
 * set to pointers to the string and symbol table for the specified table of
 * contents.
 */
static
int
toc_bsearch(
const char *symbol_name,
const struct dylib_table_of_contents *toc)
{
	return(strcmp(symbol_name,
		      toc_bsearch_strings +
		      toc_bsearch_symbols[toc->symbol_index].n_un.n_strx));
}

/*
 * validate_NSSymbol() is used by the functions that implement dyld library
 * functions that take NSSymbols as arguments.  It returns true if the symbol
 * is valid and fills in the other parameters for that symbol.  If not it
 * returns FALSE and sets the other parameters to NULL.
 */
enum bool
validate_NSSymbol(
struct nlist *symbol,		/* the symbol to validate */
module_state **defined_module,	/* the module the symbol is in */
struct image **defined_image,	/* the image the module is in */
struct library_image **defined_library_image)
{
    unsigned long i, j, isym;
    struct object_images *p;
    struct library_images *q;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols;
    struct dylib_module *dylib_modules;
    enum link_state link_state;

	*defined_module = NULL;
	*defined_image = NULL;
	*defined_library_image = NULL;

	/*
	 * First look in object_images for the symbol.
	 */
	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		/* If this object file image is currently unused skip it */
		link_state = p->images[i].module;
		if(link_state == UNUSED)
		    continue;
		linkedit_segment = p->images[i].image.linkedit_segment;
		st = p->images[i].image.st;
		dyst = p->images[i].image.dyst;
		/*
		 * Object images could be loaded that do not have the proper
		 * link edit information.
		 */
		if(linkedit_segment == NULL || st == NULL || dyst == NULL)
		    continue;
		symbols = (struct nlist *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		if(symbol >= symbols + dyst->iextdefsym &&
		   symbol < symbols + (dyst->iextdefsym + dyst->nextdefsym)){
		    if(p->images[i].module != INITIALIZING &&
		       p->images[i].module != LINKED &&
		       p->images[i].module != FULLY_LINKED)
			return(FALSE);
		    if((symbol->n_desc & N_DESC_DISCARDED) == N_DESC_DISCARDED)
			return(FALSE);

		    *defined_module = &(p->images[i].module);
		    *defined_image = &(p->images[i].image);
		    *defined_library_image = NULL;
		    return(TRUE);
		}
	    }
	    p = p->next_images;
	}while(p != NULL);

	/*
	 * Next look in the library_images for a definition of symbol_name.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		linkedit_segment = q->images[i].image.linkedit_segment;
		st = q->images[i].image.st;
		dyst = q->images[i].image.dyst;
		symbols = (struct nlist *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		if(symbol >= symbols + dyst->iextdefsym &&
		   symbol < symbols + (dyst->iextdefsym + dyst->nextdefsym)){
		    if((symbol->n_desc & N_DESC_DISCARDED) == N_DESC_DISCARDED)
			return(FALSE);
		    /*
		     * We found this symbol in this library image.  So now
		     * determine the module the symbol is in and if it is
		     * linked.
		     */
		    isym = symbol - symbols;
		    dylib_modules = (struct dylib_module *)
			(q->images[i].image.vmaddr_slide +
			 linkedit_segment->vmaddr +
			 dyst->modtaboff -
			 linkedit_segment->fileoff);
		    for(j = 0; j < dyst->nmodtab; j++){
			if(isym >= dylib_modules[j].iextdefsym &&
			   isym < dylib_modules[j].iextdefsym +
				  dylib_modules[j].nextdefsym)
			    break;
		    }
		    if(j >= dyst->nmodtab)
			return(FALSE);
		    if(q->images[i].modules[j] != INITIALIZING &&
		       q->images[i].modules[j] != LINKED &&
		       q->images[i].modules[j] != FULLY_LINKED)
			return(FALSE);

		    *defined_module = q->images[i].modules + j;
		    *defined_image = &(q->images[i].image);
		    *defined_library_image = q->images + i;
		    return(TRUE);
		}
	    }
	    q = q->next_images;
	}while(q != NULL);

	return(FALSE);
}

/*
 * validate_NSModule() is used by the functions that implement dyld library
 * functions that take NSmodules as arguments.  It returns true if the module
 * is valid and fills in the other parameters for that module.  If not it
 * returns FALSE and sets the other parameters to NULL.
 */
enum bool
validate_NSModule(
module_state *module,		/* the module to validate */
struct image **defined_image,	/* the image the module is in */
struct library_image **defined_library_image)
{
    unsigned long i;
    struct object_images *p;
    struct library_images *q;
    enum link_state link_state;

	*defined_image = NULL;
	*defined_library_image = NULL;

	/*
	 * First look in object_images for the module.
	 */
	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		/* If this object file image is currently unused skip it */
		link_state = p->images[i].module;
		if(link_state == UNUSED)
		    continue;
		if(&(p->images[i].module) == module){
		    *defined_image = &(p->images[i].image);
		    *defined_library_image = NULL;
		    return(TRUE);
		}
	    }
	    p = p->next_images;
	}while(p != NULL);

	/*
	 * Next look in the library_images for the module.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		if(module >= q->images[i].modules &&
		   module < q->images[i].modules + q->images[i].nmodules){
		    *defined_image = &(q->images[i].image);
		    *defined_library_image = q->images + i;
		    return(TRUE);
		}
	    }
	    q = q->next_images;
	}while(q != NULL);

	return(FALSE);
}

/*
 * relocate_symbol_pointers_in_object_image() looks up each symbol that is on
 * the being_linked list and if the object references the symbol cause any
 * symbol pointers in the image to be set to the value of the defined symbol.
 */
void
relocate_symbol_pointers_in_object_image(
struct image *image)
{
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols, *symbol;
    unsigned long *indirect_symtab;

    struct symbol_list *symbol_list;
    char *symbol_name, *strings;
    unsigned long value, symbol_index;

    struct nlist *defined_symbol;
    module_state *defined_module;
    struct image *defined_image;
    struct library_image *defined_library_image;


	linkedit_segment = image->linkedit_segment;
	st = image->st;
	dyst = image->dyst;
	/*
	 * Object images could be loaded that do not have the proper
	 * link edit information.
	 */
	if(linkedit_segment == NULL || st == NULL || dyst == NULL)
	    return;

	symbols = (struct nlist *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	strings = (char *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->stroff -
	     linkedit_segment->fileoff);
	indirect_symtab = (unsigned long *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     dyst->indirectsymoff -
	     linkedit_segment->fileoff);
	nlist_bsearch_strings = strings;
	/*
	 * For each symbol that is in the being linked list see if it is in the
	 * symbol table of the object (defined or undefined).
	 */
	for(symbol_list = being_linked_list.next;
	    symbol_list != &being_linked_list;
	    symbol_list = symbol_list->next){

	    symbol_name = symbol_list->name;
	    symbol = bsearch(symbol_name, symbols + dyst->iextdefsym,
		       dyst->nextdefsym, sizeof(struct nlist),
		       (int (*)(const void *, const void *))nlist_bsearch);
	    if(symbol == NULL){
		symbol = bsearch(symbol_name, symbols + dyst->iundefsym,
			   dyst->nundefsym, sizeof(struct nlist),
			   (int (*)(const void *, const void *))nlist_bsearch);
	    }
	    if(symbol == NULL)
		continue;
	    symbol_index = symbol - symbols;

	    if(symbol_list->symbol != NULL &&
	       (symbol_list->symbol->n_type & N_TYPE) == N_INDR){
		/* TODO: could this lookup fail? */
		lookup_symbol(symbol_name, &defined_symbol, &defined_module,
		      &defined_image, &defined_library_image, NULL);
		nlist_bsearch_strings = strings;

		value = defined_symbol->n_value;
		if((defined_symbol->n_type & N_TYPE) != N_ABS)
		    value += defined_image->vmaddr_slide;
#ifdef DYLD_PROFILING
		if(strcmp(symbol_name, "__exit") == 0)
		    value = (unsigned long)profiling_exit +
			    dyld_image_vmaddr_slide;
#endif
		relocate_symbol_pointers(symbol_index, indirect_symtab,
					 image, value, FALSE);
	    }
	    else{
		value = symbol_list->symbol->n_value;
		if((symbol_list->symbol->n_type & N_TYPE) != N_ABS)
		    value += symbol_list->image->vmaddr_slide;
#ifdef DYLD_PROFILING
		if(strcmp(symbol_name, "__exit") == 0)
		    value = (unsigned long)profiling_exit +
			    dyld_image_vmaddr_slide;
#endif
		relocate_symbol_pointers(symbol_index, indirect_symtab,
					 image, value, FALSE);
	    }
	}
}

/*
 * relocate_symbol_pointers_in_library_image() looks up each symbol that is on
 * the being_linked list and if the library references the symbol cause any
 * symbol pointers in the image to be set to the value of the defined symbol.
 */
void
relocate_symbol_pointers_in_library_image(
struct image *image)
{
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct dylib_table_of_contents *tocs, *toc;
    unsigned long *indirect_symtab;
    struct nlist *symbols, *symbol;
    char *strings;

    struct symbol_list *symbol_list;
    char *symbol_name;
    unsigned long value, symbol_index;

    struct nlist *defined_symbol;
    module_state *defined_module;
    struct image *defined_image;
    struct library_image *defined_library_image;


	linkedit_segment = image->linkedit_segment;
	st = image->st;
	dyst = image->dyst;

	tocs = (struct dylib_table_of_contents *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     dyst->tocoff -
	     linkedit_segment->fileoff);
	symbols = (struct nlist *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	strings = (char *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->stroff -
	     linkedit_segment->fileoff);
	indirect_symtab = (unsigned long *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     dyst->indirectsymoff -
	     linkedit_segment->fileoff);
	toc_bsearch_symbols = symbols;
	toc_bsearch_strings = strings;
	nlist_bsearch_strings = strings;
	/*
	 * For each symbol that is in the being linked list see if it is in the
	 * symbol table of the library (defined or undefined).
	 */
	for(symbol_list = being_linked_list.next;
	    symbol_list != &being_linked_list;
	    symbol_list = symbol_list->next){

	    symbol_name = symbol_list->name;
	    toc = bsearch(symbol_name, tocs, dyst->ntoc,
			  sizeof(struct dylib_table_of_contents),
			  (int (*)(const void *, const void *))toc_bsearch);
	    if(toc != NULL){
		symbol_index = toc->symbol_index;
	    }
	    else{
		symbol = bsearch(symbol_name, symbols + dyst->iundefsym,
			   dyst->nundefsym, sizeof(struct nlist),
			   (int (*)(const void *, const void *))nlist_bsearch);
		if(symbol == NULL)
		    continue;
		symbol_index = symbol - symbols;
	    }

	    if(symbol_list->symbol != NULL &&
	       (symbol_list->symbol->n_type & N_TYPE) == N_INDR){
		/* TODO: could this lookup fail? */
		lookup_symbol(symbol_name, &defined_symbol, &defined_module,
		      &defined_image, &defined_library_image, NULL);
		toc_bsearch_symbols = symbols;
		toc_bsearch_strings = strings;
		nlist_bsearch_strings = strings;

		value = defined_symbol->n_value;
		if((defined_symbol->n_type & N_TYPE) != N_ABS)
		    value += defined_image->vmaddr_slide;
#ifdef DYLD_PROFILING
		if(strcmp(symbol_name, "__exit") == 0)
		    value = (unsigned long)profiling_exit +
			    dyld_image_vmaddr_slide;
#endif
		relocate_symbol_pointers(symbol_index, indirect_symtab,
					 image, value, FALSE);
	    }
	    else{
		value = symbol_list->symbol->n_value;
		if((symbol_list->symbol->n_type & N_TYPE) != N_ABS)
		    value += symbol_list->image->vmaddr_slide;
#ifdef DYLD_PROFILING
		if(strcmp(symbol_name, "__exit") == 0)
		    value = (unsigned long)profiling_exit +
			    dyld_image_vmaddr_slide;
#endif
		relocate_symbol_pointers(symbol_index, indirect_symtab,
					 image, value, FALSE);
	    }
	}
}

/*
 * change_symbol_pointers_in_images() changes the symbol pointers in all the
 * images to the symbol_name to the new specified value.  This not used in
 * normal dynamic linking but used in response to things like multiply defined
 * symbol error handling and replacing of modules.  If only_lazy_pointers is
 * TRUE then only lazy symbol pointers are changed.
 */
void
change_symbol_pointers_in_images(
char *symbol_name,
unsigned long value,
enum bool only_lazy_pointers)
{
    unsigned long i;
    struct object_images *p;
    struct library_images *q;

    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct nlist *symbols, *symbol;
    unsigned long *indirect_symtab;
    unsigned long symbol_index;
    struct dylib_table_of_contents *tocs, *toc;
    char *strings;
    enum link_state link_state;

	/*
	 * First change the object_images that have been linked.
	 */
	p = &object_images;
	do{
	    for(i = 0; i < p->nimages; i++){
		/* If this object file image is currently unused skip it */
		link_state = p->images[i].module;
		if(link_state == UNUSED)
		    continue;

		linkedit_segment = p->images[i].image.linkedit_segment;
		st = p->images[i].image.st;
		dyst = p->images[i].image.dyst;
		/*
		 * Object images could be loaded that do not have the proper
		 * link edit information.
		 */
		if(linkedit_segment == NULL || st == NULL || dyst == NULL)
		    continue;
		symbols = (struct nlist *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		nlist_bsearch_strings = (char *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
		indirect_symtab = (unsigned long *)
		    (p->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->indirectsymoff -
		     linkedit_segment->fileoff);

		symbol = bsearch(symbol_name, symbols + dyst->iextdefsym,
			   dyst->nextdefsym, sizeof(struct nlist),
			   (int (*)(const void *, const void *))nlist_bsearch);
		if(symbol == NULL){
		    symbol = bsearch(symbol_name, symbols + dyst->iundefsym,
			       dyst->nundefsym, sizeof(struct nlist),
			    (int (*)(const void *, const void *))nlist_bsearch);
		}
		if(symbol == NULL)
		    continue;
		symbol_index = symbol - symbols;
#ifdef DYLD_PROFILING
		if(strcmp(symbol_name, "__exit") == 0)
		    value = (unsigned long)profiling_exit +
			    dyld_image_vmaddr_slide;
#endif
		relocate_symbol_pointers(symbol_index, indirect_symtab,
					 &(p->images[i].image), value,
					 only_lazy_pointers);
	    }
	    p = p->next_images;
	}while(p != NULL);

	/*
	 * Next change the library_images.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){

		linkedit_segment = q->images[i].image.linkedit_segment;
		st = q->images[i].image.st;
		dyst = q->images[i].image.dyst;

		tocs = (struct dylib_table_of_contents *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->tocoff -
		     linkedit_segment->fileoff);
		symbols = (struct nlist *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		strings = (char *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
		indirect_symtab = (unsigned long *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->indirectsymoff -
		     linkedit_segment->fileoff);
		toc_bsearch_symbols = symbols;
		toc_bsearch_strings = strings;
		nlist_bsearch_strings = strings;

		toc = bsearch(symbol_name, tocs, dyst->ntoc,
			      sizeof(struct dylib_table_of_contents),
			      (int (*)(const void *, const void *))toc_bsearch);
		if(toc != NULL){
		    symbol_index = toc->symbol_index;
		}
		else{
		    symbol = bsearch(symbol_name, symbols + dyst->iundefsym,
			       dyst->nundefsym, sizeof(struct nlist),
			    (int (*)(const void *, const void *))nlist_bsearch);
		    if(symbol == NULL)
			continue;
		    symbol_index = symbol - symbols;
		}
#ifdef DYLD_PROFILING
		if(strcmp(symbol_name, "__exit") == 0)
		    value = (unsigned long)profiling_exit +
			    dyld_image_vmaddr_slide;
#endif
		relocate_symbol_pointers(symbol_index, indirect_symtab,
					 &(q->images[i].image), value,
					 only_lazy_pointers);
	    }
	    q = q->next_images;
	}while(q != NULL);
}

/*
 * relocate_symbol_pointers() sets the value of the defined symbol for
 * symbol_name into the symbol pointers of the image which have the specified
 * symbol_index.
 */
static
void
relocate_symbol_pointers(
unsigned long symbol_index,
unsigned long *indirect_symtab,
struct image *image,
unsigned long value,
enum bool only_lazy_pointers)
{
    unsigned long i, j, k, section_type;
    struct load_command *lc;
    struct segment_command *sg;
    struct section *s;

/*
printf("In relocate_symbol_pointers only_lazy_pointers = %d\n",
only_lazy_pointers);
*/
#ifdef __ppc__
	if(only_lazy_pointers == TRUE)
	    value = image->dyld_stub_binding_helper;
#endif

	/*
	 * A symbol being linked is in this image at the symbol_index. Walk
	 * the headers looking for indirect sections.  Then for each indirect
	 * section scan the indirect table entries for this symbol_index and
	 * if found set the value of the defined symbol into the pointer.
	 */
	lc = (struct load_command *)((char *)image->mh +
				     sizeof(struct mach_header));
	for(i = 0; i < image->mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0 ; j < sg->nsects ; j++){
		    section_type = s->flags & SECTION_TYPE;
		    if((section_type == S_NON_LAZY_SYMBOL_POINTERS &&
			only_lazy_pointers == FALSE) ||
		       section_type == S_LAZY_SYMBOL_POINTERS){
			for(k = 0; k < s->size / sizeof(unsigned long); k++){
			    if(indirect_symtab[s->reserved1 + k] == 
			       symbol_index){
				*((long *)(image->vmaddr_slide +
					   s->addr + (k * sizeof(long)))) =
					value;
			    }
			}
		    }
		    s++;
		}
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * relocate_symbol_pointers_for_defined_externs() adjust the value of the
 * the non-lazy symbol pointers of the image which are for symbols that are
 * defined externals.
 */
void
relocate_symbol_pointers_for_defined_externs(
struct image *image)
{
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    unsigned long *indirect_symtab;
    struct nlist *symbols;

    unsigned long index;
    unsigned long i, j, k, section_type;
    struct load_command *lc;
    struct segment_command *sg;
    struct section *s;

	linkedit_segment = image->linkedit_segment;
	st = image->st;
	dyst = image->dyst;

	symbols = (struct nlist *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	indirect_symtab = (unsigned long *)
	    (image->vmaddr_slide +
	     linkedit_segment->vmaddr +
	     dyst->indirectsymoff -
	     linkedit_segment->fileoff);

	/*
	 * Loop through the S_NON_LAZY_SYMBOL_POINTERS sections and adjust the
	 * value of all pointers that are for defined externals by the
	 * vmaddr_slide.
	 */
	lc = (struct load_command *)((char *)image->mh +
				     sizeof(struct mach_header));
	for(i = 0; i < image->mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0 ; j < sg->nsects ; j++){
		    section_type = s->flags & SECTION_TYPE;
		    if(section_type == S_NON_LAZY_SYMBOL_POINTERS){
			for(k = 0; k < s->size / sizeof(unsigned long); k++){
			    index = indirect_symtab[s->reserved1 + k];
			    if(index == INDIRECT_SYMBOL_ABS)
				continue;
			    if(index == INDIRECT_SYMBOL_LOCAL ||
			       ((symbols[index].n_type & N_TYPE) != N_UNDF &&
			        (symbols[index].n_type & N_TYPE) != N_ABS)){
				*((long *)(image->vmaddr_slide + s->addr +
				    (k * sizeof(long)))) += image->vmaddr_slide;
			    }
			}
		    }
		    s++;
		}
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * bind_lazy_symbol_reference() is transfered to to bind a lazy reference the
 * first time it is called.  The symbol being referenced is the indirect symbol
 * for the lazy_symbol_pointer_address specified in the image with the specified
 * mach header.
 */
unsigned long
bind_lazy_symbol_reference(
struct mach_header *mh,
unsigned long lazy_symbol_pointer_address)
{
    struct object_images *p;
    struct library_images *q;
    struct image *image;

    unsigned long i, j, section_type, isym;
    struct load_command *lc;
    struct segment_command *sg;
    struct section *s;
    struct nlist *symbols, *symbol;
    unsigned long *indirect_symtab;

    char *symbol_name;
    struct nlist *defined_symbol;
    module_state *defined_module;
    struct image *defined_image;
    struct library_image *defined_library_image;
    enum link_state link_state;
    unsigned long value;

    struct library_image *library_image;
    struct dylib_module *dylib_modules;

	/* set lock for dyld data structures */
	set_lock();

	/*
	 * Figure out which image this mach header is for.
	 */
	image = NULL;
	/*
	 * First look in object_images for this mach_header.
	 */
	p = &object_images;
	do{
	    for(i = 0; image == NULL && i < p->nimages; i++){
		if(p->images[i].image.mh == mh)
		    image = &(p->images[i].image);
	    }
	    p = p->next_images;
	}while(image == NULL && p != NULL);
	library_image = NULL;
	if(image == NULL){
	    /*
	     * Next look in the library_images for this mach_header.
	     */
	    q = &library_images;
	    do{
		for(i = 0; image == NULL && i < q->nimages; i++){
		    if(q->images[i].image.mh == mh){
			image = &(q->images[i].image);
			library_image = q->images + i;
		    }
		}
		q = q->next_images;
	    }while(image == NULL && q != NULL);
	}
	if(image == NULL){
	    error("bad mach header passed to stub_binding_helper");
	    link_edit_error(DYLD_OTHER_ERROR, DYLD_LAZY_BIND, NULL);
	    exit(DYLD_EXIT_FAILURE_BASE + DYLD_OTHER_ERROR);
	}

	/*
	 * Now determine which symbol is being refered to.
	 */
	symbols = (struct nlist *)
	    (image->vmaddr_slide +
	     image->linkedit_segment->vmaddr +
	     image->st->symoff -
	     image->linkedit_segment->fileoff);
	indirect_symtab = (unsigned long *)
	    (image->vmaddr_slide +
	     image->linkedit_segment->vmaddr +
	     image->dyst->indirectsymoff -
	     image->linkedit_segment->fileoff);
	symbol = NULL;
	isym = 0;
	lc = (struct load_command *)((char *)image->mh +
				     sizeof(struct mach_header));
	for(i = 0; symbol == NULL && i < image->mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0; symbol == NULL && j < sg->nsects; j++){
		    section_type = s->flags & SECTION_TYPE;
		    if(section_type == S_LAZY_SYMBOL_POINTERS){
			if(lazy_symbol_pointer_address >=
			       s->addr + image->vmaddr_slide &&
			   lazy_symbol_pointer_address <
			       s->addr + image->vmaddr_slide + s->size){
			    isym = indirect_symtab[s->reserved1 +
				    (lazy_symbol_pointer_address - (s->addr +
				     image->vmaddr_slide)) /
				    sizeof(unsigned long)];
			    symbol = symbols + isym;
			}
		    }
		    s++;
		}
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	if(symbol == NULL){
	    error("bad address of lazy symbol pointer passed to "
		  "stub_binding_helper");
	    link_edit_error(DYLD_OTHER_ERROR, DYLD_LAZY_BIND, NULL);
	    exit(DYLD_EXIT_FAILURE_BASE + DYLD_OTHER_ERROR);
	}

	/*
	 * If this symbol pointer is for a private extern symbol that is no
	 * longer external then use it's value.
	 */
	if((symbol->n_type & N_PEXT) == N_PEXT &&
	   (symbol->n_type & N_EXT) != N_EXT){
	    /*
	     * If this is from a library make sure the module that defines this
	     * symbol is linked in.
	     */
	    if(library_image != NULL){
		dylib_modules = (struct dylib_module *)
		    (image->vmaddr_slide +
		     image->linkedit_segment->vmaddr +
		     image->dyst->modtaboff -
		     image->linkedit_segment->fileoff);
		for(j = 0; j < image->dyst->nmodtab; j++){
		    if(isym >= dylib_modules[j].ilocalsym &&
		       isym < dylib_modules[j].ilocalsym +
			      dylib_modules[j].nlocalsym)
			break;
		}
		if(j < image->dyst->nmodtab){
		    link_state = library_image->modules[j];
		    /*
		     * If this module is unlinked cause it to be linked in.
		     * Note that this is for a module in a library that defineds
		     * a private external, therefore it can't have any external
		     * references and undo_external_relocation() is not called
		     * if the state is prebound unlinked.
		     */
		    if(link_state == PREBOUND_UNLINKED ||
		       link_state == UNLINKED){
			library_image->modules[j] = BEING_LINKED;
			link_in_need_modules(FALSE, FALSE);
		    }
		    value = symbol->n_value;
		    if((symbol->n_type & N_TYPE) != N_ABS)
			value += image->vmaddr_slide;
		    *((long *)lazy_symbol_pointer_address) = value;
		    /* release lock for dyld data structures */
		    release_lock();
		    return(value);
		}
	    }
	    else {
		value = symbol->n_value;
		if((symbol->n_type & N_TYPE) != N_ABS)
		    value += image->vmaddr_slide;
		*((long *)lazy_symbol_pointer_address) = value;
		/* release lock for dyld data structures */
		release_lock();
		return(value);
	    }
	}

	/*
	 * Now that we have the image and the symbol lookup this symbol and see
	 * if it is in a linked module.  If so just set the value of the symbol
	 * into the lazy symbol pointer and return that value.
	 */
	symbol_name = (char *)
	    (image->vmaddr_slide +
	     image->linkedit_segment->vmaddr +
	     image->st->stroff -
	     image->linkedit_segment->fileoff) +
	    symbol->n_un.n_strx;
	lookup_symbol(symbol_name, &defined_symbol, &defined_module,
		      &defined_image, &defined_library_image, NULL);
	if(defined_symbol != NULL){
	    link_state = *defined_module;
	    if(link_state == LINKED || link_state == FULLY_LINKED){
		value = defined_symbol->n_value;
		if((defined_symbol->n_type & N_TYPE) != N_ABS)
		    value += defined_image->vmaddr_slide;
#ifdef DYLD_PROFILING
		if(strcmp(symbol_name, "__exit") == 0)
		    value = (unsigned long)profiling_exit +
			    dyld_image_vmaddr_slide;
#endif
		*((long *)lazy_symbol_pointer_address) = value;
		/* release lock for dyld data structures */
		release_lock();
		return(value);
	    }
	}

	if(dyld_prebind_debug > 1)
	    print("dyld: %s: linking in non-prebound lazy symbol: %s\n",
		   executables_name, symbol_name);

	/*
	 * This symbol is not defined in a linked module.  So put it on the
	 * undefined list and link in the needed modules.
	 */
	add_to_undefined_list(symbol_name, symbol, image);
	link_in_need_modules(FALSE, FALSE);

	/*
	 * Now that all the needed module are linked in there can't be any
	 * undefineds left.  Therefore the lookup_symbol can't fail.
	 */
	lookup_symbol(symbol_name, &defined_symbol, &defined_module,
		      &defined_image, &defined_library_image, NULL);
	value = defined_symbol->n_value;
	if((defined_symbol->n_type & N_TYPE) != N_ABS)
	    value += defined_image->vmaddr_slide;
#ifdef DYLD_PROFILING
	if(strcmp(symbol_name, "__exit") == 0)
	    value = (unsigned long)profiling_exit + dyld_image_vmaddr_slide;
#endif
	/* release lock for dyld data structures */
	release_lock();
	return(value);
}

void
bind_symbol_by_name(
char *symbol_name,
unsigned long *address,
module_state **module,
struct nlist **symbol,
enum bool change_symbol_pointers)
{
    struct nlist *defined_symbol;
    module_state *defined_module;
    struct image *defined_image;
    struct library_image *defined_library_image;
    enum link_state link_state;
    unsigned long value;

	/* set lock for dyld data structures */
	set_lock();

	lookup_symbol(symbol_name, &defined_symbol, &defined_module,
		      &defined_image, &defined_library_image, NULL);
	if(defined_symbol != NULL){
	    link_state = *defined_module;
	    if(link_state == LINKED || link_state == FULLY_LINKED){
		value = defined_symbol->n_value;
		if((defined_symbol->n_type & N_TYPE) != N_ABS)
		    value += defined_image->vmaddr_slide;
		if(change_symbol_pointers == TRUE)
		    change_symbol_pointers_in_images(symbol_name, value, FALSE);
		if(address != NULL)
		    *address = value;
		if(module != NULL)
		    *module = defined_module;
		if(symbol != NULL)
		    *symbol = defined_symbol;

		/* release lock for dyld data structures */
		release_lock();
		return;
	    }
	}

	if(dyld_prebind_debug > 1)
	    print("dyld: %s: linking in bind by name symbol: %s\n",
		   executables_name, symbol_name);

	/*
	 * This symbol is not defined in a linked module.  So put it on the
	 * undefined list and link in the needed modules.
	 */
	add_to_undefined_list(symbol_name, NULL, NULL);
	link_in_need_modules(FALSE, FALSE);

	/*
	 * Now that all the needed module are linked in there can't be any
	 * undefineds left.  Therefore the lookup_symbol can't fail.
	 */
	lookup_symbol(symbol_name, &defined_symbol, &defined_module,
		      &defined_image, &defined_library_image, NULL);
	value = defined_symbol->n_value;
	if((defined_symbol->n_type & N_TYPE) != N_ABS)
	    value += defined_image->vmaddr_slide;
	if(address != NULL)
	    *address = value;
	if(module != NULL)
	    *module = defined_module;
	if(symbol != NULL)
	    *symbol = defined_symbol;
	/* release lock for dyld data structures */
	release_lock();
}

/*
 * link_in_need_modules() causes any needed modules to be linked into the
 * program.  This causes resolution of undefined symbols, relocation modules
 * that got linked in, checking for and reporting of undefined symbols and
 * module initialization routines to be called.  bind_now is TRUE only when
 * fully binding an image, the normal case when lazy binding is to occur it
 * is FALSE.
 *
 * Before being called the lock for the dyld data structures must be set and
 * either:
 * 	1) symbols placed on the undefined list or
 *	2) a module's state is set to BEING_LINKED and it's non-lazy symbols
 * 	   have been placed on the undefined list (if bind_now then it's lazy
 *	   symbols have also been placed on the undefined list).
 */
void
link_in_need_modules(
enum bool bind_now,
enum bool release_lock_flag)
{
	/*
	 * Load the dependent libraries.
	 */
	load_dependent_libraries();

	/*
	 * Now resolve all non-lazy symbol references this program initially
	 * has so it can be started.
	 */
	resolve_undefineds(bind_now, FALSE);

	/*
	 * Now do all the relocation of modules being linked to that resolved
	 * undefined symbols.
	 */
	relocate_modules_being_linked(FALSE);

	/*
	 * Now check and report any non-lazy symbols that are still undefined.
	 */
	check_and_report_undefineds();

	/*
	 * Now call the functions that were registered to be called when an
	 * image gets added.
	 */
	call_registered_funcs_for_add_images();

	/*
	 * Now call the functions that were registered to be called when a
	 * module gets linked.
	 */
	call_registered_funcs_for_linked_modules();

	/*
	 * Now call module initialization routines for modules that have been
	 * linked in.
	 */
	call_module_initializers(FALSE, bind_now);

	if(release_lock_flag){
	    /* release lock for dyld data structures */
	    release_lock();
	}
}

/*
 * check_executable_for_overrides() is called when trying to launch using just
 * prebound libraries and not having a prebound executable.  This checks to see
 * that the executable does not define a symbol that is defined and referenced
 * by a library it uses.  If there are no such symbols the executable can be
 * launch using just prebound libraries and TRUE is returned.  Else FALSE is
 * returned.
 */
enum bool
check_executable_for_overrides(
void)
{
    unsigned long i;
    char *symbol_name;
    struct nlist *symbols;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct image *image;

	/*
	 * The executable is the first object on the object_image list.
	 */
	linkedit_segment = object_images.images[0].image.linkedit_segment;
	st = object_images.images[0].image.st;
	dyst = object_images.images[0].image.dyst;
	if(linkedit_segment == NULL || st == NULL || dyst == NULL)
	    return(TRUE);
	/* the vmaddr_slide of an executable is always 0, no need to add it */
	symbols = (struct nlist *)
	    (linkedit_segment->vmaddr +
	     st->symoff -
	     linkedit_segment->fileoff);
	image = &object_images.images[0].image;

	for(i = dyst->iextdefsym; i < dyst->iextdefsym + dyst->nextdefsym; i++){
	    symbol_name = (char *)
		(image->linkedit_segment->vmaddr +
		 image->st->stroff -
		 image->linkedit_segment->fileoff) +
		symbols[i].n_un.n_strx;
	    if(check_libraries_for_definition_and_refernce(symbol_name) == TRUE)
		return(FALSE);
	}
	return(TRUE);
}

/*
 * check_libraries_for_definition_and_refernce() is used to check that the
 * symbol name is defined and referenced in the same library for all the
 * libraries being used.  If there is such a library TRUE is returned else
 * FALSE is returned.
 */
static
enum bool
check_libraries_for_definition_and_refernce(
char *symbol_name)
{
    unsigned long i, j;
    struct library_images *q;
    struct segment_command *linkedit_segment;
    struct symtab_command *st;
    struct dysymtab_command *dyst;
    struct dylib_table_of_contents *tocs, *toc;
    struct dylib_reference *dylib_references;

	/*
	 * Look in the library_images for a definition of symbol_name.
	 */
	q = &library_images;
	do{
	    for(i = 0; i < q->nimages; i++){
		linkedit_segment = q->images[i].image.linkedit_segment;
		st = q->images[i].image.st;
		dyst = q->images[i].image.dyst;

		tocs = (struct dylib_table_of_contents *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     dyst->tocoff -
		     linkedit_segment->fileoff);
		toc_bsearch_symbols = (struct nlist *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->symoff -
		     linkedit_segment->fileoff);
		toc_bsearch_strings = (char *)
		    (q->images[i].image.vmaddr_slide +
		     linkedit_segment->vmaddr +
		     st->stroff -
		     linkedit_segment->fileoff);
		toc = bsearch(symbol_name, tocs, dyst->ntoc,
			      sizeof(struct dylib_table_of_contents),
			      (int (*)(const void *, const void *))toc_bsearch);
		if(toc != NULL){
		    /*
		     * This library has a definition of this symbol see if it
		     * also has a reference.
		     */
		    dylib_references = (struct dylib_reference *)
			(q->images[i].image.vmaddr_slide +
			 linkedit_segment->vmaddr +
			 dyst->extrefsymoff -
			 linkedit_segment->fileoff);
		    for(j = 0; j < dyst->nextrefsyms; j++){
			if(dylib_references[j].isym == toc->symbol_index &&
			   dylib_references[j].flags != REFERENCE_FLAG_DEFINED){
			    if(dyld_prebind_debug != 0 && prebinding == TRUE)
				print("dyld: %s: trying to use prebound "
				    "libraries failed due to overridden "
				    "symbols\n", executables_name);
			    prebinding = FALSE;
			    return(TRUE);
			}
		    }
		}
	    }
	    q = q->next_images;
	}while(q != NULL);
	return(FALSE);
}
