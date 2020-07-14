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
/*      Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *	Copyright 1997, 1998 Apple Computer Inc. All rights reserved.
 *
 * scsiTypes.h - Exported API of SCSIController class.
 *
 * HISTORY
 * 1998-3-4	gvdl at Apple
 *	Removed from the scsiTypes.h file so that it compiles in straight c
 */

#ifndef _DRIVERKIT_SCSIREQUEST_H
#define _DRIVERKIT_SCSIREQUEST_H

#import <bsd/dev/scsireg.h>
#import <kernserv/clock_timer.h>

/*
 * Argument to executeRequest:buffer:client method.
 */
typedef struct {

        /*** inputs ***/
        
        unsigned char		target;         /* SCSI target ID */
        unsigned char		lun;            /* logical unit */
        cdb_t			cdb;            /* command descriptor block - 
                                                 * one of four formats */
        char			read;     	/* BOOL expected DMA direction 
						 * (YES if read) */
        int			maxTransfer;    /* maximum number of bytes to
                                                 * transfer */
        int			timeoutLength;  /* I/O timeout in seconds */
        unsigned		disconnect:1;   /* OK to disconnect */
	unsigned		cmdQueueDisable:1;
						/* disable command queueing */
	unsigned		syncDisable:1;	/* disable synchronous transfer
						 * negotiation */
	unsigned		ignoreChkcond:1;/* to disable issuing of chk.
						   cond. cmd  */
				
        unsigned		pad:24;	
	unsigned		cdbLength:4;	/* length of CDB in bytes
						 * (optional) */
           
        /*** outputs ***/
        
        sc_status_t		driverStatus;   /* driver status */
        unsigned char		scsiStatus;     /* SCSI status byte */
        int			bytesTransferred; /* actual number of bytes 
                                                 * transferred by DMA */
	ns_time_t		totalTime;	/* total execution time */
	ns_time_t		latentTime;	/* disconnect time */
	esense_reply_t	 	senseData;	/* extended sense if
						 * driverStatus = SR_IOST_CHKSV
						 */

} IOSCSIRequest;

/*
 * Argument to executeSCSI3Request:buffer:client method.
 */
typedef struct {

        /*** inputs ***/
        
        unsigned long long	target;         /* SCSI target ID */
        unsigned long long	lun;            /* logical unit */
        scsi3_cdb_t		cdb;            /* command descriptor block - 
                                                 * one of five formats */
        char			read;     	/* BOOL expected DMA direction 
						 * (YES if read) */
        int			maxTransfer;    /* maximum number of bytes to
                                                 * transfer */
        int			timeoutLength;  /* I/O timeout in seconds */
        unsigned		disconnect:1;   /* OK to disconnect */
	unsigned		cmdQueueDisable:1;
						/* disable command queueing */
	unsigned		syncDisable:1;	/* disable synchronous transfer
						 * negotiation */
				
        unsigned		pad:25;
	unsigned		cdbLength:4;	/* length of CDB in bytes
						 * (optional) */
           
        /*** outputs ***/
        
        sc_status_t		driverStatus;   /* driver status */
        unsigned char		scsiStatus;     /* SCSI status byte */
        int			bytesTransferred; /* actual number of bytes 
                                                 * transferred by DMA */
	ns_time_t		totalTime;	/* total execution time */
	ns_time_t		latentTime;	/* disconnect time */
	esense_reply_t	 	senseData;	/* extended sense if
						 * driverStatus = SR_IOST_CHKSV
						 */
	unsigned char		pad2[4];

} IOSCSI3Request;

#endif /* _DRIVERKIT_SCSIREQUEST_H */
