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
/*	@(#)xdr_nlm.c 1.11 89/10/03 SMI	*/
#ifndef lint
static char sccsid[] =	"@(#)xdr_nlm.c	1.2 91/11/20 SMI";
#endif

#include <rpc/rpc.h>
#include "nlm_prot.h"


bool_t
xdr_nlm_stats(xdrs, objp)
	XDR *xdrs;
	nlm_stats *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_holder(xdrs, objp)
	XDR *xdrs;
	nlm_holder *objp;
{
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->svid)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->oh)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->l_offset)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->l_len)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_testrply(xdrs, objp)
	XDR *xdrs;
	nlm_testrply *objp;
{
	if (!xdr_nlm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	switch (objp->stat) {
	case nlm_denied:
		if (!xdr_nlm_holder(xdrs, &objp->nlm_testrply_u.holder)) {
			return (FALSE);
		}
		break;
	}
	return (TRUE);
}




bool_t
xdr_nlm_stat(xdrs, objp)
	XDR *xdrs;
	nlm_stat *objp;
{
	if (!xdr_nlm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_res(xdrs, objp)
	XDR *xdrs;
	nlm_res *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_nlm_stat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_testres(xdrs, objp)
	XDR *xdrs;
	nlm_testres *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_nlm_testrply(xdrs, &objp->stat)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_lock(xdrs, objp)
	XDR *xdrs;
	nlm_lock *objp;
{
	if (!xdr_string(xdrs, &objp->caller_name, LM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->fh)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->oh)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->svid)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->l_offset)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->l_len)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_lockargs(xdrs, objp)
	XDR *xdrs;
	nlm_lockargs *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->block)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_nlm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->reclaim)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->state)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_cancargs(xdrs, objp)
	XDR *xdrs;
	nlm_cancargs *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->block)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_nlm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_testargs(xdrs, objp)
	XDR *xdrs;
	nlm_testargs *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_nlm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_unlockargs(xdrs, objp)
	XDR *xdrs;
	nlm_unlockargs *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_nlm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_fsh_mode(xdrs, objp)
	XDR *xdrs;
	fsh_mode *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_fsh_access(xdrs, objp)
	XDR *xdrs;
	fsh_access *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_share(xdrs, objp)
	XDR *xdrs;
	nlm_share *objp;
{
	if (!xdr_string(xdrs, &objp->caller_name, LM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->fh)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->oh)) {
		return (FALSE);
	}
	if (!xdr_fsh_mode(xdrs, &objp->mode)) {
		return (FALSE);
	}
	if (!xdr_fsh_access(xdrs, &objp->access)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_shareargs(xdrs, objp)
	XDR *xdrs;
	nlm_shareargs *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_nlm_share(xdrs, &objp->share)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->reclaim)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_shareres(xdrs, objp)
	XDR *xdrs;
	nlm_shareres *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_nlm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->sequence)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_notify(xdrs, objp)
	XDR *xdrs;
	nlm_notify *objp;
{
	if (!xdr_string(xdrs, &objp->name, MAXNAMELEN)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->state)) {
		return (FALSE);
	}
	return (TRUE);
}


