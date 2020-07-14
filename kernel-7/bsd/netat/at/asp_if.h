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
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

#ifndef _asp_if_h
#define _asp_if_h

#define ASP_Version           0x100

#define ASPFUNC_CloseSess     1
#define ASPFUNC_Command       2
#define ASPFUNC_GetStatus     3
#define ASPFUNC_OpenSess      4
#define ASPFUNC_Tickle        5
#define ASPFUNC_Write         6
#define ASPFUNC_WriteContinue 7
#define ASPFUNC_Attention     8
#define ASPFUNC_CmdReply      9

#define ASPIOC               210 /* AT_MID_ASP */
#define ASPIOC_ClientBind    ((ASPIOC<<8) | 1)
#define ASPIOC_CloseSession  ((ASPIOC<<8) | 2)
#define ASPIOC_GetLocEntity  ((ASPIOC<<8) | 3)
#define ASPIOC_GetRemEntity  ((ASPIOC<<8) | 4)
#define ASPIOC_GetSession    ((ASPIOC<<8) | 5)
#define ASPIOC_GetStatus     ((ASPIOC<<8) | 6)
#define ASPIOC_ListenerBind  ((ASPIOC<<8) | 7)
#define ASPIOC_OpenSession   ((ASPIOC<<8) | 8)
#define ASPIOC_StatusBlock   ((ASPIOC<<8) | 9)
#define ASPIOC_SetPid        ((ASPIOC<<8) |10)
#define ASPIOC_GetSessId     ((ASPIOC<<8) |11)
#define ASPIOC_EnableSelect  ((ASPIOC<<8) |12)
#define ASPIOC_Look          ((ASPIOC<<8) |13)

typedef struct {
	at_inet_t SLSEntityIdentifier;
	at_retry_t Retry;
	int StatusBufferSize;
} asp_status_cmd_t;

typedef struct {
	at_inet_t SLSEntityIdentifier;
	at_retry_t Retry;
	unsigned short TickleInterval;
	unsigned short SessionTimer;
} asp_open_cmd_t;

typedef struct {
	int Primitive;
	int CmdResult;
	unsigned short ReqRefNum;
	unsigned short Filler;
} asp_cmdreply_req_t;

typedef struct {
	int Primitive;
	int CmdResult;
} asp_cmdreply_ind_t;

typedef struct {
	int Primitive;
	unsigned short ReqRefNum;
	unsigned char ReqType;
	unsigned char Filler;
} asp_command_ind_t;

union asp_primitives {
	int Primitive;
	asp_cmdreply_ind_t CmdReplyInd;
	asp_cmdreply_req_t CmdReplyReq;
	asp_command_ind_t CommandInd;
};

#endif
