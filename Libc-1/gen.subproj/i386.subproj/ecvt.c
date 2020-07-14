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
/*
 *	ecvt converts to decimal
 *	the number of digits is specified by ndigit
 *	decptp is set to the position of the decimal point
 *	signp is set to 0 for positive, 1 for negative
 */


static double ecvt_rint(double x);
static double ecvt_copysign(double x, double y);

static char *cvt();

/* big enough to handle %.20f conversion of 1e308 */
#define	NDIG		350

char*
ecvt(arg, ndigits, decptp, signp)
double arg;
int ndigits, *decptp, *signp;
{
	return(cvt(arg, ndigits, decptp, signp, 1));
}

char*
fcvt(arg, ndigits, decptp, signp)
double arg;
int ndigits, *decptp, *signp;
{
	return(cvt(arg, ndigits, decptp, signp, 0));
}

static char*
cvt(arg, ndigits, decptp, signp, eflag)
double arg;
int ndigits, *decptp, *signp;
int eflag;
{
	register int decpt;
	double fi, fj;
	register char *p, *p1;
	static char buf[NDIG] = { 0 };
	double modf();

	if (ndigits < 0)
		ndigits = 0;
	if (ndigits >= NDIG-1)
		ndigits = NDIG-2;

	decpt = 0;
	*signp = 0;
	p = &buf[0];

	if (arg == 0) {
		*decptp = 0;
		while (p < &buf[ndigits])
			*p++ = '0';
		*p = '\0';
		return(buf);
	} else if (arg < 0) {
		*signp = 1;
		arg = -arg;
	}

	arg = modf(arg, &fi);
	p1 = &buf[NDIG];

	/*
	 * Do integer part
	 */
	if (fi != 0) {
		while (fi != 0) {
			fj = modf(fi/10, &fi);
			/**--p1 = (int)((fj+.03)*10) + '0';*/
			*--p1 = (int)ecvt_rint((fj)*10) + '0';
			decpt++;
		}
		while (p1 < &buf[NDIG])
			*p++ = *p1++;
	} else if (arg > 0) {
		while ((fj = arg*10) < 1) {
			arg = fj;
			decpt--;
		}
	}
	*decptp = decpt;

	/*
	 * do fraction part
	 * p pts to where fraction should be concatenated
	 * p1 is how far conversion must go to
	 */
	p1 = &buf[ndigits];
	if (eflag==0) {
		/* fcvt must provide ndigits after decimal pt */
		p1 += decpt;
		/* if decpt was negative, we might done for fcvt */
		if (p1 < &buf[0]) {
			buf[0] = '\0';
			return(buf);
		}
	}
	while (p <= p1 && p < &buf[NDIG]) {
		arg *= 10;
		arg = modf(arg, &fj);
		*p++ = (int)fj + '0';
	}
	/*
	 * if we converted all the way to the end of the
	 * buf, don't mess with rounding since there's nothing
	 * significant out here anyway
	 */
	if (p1 >= &buf[NDIG]) {
		buf[NDIG-1] = '\0';
		return(buf);
	}
	/*
	 * round by adding 5 to last digit and propagating
	 * carries
	 */
	p = p1;
	*p1 += 5;
	while (*p1 > '9') {
		*p1 = '0';
		if (p1 > buf)
			++*--p1;
		else {
			*p1 = '1';
			(*decptp)++;
			if (eflag == 0) {
				if (p > buf)
					*p = '0';
				p++;
			}
		}
	}
	*p = '\0';
	return(buf);
}

#ifndef NeXT
static double L = 4503599627370496.0E0;		/* 2**52 */
static int ecvt_init = 0;
#pragma CC_OPT_OFF
static double ecvt_rint(x)
double x;
{
	double s,t,one = 1.0;
	
	if (ecvt_init == 0) {
		int i;
		L = 1.0;
		for (i = 52; i; i--)
			L *= 2.0;
		ecvt_init = 1;
	}
	if (x != x)				/* NaN */
		return (x);
	if (ecvt_copysign(x,one) >= L)		/* already an integer */
	    return (x);
	s = ecvt_copysign(L,x);
	t = x + s;				/* x+s rounded to integer */
	return (t - s);
}

#define msign ((unsigned short)0x7fff)
#define mexp ((unsigned short)0x7ff0)

static double ecvt_copysign(x,y)
double x,y;
{
        unsigned short  *px=(unsigned short *) &x,
                        *py=(unsigned short *) &y;
        *px = ( *px & msign ) | ( *py & ~msign );
        return(x);
}
#pragma CC_OPT_ON
#else
static double ecvt_rint(double x)
{
	asm("frndint" : "=t" (x) :  "0" (x));
	return(x);
}
#endif /* NeXT */
