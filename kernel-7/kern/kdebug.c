/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#import <kern/lock.h>
#import <bsd/machine/cpu.h>
#import <vm/vm_kern.h>
#import <machine/spl.h>
#import <kern/clock.h>
#import <kdebug.h>
#import <kern/kdebug.h>
#import <kern/kdebug_private.h>
#import <bsd/sys/errno.h>
#import <bsd/sys/param.h>		/* for splhigh */
#import <bsd/sys/proc.h>
#import <bsd/sys/vm.h>
#import <bsd/sys/sysctl.h>

/* kd_buf kd_buffer[kd_bufsize/sizeof(kd_buf)]; */
kd_buf * kd_bufptr;
unsigned int kd_buftomem=0;
kd_buf * kd_buffer=0;
kd_buf * kd_buflast;
kd_buf * kd_readlast;
unsigned int nkdbufs = 8192;
unsigned int kd_bufsize = 0;
unsigned int kdebug_flags = 0;
unsigned int kdebug_enable=0;
unsigned int kdebug_nolog=1;
unsigned int kdlog_beg=0;
unsigned int kdlog_end=0;

extern tvalspec_t get_timebase(void);

struct kdebug_args {
        int code;
        int arg1;
        int arg2;
        int arg3;
        int arg4;
        int arg5;
};

syscall_kdebug(p, uap, retval)
        struct proc *p;
	struct kdebug_args *uap;
	register_t *retval;
{
        if (kdebug_nolog)
	        return(EINVAL);

        kernel_debug(uap->code, uap->arg1, uap->arg2, uap->arg3, uap->arg4, uap->arg5);
	return(0);
}


void
kernel_debug(debugid, arg1, arg2, arg3, arg4, arg5)
unsigned int debugid, arg1, arg2, arg3, arg4, arg5;
{
	kd_buf * kd;

	if (kdebug_nolog ||
	    ((kdebug_flags & KDBG_RANGECHECK) &&
	     ((debugid < kdlog_beg) ||(debugid > kdlog_end))))
		return;
	kd = kd_bufptr;
	kd->debugid= debugid;
	kd->arg1 = arg1;
	kd->arg2 = arg2;
	kd->arg3 = arg3;
	kd->arg4 = arg4;
	kd->arg5 = (unsigned int)current_thread();
	kd->timestamp = get_timebase();
	kd_bufptr++;
	if(kdebug_flags & KDBG_NOWRAP) {
		if (kd_bufptr >= kd_buflast)
			kdebug_nolog = 1 ;
	} else {
		if ( kd_bufptr >= kd_buflast)
		{	kd_bufptr =  kd_buffer;
			kdebug_flags |= KDBG_WRAPPED;
		}
	}
}


kdbg_bootstrap()
{
	kd_bufsize = nkdbufs * sizeof(kd_buf);
	if (kmem_alloc(kernel_map, &kd_buftomem ,(vm_size_t)kd_bufsize) == KERN_SUCCESS) 
	kd_buffer = (kd_buf *) kd_buftomem;
	else kd_buffer= (kd_buf *) 0;
	kdebug_flags &= ~KDBG_WRAPPED;
	if (kd_buffer) {
		kdebug_flags |= (KDBG_INIT | KDBG_BUFINIT);
		kd_bufptr = kd_buffer;
		kd_buflast = &kd_bufptr[nkdbufs];
		kd_readlast = kd_bufptr;
		return(0);
	} else {
		kd_bufsize=0;
		kdebug_flags &= ~(KDBG_INIT | KDBG_BUFINIT);
		return(EINVAL);
	}
	
}

kdbg_reinit()
{
int x;
int ret=0;
	x= splhigh();
	if ((kdebug_flags & KDBG_INIT) && (kdebug_flags & KDBG_BUFINIT) && kd_bufsize && kd_buffer)
		kmem_free(kernel_map,kd_buffer,kd_bufsize);
	ret= kdbg_bootstrap();
	splx(x);
	return(ret);
}

kdbg_clear()
{
int x;
	x=splhigh();
	kdebug_flags &= ~KDBG_BUFINIT;
	kmem_free(kernel_map,kd_buffer,kd_bufsize);
	kd_buffer = (kd_buf *)0;
	kd_bufsize = 0;
	kdebug_enable = 0;
	kdebug_nolog = 1;
	splx(x);
}

kdbg_setreg(kd_regtype * kdr)
{
	int i,j, ret=0;
	unsigned int val_1, val_2, val;
	switch (kdr->type) {
	
	case KDBG_CLASSTYPE :
		val_1 = (kdr->value1 & 0xff);
		val_2 = val_1 + 1;
		kdlog_beg = (val_1<<24);
		kdlog_end = (val_2<<24);
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdebug_flags |= (KDBG_RANGECHECK | KDBG_CLASSTYPE);
		break;
	case KDBG_SUBCLSTYPE :
		val_1 = (kdr->value1 & 0xff);
		val_2 = (kdr->value2 & 0xff);
		val = val_2 + 1;
		kdlog_beg = ((val_1<<24) | (val_2 << 16));
		kdlog_end = ((val_1<<24) | (val << 16));
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdebug_flags |= (KDBG_RANGECHECK | KDBG_SUBCLSTYPE);
		break;
	case KDBG_RANGETYPE :
		kdlog_beg = (kdr->value1);
		kdlog_end = (kdr->value2);
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdebug_flags |= (KDBG_RANGECHECK | KDBG_RANGETYPE);
		break;
	case KDBG_TYPENONE :
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdlog_beg = 0;
		kdlog_end = 0;
		break;
	default :
		ret = EINVAL;
		break;
	}
	return(ret);
}

