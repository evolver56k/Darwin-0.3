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

/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *      File:   machdep/i386/fault_copy.c
 *      Author: Bruce Martin, NeXT Computer, Inc.
 *
 *      This file contains machine dependent code for kernel user copy
 *      on NeXT i386-based products.  Currently tuned for the i486.
 *
 * HISTORY
 * Oct-1-92	Bruce Martin (Bruce_Martin@NeXT.COM)
 *	Created.
 */

#import <mach/mach_types.h>
#import <sys/errno.h>

#define MIN_TO_ALIGN    16
#define UNROLL          4

/*
 * Bytes to be moved by an unrolled loop
 */
#define LOOP_STRIDE(type)       (UNROLL * sizeof(type))

/*
 * Len modulo LOOP_STRIDE
 */
#define MODULO_LOOP_UNROLL(len, type) \
        ((len) & (LOOP_STRIDE(type) - 1) & ~(sizeof(type) - 1))

/*
 * Convert a void * into a unsigned int so arithmetic can be done
 * (Technically, the gnu C compiler is tolerant of arithmetic
 * on void *, but ansi isn't; so we do this.)
 */
#define UNS(voidp)      ((unsigned int)(voidp))

/*
 * Number of bytes addr is past an integer alignment
 */
#define BYTES_PAST_INT_ALIGNMENT(voidp) (UNS(voidp) & (sizeof(int) - 1))


/*
 * Convert a 'void * + offset' into a char, short, or int reference
 * based at that address.
 */
#define CHAR(voidp, offset)	(*(char *)(UNS(voidp) + (offset)))
#define SHORT(voidp, offset)	(*(short *)(UNS(voidp) + (offset)))
#define INT(voidp, offset)	(*(int *)(UNS(voidp) + (offset)))

#if defined(NX_CURRENT_COMPILER_RELEASE) && (NX_CURRENT_COMPILER_RELEASE < 320)
#define SIREG "e"
#else
#define SIREG "S"
#endif



/*
 * Maintain the recover field in the thread_t structure
 */
static inline void
set_recover(int (*vector)())
{
    current_thread()->recover = (vm_offset_t)vector;
    return;
}

static inline void
clear_recover()
{
    current_thread()->recover = 0;
}



/*
 * Inline assembler macros used to build the copy routines
 *
 * The first set of routines move a single character or word.
 */
static inline void
_char_move_out(char *dst, char val)
{
    asm volatile (
	"movb %0, %%fs:%1"
	: /* no outputs */
	: "q" (val), "m" (*dst));
}

static inline void
_short_move_out(short *dst, short val)
{
    asm volatile (
	"movw %0, %%fs:%1"
	: /* no outputs */
	: "q" (val), "m" (*dst));
}

static inline void
_word_move_out(int *dst, int val)
{
    asm volatile (
	"movl %0, %%fs:%1"
	: /* no outputs */
	: "r" (val), "m" (*dst));
}

static inline char
_char_move_in(char *src)
{
    char val;
    asm volatile (
	"movb %%fs:(%1), %0"
	: "=q" (val)
	: "r" (src));
    return val;
}

static inline int
_word_move_in(int *src)
{
    int val;
    asm volatile (
	"movl %%fs:(%1), %0"
	: "=r" (val)
	: "r" (src));
    return val;
}



/*
 * This set of routines move multiple words or characters
 */
