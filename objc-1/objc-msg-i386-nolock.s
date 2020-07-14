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
#ifndef KERNEL
# _objc_entryPoints and _objc_exitPoints are used by objc
# to get the critical egions for which method caches 
# cannot be garbage collected.

	.data
.globl _objc_entryPoints
_objc_entryPoints:
	.long _objc_msgSend
	.long _objc_msgSendSuper
	.long _objc_msgSendv
	.long 0

.globl _objc_exitPoints
_objc_exitPoints:
	.long Lexit1
	.long Lexit5
	.long Lexit9
	.long 0
#endif /* KERNEL */

// Objective-C message dispatching for the i386

	// arguments
	self = 4
	selector = 8
	// structure indices
	isa = 0
	cache = 32
	buckets = 8
	mask = 0
	method_name = 0
	method_imp = 8

// Objective-C method send

	.text
	.globl	_objc_msgSend
	.align	4
_objc_msgSend:
#ifdef PROFILE
        pushl %ebp
        movl %esp,%ebp
        movl $LP0,%eax
        call mcount
.data
        .align 2
LP0:
        .long 0
.text
        movl %ebp,%esp
        popl %ebp
#endif
	movl	self(%esp), %eax

	// if receiver is nil
	orl	%eax, %eax
	je	L1nil_or_multi

	// load variables and save caller registers.
	// Overlapped to prevent AGI
	movl	selector(%esp), %ecx	// (1)
	movl	isa(%eax), %eax		// (1) class = self->isa;
	pushl	%edi			// (1)
	movl	cache(%eax), %eax	// (1) cache = class->cache;
	pushl	%esi			// (1)

	lea	buckets(%eax), %edi	// (1) buckets = &cache->buckets;
	movl	mask(%eax), %esi	// (1) mask = cache->mask;
	movl	%ecx, %edx		// (1) index = selector;

L1probe_cache:
	andl	%esi, %edx		// (1) index &= mask;
	movl	(%edi, %edx, 4), %eax	// (2) method_name = buckets[index];

	orl	%eax, %eax		// (1) if (method != NULL) {
	je	L1cache_miss		// (1)
	cmpl	method_name(%eax), %ecx	// (1)    method_name = method->name;
	jne	L1not_the_method	// (1)	  if (method_name == selector) {
	movl	method_imp(%eax), %eax	// (1)      imp = method->method_imp;
	popl	%esi			// (1)
	popl	%edi			// (1)
Lexit1:	jmp	*%eax			// (1)      goto *imp;
	.space 17			// area for moninitobjc to write
					//        }
L1not_the_method:
	inc	%edx			// (1)    index++;
	jmp	L1probe_cache		// (1) }

	.align 4, 0x90
L1cache_miss:
	// restore caller registers
	popl	%esi			// (1)
	popl	%edi			// (1)

	movl	self(%esp), %eax	// (1)
	movl	isa(%eax), %eax		// (1)
	pushl	%ecx			// (1)
	pushl	%eax
	call	__class_lookupMethodAndLoadCache
	addl	$8, %esp
Lexit2:	jmp	*%eax
	.space 17			// area for moninitobjc to write


L1nil_or_multi:
	ret




// Objective-C Super Send

	// arguments
	caller = 4
	// structure elements
	reciever = 0
	class = 4

	.globl	_objc_msgSendSuper
	.align	4
_objc_msgSendSuper:
#ifdef PROFILE
        pushl %ebp
        movl %esp,%ebp
        movl $LP1,%eax
        call mcount
.data
        .align 2
LP1:
        .long 0
.text
        movl %ebp,%esp
        popl %ebp
#endif

	movl	caller(%esp), %eax	// (1)
	movl	selector(%esp), %ecx	// (1)

	// load variables and save caller registers.
	// Overlapped to prevent AGI
	movl	class(%eax), %eax	// (1) class = caller->class;
	pushl	%edi			// (1)
	movl	cache(%eax), %eax	// (1) cache = class->cache;
	pushl	%esi			// (1)

	lea	buckets(%eax), %edi	// (1) buckets = &cache->buckets;
	movl	mask(%eax), %esi	// (1) mask = cache->mask;
	movl	%ecx, %edx		// (1) index = selector;

