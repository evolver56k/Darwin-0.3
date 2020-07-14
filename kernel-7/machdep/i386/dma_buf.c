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
 * dma_buf.c  -- DMA buffer handling.
 *
 * HISTORY
 *
 * 26 August 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <machdep/i386/dma_buf_internal.h>
#import <machdep/i386/dma_exported.h>

dma_buf_type_t		dma_buf_sm, dma_buf_lg;

static inline int is_aligned( void *addr, unsigned int alignment )
{
    return (((unsigned int)addr & (alignment - 1)) == 0);
}
void
dma_buf_initialize(
    void
)
{
    int maxbufs = DMA_BUF_LG_LEN / DMA_BUF_SM_LEN + 1;
    dma_buf_t buf, bufs[maxbufs];
    int nbufs = 0;
    
    dma_buf_sm.create_fcn = dma_buf_sm_create;
    dma_buf_lg.create_fcn = dma_buf_lg_create;
    
    /*
     * Allocate small buffers until the next buffer
     * will be on a large-buffer boundary.
     * This makes assumptions about the way alloc_cnvmem works -- FIXME!
     */
    do {
	if (dma_buf_alloc(&buf, DMA_BUF_SM_LEN) != TRUE)
	    break;
	bufs[nbufs++] = buf;
    } while (maxbufs-- && 
	    !is_aligned((char *)buf._ptr+DMA_BUF_SM_LEN, DMA_BUF_LG_LEN));
    while (nbufs)
	dma_buf_free(&bufs[(nbufs--) - 1]);
    
    /*
     * Now create one large buffer.
     */
    if (dma_buf_alloc(&buf, DMA_BUF_LG_LEN) == TRUE)
	dma_buf_free(&buf);
}

boolean_t
dma_buf_alloc(
    dma_buf_t		*buf,
    vm_size_t		len
)
{
    dma_buf_region_t	*ptr;
    dma_buf_type_t	*buft;
    int			s;
    
    if (len > DMA_BUF_LG_LEN)
	return (FALSE);
    else if (len > DMA_BUF_SM_LEN)
    	buft = &dma_buf_lg;
    else
    	buft = &dma_buf_sm;

    s = spldma();

    if (ptr = buft->free_head) {
	buft->free_head = ptr->next;
	buft->free_count--;
    }
    else {
	ptr = (*buft->create_fcn)();
	if (ptr)
	    buft->total_count++;
    }

    splx(s);
    
    if (ptr == 0)
	return (FALSE);

    buf->_ptr = (void *)ptr;
    buf->_type = (void *)buft;
    
    return (TRUE);
}

void
dma_buf_free(
    dma_buf_t		*buf
)
{
    dma_buf_region_t	*ptr;
    dma_buf_type_t	*buft;
    int			s;
    
    if ((ptr = (dma_buf_region_t *)buf->_ptr) == 0)
    	return;
	
    buft = (dma_buf_type_t *)buf->_type;

    s = spldma();
    
    ptr->next = buft->free_head;
    buft->free_head = ptr;
    buft->free_count++;	
    
    splx(s);

    buf->_ptr = 0;
    buf->_type = 0;
}

static
dma_buf_region_t *
dma_buf_sm_create(
    void
)
{
    caddr_t buf = (caddr_t)alloc_cnvmem(DMA_BUF_SM_LEN, DMA_BUF_SM_LEN);
    if (buf)
	bzero(buf,DMA_BUF_SM_LEN);
    return ((dma_buf_region_t *)buf );
}

/*
 * Eventually, this should allocate small buffers until we get
 * to a large buffer alignment boundary.
 */
 
static
dma_buf_region_t *
dma_buf_lg_create(
    void
)
{
    caddr_t buf = (caddr_t)alloc_cnvmem(DMA_BUF_LG_LEN, DMA_BUF_LG_LEN);
    if (buf)
	bzero(buf,DMA_BUF_LG_LEN);
    return ((dma_buf_region_t *)buf );
}
