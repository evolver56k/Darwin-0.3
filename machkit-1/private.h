/*	private.h
	Copyright  1992, 1997 NeXT Software, Inc.
	Blaine  1992

	This file contains private definitions etc.
	and is not intended to be distributed outside NeXT Software, Inc.

 	3-Jan-1997 Umesh Vaishampayan (umeshv@NeXT.com)
		Ported to ppc.
*/
#import <appkit/NXProxy.h>

extern int getSizeOf(const char *type, unsigned int *align);	// runtime? XXX
extern void repackDescription(struct objc_method_description *method_desc, struct objc_method_description *m68k_method_desc);


enum objc_types {
	IN = 'n',
	OUT = 'o',
	INOUT = 'N',
	ONEWAY = 'V',
	BYCOPY = 'O',
	POINTER = '^',
	CONST = 'r',
};

typedef struct { @defs(NXProxy) } proxy_t;

/* configuration options */

#if defined(m68k) || defined(i386) || defined(hppa) || defined(sparc) || defined(ppc)
#define	STRUCT_RETURN_OK	1
#define PRIVATE_MSG_SENDV	0
#else
#define	STRUCT_RETURN_OK	0
#define PRIVATE_MSG_SENDV	0
#endif

/* if true, method_descriptions sent over the wire will look like they
 * originated on an m68k.  3.0 m68k code can only handle this style info.
 * This code does not compute offsets of chars & shorts correctly !!!!
 */
#define USE68KOFFSETS 0


#define STRIP_NEGATIVE_OFFSETS 1
