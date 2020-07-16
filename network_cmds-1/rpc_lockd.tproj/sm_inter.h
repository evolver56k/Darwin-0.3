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
/*	@(#)sm_inter.h 1.3 89/10/03 SMI	*/

#define SM_PROG 100024
#define SM_VERS 1
#define SM_STAT 1
#define SM_MON 2
#define SM_UNMON 3
#define SM_UNMON_ALL 4
#define SM_SIMU_CRASH 5

#define SM_MAXSTRLEN 1024

struct sm_name {
	char *mon_name;
};
typedef struct sm_name sm_name;
bool_t xdr_sm_name();


struct my_id {
	char *my_name;
	int my_prog;
	int my_vers;
	int my_proc;
};
typedef struct my_id my_id;
bool_t xdr_my_id();


struct mon_id {
	char *mon_name;
	struct my_id my_id;
};
typedef struct mon_id mon_id;
bool_t xdr_mon_id();


struct mon {
	struct mon_id mon_id;
	char priv[16];
};
typedef struct mon mon;
bool_t xdr_mon();


struct sm_stat {
	int state;
};
typedef struct sm_stat sm_stat;
bool_t xdr_sm_stat();


enum res {
	stat_succ = 0,
	stat_fail = 1,
};
typedef enum res res;
bool_t xdr_res();


struct sm_stat_res {
	res res_stat;
	int state;
};
typedef struct sm_stat_res sm_stat_res;
bool_t xdr_sm_stat_res();


struct status {
	char *mon_name;
	int state;
	char priv[16];
};
typedef struct status status;
bool_t xdr_status();
