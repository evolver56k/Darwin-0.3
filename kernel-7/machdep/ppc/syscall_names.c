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

char *mach_callnames[] = {
	"#0",			/* 0 */		/* Unix */
	"#1",			/* 1 */		/* Unix */
	"#2",			/* 2 */		/* Unix */
	"#3",			/* 3 */		/* Unix */
	"#4",			/* 4 */		/* Unix */
	"#5",			/* 5 */		/* Unix */
	"#6",			/* 6 */		/* Unix */
	"#7",			/* 7 */		/* Unix */
	"#8",			/* 8 */		/* Unix */
	"#9",			/* 9 */		/* Unix */

	"task_self",		/* 10 */	/* obsolete */
	"thread_reply",		/* 11 */	/* obsolete */
	"task_notify",		/* 12 */	/* obsolete */
	"thread_self",		/* 13 */	/* obsolete */
	"#14",			/* 14 */
	"#15",			/* 15 */
	"#16",			/* 16 */
	"#17",			/* 17 */
	"#18",			/* 18 */
	"#19",			/* 19 */

	"msg_send_trap",	/* 20 */	/* obsolete */
	"msg_receive_trap",	/* 21 */	/* obsolete */
	"msg_rpc_trap",		/* 22 */	/* obsolete */
	"#23",			/* 23 */
	"mach_msg_simple_trap",	/* 24 */
	"mach_msg_trap",	/* 25 */
	"mach_reply_port",	/* 26 */
	"mach_thread_self",	/* 27 */
	"mach_task_self",	/* 28 */
	"mach_host_self",	/* 29 */

	"#30",			/* 30 */
	"#31",			/* 31 */
	"mach_msg_overwrite_trap", /* 32 */
	"task_by_pid",		/* 33 */
	"#34",			/* 34 */
	"_lookupd_port",	/* 35 */
	"#36",			/* 36 */
	"#37",			/* 37 */
	"#38",			/* 38 */

	"#39",			/* 39 */
	"mach_swapon",		/* 40 */

	"init_process",		/* 41 */
	"#42",			/* 42 */
	"map_fd",		/* 43 */
	"#44",			/* 44 */
	"mach_swapon",		/* 45 */
	"#46",			/* 46 */
	"#47",			/* 47 */
	"#48",			/* 48 */
	"#49",			/* 49 */

	"#50",			/* 50 */
	"kern_timestamp",	/* 51 */
	"#52",			/* 52 */
	"#53",			/* 53 */
	"#54",			/* 54 */
	"host_self",		/* 55 */
	"host_priv_self",	/* 56 */
	"#57",			/* 57 */
	"#58",			/* 58 */
 	"swtch_pri",		/* 59 */

	"swtch",		/* 60 */
	"thread_switch",	/* 61 */
	"#62",			/* 62 */
	"#63",			/* 63 */
	"#64",			/* 64 */
	"#65",			/* 65 */
	"#66",			/* 66 */
	"#67",			/* 67 */
	"_event_port_by_tag",	/* 68 */
	"device_master_self",	/* 69 */
};

int	mach_names_count = (sizeof(mach_callnames) / sizeof(mach_callnames[0]));
