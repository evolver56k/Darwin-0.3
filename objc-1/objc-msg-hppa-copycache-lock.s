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
#ifdef KERNEL
#define OBJC_LOCK_ROUTINE _simple_lock
#else
#ifdef DYLIB
#define OBJC_LOCK_ROUTINE _spin_lock$non_lazy_ptr
#else
#define OBJC_LOCK_ROUTINE _spin_lock
#endif
#endif /* KERNEL */

#define isa 0
#define cache 32
#define mask  0
#define buckets 8
#define method_name 0
#define method_imp 4

;; Optimized specifically for HP7100 (Gecko architecture)
;; It is assumed, that the cache line containing __objc_multithread_mask
;; is not missing.  This version has only two (2) back-to-back insn
;; output dependencies, of which one is in the inner loop at 2:.
;; This version has also no read insns with output dependency on the
;; following insn.  It is optimized with regard to branch prediction for
;; hit in the first probe (average probe length assumed to be .45), that
;; is why the final branch is moved before _objc_msgSend.

;;  non-pic: 14 clocks best case, 4 clock / probe
;;           (4miss + 1hit) + (2hit) / probe

;;      pic: 16 clocks best case, 4 clocks / probe
;; 	     (4miss + 2hit) + (2hit) / probe

        .text
	.align 2
	.globl _objc_msgSend
	
LX0: 	bv,n     0(%r1)		        ;     goto *imp;  (nullify delay)
_objc_msgSend:
	comib,=,n 0,%r26, Lnull		;     <?not taken?>
#ifdef DYLIB
	bl 	LX1,%r21
      	ldw     isa(0,%r26),%r19       	;     class = self->isa;
LX1:
        depi 	0,31,2,%r21
        addil 	L`__objc_multithread_mask$non_lazy_ptr-LX1,%r21
        ldw     cache(0,%r19),%r20     	;     cache = class->cache
	ldw 	R`__objc_multithread_mask$non_lazy_ptr-LX1(%r1),%r22 
        ldw     mask(0,%r20),%r21      	;     mask = cache->mask
	ldw 	0(%r22),%r22		;     <indirect non lazy pointer>
	ldo     buckets(%r20),%r20     	;     buckets = cache->buckets
	comib,=,n 0,%r22, SendLocking	;     <?not taken?>
#else	
	ldil	L`__objc_multithread_mask,%r1
      	ldw     isa(0,%r26),%r19       	;     class = self->isa;
	ldw	R`__objc_multithread_mask(%r1),%r22
        ldw     cache(0,%r19),%r20     	;     cache = class->cache
	comib,=,n 0,%r22, SendLocking	;     <?not taken?>
        ldw     mask(0,%r20),%r21      	;     mask = cache->mask
	ldo     buckets(%r20),%r20     	;     buckets = cache->buckets
#endif

        and     %r21,%r25,%r22          ;     index = selector & mask;
2:	sh3add %r22,%r20,%r1		;     bucket = (index * 4) + buckets
        ldw method_name(0,%r1),%r19	;     op = bucket->selector_name

        ldo 1(%r22),%r22		;     index += 1
        comb,= %r25,%r19,LX0      	;     if (selector == op) goto Lexit1 <?taken?>
        ldw method_imp(0,%r1),%r1	;     <delay slot> imp = bucket->selector_imp
        comib,<> 0,%r19,2b 		;     if (op != 0) goto 2 <?taken?>

        and %r22,%r21,%r22		;     <delay slot> index &= mask
	b,n L2				;     else goto cacheMiss
L2:
; We have to save all the register based arguments (including floating
; point) before calling _class_lookupMethodAndLoadCache.  This is because
; we do not know what arguments were passed to us, and the arguments are
; not guaranteed to be saved across procedure calls (they are all caller-saved)
; We also have to save the return address (since we did not save it on entry).


        copy    %r30,%r19
        ldo     128(%r30),%r30          ; Allocate space on stack
        stwm    %r2,4(0,%r19)           ; Save return pointer
        stwm    %r23,4(0,%r19)          ; Save old args
        stwm    %r24,4(0,%r19)          ;
        stwm    %r25,4(0,%r19)          ;
        stwm    %r26,4(0,%r19)          ;
