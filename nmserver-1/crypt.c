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
#include <mach/mach_types.h>
#include <sys/types.h>

#include "crypt.h"
#include "netipc.h"
#include "debug.h"
#include "key_defs.h"
#include "ls_defs.h"
#include "netmsg.h"
#include "network.h"
#include <servers/nm_defs.h>

int crypt_null() { return 0; }

#define encrypt_xor	crypt_null
#define decrypt_xor	crypt_null

#define encrypt_newdes	crypt_null
#define decrypt_newdes	crypt_null

#define encrypt_multperm	crypt_null
#define decrypt_multperm	crypt_null

#define encrypt_des	crypt_null
#define decrypt_des	crypt_null

crypt_function_t crypt_functions[] = {
{
	crypt_null, crypt_null},
	{(int (*)())encrypt_xor, (int (*)())decrypt_xor},
	{(int (*)())encrypt_newdes, (int (*)())decrypt_newdes},
	{(int (*)())encrypt_multperm, (int (*)())decrypt_multperm},
	{(int (*)())encrypt_des, (int (*)())decrypt_des}
};



/*
 * crypt_encrypt_packet
 *	Encrypts the data in a packet.
 *
 * Parameters:
 *	packet_ptr	: pointer to packet which is to be encrypted
 *	crypt_level	: level at which this packet is to be encrypted
 *
 * Results:
 *	CRYPT_SUCCESS or CRYPT_FAILURE
 *
 * Design:
 *	Pad out data with zero bytes to a multiple of eight bytes.
 *	Calculate the checksum of the packet data.
 *	Look up the key for the destination host of this packet.
 *	Encrypt the checksum and the packet data using the key.
 *
 * Note:
 *	We should have a key for the host at this point.
 *
 */
EXPORT int crypt_encrypt_packet(packet_ptr, crypt_level)
netipc_ptr_t	packet_ptr;
int		crypt_level;
{
    int			crypt_size, data_size;
    int			bytes_to_pad;
    char		*pad_ptr;

    /*
     * Round up the size of the encrypted data to a multiple of 8 bytes.
     * Note that the last two shorts of the crypt header are also encrypted.
     */
    data_size = ntohs(packet_ptr->ni_header.nih_crypt_header.ch_data_size);
    bytes_to_pad = crypt_size = data_size + (2 * (sizeof(short)));
    crypt_size = (crypt_size + 7) & (~(07));
    bytes_to_pad = crypt_size - bytes_to_pad;

    /*
     * Pad out the data with zero bytes.
     */
    pad_ptr = (char *)&(packet_ptr->ni_header.nih_crypt_header.ch_checksum);
    while (bytes_to_pad--) {
	pad_ptr[crypt_size - bytes_to_pad] = 0;
    }

    /*
     * Calculate the checksum.
     */
    packet_ptr->ni_header.nih_crypt_header.ch_checksum = 0;
    packet_ptr->ni_header.nih_crypt_header.ch_checksum =
	udp_checksum((unsigned short *)&(packet_ptr->ni_header.nih_crypt_header.ch_crypt_level),
			(crypt_size + sizeof(long)));

    if (crypt_level == CRYPT_DONT_ENCRYPT) {
	RETURN(CRYPT_SUCCESS);
    }

    RETURN(CRYPT_FAILURE);
}



/*
 * crypt_decrypt_packet
 *	Decrypts the data in a packet.
 *
 * Parameters:
 *	packet_ptr	: pointer to packet which is to be decrypted
 *	crypt_level	: level at which this packet is to be decrypted
 *
 * Results:
 *	CRYPT_SUCCESS, CRYPT_FAILURE or CRYPT_CHECKSUM_FAILURE.
 *
 * Design:
 *	Look up the key for the destination host of this packet.
 *	Decrypt the checksum and the packet data using the key.
 *	Check the checksum of the packet data.
 *
 * Note:
 *	We should have a key for the host at this point.
 *
 */
EXPORT int crypt_decrypt_packet(packet_ptr, crypt_level)
netipc_ptr_t	packet_ptr;
int		crypt_level;
{

    RETURN(CRYPT_SUCCESS);

}



/*
 * decryption_enabled
 *	Indicates whether or not decryption is configured.
 *
 * Results:
 *	CRYPT_ENCRYPT or CRYPT_DONT_ENCRYPT
 *
 *
 */
EXPORT int decryption_enabled()
{

    if	(((param.crypt_algorithm) >= CRYPT_NULL)
		&& ((param.crypt_algorithm) <= CRYPT_MAX)
		&& (crypt_functions[(param.crypt_algorithm)].decrypt)
		&& ((crypt_functions[(param.crypt_algorithm)].decrypt) != crypt_null)) {
	RETURN(CRYPT_ENCRYPT);
    }
    else {
	RETURN(CRYPT_DONT_ENCRYPT);
    }

}
