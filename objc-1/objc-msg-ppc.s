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
 *	Copyright 1988-1996 NeXT Software, Inc.
 *
 *	objc-msg-ppc.s
 *
 *  1-May-97  Umesh Vaishampayan  (umeshv@NeXT.com)
 *		Incorporated locking code fixes from David Harrison (harrison@NeXT.com)
 *
 *	2-Apr-97   Umesh Vaishampayan  (umeshv@NeXT.com)
 *		Incarporated changes for messanger with struct return
 *		Cleaned up the labels to use local labels
 *		Fixed bug in the msgSendSuper that did not do the locking.
 *
 *	31-Dec-96  Umesh Vaishampayan  (umeshv@NeXT.com)
 *		Created from m98k.
 *
 */

#define STACK_ALIGN_MASK   0xfff0

#ifndef KERNEL
; _objc_entryPoints and _objc_exitPoints are used by moninitobjc() to setup
; objective-C messages for profiling.  The are made private_externs when in
; a shared library.
	.reference _moninitobjc
	.const
	.align 2
.globl _objc_entryPoints
_objc_entryPoints:
	.long _objc_msgSend
	.long _objc_msgSendSuper
	.long _objc_msgSendv
	.long _objc_msgSend_stret
	.long _objc_msgSendSuper_stret
	.long 0

.globl _objc_exitPoints
_objc_exitPoints:
	.long Lexit1
	.long Lexit2
	.long Lexit3
	.long Lexit4
	.long Lexit5
	.long Lexit6
	.long Lexit7
	.long Lexit8
	.long LRexit1
	.long LRexit2
	.long LRexit3
	.long LRexit4
	.long LRexit5
	.long LRexit6
	.long LRexit7
	.long LRexit8
	.long 0
#endif /* ! KERNEL */

	isa = 0
	cache = 32
	mask = 0
	buckets = 8
	method_name = 0
	method_imp = 8

	.text
	.align 2
	.globl	 _objc_msgSend
_objc_msgSend:
	stw	 r7, 40(r1)		; save r7 through r10 for use as temps
	stw	 r8, 44(r1)
	stw	 r9, 48(r1)
	stw	 r10,52(r1)

	addis	 r7,0,hi16(__objc_multithread_mask)
	ori	 r7,r7,lo16(__objc_multithread_mask)
	lwz	 r7,0(r7)
	and.	 r7,r3,r7
	bne	 cr0,L0			; branch to the "normal" case,
					; no locking, id != nil

	cmplwi	 cr0,r3,0		; if id == nil
	bne	 cr0,LL0		; LL0 is the "locking" case
	b	 L3			; L3 is the return nil case
L0:
	lwz	 r10,isa(r3)		; class = self->isa;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r4,r11		; index = selector & mask;
L1:		
	slwi	 r0,r10,2		;
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,L2			;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r4,r8			; if (name != selector)
	bne	 cr0,L1			; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
Lexit1:
	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
L2:					;
	lwz	 r10,isa(r3)		; class = self->isa;

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;
	stw	 r6, 36(r1)		;

	mflr	 r0			; save LR
	stw	 r0,8(r1)		;
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	mtlr	 r0			; restore return pc
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;
	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
Lexit2:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */
L3:					;
	addi	 r3,0,0			;     (return value is nil)
	blr				;     (return)

LL0:	
	stw	 r6, 36(r1)		; save r6
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)

Lgetit:
	lwarx	 r0,0,r6		; try to get the lock
	cmpwi	 cr0,r0,0
	bne-	 Lgetit
	addi	 r0,0,1
	sync					; Bugfix for 3.2 and older cpus
	stwcx.	 r0,0,r6
	bne-	 Lgetit
	isync

	lwz	 r10,isa(r3)		; class = self->isa;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r4,r11		; index = selector & mask;
LL1:		
	slwi	 r0,r10,2		;
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,LL2		;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r4,r8			; if (name != selector)
	bne	 cr0,LL1		; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;

	sync				; release the lock
	addi	 r0,0,0
	stw	 r0,0(r6)
	lwz	 r6,36(r1);		; restore r6

Lexit3:
	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LL2:					;
	lwz	 r10,isa(r3)		; class = self->isa;

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;

	mflr	 r0			; move LR to r0
	stw	 r0,8(r1)		; save LR
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	mtlr	 r0			; restore return pc
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;

	sync				; release the lock
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)
	addi	 r0,0,0
	stw	 r0,0(r6)

	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