static inline void
_fwd_char_copy(void *dst, const void *src, int len)
{
    asm("rep; movsb"
	: /* no outputs */
	: "&c" (len), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}

static inline void
_fwd_char_copy_in(void *dst, const void *src, int len)
{
    asm("rep; movsb %%fs:(%%esi), (%%edi)"
	: /* no outputs */
	: "&c" (len), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}

/* NOTE: we can't use rep;movsb because of segment override restrictions */
static inline void
_fwd_char_copy_out(void *dst, const void *src, int len)
{
    if (len & 0x1) {
	_char_move_out(dst, CHAR(src, 0));
	UNS(dst) += sizeof(char);
	UNS(src) += sizeof(char);
    }
    if (len & 0x2) {
	_short_move_out(dst, SHORT(src, 0));
	UNS(dst) += sizeof(short);
	UNS(src) += sizeof(short);
    }
    len >>= 2;
    while (len--) {
	_word_move_out(dst, INT(src, 0));
	UNS(dst) += sizeof(int);
	UNS(src) += sizeof(int);
    }
}


static inline void
_bkwd_char_copy(void *dst, const void *src, int len)
{
    /* NOTE: assumes DF flag is set */
    UNS(src)--;
    UNS(dst)--;
    asm("rep; movsb"
	: /* no outputs */
	: "&c" (len), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}


static inline void
_fwd_int_copy(void *dst, const void *src, int len)
{
    asm("rep; movsl"
	: /* no outputs */
	: "&c" (len>>2), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}

static inline void
_fwd_int_copy_in(void *dst, const void *src, int len)
{
    asm("rep; movsl %%fs:(%%esi), (%%edi)"
	: /* no outputs */
	: "&c" (len>>2), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}

/* NOTE: can't use rep;movsl because of segment override restrictions. */
static inline void
_fwd_int_copy_out(void *dst, const void *src, int len)
{
    int alignment;
    alignment = MODULO_LOOP_UNROLL(len, int);
    UNS(src) += alignment - LOOP_STRIDE(int);
    UNS(dst) += alignment - LOOP_STRIDE(int);
    switch (alignment) {
	do {
		UNS(src) += LOOP_STRIDE(int);
		UNS(dst) += LOOP_STRIDE(int);
		_word_move_out(&((int *)dst)[0], INT(src, 0));
    case 12:    _word_move_out(&((int *)dst)[1], INT(src, 4));
    case 8:     _word_move_out(&((int *)dst)[2], INT(src, 8));
    case 4:     _word_move_out(&((int *)dst)[3], INT(src, 12));
    case 0:
/*
		INT(dst, 0) = INT(src, 0);
    case 12:    INT(dst, 4) = INT(src, 4);
    case 8:     INT(dst, 8) = INT(src, 8);
    case 4:     INT(dst, 12) = INT(src, 12);
    case 0:
*/
		len -= LOOP_STRIDE(int);
	} while (len >= 0);
    }
}

static inline void
_bkwd_int_copy(void *dst, const void *src, int len)
{
    /* NOTE: assumes DF flag is set */
    UNS(dst) -= sizeof(int);
    UNS(src) -= sizeof(int);
    asm("rep; movsl"
	: /* no outputs */
	: "&c" (len>>2), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}






/*
 * Copy count bytes from %fs:(src) to (dst)
 */
inline
int copyin(char *src, char *dst, vm_size_t count)
{
    int len = count;
    int need_to_move;

    set_recover(&&do_fault);

    /* Always do a forward copy */

    if (len < MIN_TO_ALIGN) {
	_fwd_char_copy_in(dst, src, len);
	return 0;
    }

    if (need_to_move = BYTES_PAST_INT_ALIGNMENT(src)) {
	need_to_move = sizeof(int) - need_to_move;
	_fwd_char_copy_in(dst, src, need_to_move);
	len -= need_to_move;
	UNS(dst) += need_to_move;
	UNS(src) += need_to_move;
    }

    _fwd_int_copy_in(dst, src, len);

    if (need_to_move = (len & 3)) {
	UNS(src) += (len & ~3);
	UNS(dst) += (len & ~3);
	switch(need_to_move) {
	    case 3: dst[2] = _char_move_in(&src[2]);
	    case 2: dst[1] = _char_move_in(&src[1]);
	    case 1: dst[0] = _char_move_in(&src[0]);
	}
    }
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return EFAULT;
}

int copyinmsg(char *src, char *dst, vm_size_t count)
{
    return copyin(src, dst, count);
}

/*
 * Copy count bytes from (src) to (dst)
 */
int copywithin(char *src, char *dst, vm_size_t count)
{
    int len = count;
    int need_to_move;

    set_recover(&&do_fault);

    /* Always do a forward copy */

    if (len < MIN_TO_ALIGN) {
	_fwd_char_copy(dst, src, len);
	return 0;
    }

    if (need_to_move = BYTES_PAST_INT_ALIGNMENT(src)) {
	need_to_move = sizeof(int) - need_to_move;
	_fwd_char_copy(dst, src, need_to_move);
	len -= need_to_move;
	UNS(dst) += need_to_move;
	UNS(src) += need_to_move;
    }

    _fwd_int_copy(dst, src, len);

    if (need_to_move = (len & 3)) {
	UNS(src) += (len & ~3);
	UNS(dst) += (len & ~3);
	switch(need_to_move) {
	    case 3: dst[2] = src[2];
	    case 2: dst[1] = src[1];
	    case 1: dst[0] = src[0];
	}
    }
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return EFAULT;
}


/*
 * Copy count bytes from (src) to %fs:(dst)
 */
inline
int copyout(char *src, char *dst, vm_size_t count)
{
    int len = count;
    int need_to_move;

    set_recover(&&do_fault);

    /* Always do a forward copy */

    if (len < MIN_TO_ALIGN) {
	_fwd_char_copy_out(dst, src, len);
	return 0;
    }

    if (need_to_move = BYTES_PAST_INT_ALIGNMENT(src)) {
	need_to_move = sizeof(int) - need_to_move;
	_fwd_char_copy_out(dst, src, need_to_move);
	len -= need_to_move;
	UNS(dst) += need_to_move;
	UNS(src) += need_to_move;
    }

    _fwd_int_copy_out(dst, src, len);

    if (need_to_move = (len & 3)) {
	UNS(src) += (len & ~3);
	UNS(dst) += (len & ~3);
	switch(need_to_move) {
	    case 3: _char_move_out(&dst[2], src[2]);
	    case 2: _char_move_out(&dst[1], src[1]);
	    case 1: _char_move_out(&dst[0], src[0]);
	}
    }
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return EFAULT;
}

int copyoutmsg(char *src, char *dst, vm_size_t count)
{
    return copyout(src, dst, count);
}


/*
 * copy up to max bytes from src to dst.  Return number of bytes copied in len.
 */
int copystr(const char *src, char *dst, int max, int *len)
{
    int i = max;

    set_recover(&&do_fault);
    while (i-- > 0) {
	if (!(*dst++ = *src++))
	    break;
    }

    if (len)
	*len = max - i;
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return EFAULT;
}


/*
 * Same as copystr, but moves bytes from %fs:src to dst
 */
int copyinstr(char *src, char *dst, int max, int *len)
{
    int i = max;

    set_recover(&&do_fault);
    while (i-- > 0) {
	char c;
	c = dst[0] = _char_move_in(src);
	dst++;
	src++;
	if (!c)
	    break;
    }

    if (len)
	*len = max - i;
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return EFAULT;
}


/*
 * Same as copystr, but moves bytes from src to %fs:dst
 */
int copyoutstr(char *src, char *dst, int max, int *len)
{
    int i = max;

    set_recover(&&do_fault);
    while (i-- > 0) {
	char c;
	c = *src;
	_char_move_out(dst, c);
	dst++;
	src++;
	if (!c)
	    break;
    }

    if (len)
	*len = max - i;
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return EFAULT;
}




int fuword(int *addr)
{
    int val;
    set_recover(&&do_fault);
    val = _word_move_in(addr);
    clear_recover();
    return val;

do_fault:
    clear_recover();
    return -1;
}


char fubyte(char *addr)
{
    int val;

    set_recover(&&do_fault);
    val = _char_move_in(addr);
    clear_recover();
    return val;

do_fault:
    clear_recover();
    return -1;
}


char fuibyte(char *addr)
{
    int val;

    set_recover(&&do_fault);
    val = _char_move_in(addr);
    clear_recover();
    return val;

do_fault:
    clear_recover();
    return -1;
}



int suword(int *addr, int val)
{
    set_recover(&&do_fault);
    _word_move_out(addr, val);
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return -1;
}


int subyte(char *addr, char val)
{
    set_recover(&&do_fault);
    _char_move_out(addr, val);
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return -1;
}


int
suibyte(char *addr, char val)
{
    set_recover(&&do_fault);
    _char_move_out(addr, val);
    clear_recover();
    return 0;

do_fault:
    clear_recover();
    return -1;
}
