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

#ifndef _asp_errno_h
#define _asp_errno_h

#ifndef OSErr
#define OSErr int
#endif

#define ASPERR_NoError         0
#define ASPERR_NoSuchDevice    -1058
#define ASPERR_BindErr         -1059
#define ASPERR_CmdReply        -1060
#define ASPERR_CmdRequest      -1061
#define ASPERR_SystemErr       -1062
#define ASPERR_ProtoErr        -1063
#define ASPERR_NoSuchEntity    -1064
#define ASPERR_RegisterErr     -1065

#define ASPERR_BadVersNum      -1066
#define ASPERR_BufTooSmall     -1067
#define ASPERR_NoMoreSessions  -1068
#define ASPERR_NoServers       -1069
#define ASPERR_ParamErr        -1070
#define ASPERR_ServerBusy      -1071
#define ASPERR_SessClosed      -1072
#define ASPERR_SizeErr         -1073
#define ASPERR_TooManyClients  -1074
#define ASPERR_NoAck           -1075

#endif