Lexit4:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */


	receiver = 0
	class = 4

	.text
	.align 2
	.globl	 _objc_msgSendSuper
_objc_msgSendSuper:
	stw	 r7, 40(r1)		; save r7 through r10 for use as temps
	stw	 r8, 44(r1)
	stw	 r9, 48(r1)
	stw	 r10,52(r1)

	addis	 r7,0,hi16(__objc_multithread_mask)
	ori	 r7,r7,lo16(__objc_multithread_mask)
	lwz	 r7,0(r7)
	cmplwi	 cr0,r7,0
	beq	 cr0,LSL0		; branch to the "locking" case,
; normal case, id != nil
	lwz	 r10,class(r3)		; class = super->class;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r4,r11		; index = selector & mask;
LS1:					;
	slwi	 r0,r10,2
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,LS2		;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r4,r8			; if (name != selector)
	bne	 cr0,LS1		; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
	lwz	 r3,receiver(r3)	; restore the receiver
Lexit5:	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LS2:					;
	lwz	 r10,class(r3)		; class = self->class;
	lwz	 r3,receiver(r3)	; restore the receiver

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;
	stw	 r6, 36(r1)		;

	mflr	 r0			; save LR
	stw	 r0,8(r1)		;
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	mtlr	 r0			; restore return pc
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;
	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
Lexit6:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */

LSL0:					; Locking case for _objc_msgSendSuper
	stw	 r6,36(r1)		; save r6
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)

LSgetit:
	lwarx	 r0,0,r6		; try to get the lock
	cmpwi	 cr0,r0,0
	bne-	 LSgetit
	addi	 r0,0,1
	sync					; Bugfix for 3.2 and older cpus
	stwcx.	 r0,0,r6
	bne-	 LSgetit
	isync

	lwz	 r10,class(r3)		; class = super->class;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r4,r11		; index = selector & mask;
LSL1:					;
	slwi	 r0,r10,2
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,LSL2		;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r4,r8			; if (name != selector)
	bne	 cr0,LSL1		; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;

	sync				; release the lock
	addi	 r0,0,0
	stw	 r0,0(r6)
	lwz	 r6,36(r1);		; restore r6
	lwz	 r3,receiver(r3)	; r3 = self

Lexit7:
	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LSL2:					;
	lwz	 r10,class(r3)		; class = self->class;
	lwz	 r3,receiver(r3)	; r3 = self for implementation

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;

	mflr	 r0			; save LR
	stw	 r0,8(r1)		;
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	mtlr	 r0			; restore return pc
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;

	sync				; release the lock
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)
	addi	 r0,0,0
	stw	 r0,0(r6)

	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
Lexit8:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */


;--------------------------------------------------------------------
;
; _objc_msgSend_stret is the [Apple/Rhapsody] PowerPC struct-return
;  version.  The ABI calls for r3 to be used as the address of
;  the structure being returned; and the other args are in the
;  next couple of registers.
;
; This is a pasted copy of _objc_msgSend above, mutatis mutandis.
;
;
; On entry:	r3 is the address in which the returned struct is put,
;		r4 is the message receiver,
;		r5 is the selector
;
	.text
	.align 2
	.globl	 _objc_msgSend_stret
_objc_msgSend_stret:
	stw	 r7, 40(r1)		; save r7 through r10 for use as temps
	stw	 r8, 44(r1)
	stw	 r9, 48(r1)
	stw	 r10,52(r1)

	addis	 r7,0,hi16(__objc_multithread_mask)
	ori	 r7,r7,lo16(__objc_multithread_mask)
	lwz	 r7,0(r7)
	and.	 r7,r4,r7
	bne	 cr0,LR0		; branch to the "normal" case,
					; no locking, id != nil

	cmplwi	 cr0,r4,0		; if id == nil
	bne	 cr0,LRL0		; LRL0 is the "locking" case
	b	 LR3			; LR3 is the return nil case
LR0:
	lwz	 r10,isa(r4)		; class = self->isa;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r5,r11		; index = selector & mask;
LR1:		
	slwi	 r0,r10,2		;
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,LR2		;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r5,r8			; if (name != selector)
	bne	 cr0,LR1		; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
LRexit1:
	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LR2:					;
	lwz	 r10,isa(r4)		; class = self->isa;

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;
	stw	 r6, 36(r1)		;

	mflr	 r0			; save LR
	stw	 r0,8(r1)		;
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	ori	 r4,r5,0		; second arg is the selector 
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	mtlr	 r0			; restore return pc
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;
	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
