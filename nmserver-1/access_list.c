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
 * Copyright (c) 1998 Apple Computer, Inc.
 * All rights reserved.  
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <mach/mach.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif // !WIN32
#include "mem.h"
#include "netmsg.h"
#include "nn_defs.h"
#include <servers/netname_defs.h>
#include <servers/nm_defs.h>

#ifndef INADDR_ANY
#define	INADDR_ANY		(u_long)0x00000000
#endif // !INADDR_ANY
#ifndef INADDR_BROADCAST
#define	INADDR_BROADCAST	(u_long)0xffffffff
#endif // !INADDR_BROADCAST

typedef struct access_addr {
    netaddr_t lower_address;
    netaddr_t upper_address;
    netaddr_t netmask;
} access_addr_t, *access_addr_ptr_t;

typedef struct access_grant {
    netname_name_t name;
    unsigned int length;
    access_addr_t addresses[0];
} access_grant_t, *access_grant_ptr_t;

static access_grant_ptr_t *access_list = NULL;
static unsigned int access_list_len = 0;

#define FIRST_MATCH_GOVERNS
#define NO_MATCH_DEFAULT FALSE

#define ACCESS_CONFIG_FILE "/etc/nmserver.access"
#define NULL_CHAR '\0'
#define COMMENT_CHAR '#'
#define ESC_CHAR '\\'
#define GLOB_CHAR_EXT '*'
#define GLOB_CHAR_INT 'a'
#define WILD_CHAR_EXT '?'
#define WILD_CHAR_INT 'q'
#define LINE_SEP_CHAR '\n'
#define NAME_SEP_CHAR ':'
#define ENTRY_SEP_CHAR ','
#define QUAD_SEP_CHAR '.'
#define RANGE_SEP_CHAR '-'
#define MASK_SEP_CHAR '/'
#define ANY_HOST_CHAR '+'

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)<(b))?(b):(a))
#define ispart(a) ((a)<256)
#define isshift(a) ((a)<=32)
#define ignore(a) (isspace(a)||(COMMENT_CHAR==a)||(ENTRY_SEP_CHAR==a)||(QUAD_SEP_CHAR==a)||(RANGE_SEP_CHAR==a)||(MASK_SEP_CHAR==a))
#define access_address(a, b, c, d) ((a<<24)+(b<<16)+(c<<8)+d)
#define access_netmask(a) ((a>0)?(INADDR_BROADCAST<<(32-a)):(INADDR_ANY))

PRIVATE
unsigned int
access_value(start, end, address_ptr, netmask_ptr)
    char 		*start;
    char 		*end;
    netaddr_t  		*address_ptr;
    netaddr_t  		*netmask_ptr;
{
    unsigned int retval = 0, a = 0, b = 0, c = 0, d = 0, bits = 32;
    int n = 0, length;
    netaddr_t address = INADDR_BROADCAST, netmask = INADDR_ANY;
    while (start <= end && ignore(*(end-1))) end--;
    while (start <= end && ignore(*start)) start++;
    length = end - start;
    if ((length > 6 && 4 == sscanf(start, "%3u . %3u . %3u . %3u%n", &a, &b, &c, &d, &n) && n == length && ispart(a) && ispart(b) && ispart(c) && ispart(d)) ||
        (length > 4 && 3 == sscanf(start, "%3u . %3u . %3u%n", &a, &b, &c, &n) && n == length && ispart(a) && ispart(b) && ispart(c)) ||
        (length > 2 && 2 == sscanf(start, "%3u . %3u%n", &a, &b, &n) && n == length && ispart(a) && ispart(b)) ||
        (length > 0 && 1 == sscanf(start, "%3u%n", &a, &n) && n == length && ispart(a)) ||
        (length == 1 && ANY_HOST_CHAR == *start)) {
        address = access_address(a, b, c, d);
        bits = (d != 0) ? 32 : ((c != 0) ? 24 : ((b != 0) ? 16 : ((a != 0) ? 8 : 0)));
        netmask = access_netmask(bits);
        if (address_ptr) *address_ptr = address;
        if (netmask_ptr) *netmask_ptr = netmask;
        retval = 1;
    }
    return retval;
}

PRIVATE
unsigned int
access_maskvalue(start, end, netmask_ptr)
    char 		*start;
    char 		*end;
    netaddr_t  		*netmask_ptr;
{
    unsigned int retval = 0, bits = 32;
    int n = 0, length;
    netaddr_t netmask = INADDR_ANY;
    char c;
    while (start <= end && ignore(*(end-1))) end--;
    while (start <= end && ignore(*start)) start++;
    length = end - start;
    if (length > 0) {
        retval = 1;
        c = toupper(*start);
        if (1 == length && 'A' == c) {
            netmask = access_netmask(8);
        } else if (1 == length && 'B' == c) {
            netmask = access_netmask(16);
        } else if (1 == length && 'C' == c) {
            netmask = access_netmask(24);
        } else if (1 == sscanf(start, "%3u%n", &bits, &n) && n == length && isshift(bits)) {
            netmask = access_netmask(bits);
        } else {
            retval = access_value(start, end, &netmask, NULL);
        }
        if (retval && netmask_ptr) *netmask_ptr = netmask;
    }
    return retval;
}

