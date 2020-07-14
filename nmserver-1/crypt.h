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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef	_CRYPT_
#define	_CRYPT_

/*
 * Security header for packets.
 */
typedef struct crypt_header {
    unsigned long	ch_crypt_level;	/* This must not be encrypted. */
    unsigned short	ch_checksum;	/* Everything hereon can be encrypted. */
    unsigned short	ch_data_size;
} crypt_header_t, *crypt_header_ptr_t;

#define CRYPT_HEADER_SIZE	(sizeof(short) + sizeof(short) + sizeof(long))

/*
 * Encryption levels for data sent over the network.
 */
#define CRYPT_DONT_ENCRYPT		0
#define CRYPT_ENCRYPT			1

/*
 * Encryption return codes.
 */
#define CRYPT_SUCCESS			0
#define CRYPT_FAILURE			-1
#define CRYPT_REMOTE_FAILURE		-2
#define CRYPT_CHECKSUM_FAILURE		-3

/*
 * Encryption algorithms.
 */
#define CRYPT_NULL	0
#define	CRYPT_XOR	1
#define	CRYPT_NEWDES	2
#define	CRYPT_MULTPERM	3
#define CRYPT_DES	4
#define CRYPT_MAX	4

typedef struct {
    int		(*encrypt)();
    int		(*decrypt)();
} crypt_function_t;

extern crypt_function_t		crypt_functions[];

#define CHECK_ENCRYPT_ALGORITHM(algorithm)				\
	(((algorithm) >= CRYPT_NULL) && ((algorithm) <= CRYPT_MAX)	\
		&& (crypt_functions[(algorithm)].encrypt))

#define CHECK_DECRYPT_ALGORITHM(algorithm)				\
	(((algorithm) >= CRYPT_NULL) && ((algorithm) <= CRYPT_MAX)	\
		&& (crypt_functions[(algorithm)].decrypt))


int crypt_encrypt_packet ();
int crypt_decrypt_packet ();
int decryption_enabled ();

#endif	_CRYPT_
