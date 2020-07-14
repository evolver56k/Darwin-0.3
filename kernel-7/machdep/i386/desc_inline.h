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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Descriptor manipulation.
 *
 * HISTORY
 *
 * 1 April 1992 ? at NeXT
 *	Created.
 */

#import <mach/vm_param.h>	/* for round_page() */
 
#import <architecture/i386/table.h>

/*
 * Convert a segment size
 * into a page-granular limit.
 */
#define page_limit(size)	\
    (i386_round_page(size) - I386_PGBYTES)
    
/*
 * Convert a segment size
 * into a byte-granular limit.
 */
#define byte_limit(size)	\
    ((size) - 1)

/*
 * These types allow the
 * breaking up of base and
 * limit values so that the
 * descriptors can be filled
 * in easily.
 */
    
typedef union {
    unsigned int	word;
    struct {
	unsigned int		base00	:16,
				base16	:8,
				base24	:8;
    } 			fields;
} conv_base_t;

typedef union {
    unsigned int	word;
    struct {
	unsigned int		limit00	:16,
				limit16	:4,
					:12;
    }			fields_byte;
    struct {
	unsigned int			:12,
	    			limit00	:16,
	    			limit16	:4;
    }			fields_page;
} conv_limit_t;

/*
 * Initialize a descriptor
 * to map an LDT segment.
 */

static inline
void
map_ldt(desc, base, size)
ldt_desc_t *	desc;
vm_offset_t	base;
vm_size_t	size;
{
    conv_base_t		tbase;
    conv_limit_t	tlimit;
    
    tbase.word = base;
    tlimit.word = byte_limit(size);

    desc->base00	= tbase.fields.base00;
    desc->base16	= tbase.fields.base16;
    desc->base24	= tbase.fields.base24;
    desc->type		= DESC_LDT;
    desc->present	= TRUE;
    desc->granular	= DESC_GRAN_BYTE;
    desc->limit00	= tlimit.fields_byte.limit00;
    desc->limit16	= tlimit.fields_byte.limit16;
}

/*
 * Initialize a descriptor
 * to map a TSS.
 */

static inline
void
map_tss(desc, base, size)
tss_desc_t *	desc;
vm_offset_t	base;
vm_size_t	size;
{
    conv_base_t		tbase;
    conv_limit_t	tlimit;
    
    tbase.word = base;
    tlimit.word = byte_limit(size);

    desc->base00	= tbase.fields.base00;
    desc->base16	= tbase.fields.base16;
    desc->base24	= tbase.fields.base24;
    desc->type		= DESC_TSS;
    desc->dpl		= KERN_PRIV;
    desc->present	= TRUE;
    desc->granular	= DESC_GRAN_BYTE;
    desc->limit00	= tlimit.fields_byte.limit00;
    desc->limit16	= tlimit.fields_byte.limit16;
}

/*
 * Initialize a descriptor
 * to map a code segment.
 */

static inline
void
map_code(desc, base, size, priv, exec_only)
code_desc_t *	desc;
vm_offset_t	base;
vm_size_t	size;
int		priv;
boolean_t	exec_only;
{
    conv_base_t		tbase;
    conv_limit_t	tlimit;

    tbase.word = base;

    desc->base00	= tbase.fields.base00;
    desc->base16	= tbase.fields.base16;
    desc->base24	= tbase.fields.base24;
    desc->type		= exec_only? DESC_CODE_EXEC: DESC_CODE_READ;
    desc->dpl		= priv;
    desc->present	= TRUE;
    desc->opsz		= DESC_CODE_32B;

    if (byte_limit(size) < (1 << 20)) {		/* 2 ^ 20 or 1MB */
	tlimit.word = byte_limit(size);

	desc->granular	= DESC_GRAN_BYTE;
	desc->limit00	= tlimit.fields_byte.limit00;
	desc->limit16	= tlimit.fields_byte.limit16;
    }
    else {
	tlimit.word = page_limit(size);

	desc->granular	= DESC_GRAN_PAGE;
	desc->limit00	= tlimit.fields_page.limit00;
	desc->limit16	= tlimit.fields_page.limit16;
    }
}

/*
 * Initialize a descriptor
 * to map a 16-bit code segment.
 */

static inline
void
map_code_16(desc, base, size, priv, exec_only)
code_desc_t *	desc;
vm_offset_t	base;
vm_size_t	size;
int		priv;
boolean_t	exec_only;
{
    conv_base_t		tbase;
    conv_limit_t	tlimit;

    tbase.word = base;

    desc->base00	= tbase.fields.base00;
    desc->base16	= tbase.fields.base16;
    desc->base24	= tbase.fields.base24;
    desc->type		= exec_only? DESC_CODE_EXEC: DESC_CODE_READ;
    desc->dpl		= priv;
    desc->present	= TRUE;
    desc->opsz		= DESC_CODE_16B;

    if (byte_limit(size) < (1 << 20)) {		/* 2 ^ 20 or 1MB */
	tlimit.word = byte_limit(size);

	desc->granular	= DESC_GRAN_BYTE;
	desc->limit00	= tlimit.fields_byte.limit00;
	desc->limit16	= tlimit.fields_byte.limit16;
    }
    else {
	tlimit.word = page_limit(size);

	desc->granular	= DESC_GRAN_PAGE;
	desc->limit00	= tlimit.fields_page.limit00;
	desc->limit16	= tlimit.fields_page.limit16;
    }
}

