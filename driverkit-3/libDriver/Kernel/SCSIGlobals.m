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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * SCSIGlobals.m - Common global data.
 *
 * HISTORY
 * 25-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/driverTypes.h>
#import <bsd/dev/scsireg.h>

/* 
 * The following are exported to any SCSI Device objects we might be 
 * linked with...
 */
IONamedValue IOScStatusStrings[] = {
        {SR_IOST_GOOD, 		"Success"				},
        {SR_IOST_SELTO,		"Selection Timeout"			},
	{SR_IOST_CHKSV,		"Check Status, Sense Valid"		},
	{SR_IOST_CHKSNV,	"Check Status"				},
	{SR_IOST_DMAOR,		"DMA Over/Underrun"			},
        {SR_IOST_IOTO, 		"I/O Timeout"				},
        {SR_IOST_BV, 		"SCSI Bus Violation"			},
        {SR_IOST_CMDREJ, 	"Command Reject"			},
        {SR_IOST_MEMALL, 	"Memory Allocation Error"		},
        {SR_IOST_MEMF, 		"Memory Failure"			},
	{SR_IOST_PERM,		"Permission Failure"			},
	{SR_IOST_NOPEN,		"Device Not Open"			},
        {SR_IOST_TABT, 		"Target Dropped Busy"			},
        {SR_IOST_BADST, 	"Bad SCSI Status Byte"			},
        {SR_IOST_INT, 		"Internal Driver Error"			},
        {SR_IOST_BCOUNT, 	"SCSI Data Overflow"			},
        {SR_IOST_VOLNA, 	"Desired Volume Not Available"		},
        {SR_IOST_WP, 		"Media Write Protected"			},
 	{SR_IOST_ALIGN,		"DMA Alignment error"			},
        {SR_IOST_IPCFAIL, 	"Mach IPC Error"			},
        {SR_IOST_RESET, 	"Bus Reset Detected"			},
        {SR_IOST_PARITY, 	"SCSI Parity Error"			},
        {SR_IOST_HW, 		"Gross hardware Failure"		},
	{SR_IOST_DMA,		"DMA Error"				},
        {SR_IOST_INVALID,	"INVALID STATUS (internal error)"	},
	{0, 			NULL					},

};

IONamedValue IOSCSISenseStrings[] = {
	{SENSE_NOSENSE,			"No error to report"		},
	{SENSE_RECOVERED,		"Recovered error"		},
	{SENSE_NOTREADY,		"Target not ready"		},
	{SENSE_MEDIA,			"Media Error"			},
	{SENSE_HARDWARE,		"Hardware failure"		},
	{SENSE_ILLEGALREQUEST,		"Illegal request"		},
	{SENSE_UNITATTENTION,		"Unit attention"		},
	{SENSE_DATAPROTECT,		"Write Protected"		},
	{SENSE_ABORTEDCOMMAND,		"Target aborted command"	},
	{SENSE_VOLUMEOVERFLOW,		"EOM, some data not transfered"	},
	{SENSE_MISCOMPARE,		"Source/media data mismatch"	},
	{0,				NULL				}

};

IONamedValue IOSCSIOpcodeStrings[] = {
	{C6OP_TESTRDY,			"Test unit ready"		}, 
	{C6OP_REWIND,			"Rewind"			},
	{C6OP_REQSENSE,			"Request sense "		},
	{C6OP_READ,			"Read data (6-byte)"		},
	{C6OP_WRITE,			"Write data (10-byte)"		},
	{C6OP_WRTFM,			"Write filemarks"		},
	{C6OP_SPACE,			"Space records/filemarks"	},
	{C6OP_INQUIRY,			"Inquiry"			},
	{C6OP_MODESELECT,		"Mode Select"			},
	{C6OP_MODESENSE,		"Mode Sense"			},
	{C6OP_STARTSTOP,		"Start/Stop"			},	
	{C10OP_READCAPACITY,		"Read capacity"			},
	{C10OP_READEXTENDED,		"Read data (10-byte)"		},
	{C10OP_WRITEEXTENDED,		"Write data (10-byte)"		},
	{0,				NULL				}
};

