/* $RCSfile: INTERN.h,v $$Revision: 1.1.1.1 $$Date: 1999/04/23 01:28:57 $
 *
 *    Copyright (c) 1991-1997, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * $Log: INTERN.h,v $
 * Revision 1.1.1.1  1999/04/23 01:28:57  wsanchez
 * Import of perl 5.004_04
 *
 * Revision 1.1.1.1  1998/08/12 17:33:15  wsanchez
 * Import of perl 5.004_04
 *
 */

#undef EXT
#define EXT

#undef INIT
#define INIT(x) = x

#define DOINIT
