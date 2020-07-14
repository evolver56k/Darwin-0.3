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

#ifndef	_NN_DEFS_
#define	_NN_DEFS_

#include <ctype.h>

#include "mem.h"
#include "disp_hdr.h"
#include "lock_queue.h"
#include <servers/netname_defs.h>
#include "port_defs.h"

typedef struct nn_entry {
    struct nn_entry	*next;
    netname_name_t	nne_name;
    port_t		nne_port;
    port_t		nne_signature;
} nn_entry_t, *nn_entry_ptr_t;

#define NN_TABLE_SIZE	32
extern struct lock_queue	nn_table[NN_TABLE_SIZE];

#define NN_NAME_HASH(index, name) { 			\
    register char *cp = (char *)(name);			\
    (index) = 0;					\
    while ((*cp) != '\0') { (index) += *cp; cp++;}	\
    (index) = (index) % NN_TABLE_SIZE;			\
}

#define NN_CONVERT_TO_UPPER(name) {			\
    register char *cp = (char *)(name);			\
    while ((*cp) != '\0') {				\
	if (islower(*cp)) *cp = toupper(*cp);		\
	cp++;						\
    }							\
}


/*
 * Structures used to make and remember about network name requests.
 */
typedef struct {
    disp_hdr_t		nnr_disp_hdr;
    netname_name_t	nnr_name;
    network_port_t	nnr_nport;
} nn_req_t, *nn_req_ptr_t;

typedef struct {
    struct condition	nnrr_condition;
    struct mutex	nnrr_lock;
    port_t		nnrr_lport;
    int			nnrr_result;
} nn_req_rec_t, *nn_req_rec_ptr_t;


/*
 * Functions public to the network name service module.
 */
extern nn_handle_request();
extern nn_handle_reply();
extern nn_name_test();
extern nn_network_look_up();

/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_NNREC;


#endif	_NN_DEFS_