PRIVATE
unsigned int
access_range(start, end, access)
    char 		*start;
    char 		*end;
    access_addr_ptr_t 	access;
{
    char *range_separator, *mask_separator;
    unsigned int retval = 1;
    netaddr_t lower_address = INADDR_BROADCAST, upper_address = INADDR_BROADCAST, lower_mask = INADDR_ANY, upper_mask = INADDR_ANY, netmask = INADDR_ANY;
    while (start <= end && ignore(*(end-1))) end--;
    while (start <= end && ignore(*start)) start++;
    range_separator = (char *)memchr(start, RANGE_SEP_CHAR, end-start);
    if (!range_separator) range_separator = start-1;
    mask_separator = (char *)memchr(range_separator+1, MASK_SEP_CHAR, end-range_separator-1);
    if (!mask_separator) mask_separator = end;
    if (retval && mask_separator > range_separator+1) {
        retval = access_value(range_separator+1, mask_separator, &upper_address, &upper_mask);
    }
    if (retval && range_separator > start) {
        retval = access_value(start, range_separator, &lower_address, &lower_mask);
    } else {
        lower_address = upper_address;
        lower_mask = upper_mask;
    }
    if (retval && end > mask_separator+1) {
        retval = access_maskvalue(mask_separator+1, end, &netmask);
    } else {
        netmask = lower_mask | upper_mask;
    }
    if (retval && access) {
        lower_address &= netmask;
        upper_address &= netmask;
        access->lower_address = min(lower_address, upper_address);
        access->upper_address = max(lower_address, upper_address);
        access->netmask = netmask;
    }
    return retval;
}

PRIVATE
unsigned int
access_entry(start, end, max, addresses)
    char 		*start;
    char 		*end;
    unsigned int 	max;
    access_addr_ptr_t 	addresses;
{
    unsigned int retval = 0, thisval;
    access_addr_t access;
    if (start < end && (thisval = access_range(start, end, &access)) > 0 && retval < max) {
        if (addresses) {
            addresses[retval] = access;
        }
        retval += thisval;
    }
    return retval;
}

#ifndef NeXT_PDO

PRIVATE
void
access_read(stream)
    FILE 		*stream;
{
    unsigned int i, j;
    char *line, *line_end, *entries_start, *address_start, *address_end, *cp, c;
    size_t line_length;
    netname_name_t name;
    unsigned int grant_length, name_length;
    boolean_t esc_mode;
    while (fgetln(stream, &line_length)) {
        access_list_len++;
    }
    rewind(stream);
    MEM_ALLOC(access_list, access_grant_ptr_t *, access_list_len * sizeof(access_grant_ptr_t), FALSE);
    for (i = 0; i < access_list_len && (line = fgetln(stream, &line_length)); i++) {
        if (line_length == 0 || COMMENT_CHAR == *line) continue;
        line_end = line + line_length;
        esc_mode = FALSE;
        name_length = 0;
        for (cp = line; cp < line_end; cp++) {
            c = *cp;
            if (!esc_mode && (NAME_SEP_CHAR == c || LINE_SEP_CHAR == c || COMMENT_CHAR == c)) break;
            if (name_length < sizeof(netname_name_t)-1) {
                if (esc_mode) {
                    name[name_length++] = islower(c) ? toupper(c) : c;
                } else {
                    if (GLOB_CHAR_EXT == c) name[name_length++] = GLOB_CHAR_INT;
                    else if (WILD_CHAR_EXT == c) name[name_length++] = WILD_CHAR_INT;
                    else if (ESC_CHAR != c) name[name_length++] = islower(c) ? toupper(c) : c;
                }
            }
            if (esc_mode || ESC_CHAR == c) esc_mode = !esc_mode;
        }
        name[name_length] = NULL_CHAR;
        entries_start = ++cp;
        grant_length = 0;
        c = NULL_CHAR;
        while (cp < line_end && c != LINE_SEP_CHAR && c != COMMENT_CHAR) {
            address_start = address_end = cp;
            while (cp < line_end && (c = *cp++) != ENTRY_SEP_CHAR && c != LINE_SEP_CHAR && c != COMMENT_CHAR) address_end++;
            grant_length += access_entry(address_start, address_end, UINT_MAX, 0);
        }
        MEM_ALLOC(access_list[i], access_grant_ptr_t, sizeof(access_grant_t) + grant_length * sizeof(access_addr_t), FALSE);
        (void)strncpy(access_list[i]->name, name, name_length);
        access_list[i]->name[name_length] = NULL_CHAR;
        access_list[i]->length = grant_length;
        for (j = 0; j < grant_length; j++) {
            access_list[i]->addresses[j].lower_address = INADDR_BROADCAST;
            access_list[i]->addresses[j].upper_address = INADDR_BROADCAST;
            access_list[i]->addresses[j].netmask = INADDR_ANY;
        }
        cp = entries_start;
        c = NULL_CHAR;
        for (j = 0; j < grant_length && cp < line_end && c != LINE_SEP_CHAR && c != COMMENT_CHAR;) {
            address_start = address_end = cp;
            while (cp < line_end && (c = *cp++) != ENTRY_SEP_CHAR && c != LINE_SEP_CHAR && c != COMMENT_CHAR) address_end++;
            j += access_entry(address_start, address_end, grant_length-j, &(access_list[i]->addresses[j]), FALSE);
        }
    }
}

