/*	NXMethodSignature.h
	Copyright 1989,1991, 1992, 1997 NeXT Software, Inc.

 	3-Jan-1997 Umesh Vaishampayan (umeshv@NeXT.com)
		Ported to ppc.
*/


#import <appkit/NXPortPortal.h>

/*****************		NXMethodSignature 		**********************
 * ObjC does not keep type information with the selectors, so remote
 * objects needs to acquire and keep this info as appropriate.
 *
 * This class encodes/decodes information from/to the stack frame to/from
 * a <Portal> buffer using the type information supplied.
 **********************************************************************/

#if	defined(hppa) || defined(sparc) || defined(ppc)
/*
 * HPPA always returns structs as pointers (4 bytes), so
 * we use long long as the result value instead which will
 * get returned in registers.
 *
 * Ditto for sparc and ppc
 */
typedef long long two_int_t;
#else	/* hppa || sparc || ppc */
typedef struct {
	int	int0;
	int	int1;
}
	two_int_t;
#endif	/* hppa || sparc || ppc */

typedef struct {
	int	offset;
	int	size;
	char	*type;
} parameter_info_t;

@interface NXMethodSignature: Object {
@public
	struct objc_method_description	sig;
	const char *selName;
	int			nargs;
	unsigned	sizeofParams;
	parameter_info_t	*parmInfoP;
}

+ fromDescription:(struct objc_method_description *)method_desc fromZone:(NXZone *)zone;

- encodeMethodParams:(void *)args onto:(id <NXPortal>)stream;
	/* encode the method frame onto stream (excluding self and sel) */
	
- (void *)decodeMethodParamsFrom:(id <NXPortal>)stream;
	/* decode the method frame from stream;
	return a freshly malloced pointer, never NULL (unless error) */

- encodeMethodRet:(two_int_t *)result withargs:(void *)args onto:(id  <NXPortal>)stream;
	/* encode the method return value and any "out" args */
	
- (two_int_t) decodeMethodRetFrom:(id <NXPortal>)stream withargs:(void *)args atAddr:(void *)struct_addr;
	/* decode the return value and any "out" args */

- (BOOL) isOneway;
	/* is this method asynchronous? */

- (char *) methodReturnType;

- (void *) methodReturnArea;
	/* allocate memory for a return structure */

@end