#ifndef KERNEL
        fstds,mb  %fr4,4(0,%r19)        ; Save floating point args
        fstds,mb  %fr5,8(0,%r19)        ;    mb (modify before) is used instead
        fstds,mb  %fr6,8(0,%r19)        ;    of ma (as is implicit in above
        fstds,mb  %fr7,8(0,%r19)        ;    stores) with an initial value of 4
                                        ;    so that doubles are aligned
                                        ;    to 8 byte boundaries.
                                        ; Arg 1 (selector) is the same
#endif /* KERNEL */		

        stw     %r28,8(0,%r19)          ; save return struct ptr
        jbsr      __class_lookupMethodAndLoadCache,%r2,0f
        ldw      isa(0,%r26),%r26       ; <delay slot> arg 0 = self->isa

        ldo     -128(%r30),%r30         ;   deallocate
        copy    %r30,%r19               ;
        ldwm    4(0,%r19),%r2           ; restore everything
        ldwm    4(0,%r19),%r23          ; 
        ldwm    4(0,%r19),%r24          ;
        ldwm    4(0,%r19),%r25          ;
        ldwm    4(0,%r19),%r26          ;
#ifndef KERNEL
        fldds,mb  4(0,%r19),%fr4        ; see comment above about alignment
        fldds,mb  8(0,%r19),%fr5        ;
        fldds,mb  8(0,%r19),%fr6        ;
        fldds,mb  8(0,%r19),%fr7        ;
#endif /* KERNEL */		
        ldw     8(0,%r19),%r20          ; get ret structure ptr

        copy    %r28,%r19
        copy    %r20,%r28               ; restore ret structure ptr

        bv,n    0(%r19)                 ;  goto *imp   (nullify delay)

; stub for far call

	.align 2
#ifdef DYLIB
0:	bl   LX5,%r1
	nop
LX5:	depi 	0,31,2,%r1
	addil L`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX5,%r1
	ldw R`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX5(%r1),%r1
	be,n 0(4,%r1)
#else
0:      ldil L`__class_lookupMethodAndLoadCache,%r1
        be,n R`__class_lookupMethodAndLoadCache(4,%r1)
#endif

	.align 2
Lnull:
        bv      0(%r2)                  ; return null
        copy    0,%r28                  ; <delay slot> return val = 0



; Locking version of objc_msgSend
; uses spin_lock() to lock the mutex.

	.align 2
SendLocking:
        copy    %r30,%r19
        ldo     128(%r30),%r30          ; Allocate space on stack
        stwm    %r2,4(0,%r19)           ; Save return pointer
        stwm    %r23,4(0,%r19)          ; Save old args

        stwm    %r24,4(0,%r19)          ;
        stwm    %r25,4(0,%r19)          ;
        stwm    %r26,4(0,%r19)          ;
        stwm    %r28,4(0,%r19)          ; save return struct ptr

#ifndef KERNEL		
        fstds,ma  %fr4,8(0,%r19)        ; Save floating point args
        fstds,ma  %fr5,8(0,%r19)        ;
        fstds,ma  %fr6,8(0,%r19)        ;
        fstds,ma  %fr7,8(0,%r19)        ;
#endif /* KERNEL */		
	
#ifdef DYLIB
0:	bl   LX6,%r24
	nop
LX6:	depi 	0,31,2,%r24
	addil	L`_messageLock$non_lazy_ptr-LX6,%r24
	ldw	R`_messageLock$non_lazy_ptr-LX6(%r1),%r26
	addil	L`OBJC_LOCK_ROUTINE -LX6,%r24	; call spin_lock() with _messageLock
	ldw	R`OBJC_LOCK_ROUTINE -LX6(%r1),%r24
	ble	0(%sr4,%r24)
