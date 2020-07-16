/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <limits.h>
#include <locale.h>

static char empty[] = "";

/*
 * Default (C) locale conversion.
 */
static struct lconv C_lconv = {
	".",			/* decimal_point */
	empty,			/* thousands_sep */
	empty,			/* grouping */
	empty,			/* int_curr_symbol */
	empty,			/* currency_symbol */
	empty,			/* mon_decimal_point */
	empty,			/* mon_thousands_sep */
	empty,			/* mon_grouping */
	empty,			/* positive_sign */
	empty,			/* negative_sign */
	CHAR_MAX,		/* int_frac_digits */
	CHAR_MAX,		/* frac_digits */
	CHAR_MAX,		/* p_cs_precedes */
	CHAR_MAX,		/* p_sep_by_space */
	CHAR_MAX,		/* n_cs_precedes */
	CHAR_MAX,		/* n_sep_by_space */
	CHAR_MAX,		/* p_sign_posn */
	CHAR_MAX,		/* n_sign_posn */
};

/*
 * Current locale conversion.
 */
struct lconv *__lconv = &C_lconv;