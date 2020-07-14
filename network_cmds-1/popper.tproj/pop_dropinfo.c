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
static char SccsId[] = "@(#)@(#)pop_dropinfo.c	2.1  2.1 3/18/91";
#endif not lint

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "popper.h"

#ifndef NeXT
extern int      errno;
extern int      sys_nerr;
extern char    *sys_errlist[];
#endif /* NeXT */

/* 
 *  dropinfo:   Extract information about the POP maildrop and store 
 *  it for use by the other POP routines.
 */

pop_dropinfo(p)
POP     *   p;
{
    char                    buffer[BUFSIZ];         /*  Read buffer */
    MsgInfoList         *   mp;                     /*  Pointer to message 
                                                        info list */
    register int            msg_num;                /*  Current message 
                                                        counter */
    int                     nchar;                  /*  Bytes written/read */

    /*  Initialize maildrop status variables in the POP parameter block */
    p->msg_count = 0;
    p->msgs_deleted = 0;
    p->last_msg = 0;
    p->bytes_deleted = 0;
    p->drop_size = 0;

    /*  Allocate memory for message information structures */
    p->msg_count = ALLOC_MSGS;
    p->mlp = (MsgInfoList *)calloc((unsigned)p->msg_count,sizeof(MsgInfoList));
    if (p->mlp == NULL){
        (void)fclose (p->drop);
        p->msg_count = 0;
        return pop_msg (p,POP_FAILURE,
            "Can't build message list for '%s': Out of memory", p->user);
    }

    rewind (p->drop);

    /*  Scan the file, loading the message information list with 
        information about each message */

    for (msg_num = p->drop_size = 0, mp = p->mlp - 1;
             fgets(buffer,MAXMSGLINELEN,p->drop);) {

        nchar  = strlen(buffer);

        if (strncmp(buffer,"From ",5) == 0) {

            if (++msg_num > p->msg_count) {
                p->mlp=(MsgInfoList *) realloc(p->mlp,
                    (p->msg_count+=ALLOC_MSGS)*sizeof(MsgInfoList));
                if (p->mlp == NULL){
                    (void)fclose (p->drop);
                    p->msg_count = 0;
                    return pop_msg (p,POP_FAILURE,
                        "Can't build message list for '%s': Out of memory",
                            p->user);
                }
                mp = p->mlp + msg_num - 2;
            }
#ifdef DEBUG
            if(p->debug)
                pop_log(p,POP_DEBUG,
                    "Msg %d at offset %d is %d octets long and has %u lines.",
                        mp->number,mp->offset,mp->length,mp->lines);
#endif DEBUG
            ++mp;
            mp->number = msg_num;
            mp->length = 0;
            mp->lines = 0;
            mp->offset = ftell(p->drop) - nchar;
            mp->del_flag = FALSE;
            mp->retr_flag = FALSE;
#ifdef DEBUG
            if(p->debug)
                pop_log(p,POP_DEBUG, "Msg %d being added to list", mp->number);
#endif DEBUG
        }
        mp->length += nchar;
        p->drop_size += nchar;
        mp->lines++;
    }
    p->msg_count = msg_num;

#ifdef DEBUG
    if(p->debug && msg_num > 0) {
        register    i;
        for (i = 0, mp = p->mlp; i < p->msg_count; i++, mp++)
            pop_log(p,POP_DEBUG,
                "Msg %d at offset %d is %d octets long and has %u lines.",
                    mp->number,mp->offset,mp->length,mp->lines);
    }
#endif DEBUG

    return(POP_SUCCESS);
}