/*
 * Initialize a descriptor
 * to map a data segment.
 */

static inline
void
map_data(desc, base, size, priv, read_only)
data_desc_t *	desc;
vm_offset_t	base;
vm_size_t	size;
int		priv;
boolean_t	read_only;
{
    conv_base_t		tbase;
    conv_limit_t	tlimit;
    
    tbase.word = base;

    desc->base00	= tbase.fields.base00;
    desc->base16	= tbase.fields.base16;
    desc->base24	= tbase.fields.base24;
    desc->type		= read_only? DESC_DATA_RONLY: DESC_DATA_WRITE;
    desc->dpl		= priv;
    desc->present	= TRUE;
    desc->stksz		= DESC_DATA_32B;

    if (byte_limit(size) < (1 << 20)) {		/* 2 ^ 20 or 1MB */
	tlimit.word = byte_limit(size);

	desc->granular	= DESC_GRAN_BYTE;
	desc->limit00	= tlimit.fields_byte.limit00;
	desc->limit16	= tlimit.fields_byte.limit16;
    }
    else {
	tlimit.word = page_limit(size);

	desc->granular	= DESC_GRAN_PAGE;
	desc->limit00	= tlimit.fields_page.limit00;
	desc->limit16	= tlimit.fields_page.limit16;
    }
}

/*
 * Change a descriptor to
 * map a different LDT.  Only
 * change the segment base.
 */

static inline
void
remap_ldt(desc, base)
ldt_desc_t *	desc;
vm_offset_t	base;
{
    conv_base_t		tbase;
    
    tbase.word = base;

    desc->base00	= tbase.fields.base00;
    desc->base16	= tbase.fields.base16;
    desc->base24	= tbase.fields.base24;
}

/*
 * Change a descriptor to
 * map a different TSS.  Only
 * change the segment base and
 * reset the type field (busy bit).
 */

static inline
void
remap_tss(desc, base)
tss_desc_t *	desc;
vm_offset_t	base;
{
    conv_base_t		tbase;
    
    tbase.word = base;

    desc->base00	= tbase.fields.base00;
    desc->base16	= tbase.fields.base16;
    desc->base24	= tbase.fields.base24;
    desc->type		= DESC_TSS;
}

/*
 * This type allows the
 * breaking up of the offset
 * value so that the gate
 * can be filled in easily.
 */
    
typedef union {
    unsigned int	word;
    struct {
	unsigned int		offset00:16,
				offset16:16;
    } 			fields;
} conv_offset_t;

/*
 * Initialize a call gate.
 */

static inline
void
make_call_gate(desc, seg, offset, priv, argcnt)
call_gate_t *	desc;
sel_t		seg;
vm_offset_t	offset;
int		priv, argcnt;
{
    conv_offset_t	toffset;
    
    toffset.word = offset;
    
    desc->offset00	= toffset.fields.offset00;
    desc->offset16	= toffset.fields.offset16;
    desc->seg		= seg;
    desc->argcnt	= argcnt;
    desc->type		= DESC_CALL_GATE;
    desc->dpl		= priv;
    desc->present	= TRUE;
}

/*
 * Initialize a trap gate.
 */

static inline
void
make_trap_gate(desc, seg, offset, priv)
trap_gate_t *	desc;
sel_t		seg;
vm_offset_t	offset;
int		priv;
{
    conv_offset_t	toffset;
    
    toffset.word = offset;
    
    desc->offset00	= toffset.fields.offset00;
    desc->offset16	= toffset.fields.offset16;
    desc->seg		= seg;
    desc->type		= DESC_TRAP_GATE;
    desc->dpl		= priv;
    desc->present	= TRUE;
}

/*
 * Initialize an interrupt gate.
 */

static inline
void
make_intr_gate(desc, seg, offset, priv)
intr_gate_t *	desc;
sel_t		seg;
vm_offset_t	offset;
int		priv;
{
    conv_offset_t	toffset;
    
    toffset.word = offset;
    
    desc->offset00	= toffset.fields.offset00;
    desc->offset16	= toffset.fields.offset16;
    desc->seg		= seg;
    desc->type		= DESC_INTR_GATE;
    desc->dpl		= priv;
    desc->present	= TRUE;
}

/*
 * Initialize a task gate.
 */

static inline
void
make_task_gate(desc, tss, priv)
task_gate_t *	desc;
sel_t		tss;
int		priv;
{
    desc->tss		= tss;
    desc->type		= DESC_TASK_GATE;
    desc->dpl		= priv;
    desc->present	= TRUE;
}