#endif // !NeXT_PDO

PRIVATE
void
access_free()
{
    unsigned int i;
    access_grant_ptr_t grant;
    if (access_list) {
        for (i = 0; i < access_list_len; i++) {
            grant = access_list[i];
            if (grant) MEM_DEALLOC(grant, sizeof(access_grant_t) + grant->length * sizeof(access_addr_t));
        }
        MEM_DEALLOC(access_list, access_list_len * sizeof(access_grant_ptr_t));
    }
    access_list = NULL;
    access_list_len = 0;
}

PUBLIC
void
access_init(void)
{
    FILE *access_fp;
    access_free();
#ifndef NeXT_PDO
    access_fp = fopen(ACCESS_CONFIG_FILE, "r");
    if (access_fp) {
        access_read(access_fp);
        fclose(access_fp);
    }
#ifdef ACCESS_DEBUG_OUTPUT
    access_fp = fopen("/etc/nmserver.out", "w+");
    if (access_fp) {
        access_print(access_fp);
        fclose(access_fp);
    }
#endif // ACCESS_DEBUG_OUTPUT
#endif // !NeXT_PDO
}

PRIVATE 
boolean_t
access_name_match(pattern, name)
    char		*pattern;
    char 		*name;
{
    char *part;
    unsigned int num_wild;
    while (1) {
        if (*name == *pattern || (NULL_CHAR != *name && WILD_CHAR_INT == *pattern)) {
            if (NULL_CHAR == *pattern) return TRUE;
            pattern++;
            name++;
        } else if (GLOB_CHAR_INT == *pattern) {
            num_wild = 0;
            while (GLOB_CHAR_INT == *pattern || WILD_CHAR_INT == *pattern) {
                if (WILD_CHAR_INT == *pattern) num_wild++;
                pattern++;
            }
            if (0 == num_wild && NULL_CHAR == *pattern) return TRUE;
            part = name;
            while (1) {
                if (part-name >= num_wild && *part == *pattern && access_name_match(pattern, part)) return TRUE;
                if (NULL_CHAR == *part) return FALSE;
                part++;
            }
        } else return FALSE;
    }
}

PUBLIC
boolean_t 
access_allowed(from, name)
    netaddr_t 		from;
    char 		*name;
{
    unsigned int i, j;
    access_grant_ptr_t grant;
    netaddr_t masked_address;
    from = ntohl(from);
    if (access_list) {
        for (i = 0; i < access_list_len; i++) {
            grant = access_list[i];
            if (grant && access_name_match(grant->name, name)) {
                for (j = 0; j < grant->length; j++) {
                    masked_address = from & grant->addresses[j].netmask;
                    if (grant->addresses[j].lower_address <= masked_address && masked_address <= grant->addresses[j].upper_address) return TRUE;
                }
#ifdef FIRST_MATCH_GOVERNS
                return FALSE;
#endif // FIRST_MATCH_GOVERNS
            }
        }
    }
    return NO_MATCH_DEFAULT;
}

/* for debugging purposes only */
PRIVATE
void
access_print(stream)
    FILE 		*stream;
{
    unsigned int i, j;
    access_grant_ptr_t grant;
    struct in_addr in;
    if (access_list) {
        for (i = 0; i < access_list_len; i++) {
            grant = access_list[i];
            if (grant) {
                fprintf(stream, "%s:", grant->name);
                for (j = 0; j < grant->length; j++) {
                    in.s_addr = htonl(grant->addresses[j].lower_address);
                    fprintf(stream, j ? ", %s" : " %s", inet_ntoa(in));
                    in.s_addr = htonl(grant->addresses[j].upper_address);
                    fprintf(stream, "-%s", inet_ntoa(in));
                    in.s_addr = htonl(grant->addresses[j].netmask);
                    fprintf(stream, "/%s", inet_ntoa(in));
                }
                fprintf(stream, "\n");
            }
        }
    }
}
