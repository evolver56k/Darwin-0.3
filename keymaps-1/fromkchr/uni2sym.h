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
#
#       Name:             Adobe Symbol Encoding to Unicode
#       Unicode version:  1.1
#       Table version:    0.1
#       Table format:     Format A
#       Date:             05 May 1995
#
#       Copyright (c) 1991-1995 Unicode, Inc.  All Rights reserved.
#
#       This file is provided as-is by Unicode, Inc. (The Unicode Consortium).
#       No claims are made as to fitness for any particular purpose.  No
#       warranties of any kind are expressed or implied.  The recipient
#       agrees to determine applicability of information provided.  If this
#       file has been provided on magnetic media by Unicode, Inc., the sole
#       remedy for any claim will be exchange of defective media within 90
#       days of receipt.
#
#       Recipient is granted the right to make copies in any form for
#       internal distribution and to freely use the information supplied
#       in the creation of products supporting Unicode.  Unicode, Inc.
#       specifically excludes the right to re-distribute this file directly
#       to third parties or other organizations whether for profit or not.
#
#       Format:  Three tab-separated columns
#                Column #1 is the Unicode code (in hex)
#                Column #2 is the Adobe code (in hex)
#                Column #3 # Unicode name <tab> # Adobe name
#
#       General notes:  There are a number of glyph parts in the Adobe Symbol
#       Encoding which are not in the Unicode set.  They are as follows:
#
#       BD	arrowvertex
#       BE	arrowhorizex
#       E6	parenlefttp
#       E7	parenleftex
#       E8	parenleftbt
#       E9	bracketlefttp
#       EA	bracketleftex
#       EB	bracketleftbt
#       EC	bracelefttp
#       ED	braceleftmid
#       EE	braceleftbt
#       EF	braceex
#       F4	integralex
#       F6	parenrighttp
#       F7	parenrightex
#       F8	parenrightbt
#       F9	bracketrighttp
#       FA	bracketrightex
#       FB	bracketrightbt
#       FC	bracerighttp
#       FD	bracerightmid
#       FE	bracerightbt
#
#       Any comments or questions contact: unicode-inc@unicode.org
#
#
*/

