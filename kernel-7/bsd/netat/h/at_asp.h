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
 *	Copyright (c) 1995 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 *	Change Log:
 *	  Created February 20, 1995 by Tuyen Nguyen
 */

#ifndef _at_asp_h
#define _at_asp_h

#define ASPSTATE_Close                      0
#define ASPSTATE_Idle                       1
#define ASPSTATE_WaitingForGetStatusRsp     2
#define ASPSTATE_WaitingForOpenSessRsp      3
#define ASPSTATE_WaitingForCommandRsp       4
#define ASPSTATE_WaitingForWriteContinue    5
#define ASPSTATE_WaitingForWriteRsp         6
#define ASPSTATE_WaitingForWriteContinueRsp 7
#define ASPSTATE_WaitingForCloseSessRsp     8
#define ASPSTATE_WaitingForCfgAck           9

/*
 * ATP state block
 */
typedef struct {
	gref_t *atp_gref; /* gref must be the first entry */
	int pid; /* process id, must be the second entry */
	gbuf_t *atp_msgq; /* data msg, must be the third entry */
	unsigned char dflag; /* structure flag, must be the fourth entry */
	unsigned char filler[3];
} atp_state_t;

/*
 * ASP word
 */
typedef struct {
	unsigned char  func;
	unsigned char  param1;
	unsigned short param2;
} asp_word_t;

/*
 * ASP session control block
 */
typedef struct asp_scb {
	gref_t *gref; /* read queue pointer, must be the first entry */
	int pid; /* process id, must be the second entry */
	atp_state_t *atp_state; /* atp state info, must be the third entry */
	unsigned char  dflag; /* structure flag, must be the fourth entry */
	unsigned char  state;
	unsigned char  sess_id;
	unsigned char  tmo_delta;
	unsigned char  tmo_cnt;
	unsigned char  rem_socket;
	unsigned char  rem_node;
	unsigned char  magic_num;
	unsigned short snd_seq_num;
	unsigned short rcv_seq_num;
	unsigned short filler;
	unsigned short tickle_tid;
	unsigned short tickle_interval;
	unsigned short session_timer;
	unsigned short attn_tid;
	unsigned char  attn_flag;
	unsigned char  req_flag;
	gbuf_t *req_msgq;
	unsigned short wrt_seq_num;
	unsigned char get_wait;
	unsigned char ioc_wait;
	at_retry_t cmd_retry;
	at_inet_t loc_addr;
	at_inet_t rem_addr;
	at_inet_t svc_addr;
	gbuf_t *sess_ioc;
	gbuf_t *stat_msg;
	void (*tmo_func)();
	struct asp_scb *next_tmo;
	struct asp_scb *prev_tmo;
	struct asp_scb *sess_scb;
	struct asp_scb *next_scb;
	struct asp_scb *prev_scb;
	unsigned char sel_on;
	unsigned char user;
	unsigned char rcv_cnt;
	unsigned char snd_stop;
	unsigned char reply_socket;
	unsigned char if_num;
	unsigned char pad[2];
	atlock_t lock;
	atlock_t delay_lock;
	atevent_t event;
	atevent_t delay_event;
} asp_scb_t;

#endif
