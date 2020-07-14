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
 * Definitions from strgeneric.h
 */

#define STR_IGNORE	0
#define STR_PUTNEXT	1
#define STR_PUTBACK	2
#define STR_QTIME	(HZ >> 3)

/*
 * Definition of DDP_DATA_SIZE from at/ddp.h 
 */
#define DDP_DATA_SIZE	586


extern int adspInit();
extern int adspOpen();
extern int adspCLListen();
extern int adspClose();
extern int adspCLDeny();
extern int adspStatus();
extern int adspRead();
extern int adspWrite();
extern int adspAttention();
extern int adspOptions();
extern int adspReset();
extern int adspNewCID();
extern int adspPacket();


struct adsp_debug {
    int ad_time;
    int ad_seq;
    int ad_caller;
    int ad_descriptor;
    int ad_bits;
    short ad_sendCnt;
    short ad_sendMax;
    int ad_maxSendSeq;
    int ad_sendWdwSeq;
};