#else
	ldil	L`_messageLock,%r1
	ldo	R`_messageLock(%r1),%r26
	ldil	L`OBJC_LOCK_ROUTINE,%r1	; call spin_lock() with _messageLock
	ble	R`OBJC_LOCK_ROUTINE(%sr4,%r1)
#endif

	copy	%r31,%r2
        ldw	-112(%r30),%r26		; restore arg0 
	ldw     -108(%r30),%r28         ; and ret0
	ldw      isa(0,%r26),%r19       ;     class = self->isa;

        ldw      cache(0,%r19),%r20     ;     cache = class->cache
        ldw      mask(0,%r20),%r21      ;     mask = cache->mask
        ldo      buckets(%r20),%r20     ;     buckets = cache->buckets
        and      %r21,%r25,%r22         ;     index = selector & mask;

LL1:	sh3add %r22,%r20,%r1		;     bucket = (index * 4) + buckets
	ldw method_name(0,%r1),%r19	;     op = bucket->selector_name
        ldo 1(%r22),%r22		;     index += 1
	comb,=,n %r25,%r19,LLhit1      	;     if (selector == op) goto LLhit1

        comib,<> 0,%r19,LL1 		;     if (op != 0) goto LL1
        and %r22,%r21,%r22		;     <delay slot> index &= mask
	b,n LL2				;     else goto cacheMiss
LLhit1:
	ldw method_imp(0,%r1),%r19	;     imp = bucket->selector_imp
#if KERNEL
	ldil	L`_messageLock,%r1
	ldo	R`_messageLock(%r1),%r20
	addi        0xc,%r20,%r20
	depi        0,31,4,%r20
	zdepi       1,31,1,%r1
    	stw         %r1,0(0,%r20)
#else
#ifdef DYLIB
	bl   LX7,%r21
	nop
LX7:	depi 	0,31,2,%r21
	addil	L`_messageLock$non_lazy_ptr-LX7,%r21
	ldw	R`_messageLock$non_lazy_ptr-LX7(%r1),%r21
	stw	%r0,0(%r21) ; unlock the lock
#else
	ldil	L`_messageLock,%r1
	stw	%r0,R`_messageLock(%r1) ; unlock the lock
#endif
#endif 
	ldwm	-128(%r30),%r2		; restore original rp and deallocate
	bv,n     0(%r19)                ;    goto *imp;  (nullify delay)

LL2:
        jbsr      __class_lookupMethodAndLoadCache,%r2,0f
        ldw      isa(0,%r26),%r26       ; <delay slot> arg 0 = self->isa

        ldo     -128(%r30),%r30         ;   deallocate
        copy    %r30,%r19               ;
        ldwm    4(0,%r19),%r2           ; restore everything
        ldwm    4(0,%r19),%r23          ; 
        ldwm    4(0,%r19),%r24          ;
        ldwm    4(0,%r19),%r25          ;
        ldwm    4(0,%r19),%r26          ;
        ldwm    4(0,%r19),%r20          ; get ret structure ptr
#ifndef KERNEL		
        fldds,ma  8(0,%r19),%fr4        ;
        fldds,ma  8(0,%r19),%fr5        ;
        fldds,ma  8(0,%r19),%fr6        ;
        fldds,ma  8(0,%r19),%fr7        ;
#endif /* KERNEL */		
 
        copy    %r28,%r19
        copy    %r20,%r28               ; restore ret structure ptr
#if KERNEL
	ldil	L`_messageLock,%r1
	ldo	R`_messageLock(%r1),%r20
	addi        0xc,%r20,%r20
	depi        0,31,4,%r20
	zdepi       1,31,1,%r1
    	stw         %r1,0(0,%r20)
#else
#ifdef DYLIB
	bl   LX8,%r19
	nop
