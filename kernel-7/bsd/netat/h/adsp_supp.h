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

void	CallUserRoutine();	/* (CCB FPTR sp); */


/*
 *	Add queue element to end of queue.  Pass Address of ptr to 
 *	1st element of queue
 */
int	qAddToEnd(); /* (void FPTR FPTR qhead, void FPTR qelem); */

/*
 *	Hunt down a linked list of queue elements looking for an element with
 *	'data' at 'offset' bytes into the queue element.
 */
void *qfind_b();		/* (void *qhead, word offset, word data); */ 
void *qfind_w();		/* (void *qhead, word offset, word data); */
void *qfind_p();		/* (void *qhead, word offset, void *ptr); */
void *qfind_o();		/* (void *qhead, word offset, void *ptr); */
void *qfind_m();		/* (void *qhead, void *match, 
				   ProcPtr compare_fnx); */


/*
 * Routines to handle sorted timer queues
 */
void InsertTimerElem();		/* (TimerElemPtr *qhead, TimerElemPtr t, 
				   word val); */
void RemoveTimerElem();		/* (TimerElemPtr *qhead, TimerElemPtr t); */
void TimerQueueTick();		/* (TimerElemPtr *qhead);*/

