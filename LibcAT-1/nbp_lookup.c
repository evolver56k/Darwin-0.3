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
 *	Copyright (c) 1988, 1989, 1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 * $Id: nbp_lookup.c,v 1.1.1.1 1999/04/13 22:26:04 wsanchez Exp $
 */

/* "@(#)nbp_lookup.c: 2.0, 1.7; 9/27/89; Copyright 1988-89, Apple Computer, Inc." */

/*
 * Title:	nbp_lookup.c
 *
 * Facility:	AppleTalk Zone Information Protocol Library Interface
 *
 * Author:	Gregory Burns, Creation Date: Jul-14-1988
 *
 * History:
 * X01-001	Gregory Burns	14-Jul-1988
 *	 	Initial Creation.
 *
 */

#include <string.h>
#include <stdio.h>
#include <at/appletalk.h>
#include <at/nbp.h>
#include <at/zip.h>
#include <h/lap.h>
#include <h/debug.h>
#include <errno.h>

#include <at/atp.h>
#include <at/asp_errno.h>

#include <mach/cthreads.h>

#include "at_proto.h"

#ifndef PR_2206317_FIXED
#define	SET_ERRNO(e)	(cthread_set_errno_self(e), errno = e)
#else
#define	SET_ERRNO(e)	cthread_set_errno_self(e)
#endif

#ifndef NULL
#define	NULL	0
#endif /* NULL */

static int change_embedded_wildcard(at_entity_t *entity);
static int change_nvestr(at_nvestr_t *nve);

int
nbp_lookup (entity, buf, max, retry)
	at_entity_t	*entity;
	at_nbptuple_t	*buf;
	int		max;
	at_retry_t	*retry;
{
	int		got;
	int		iff;
	at_nvestr_t	this_zone;
	at_entity_t	l_entity;

	/* Make a copy of the entity param so we can freely modify it. */
	l_entity = *entity;

	if (l_entity.zone.len == 1 && l_entity.zone.str[0] == '*') {
		/* looking for the entity in THIS zone.... if this is
		 * an ethertalk iff, then susbstitute by the
		 * real zone name
		 */
		if ((iff = ddp_primary_interface()) == -1) {
			FPRINTF(stderr, "nbp_lookup: ddp_primary_interface failed\n");
			return (-1);
		}
		if (iff == AT_IFF_ETHERTALK) {
			if (zip_getmyzone (&this_zone) == -1) {
				FPRINTF(stderr,"nbp_lookup: zip_getmyzone failed\n");
				return (-1);
			}
			l_entity.zone = this_zone;
		}
	}

	if (change_embedded_wildcard(&l_entity) < 0) {
	    SET_ERRNO(EINVAL);
	    FPRINTF(stderr,"nbp_lookup: change_embedded failed\n");
	    return (-1);
	}

	if ((got = _nbp_send_(NBP_LKUP, NULL, &l_entity, buf, max, retry)) < 0) {
		FPRINTF(stderr,"nbp_lookup: _nbp_send_ failed got=%d\n", got);
		return (-1);
	}
	return (got);
}	

int
nbp_lookup_multiple (entity, buf, max, retry, cnt)
	at_entity_t	*entity;
	char		*buf;
	int		max;
	int		cnt;		/* # of entities to register */
	at_retry_t	*retry;
{
	int		got;

	/* this is for nbp_resitration, so we know that there will be no
		wild cards, no need to check
	 */


	if ((got = nbp_send_multi(NBP_LKUP, NULL, entity, buf, max, retry,cnt)) < 0) {
		FPRINTF(stderr,"nbp_lookup: _nbp_send_ failed got=%d\n", got);
		return (-1);
	}
	return (got);
}	

static	int
change_embedded_wildcard(entity)
at_entity_t	*entity;
{
	if ((entity->object.len > 1) &&
	    !change_nvestr (&entity -> object))
	    return (-1);
	
	if ((entity->type.len > 1) &&
	    !change_nvestr (&entity -> type))
	    return (-1);

	return (0);
}

static int change_nvestr (nve)
at_nvestr_t	*nve;
{
    u_char		*c;
    int		one_meta = 0;
    
    for (c = nve -> str; c < (nve -> str + nve -> len); c++) {
	if ((*c == '=' ) || (*c == NBP_SPL_WILDCARD)) {
	    if (one_meta)
		return (0);
	    else {
		one_meta++;
		*c = NBP_SPL_WILDCARD;
	    }
	}
    }
    return (1); 
}

