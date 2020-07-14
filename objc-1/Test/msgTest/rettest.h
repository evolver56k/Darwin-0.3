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
/* rettest.h:
 * Definitions used in classes which test correct operation
 * for methods of all possible return types
 */

/* structure types */

/* structure which is 16 bits wide */
struct S_BITS_16 { char buf[2]; };

/* structure which is 32 bits wide */
struct S_BITS_32 { char buf[4]; };

/* structure which is 64 bits wide */
struct S_BITS_64 { char buf[8]; };

/* structure wider than 64 bits */
struct S_BITS_BIG { char buf[64]; };

/* union which is 16 bits wide */
union U_BITS_16 { struct S_BITS_16 mem; short int s; };

/* union which is 32 bits wide */
union U_BITS_32 { struct S_BITS_32 mem; long int i; };

/* union which is 64 bits wide */
union U_BITS_64 { struct S_BITS_64 mem; double d; };

/* union wider than 64 bits */
union U_BITS_BIG { struct S_BITS_BIG mem; char buf[64]; };

/* enum which only needs 8 bits to represent values */
enum E_BITS_8 { e8_one=0, e8_two=127 };

/* enum which only needs 16 bits to represent values */
enum E_BITS_16 { e16_one=0, e16_two=32767 };

#ifdef INT_32
/* enum which needs 32 bits to represent values
 * only use this if 'INT_32' is defined
 * really need 'sizeof' operator in preprocessor!!!!
 */
enum E_BITS_32 { e32_one=0, e32_two=100000L };
typedef enum E_BITS_32 E_BITS_32_t;
#endif

/* macros to generate methods testing various return types
 * These macros require V7 preprocessor abilities: formal
 * expansion inside string literals, token concatenation
 * by elimination of comments.
 */

/* Define a method to return value <VAL> as type <TYP>,
 * with optional local declarations <DECLS>.  This expects
 * the macro <THIS_CLASS> has already been defined as the
 * factory id for the current class.
 */
#define makeMethod(TYP,VAL,DECLS) \
- (TYP) ret_ ## TYP:(TYP*)retRef { \
	DECLS ; \
	doArgCheck((Class)THIS_CLASS,@selector(ret_ ## TYP:),self,_cmd);\
	*retRef = VAL; return VAL; }

/* Define a method to return a value of type <TYP>
 * gotten by passing the message on to "super"
 */
#define makeSuperMethod(TYP) \
- (TYP) ret_ ## TYP:(TYP*)retRef { \
	doArgCheck((Class)THIS_CLASS,@selector(ret_ ## TYP:),self,_cmd);\
	return [super ret_ ## TYP:retRef]; }

/* Macro to print a diagnostic complaining about
 * a return value other than expected.
 */
#define RETERR(FMT,V1,V2) \
	retError(FMT,__FILE__,__LINE__,V1,V2)
#define RETERR4(FMT,V1,V2,V3,V4) \
	retError(FMT,__FILE__,__LINE__,V1,V2,V3,V4)

/* Macro to declare instances of various types
 * for return value testing.
 */
#if 0
#define RETDECL(TYP)	TYP TYP/**/_value, TYP/**/_refValue
#endif

#define RETDECL(TYP)	TYP TYP ## _value, TYP ## _refValue

/* Macro to call a method and test the return
 */
#if 0
#define RETTEST(CLS,TYP) \
	((TYP/**/_value = [CLS ret_/**/TYP:&TYP/**/_refValue]), \
		(TYP/**/_value != TYP/**/_refValue))

#define SRETTEST(CLS,TYP) \
	((TYP/**/_value = [CLS ret_/**/TYP:&TYP/**/_refValue]), \
		retCompare(&TYP/**/_value, &TYP/**/_refValue, sizeof(TYP)))
#endif
#define RETTEST(CLS,TYP) \
	((TYP ## _value = [CLS ret_ ## TYP:&TYP ## _refValue]), \
		(TYP ## _value != TYP ## _refValue))

#define SRETTEST(CLS,TYP) \
	((TYP ## _value = [CLS ret_ ## TYP:&TYP ## _refValue]), \
		retCompare(&TYP ## _value, &TYP ## _refValue, sizeof(TYP)))

/* Macro to set what we're about to test for fault recovery */
extern char* currentTest, *currentFile;
extern int currentLine;
#define NEXTTEST(STR) \
	(currentTest = STR, currentFile = __FILE__, currentLine = __LINE__)


/* typedefs for multi-word C type names */
typedef unsigned char uchar_t;
typedef unsigned short ushort_t;
typedef unsigned long ulong_t;

typedef struct S_BITS_16 S_BITS_16_t;
typedef struct S_BITS_32 S_BITS_32_t;
typedef struct S_BITS_64 S_BITS_64_t;
typedef struct S_BITS_BIG S_BITS_BIG_t;

typedef union U_BITS_16 U_BITS_16_t;
typedef union U_BITS_32 U_BITS_32_t;
typedef union U_BITS_64 U_BITS_64_t;
typedef union U_BITS_BIG U_BITS_BIG_t;

typedef enum E_BITS_8 E_BITS_8_t;
typedef enum E_BITS_16 E_BITS_16_t;
