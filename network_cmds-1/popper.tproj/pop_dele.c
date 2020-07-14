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
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_dele.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include "popper.h"

/* 
 *  dele:   Delete a message from the POP maildrop
 */
pop_dele (p)
POP     *   p;
{
    MsgInfoList     *   mp;         /*  Pointer to message info list */
    int                 msg_num;

    /*  Convert the message number parameter to an integer */
    msg_num = atoi(p->pop_parm[1]);

    /*  Is requested message out of range? */
    if ((msg_num < 1) || (msg_num > p->msg_count))
        return (pop_msg (p,POP_FAILURE,"Message %d does not exist.",msg_num));

    /*  Get a pointer to the message in the message list */
    mp = &(p->mlp[msg_num-1]);

    /*  Is the message already flagged for deletion? */
    if (mp->del_flag)
        return (pop_msg (p,POP_FAILURE,"Message %d has already been deleted.",
            msg_num));

    /*  Flag the message for deletion */
    mp->del_flag = TRUE;

#ifdef DEBUG
    if(p->debug)
        pop_log(p,POP_DEBUG,"Deleting message %u at offset %u of length %u\n",
            mp->number,mp->offset,mp->length);
#endif DEBUG

    /*  Update the messages_deleted and bytes_deleted counters */
    p->msgs_deleted++;
    p->bytes_deleted += mp->length;

    /*  Update the last-message-accessed number if it is lower than 
        the deleted message */
    if (p->last_msg < msg_num) p->last_msg = msg_num;

    return (pop_msg (p,POP_SUCCESS,"Message %d has been deleted.",msg_num));
}
