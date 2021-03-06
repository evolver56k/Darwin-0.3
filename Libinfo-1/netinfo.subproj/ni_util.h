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
/*
 * Utility routines for NetInfo
 * Copyright (C) 1989 by NeXT, Inc.
 */

#ifndef _NI_UTIL_H_
#define _NI_UTIL_H_

#include <netinfo/ni.h>

#define NI_INDEX_NULL ((ni_index)-1)
#define NI_INIT(objp) bzero((void *)(objp), sizeof(*(objp)))

ni_name ni_name_dup(ni_name_const);
void ni_name_free(ni_name *);
int ni_name_match(ni_name_const, ni_name_const);

ni_namelist ni_namelist_dup(const ni_namelist);
void ni_namelist_free(ni_namelist *);
void ni_namelist_insert(ni_namelist *, ni_name_const, ni_index);
void ni_namelist_delete(ni_namelist *, ni_index);
ni_index ni_namelist_match(const ni_namelist, ni_name_const);

ni_property ni_prop_dup(const ni_property);
void ni_prop_free(ni_property *);

void ni_proplist_insert(ni_proplist *, const ni_property, ni_index);
void ni_proplist_delete(ni_proplist *, ni_index);
ni_index ni_proplist_match(const ni_proplist, ni_name_const, ni_name_const);
ni_proplist ni_proplist_dup(const ni_proplist);
void ni_proplist_free(ni_proplist *);

void ni_proplist_list_free(ni_proplist_list *);

void ni_idlist_insert(ni_idlist *, ni_index, ni_index);
int ni_idlist_delete(ni_idlist *, ni_index);
ni_idlist ni_idlist_dup(const ni_idlist);
void ni_idlist_free(ni_idlist *);

void ni_entrylist_insert(ni_entrylist *, ni_entry);
void ni_entrylist_delete(ni_entrylist *, ni_index);
void ni_entrylist_free(ni_entrylist *);

int innetgr(const char *, const char *, const char *, const char *);

#endif !_NI_UTIL_H_