kdbg_getreg(kd_regtype * kdr)
{
	int i,j, ret=0;
	unsigned int val_1, val_2, val;
#if 0	
	switch (kdr->type) {
	case KDBG_CLASSTYPE :
		val_1 = (kdr->value1 & 0xff);
		val_2 = val_1 + 1;
		kdlog_beg = (val_1<<24);
		kdlog_end = (val_2<<24);
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdebug_flags |= (KDBG_RANGECHECK | KDBG_CLASSTYPE);
		break;
	case KDBG_SUBCLSTYPE :
		val_1 = (kdr->value1 & 0xff);
		val_2 = (kdr->value2 & 0xff);
		val = val_2 + 1;
		kdlog_beg = ((val_1<<24) | (val_2 << 16));
		kdlog_end = ((val_1<<24) | (val << 16));
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdebug_flags |= (KDBG_RANGECHECK | KDBG_SUBCLSTYPE);
		break;
	case KDBG_RANGETYPE :
		kdlog_beg = (kdr->value1);
		kdlog_end = (kdr->value2);
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdebug_flags |= (KDBG_RANGECHECK | KDBG_RANGETYPE);
		break;
	case KDBG_TYPENONE :
		kdebug_flags &= (unsigned int)~KDBG_CKTYPES;
		kdlog_beg = 0;
		kdlog_end = 0;
		break;
	default :
		ret = EINVAL;
		break;
	}
#endif /* 0 */
	return(EINVAL);
}

kdbg_control(name, namelen, where, sizep)
int *name;
u_int namelen;
char *where;
size_t *sizep;
{
int ret=0;
int size=*sizep;
unsigned int value = name[1];
kd_regtype kd_Reg;
int kd_val[3];

	switch(name[0]) {
		case KERN_KDEFLAGS:
			value &= KDBG_USERFLAGS;
			kdebug_flags |= value;
			break;
		case KERN_KDDFLAGS:
			value &= KDBG_USERFLAGS;
			kdebug_flags &= ~value;
			break;
		case KERN_KDENABLE:
			kdebug_enable=(value)?1:0;
			kdebug_nolog = (value)?0:1;;
			break;
		case KERN_KDSETBUF:
			if (value <= (KDBG_MAXBUFSIZE))
				nkdbufs = value;
			else ret= EINVAL;
			break;
		case KERN_KDGETBUF:
		  if(size < sizeof(kd_val)) {
		    ret=EINVAL;
		    break;
		  }
		  kd_val[0] = nkdbufs;
		  kd_val[1] = kdebug_nolog;
		  kd_val[2] = kdebug_flags;
		  if(copyout (&kd_val, where, sizeof(kd_val))) {
		    ret=EINVAL;
		  }
		  break;
		case KERN_KDSETUP:
			ret=kdbg_reinit();
			break;
		case KERN_KDREMOVE:
			kdbg_clear();
			break;
		case KERN_KDSETREG:
			if(size < sizeof(kd_regtype)) {
				ret=EINVAL;
				break;
			}
			if (copyin(where, &kd_Reg, sizeof(kd_regtype))) {
				ret= EINVAL;
				break;
			}
			ret = kdbg_setreg(&kd_Reg);
			break;
		case KERN_KDGETREG:
			if(size < sizeof(kd_regtype)) {
				ret = EINVAL;
				break;
			}
			ret = kdbg_getreg(&kd_Reg);
		 	if (copyout(&kd_Reg, where, sizeof(kd_regtype))){
				ret=EINVAL;
			}
			break;
		case KERN_KDREADTR:
			ret = kdbg_read(where, sizep);
			break;
		default:
			ret= EINVAL;
	}
	return(ret);
}


kdbg_read(kd_buf * buffer, size_t *number)
{
int x,i;
kd_buf *start; 
int ret=0;
int avail=*number;
int count=0;

	count = avail/sizeof(kd_buf);
	if (count) {
		if ((kdebug_flags & KDBG_BUFINIT) && kd_bufsize && kd_buffer) {
			if (count > nkdbufs) count = nkdbufs;
			x= splhigh();
			if (kdebug_flags & KDBG_WRAPPED)
			{	kd_readlast = kd_bufptr+1;
				if (kd_readlast > kd_buflast)
					kd_readlast = kd_buffer;
				kdebug_flags &= ~KDBG_WRAPPED;
			}
			for (i = 0 ; i < count ; i++ ) {
				if (kd_readlast == kd_bufptr) break;
				if(copyout(kd_readlast,buffer,sizeof(kd_buf))) {
					ret=EINVAL;
					break;
				}
				kd_readlast++;
				buffer ++;
				if (kd_readlast >= kd_buflast)
					kd_readlast = kd_buffer;
			}
			splx(x);
			*number = i;
		} else ret = EINVAL;
	} else ret=EINVAL;

	return (ret);
}
