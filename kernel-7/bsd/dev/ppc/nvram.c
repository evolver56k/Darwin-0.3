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

/*
 * Driver for Non-volatile RAM.
 */
#import <sys/param.h>
#import <sys/systm.h>
#import <sys/conf.h>
#import <sys/ioctl.h>
#import <sys/tty.h>
#import <sys/proc.h>
#import <sys/uio.h>
#include <machdep/ppc/DeviceTree.h>
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/powermac.h>

#define	NVRAM_SIZE		8192

enum {
	NVRAM_IOMEM,
	NVRAM_PORT,
	NVRAM_PMU,
	NVRAM_NONE
} nvram_kind = NVRAM_NONE;

static volatile unsigned char *nvram_data = NULL;
static volatile unsigned char *nvram_port = NULL;

int nvopened;

typedef struct {
    long	address;
    long 	length;
} nvram_reg;

nvprobe()
{
    DTEntryIterator iter;
    DTEntry entry;
    char *str;
    int dummy;
    int result;
    nvram_reg *regs;
    int len;

    if ( HasPMU() ) {
	nvram_kind = NVRAM_PMU;
	return;
    }

    if (DTLookupEntry(NULL, "/", &entry) != kSuccess) {
	return;		// failed
    }
    if (DTCreateEntryIterator(0, &iter) != kSuccess) {
	return;		// failed
    }
    do {
	result = DTGetProperty(entry, "device_type", (void **)&str, &dummy);
	if (result == kSuccess && strcmp(str, "nvram") == 0) {
	    result = DTGetProperty(entry, "reg", (void **)&regs, &len);
	    if (result == kSuccess) {
		switch (len) {
		case	8:
		    nvram_kind = NVRAM_IOMEM;
		    nvram_data = (volatile unsigned char *)
				PCI_IO_BASE_PHYS+regs[0].address;
		    break;
		case	16:
		    nvram_kind = NVRAM_PORT;
		    nvram_port = (volatile unsigned char *)
				PCI_IO_BASE_PHYS+regs[0].address;
		    nvram_data = (volatile unsigned char *)
				PCI_IO_BASE_PHYS+regs[1].address;
		    break;
		default:
			break;
		}
		break;
	    }
	}

	while ((result = DTIterateEntries(iter, &entry)) == kIterationDone) {
	    if ((result = DTExitEntry(iter, &entry)) != kSuccess) {
		break;
	    }
	}
	if (result == kSuccess) {
	    result = DTEnterEntry(iter, entry);
	}
    } while (result == kSuccess);

    DTDisposeEntryIterator(iter);
}

/*ARGSUSED*/
nvopen(dev, flag, devtype, pp)
	dev_t dev;
	int flag, devtype;
	struct proc *pp;
{
    if (!nvopened) {
	nvprobe();
    }
    if (nvram_kind == NVRAM_NONE) {
	return (ENODEV);
    }
    return 0;
}

/*ARGSUSED*/
nvclose(dev, flag, mode, pp)
	dev_t dev;
	int flag, mode;
	struct proc *pp;
{
    return 0;
}

/*ARGSUSED*/
nvread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	register struct iovec *iov;
	long offset;
	long size;
	int c;
	unsigned char cc;
	long	read = 0;
	int error = 0;

	offset = uio->uio_offset;
	if (offset >= NVRAM_SIZE) {
		error = EIO;
		goto failed;
	}

	size = uio->uio_resid;

	if (offset+size > NVRAM_SIZE) {
		size = NVRAM_SIZE-offset;
	}

	switch (nvram_kind) {
	case	NVRAM_IOMEM:
		for (read = 0; read < size; read++, offset++)  {
			c = nvram_data[offset << 4];
			eieio(); /* better safe than sorry! */
			error = ureadc(c, uio);
			if (error)
				goto failed;
		}
		break;

	case	NVRAM_PORT:
		for (read = 0; read < size; read++, offset++) {
			*nvram_port = offset >> 5; eieio();
			c = nvram_data[(offset & 0x1f) << 4];
			eieio();
			error = ureadc(c, uio);
			if (error)
				goto failed;
		}
		break;

	case	NVRAM_PMU:
		for (read = 0; read < size; read++, offset++)  {
			ReadNVRAM(offset, 1, &cc);
			c = (int)cc;
			error = ureadc(c, uio);
			if (error)
				goto failed;
		}
		break;
	}
failed:
	return error;
}

/*ARGSUSED*/
nvwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	register struct iovec *iov;
	long offset;
	long size;
	int c;
	unsigned char cc;
	long	wrote = 0;

	offset = uio->uio_offset;
	if (offset >= NVRAM_SIZE) {
		return EIO;
	}

	size = uio->uio_resid;
	if (offset+size > NVRAM_SIZE)
		size = NVRAM_SIZE-offset;

	switch (nvram_kind) {
	case	NVRAM_IOMEM:
		for (wrote = 0; wrote < size; wrote++, offset++)  {
			c = uwritec(uio);
			if (c < 0)
				goto failed;
			nvram_data[offset << 4] = c;
			eieio();
		}
		break;

	case	NVRAM_PORT:
		for (wrote = 0; wrote < size; wrote++, offset++) {
			c = uwritec(uio);
			if (c < 0)
				goto failed;
			*nvram_port = offset >> 5; eieio();
			nvram_data[(offset & 0x1f) << 4] = c;
			eieio();
		}
		break;

	case	NVRAM_PMU:
		for (wrote = 0; wrote < size; wrote++, offset++) {
			c = uwritec(uio);
			if (c < 0)
				goto failed;
			cc = (unsigned char)c;
			WriteNVRAM(offset, 1, &cc);
		}
		break;
	}
failed:
	return 0;
}