L2probe_cache:
	andl	%esi, %edx		// (1) index &= mask;
	movl	(%edi, %edx, 4), %eax	// (2) method_name = buckets[index];

	orl	%eax, %eax		// (1) if (method != NULL) {
	je	L2cache_miss		// (1)
	cmpl	method_name(%eax), %ecx	// (1)    method_name = method->name;
	jne	L2not_the_method	// (1)	  if (method_name == selector) {

	// clobber "caller" arg with "self" and get method pointer
	movl	caller+8(%esp), %edi	// (1)
	movl	method_imp(%eax), %eax	// (1)      imp = method->method_imp;
	movl	reciever(%edi), %esi	// (1)
	movl	%esi, caller+8(%esp)	// (1)

	// restore caller registers
	popl	%esi			// (1)
	popl	%edi			// (1)
Lexit5:	jmp	*%eax			// (1)      goto *imp;
	.space 17			// area for moninitobjc to write
					//        }
//	.align 4, 0x90
L2not_the_method:
	inc	%edx			// (1)    index++;
	jmp	L2probe_cache		// (1) }

	.align 4, 0x90
L2cache_miss:
	// clobber "caller" arg with "reciever"
	movl	caller+8(%esp), %edi	// (1)
	movl	reciever(%edi), %esi	// (1)
	movl	%esi, caller+8(%esp)	// (1)

	// get class argument
	movl	class(%edi), %eax	// (1)

	// restore caller registers
	popl	%esi			// (1)
	popl	%edi			// (1)

	// push args (class, selector)
	pushl	%ecx			// (1)
	pushl	%eax
	call	__class_lookupMethodAndLoadCache
	addl	$8, %esp
Lexit6:	jmp	*%eax
	.space 17			// area for moninitobjc to write



// Objective-C message forwarder

        .objc_meth_var_names
        .align 2
L30:    .ascii "forward::\0"

        .objc_message_refs
        .align 2
L31:    .long L30

        .cstring
        .align 2
L32:    .ascii "Does not recognize selector %s\0"

        .text
        .align 4
        .globl __objc_msgForward
__objc_msgForward:
	pushl   %ebp
	movl    %esp,%ebp
	movl	(selector+4)(%esp), %eax
	cmpl	L31, %eax
	je	L33

	leal	(self+4)(%esp), %ecx
	pushl	%ecx
	pushl	%eax
	movl	L31, %ecx
	pushl	%ecx
	pushl	(self + 16)(%esp)
	call	_objc_msgSend
	movl    %ebp,%esp
	popl    %ebp
	ret

	.align 4
L33:
	pushl	$L30
	pushl	$L32
	pushl	(self + 12)(%esp)
	call	___objc_error		// volatile, will not return



// Objective-C vararg message send

	// arguments
	args = 16
	size = 12
	sel = 8
	self = 4

	.text
	.align 4
	.globl _objc_msgSendv
_objc_msgSendv:
	pushl	%ebp
	movl	%esp, %ebp
	movl	(args + 4)(%ebp), %edx
	addl	$8, %edx		// skip self & selector
	movl	(size + 4)(%ebp), %ecx
	shrl	$2, %ecx
	subl	$2, %ecx		// skip self & selector
	jle	L42
L41:
	decl	%ecx
	movl	0(%edx, %ecx, 4), %eax
	pushl	%eax
	jg	L41

L42:
	movl	(sel + 4)(%ebp), %ecx
	pushl	%ecx
        movl	(self + 4)(%ebp),%ecx
        pushl	%ecx
        call	_objc_msgSend
        movl	%ebp,%esp
        popl	%ebp
Lexit9:
        ret