LX8:	depi 	0,31,2,%r19
	addil	L`_messageLock$non_lazy_ptr-LX8,%r19
	ldw	_messageLock$non_lazy_ptr-LX8(%r1),%r19
	stw	%r0,0(%r19)	 ; unlock the lock
#else
	ldil	L`_messageLock,%r1
	stw	%r0,R`_messageLock(%r1) ; unlock the lock
#endif
#endif
        bv,n    0(%r19)                 ;  goto *imp   (nullify delay)

; stub for far call

	.align 2
#ifdef DYLIB
0:	bl   LX2,%r1
	nop
LX2:	depi 	0,31,2,%r1
	addil L`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX2,%r1
	ldw R`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX2(%r1),%r1
	be,n 0(4,%r1)
#else
0:      ldil L`__class_lookupMethodAndLoadCache,%r1
        be,n R`__class_lookupMethodAndLoadCache(4,%r1)
#endif


#define receiver 0
#define class 4

        .globl _objc_msgSendSuper
	.align 2
_objc_msgSendSuper:
	ldil	L`__objc_multithread_mask,%r1
	ldw	R`__objc_multithread_mask(%r1),%r19
	combt,= %r0,%r19,SuperLocking	; 
        ldw      class(0,%r26),%r19     ;     class = caller->class;

        ldw      cache(0,%r19),%r20     ;     cache = class->cache
        ldw      mask(0,%r20),%r21      ;     mask = cache->mask
        ldo      buckets(%r20),%r20     ;     buckets = cache->buckets
        and      %r21,%r25,%r22         ;     index = selector & mask;

LS1:    sh3add %r22,%r20,%r1		;     bucket = (index * 4) + buckets
        ldw method_name(0,%r1),%r19	;     op = bucket->selector_name
        ldo 1(%r22),%r22		;     index += 1
        comb,= %r25,%r19,LShit1      	;     if (selector == op) goto LShit1

        ldw method_imp(0,%r1),%r1	;     <delay slot> imp = bucket->selector_imp
        comib,<> 0,%r19,LS1 		;     if (op != 0) goto LS1
        and %r22,%r21,%r22		;     <delay slot> index &= mask
	b,n LS2				;     else goto cacheMiss

LShit1:
        bv       0(%r1)                	;    goto *imp;  (nullify delay)
        ldw      receiver(0,%r26),%r26	;    <delay slot> self = caller->receiver;
                                        ;
LS2:					;
        copy    %r30,%r19
        ldo     128(%r30),%r30          ; Allocate space on stack
        stwm    %r2,4(0,%r19)           ; Save return pointer
        stwm    %r23,4(0,%r19)          ; Save old args
        stwm    %r24,4(0,%r19)          ;
        stwm    %r25,4(0,%r19)          ;
        stwm    %r26,4(0,%r19)          ;
#ifndef KERNEL		
        fstds,mb  %fr4,4(0,%r19)        ; Save floating point args
        fstds,mb  %fr5,8(0,%r19)        ;    mb (modify before) is used instead
        fstds,mb  %fr6,8(0,%r19)        ;    of ma (as is implicit in above
        fstds,mb  %fr7,8(0,%r19)        ;    stores) with an initial value of 4
                                        ;    so that doubles are aligned
                                        ;    to 8 byte boundaries.
                                        ; Arg 1 (selector) is the same
#endif /* KERNEL */										
        stw     %r28,8(0,%r19)          ; save return struct ptr
        jbsr      __class_lookupMethodAndLoadCache,%r2,0f
        ldw      class(0,%r26),%r26     ; <delay slot> arg 0 = caller->class;
        ldo     -128(%r30),%r30         ;   deallocate
        copy    %r30,%r19               ;
        ldwm    4(0,%r19),%r2           ; restore everything
        ldwm    4(0,%r19),%r23          ; 
        ldwm    4(0,%r19),%r24          ;
        ldwm    4(0,%r19),%r25          ;
        ldwm    4(0,%r19),%r26          ;
#ifndef KERNEL				
        fldds,mb  4(0,%r19),%fr4        ; see comment above about alignment
        fldds,mb  8(0,%r19),%fr5        ;
        fldds,mb  8(0,%r19),%fr6        ;
        fldds,mb  8(0,%r19),%fr7        ;
#endif /* KERNEL */										
        ldw     8(0,%r19),%r20          ; get ret structure ptr
        ldw      receiver(0,%r26),%r26  ;     self = caller->receiver;
        copy    %r28,%r19
        copy    %r20,%r28
  	bv,n    0(%r19)                 ;  goto *imp   (nullify delay)

; stub for far call

	.align 2
#ifdef DYLIB
0:	bl   LX3,%r1
	nop
LX3:	depi 	0,31,2,%r1
	addil L`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX3,%r1
	ldw R`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX3(%r1),%r1
	be,n 0(4,%r1)
#else
0:      ldil L`__class_lookupMethodAndLoadCache,%r1
        be,n R`__class_lookupMethodAndLoadCache(4,%r1)
#endif




; locking version of objc_msgSendSuper
; uses spin_lock() to lock the lock.

	.align 2
SuperLocking:
        copy    %r30,%r19
        ldo     128(%r30),%r30          ; Allocate space on stack
        stwm    %r2,4(0,%r19)           ; Save return pointer
        stwm    %r23,4(0,%r19)          ; Save old args
        stwm    %r24,4(0,%r19)          ;
        stwm    %r25,4(0,%r19)          ;
        stwm    %r26,4(0,%r19)          ;
        stwm    %r28,4(0,%r19)          ; save return struct ptr
#ifndef KERNEL		
        fstds,ma  %fr4,8(0,%r19)        ; Save floating point args
        fstds,ma  %fr5,8(0,%r19)        ;
        fstds,ma  %fr6,8(0,%r19)        ;
        fstds,ma  %fr7,8(0,%r19)        ;
#endif /* KERNEL */										
#ifdef DYLIB
0:	bl   LX9,%r24
	nop
LX9:	depi 	0,31,2,%r24
	addil	L`_messageLock$non_lazy_ptr-LX9,%r24
	ldw	R`_messageLock$non_lazy_ptr-LX9(%r1),%r26
	addil	L`OBJC_LOCK_ROUTINE-LX9,%r24	; call spin_lock() with _messageLock
	ldw	R`OBJC_LOCK_ROUTINE-LX9(%r1),%r24
	ble	0(%sr4,%r24)
#else
	ldil	L`_messageLock,%r1
	ldo	R`_messageLock(%r1),%r26
	ldil	L`OBJC_LOCK_ROUTINE,%r1	; call spin_lock() with _messageLock
	ble	R`OBJC_LOCK_ROUTINE(%sr4,%r1)
#endif
	copy	%r31,%r2
        ldw	-112(%r30),%r26		; restore arg0 
	ldw	-108(%r30),%r28  	; and ret0 (spin_lock doesnt
					; touch anything else)
        ldw      class(0,%r26),%r19     ;     class = caller->class;
        ldw      cache(0,%r19),%r20     ;     cache = class->cache
        ldw      mask(0,%r20),%r21      ;     mask = cache->mask
        ldo      buckets(%r20),%r20     ;     buckets = cache->buckets
        and      %r21,%r25,%r22         ;     index = selector & mask;

LLS1:   sh3add %r22,%r20,%r1		;     bucket = (index * 4) + buckets
        ldw method_name(0,%r1),%r19	;     op = bucket->selector_name
        ldo 1(%r22),%r22		;     index += 1
        comb,=,n %r25,%r19,LLShit1      ;     if (selector == op) goto LShit1

        comib,<> 0,%r19,LLS1 		;     if (op != 0) goto LS1
        and %r22,%r21,%r22		;     <delay slot> index &= mask
	b,n LLS2			;     else goto cacheMiss

LLShit1:
        ldw method_imp(0,%r1),%r19	;     imp = bucket->selector_imp
        ldw receiver(0,%r26),%r26	;     self = caller->receiver;

#if KERNEL
	ldil	L`_messageLock,%r1
	ldo	R`_messageLock(%r1),%r20
	addi        0xc,%r20,%r20
	depi        0,31,4,%r20
	zdepi       1,31,1,%r1
    stw         %r1,0(0,%r20)
#else
#ifdef DYLIB
	bl   LX10,%r19
	nop
LX10:	depi 	0,31,2,%r19
	addil	L`_messageLock$non_lazy_ptr-LX10,%r19
	ldw	R`_messageLock$non_lazy_ptr-LX10(%r1),%r19
	stw	%r0,0(%r19) ; unlock the lock
#else
	ldil	L`_messageLock,%r1
	stw	%r0,R`_messageLock(%r1) ; unlock the lock
#endif
#endif
	ldwm	-128(%r30),%r2		; restore original rp and deallocate
        bv,n     0(%r19)                ;    goto *imp;  (nullify delay)
#ifdef MONINIT
        .space 128                      ; /* area for moninitobjc to write */
#endif

                                        ;
LLS2:					;
        jbsr    __class_lookupMethodAndLoadCache,%r2,0f
        ldw     class(0,%r26),%r26     ; <delay slot> arg 0 = caller->class;
        ldo     -128(%r30),%r30         ;   deallocate
        copy    %r30,%r19               ;
        ldwm    4(0,%r19),%r2           ; restore everything
        ldwm    4(0,%r19),%r23          ; 
        ldwm    4(0,%r19),%r24          ;
        ldwm    4(0,%r19),%r25          ;
        ldwm    4(0,%r19),%r26          ;
        ldwm    4(0,%r19),%r20          ; get ret structure ptr
#ifndef KERNEL
        fldds,ma  8(0,%r19),%fr4        ; 
        fldds,ma  8(0,%r19),%fr5        ;
        fldds,ma  8(0,%r19),%fr6        ;
        fldds,ma  8(0,%r19),%fr7        ;
#endif /* KERNEL */												
        ldw      receiver(0,%r26),%r26  ;     self = caller->receiver;
        copy    %r28,%r19
        copy    %r20,%r28
#if KERNEL
	ldil	L`_messageLock,%r1
	ldo	R`_messageLock(%r1),%r20
	addi        0xc,%r20,%r20
	depi        0,31,4,%r20
	zdepi       1,31,1,%r1
    	stw         %r1,0(0,%r20)
#else
        ldil	L`_messageLock,%r1
	stw	%r0,R`_messageLock(%r1) ; unlock the lock
#endif
	bv,n    0(%r19)                 ;  goto *imp   (nullify delay)

; stub for far call

	.align 2
#ifdef DYLIB
0:	bl   LX4,%r1
	nop
LX4:	depi 	0,31,2,%r1
	addil L`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX4,%r1
	ldw R`__class_lookupMethodAndLoadCache$non_lazy_ptr-LX4(%r1),%r1
	be,n 0(4,%r1)
#else
0:      ldil L`__class_lookupMethodAndLoadCache,%r1
        be,n R`__class_lookupMethodAndLoadCache(4,%r1)
#endif

        
        .objc_meth_var_names
L30:    .ascii "forward::\0"

        .objc_message_refs
	.align 2
L31:    .long L30

        .cstring
L32:    .ascii "Does not recognize selector %s\0"

        .text
        .align 2
;
; NOTE: Because the stack grows from low mem to high mem on this machine
; and the args go the other way, the marg_list pointer is to the first argument
; and subsequent arguments are at NEGATIVE offsets from the marg_list.
; This means that marg_getValue() and related macros will have to be adjusted
; appropriately.
;
	.globl __objc_msgForward
__objc_msgForward:
        stw     %r2,-20(0,%r30)         ; save rp
        ldo     64(%r30),%r30           ; create frame area (no locals needed)
        ldil    L`L31,%r1
        ldo     R`L31(%r1),%r19
        ldw     0(0,%r19),%r19
        combt,=,n %r19, %r25,L34	; if (sel==@selector(forward::))
        ldo     -112(%r30),%r20         ; ptr to arg3 homing area
        stwm    %r23,4(0,%r20)          ; Mirror registers onto stack
        stwm    %r24,4(0,%r20)          ;
        stwm    %r25,4(0,%r20)          ;
        stwm    %r26,4(0,%r20)          ;
        
        copy    %r25,%r24
        copy    %r19,%r25               ; [self forward:sel :marg_list]

        bl      _objc_msgSend,%r2
        copy    %r20,%r23               ; <delay slot> copy original sel

        ldo     -64(%r30),%r30		; deallocate
        ldw     -20(0,%r30),%r2		; restore rp
        bv,n    0(%r2)			; return
L34:
        ldil    L`L32,%r1
        ldo     R`L32(%r1),%r25
        ldil	L`__objc_error,%r1
	be      R`__objc_error(%sr4,%r1) ; __objc_error never returns, so no
        copy    %r19,%r24                ; need to clean up.


; Algorithm is as follows:
; . Calculate how much stack size is needed for any arguments not in the
;   general registers and allocate space on stack.
; . Restore general argument regs from the bottom of the marg_list.
; . Restore fp argument regs from the same area.
;   (The first two args in the marg list are always old obj and old SEL.)
; . Copy any remaining args from the marg_list to the new frame
; . Call the new method.
	.align 2
	.globl _objc_msgSendv
_objc_msgSendv:
                                        ; objc_msgSendv(self, sel, size, margs)
        stw     %r2,-20(0,%r30)         ;
        copy	%r4, %r1		; stanard prologue
        copy    %r30,%r4                ;
	stw     %r1,0(0,%r4)            ;
        ldo     99(%r24),%r19           ; Calculate frame size, rounded
        depi    0,31,6,%r19             ; up to 64 byte boundary...

        add     %r19,%r30,%r30          ; Allocate frame area (no locals)
        copy    %r24,%r20               ; r20 now holds arg size
        ldo     -16(%r23),%r21          ; r21 now holds marg_list+16
        ldws    0(0,%r21),%r23          ; Get old general register args (dont
        ldws    4(0,%r21),%r24          ; need first two: always self & SEL)
#ifndef KERNEL		
        fldds   0(0,%r21),%fr7          ; Mirror to fp regs
        fldws   4(0,%r21),%fr6          ; 
#endif /* KERNEL */		

        ldo     -52(%r30),%r22          ; newly allocated stack area.
        ldo     -16(%r20),%r20          ; Size -= 16
        comibf,<,n 0,%r20,L36
L35:    ldws,mb -4(0,%r21),%r19         ; while(size>0)
        addibf,<= -4,%r20,L35		;  { *(dest--) = *(src--); size-=4; }
        stws,ma %r19,-4(0,%r22)         ; <delay slot>
L36:    bl      _objc_msgSend,%r2
        nop
        copy    %r4,%r30                ; deallocate
        ldw     -20(0,%r30), %r2
        bv	0(%r2)
        ldw     0(0,%r30), %r4

#ifdef DYLIB
.non_lazy_symbol_pointer
_messageLock$non_lazy_ptr:
        .indirect_symbol _messageLock
        .long 0
_spin_lock$non_lazy_ptr:
        .indirect_symbol _spin_lock
        .long 0
__objc_multithread_mask$non_lazy_ptr:
	.indirect_symbol __objc_multithread_mask
	.long 0
__class_lookupMethodAndLoadCache$non_lazy_ptr:
	.indirect_symbol __class_lookupMethodAndLoadCache
	.long 0
#endif