static unsigned int uni2sym[] = {

#if 0

0020, 0x20,       //SPACE,       //space
0x0021, 0x21,       //EXCLAMATION MARK,       //exclam
0x0023, 0x23,       //NUMBER SIGN,       //numbersign
0x0025, 0x25,       //PERCENT SIGN,       //percent
0x0026, 0x26,       //AMPERSAND,       //ampersand
0x0028, 0x28,       //OPENING PARENTHESIS,       //parenleft
0x0029, 0x29,       //CLOSING PARENTHESIS,       //parenright
0x002B, 0x2B,       //PLUS SIGN,       //plus
002C, 0x2C,       //COMMA,       //comma
0x002E, 0x2E,       //PERIOD,       //period
0x002F, 0x2F,       //SLASH,       //slash
0x0030, 0x30,       //DIGIT ZERO,       //zero
0x0031, 0x31,       //DIGIT ONE,       //one
0x0032, 0x32,       //DIGIT TWO,       //two
0x0033, 0x33,       //DIGIT THREE,       //three
0x0034, 0x34,       //DIGIT FOUR,       //four
0x0035, 0x35,       //DIGIT FIVE,       //five
0x0036, 0x36,       //DIGIT SIX,       //six
0x0037, 0x37,       //DIGIT SEVEN,       //seven
0x0038, 0x38,       //DIGIT EIGHT,       //eight
0x0039, 0x39,       //DIGIT NINE,       //nine
0x003A, 0x3A,       //COLON,       //colon
0x003B, 0x3B,       //SEMICOLON,       //semicolon
0x003C, 0x3C,       //LESS-THAN SIGN,       //less
0x003D, 0x3D,       //EQUALS SIGN,       //equal
0x003E, 0x3E,       //GREATER-THAN SIGN,       //greater
0x003F, 0x3F,       //QUESTION MARK,       //question
0x005B, 0x5B,       //OPENING SQUARE BRACKET,       //bracketleft
0x005D, 0x5D,       //CLOSING SQUARE BRACKET,       //bracketright
0x005F, 0x5F,       //SPACING UNDERSCORE,       //underscore
0x007B, 0x7B,       //OPENING CURLY BRACKET,       //braceleft
0x007C, 0x7C,       //VERTICAL BAR,       //bar
0x007D, 0x7D,       //CLOSING CURLY BRACKET,       //braceright

#endif

0x00A9, 0xD3,       //COPYRIGHT SIGN,       //copyrightserif
0x00A9, 0xE3,       //COPYRIGHT SIGN,       //copyrightsans
0x00AC, 0xD8,       //NOT SIGN,       //logicalnot
0x00AE, 0xD2,       //REGISTERED TRADE MARK SIGN,       //registeredserif
0x00AE, 0xE2,       //REGISTERED TRADE MARK SIGN,       //registeredsans
0x00B0, 0xB0,       //DEGREE SIGN,       //degree
0x00B1, 0xB1,       //PLUS-OR-MINUS SIGN,       //plusminus
0x00D7, 0xB4,       //MULTIPLICATION SIGN,       //multiply
0x00F7, 0xB8,       //DIVISION SIGN,       //divide
0x0192, 0xA6,       //LATIN SMALL LETTER SCRIPT F,       //florin
0x0391, 0x41,       //GREEK CAPITAL LETTER ALPHA,       //Alpha
0x0392, 0x42,       //GREEK CAPITAL LETTER BETA,       //Beta
0x0393, 0x47,       //GREEK CAPITAL LETTER GAMMA,       //Gamma
0x0394, 0x44,       //GREEK CAPITAL LETTER DELTA,       //Delta
0x0395, 0x45,       //GREEK CAPITAL LETTER EPSILON,       //Epsilon
0x0396, 0x5A,       //GREEK CAPITAL LETTER ZETA,       //Zeta
0x0397, 0x48,       //GREEK CAPITAL LETTER ETA,       //Eta
0x0398, 0x51,       //GREEK CAPITAL LETTER THETA,       //Theta
0x0399, 0x49,       //GREEK CAPITAL LETTER IOTA,       //Iota
0x039A, 0x4B,       //GREEK CAPITAL LETTER KAPPA,       //Kappa
0x039B, 0x4C,       //GREEK CAPITAL LETTER LAMBDA,       //Lambda
0x039C, 0x4D,       //GREEK CAPITAL LETTER MU,       //Mu
0x039D, 0x4E,       //GREEK CAPITAL LETTER NU,       //Nu
0x039E, 0x58,       //GREEK CAPITAL LETTER XI,       //Xi
0x039F, 0x4F,       //GREEK CAPITAL LETTER OMICRON,       //Omicron
0x03A0, 0x50,       //GREEK CAPITAL LETTER PI,       //Pi
0x03A1, 0x52,       //GREEK CAPITAL LETTER RHO,       //Rho
0x03A3, 0x53,       //GREEK CAPITAL LETTER SIGMA,       //Sigma
0x03A4, 0x54,       //GREEK CAPITAL LETTER TAU,       //Tau
0x03A5, 0x55,       //GREEK CAPITAL LETTER UPSILON,       //Upsilon
0x03A6, 0x46,       //GREEK CAPITAL LETTER PHI,       //Phi
0x03A7, 0x43,       //GREEK CAPITAL LETTER CHI,       //Chi
0x03A8, 0x59,       //GREEK CAPITAL LETTER PSI,       //Psi
0x03A9, 0x57,       //GREEK CAPITAL LETTER OMEGA,       //Omega
0x03B1, 0x61,       //GREEK SMALL LETTER ALPHA,       //alpha
0x03B2, 0x62,       //GREEK SMALL LETTER BETA,       //beta
0x03B3, 0x67,       //GREEK SMALL LETTER GAMMA,       //gamma
0x03B4, 0x64,       //GREEK SMALL LETTER DELTA,       //delta
0x03B5, 0x65,       //GREEK SMALL LETTER EPSILON,       //epsilon
0x03B6, 0x7A,       //GREEK SMALL LETTER ZETA,       //zeta
0x03B7, 0x68,       //GREEK SMALL LETTER ETA,       //eta
0x03B8, 0x71,       //GREEK SMALL LETTER THETA,       //theta
0x03B9, 0x69,       //GREEK SMALL LETTER IOTA,       //iota
0x03BA, 0x6B,       //GREEK SMALL LETTER KAPPA,       //kappa
0x03BB, 0x6C,       //GREEK SMALL LETTER LAMBDA,       //lambda
0x03BC, 0x6D,       //GREEK SMALL LETTER MU,       //mu
0x03BD, 0x6E,       //GREEK SMALL LETTER NU,       //nu
0x03BE, 0x78,       //GREEK SMALL LETTER XI,       //xi
0x03BF, 0x6F,       //GREEK SMALL LETTER OMICRON,       //omicron
0x03C0, 0x70,       //GREEK SMALL LETTER PI,       //pi
0x03C1, 0x72,       //GREEK SMALL LETTER RHO,       //rho
0x03C2, 0x56,       //GREEK SMALL LETTER FINAL SIGMA,       //sigma1
0x03C3, 0x73,       //GREEK SMALL LETTER SIGMA,       //sigma
0x03C4, 0x74,       //GREEK SMALL LETTER TAU,       //tau
0x03C5, 0x75,       //GREEK SMALL LETTER UPSILON,       //upsilon
0x03C6, 0x66,       //GREEK SMALL LETTER PHI,       //phi
0x03C7, 0x63,       //GREEK SMALL LETTER CHI,       //chi
0x03C8, 0x79,       //GREEK SMALL LETTER PSI,       //psi
0x03C9, 0x77,       //GREEK SMALL LETTER OMEGA,       //omega
0x03D1, 0x4A,       //GREEK SMALL LETTER SCRIPT THETA,       //theta1
0x03D2, 0xA1,       //GREEK CAPITAL LETTER UPSILON HOOK,       //Upsilon1
0x03D5, 0x6A,       //GREEK SMALL LETTER SCRIPT PHI,       //phi1
0x03D6, 0x76,       //GREEK SMALL LETTER OMEGA PI,       //omega1
0x2022, 0xB7,       //BULLET,       //bullet
0x2026, 0xBC,       //HORIZONTAL ELLIPSIS,       //ellipsis
0x2032, 0xA2,       //PRIME,       //minute
0x2033, 0xB2,       //DOUBLE PRIME,       //second
0x203E, 0x60,       //SPACING OVERSCORE,       //radicalex
0x2044, 0xA4,       //FRACTION SLASH,       //fraction
0x2111, 0xC1,       //BLACK-LETTER I,       //Ifraktur
0x2118, 0xC3,       //SCRIPT P,       //weierstrass
0x211C, 0xC2,       //BLACK-LETTER R,       //Rfraktur
0x2122, 0xD4,       //TRADEMARK,       //trademarkserif
0x2122, 0xE4,       //TRADEMARK,       //trademarksans
0x2126, 0x57,       //OHM,       //Omega
0x2135, 0xC0,       //FIRST TRANSFINITE CARDINAL,       //aleph
0x2190, 0xAC,       //LEFT ARROW,       //arrowleft
0x2191, 0xAD,       //UP ARROW,       //arrowup
0x2192, 0xAE,       //RIGHT ARROW,       //arrowright
0x2193, 0xAF,       //DOWN ARROW,       //arrowdown
0x2194, 0xAB,       //LEFT RIGHT ARROW,       //arrowboth
0x21B5, 0xBF,       //DOWN ARROW WITH CORNER LEFT,       //carriagereturn
0x21D0, 0xDC,       //LEFT DOUBLE ARROW,       //arrowdblleft
0x21D1, 0xDD,       //UP DOUBLE ARROW,       //arrowdblup
0x21D2, 0xDE,       //RIGHT DOUBLE ARROW,       //arrowdblright
0x21D3, 0xDF,       //DOWN DOUBLE ARROW,       //arrowdbldown
0x21D4, 0xDB,       //LEFT RIGHT DOUBLE ARROW,       //arrowdblboth
0x2200, 0x22,       //FOR ALL,       //universal
0x2202, 0xB6,       //PARTIAL DIFFERENTIAL,       //partialdiff
0x2203, 0x24,       //THERE EXISTS,       //existential
0x2205, 0xC6,       //EMPTY SET,       //emptyset
0x2206, 0x44,       //INCREMENT,       //Delta
0x2207, 0xD1,       //NABLA,       //gradient
0x2208, 0xCE,       //ELEMENT OF,       //element
0x2209, 0xCF,       //NOT AN ELEMENT OF,       //notelement
0x220B, 0x27,       //CONTAINS AS MEMBER,       //suchthat
0x220F, 0xD5,       //N-ARY PRODUCT,       //product
0x2211, 0xE5,       //N-ARY SUMMATION,       //summation
0x2212, 0x2D,       //MINUS SIGN,       //minus
0x2215, 0xA4,       //DIVISION SLASH,       //fraction
0x2217, 0x2A,       //ASTERISK OPERATOR,       //asteriskmath
0x221A, 0xD6,       //SQUARE ROOT,       //radical
0x221D, 0xB5,       //PROPORTIONAL TO,       //proportional
0x221E, 0xA5,       //INFINITY,       //infinity
0x2220, 0xD0,       //ANGLE,       //angle
0x2227, 0xD9,       //LOGICAL AND,       //logicaland
0x2228, 0xDA,       //LOGICAL OR,       //logicalor
0x2229, 0xC7,       //INTERSECTION,       //intersection
0x222A, 0xC8,       //UNION,       //union
0x222B, 0xF2,       //INTEGRAL,       //integral
0x2234, 0x5C,       //THEREFORE,       //therefore
0x223C, 0x7E,       //TILDE OPERATOR,       //similar
0x2245, 0x40,       //APPROXIMATELY EQUAL TO,       //congruent
0x2248, 0xBB,       //ALMOST EQUAL TO,       //approxequal
0x2260, 0xB9,       //NOT EQUAL TO,       //notequal
0x2261, 0xBA,       //IDENTICAL TO,       //equivalence
0x2264, 0xA3,       //LESS THAN OR EQUAL TO,       //lessequal
0x2265, 0xB3,       //GREATER THAN OR EQUAL TO,       //greaterequal
0x2282, 0xCC,       //SUBSET OF,       //propersubset
0x2283, 0xC9,       //SUPERSET OF,       //propersuperset
0x2284, 0xCB,       //NOT A SUBSET OF,       //notsubset
0x2286, 0xCD,       //SUBSET OF OR EQUAL TO,       //reflexsubset
0x2287, 0xCA,       //SUPERSET OF OR EQUAL TO,       //reflexsuperset
0x2295, 0xC5,       //CIRCLED PLUS,       //circleplus
0x2297, 0xC4,       //CIRCLED TIMES,       //circlemultiply
0x22A5, 0x5E,       //UP TACK,       //perpendicular
0x22C5, 0xD7,       //DOT OPERATOR,       //dotmath
0x2320, 0xF3,       //TOP HALF INTEGRAL,       //integraltp
0x2321, 0xF5,       //BOTTOM HALF INTEGRAL,       //integralbt
0x2329, 0xE1,       //BRA,       //angleleft
0x232A, 0xF1,       //KET,       //angleright
0x25CA, 0xE0,       //LOZENGE,       //lozenge
0x2660, 0xAA,       //BLACK SPADE SUIT,       //spade
0x2663, 0xA7,       //BLACK CLUB SUIT,       //club
0x2665, 0xA9,       //BLACK HEART SUIT,       //heart
0x2666, 0xA8,       //BLACK DIAMOND SUIT,       //diamond

};