LRexit2:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LR3:					;
	addi	 r3,0,0			;     (return value is nil)
	blr				;     (return)

LRL0:
	stw	 r6, 36(r1)		; save r6
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)

LRgetit:
	lwarx	 r0,0,r6		; try to get the lock
	cmpwi	 cr0,r0,0
	bne-	 LRgetit
	addi	 r0,0,1
	sync					; Bugfix for 3.2 and older cpus
	stwcx.	 r0,0,r6
	bne-	 LRgetit
	isync

	lwz	 r10,isa(r4)		; class = self->isa;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r5,r11		; index = selector & mask;
LRL1:		
	slwi	 r0,r10,2		;
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,LRL2		;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r5,r8			; if (name != selector)
	bne	 cr0,LRL1		; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;

	sync				; release the lock
	addi	 r0,0,0
	stw	 r0,0(r6)
	lwz	 r6,36(r1);		; restore r6

LRexit3:
	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LRL2:					;
	lwz	 r10,isa(r4)		; class = self->isa;

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;

	mflr	 r0			; save LR
	stw	 r0,8(r1)		;
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	ori	 r4,r5,0		; second arg is the selector 
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	mtlr	 r0			; restore return pc
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;

	sync				; release the lock
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)
	addi	 r0,0,0
	stw	 r0,0(r6)

	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
LRexit4:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */


	.text
	.align 2
	.globl	 _objc_msgSendSuper_stret
_objc_msgSendSuper_stret:
	stw	 r7, 40(r1)		; save r7 through r10 for use as temps
	stw	 r8, 44(r1)
	stw	 r9, 48(r1)
	stw	 r10,52(r1)

	addis	 r7,0,hi16(__objc_multithread_mask)
	ori	 r7,r7,lo16(__objc_multithread_mask)
	lwz	 r7,0(r7)
	cmplwi	 cr0,r7,0
	beq	 cr0,LRSL0 		; branch to the "locking" case
; "normal" case, no locking, id != nil
	lwz	 r10,class(r4)		; class = super->class;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r5,r11		; index = selector & mask;
LRS1:					;
	slwi	 r0,r10,2
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,LRS2		;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r5,r8			; if (name != selector)
	bne	 cr0,LRS1		; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
	lwz	 r4,receiver(r4)	; restore the receiver
LRexit5:
	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LRS2:					;
	lwz	 r10,class(r4)		; class = self->class;
	lwz	 r4,receiver(r4)	; restore the receiver

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;
	stw	 r6, 36(r1)		;

	mflr	 r0			; save LR
	stw	 r0,8(r1)		;
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	ori	 r4,r5,0		; second arg is the selector 
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	mtlr	 r0			; restore return pc
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;
	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
LRexit6:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */

LRSL0:					; Locking case _objc_msgSendSuper_stret
	stw	 r6,36(r1)		; save r6
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)

LRSgetit:
	lwarx	 r0,0,r6		; try to get the lock
	cmpwi	 cr0,r0,0
	bne-	 LRSgetit
	addi	 r0,0,1
	sync					; Bugfix for 3.2 and older cpus
	stwcx.	 r0,0,r6
	bne-	 LRSgetit
	isync

	lwz	 r10,class(r4)		; class = super->class;
	lwz	 r10,cache(r10)		; cache = class->cache
	lwz	 r11,mask(r10)		; mask = cache->mask
	addi	 r9,r10,buckets		; buckets = cache->buckets
	and	 r10,r5,r11		; index = selector & mask;
LRSL1:					;
	slwi	 r0,r10,2
	lwzx	 r7,r9,r0		; method = cache->buckets[index];
	cmplwi	 cr0,r7,0		; if (method == NULL)
	beq	 cr0,LRSL2		;
	addi	 r10,r10,1		; index++;
	lwz	 r8,method_name(r7)	; name = method->method_name;
	and	 r10,r10,r11		; index &= mask
	lwz	 r7,method_imp(r7)	; imp = method->method_imp;
	cmpw	 r5,r8			; if (name != selector)
	bne	 cr0,LRSL1		; goto loop;
	mtspr    ctr,r7			;
	lwz	 r7, 40(r1)		; restore r7 through r10
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;

	sync				; release the lock
	addi	 r0,0,0
	stw	 r0,0(r6)
	lwz	 r6,36(r1);		; restore r6
	lwz	 r4,receiver(r4)	; r4 = self for implementation

LRexit7:
	bctr	 			; goto *imp;
	.space 112			; /* area for moninitobjc to write */
LRSL2:					;
	lwz	 r10,class(r4)		; class = super->class;
	lwz	 r4,receiver(r4)	; r4 = self for implementation

	stw	 r3, 24(r1)		; save the other register parameters
	stw	 r4, 28(r1)		;
	stw	 r5, 32(r1)		;

	mflr	 r0			; save LR
	stw	 r0,8(r1)		;
	stwu	 r1,-64(r1)		; grow the stack

	ori	 r3,r10,0		; first arg is the isa pointer
	ori	 r4,r5,0		; second arg is the selector 
	bl	 __class_lookupMethodAndLoadCache
	mtspr	 ctr,r3
	lwz	 r1,0(r1)		; restore the stack pointer

	lwz	 r0,8(r1)		;
	lwz	 r3, 24(r1)		;
	lwz	 r4, 28(r1)		;
	lwz	 r5, 32(r1)		;

	sync				; release the lock
	addis	 r6,0,hi16(_messageLock)
	ori	 r6,r6,lo16(_messageLock)
	addi	 r0,0,0
	stw	 r0,0(r6)

	lwz	 r6, 36(r1)		;
	lwz	 r7, 40(r1)		;
	lwz	 r8, 44(r1)		;
	lwz	 r9, 48(r1)		;
	lwz	 r10,52(r1)		;
LRexit8:
	bctr				; goto *imp;
	.space 112			; /* area for moninitobjc to write */

;
; End of code duplicated for struct-returns
;--------------------------------------------------------------------

# Location L30 contains the string forward::
# Location L31 contains a pointer to L30, that can be changed
# to point to another forward:: string for selector uniquing
# purposes.  ALWAYS dereference L31 to get to forward:: !!
	.objc_meth_var_names
	.align 1
L30:	.ascii "forward::\0"

	.objc_message_refs
	.align 2
L31:	.long L30

	.cstring
	.align 1
L32:	.ascii "Does not recognize selector %s\0"

	.text
	.align 2
	.globl __objc_msgForward
__objc_msgForward:
#if defined(KERNEL)
	trap                            ;  this code isnt ready yet!!!!!!!!!!!
#else
	addis	 r11,0,hi16(L31)	;
	addi	 r11,r11,lo16(L31)	;
	cmpw	 cr0,r11,r4		;  if (sel == @selector (forward::))
	bne	 L_error		;
	
	stw	 r3, 24(r1)		;  home the register arguments, is there a "float" problem?
	stw	 r4, 28(r1)
	stw	 r5, 32(r1)
	stw	 r6, 36(r1)
	stw	 r7, 40(r1)
	stw	 r8, 44(r1)
	stw	 r9, 48(r1)
	stw	 r10,52(r1)
					;  (first arg remains self)
	addis	 r4,0,hi16(L30)		;  (second arg is "forward:")
	addi	 r4,r4,lo16(L30)
	andi.	 r5,r4,0		;  (third arg is previous selector)
	addi	 r6,r1,24		;  (forth arg is &self)

	addi	 r1,r1,64		;  (grow the stack)
	bl	 _objc_msgSend		;  [self forward: sel : &self]
	addi	 r1,r1,-64		;  (deallocate storage)
	blr				;  return
L_error:
	addis	 r4,0,hi16(L32)
	addi	 r4,r4,lo16(L32)
	addis	 r5,0,hi16(L30)
	addi	 r5,r5,lo16(L30)
	bl	 ___objc_error		;  
#endif /* ! KERNEL */

	.text
	.align 2
	.globl _objc_msgSendv
_objc_msgSendv:
#if defined(KERNEL)
	trap                            ;  this code isnt ready yet!!!!!!!!!!!
#else
	mflr	 r0
	stw	 r0,8(r1)		;  (save return pc)
	addi	 r1,r1,32		;  (allocate storage)

	cmpwi	 cr0,r5,0
	bgt	 cr0,L34		;  if (size > 0) ...

	lwz	 r10,28(r6)		;  (restore some of the args)
	lwz	 r9, 24(r6)
	lwz	 r8, 20(r6)
	lwz	 r7, 16(r6)
	lwz	 r1,0(r1)		; restore stack pointer
	lwz	 r0,8(r1)		; restore return pc

	bl	 _objc_msgSend		;  objc_msgSend (self, selector, ...)

	subi	 r31,r30,32		;  (deallocate variable storage)
	addi	 r31,r31,16		;  (deallocate storage)
	lwz	 r0,8(r1)
	mtlr	 r0
	blr				; return
#endif /* ! KERNEL */
