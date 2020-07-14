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
 * Copyright (c) 1992, 1993 NeXT Computer, Inc.
 *
 * Byte ordering conversion inlines for SCSI structures.
 *
 * HISTORY
 *
 * 25 June 1993 David E. Bohman at NeXT
 *	Major cleanup.
 *
 * 8 July 1992 David E. Bohman at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <bsd/dev/scsireg.h>

#if	__LITTLE_ENDIAN__
typedef union {
    struct {
	unsigned char	byte0,
			byte1,
			byte2,
			byte3;
    } bytes;
    unsigned int	word;
} _conv_t;
#endif

/*
 * Setup SCSI command blocks.
 */

static __inline__
void
scsi_testrdy_setup(
    struct cdb_6	*c6p,
    int			lun
)
{
    c6p->c6_opcode	= C6OP_TESTRDY;
    c6p->c6_lun		= lun;
}

static __inline__
void
scsi_inquiry_setup(
    struct cdb_6	*c6p,
    int			lun,
    int			len
)
{
    c6p->c6_opcode	= C6OP_INQUIRY;
    c6p->c6_lun		= lun;
    c6p->c6_len		= len;
}

static __inline__
void
scsi_modesense_setup(
    struct cdb_6	*c6p,
    int			lun,
    int			len
)
{
    c6p->c6_opcode	= C6OP_MODESENSE;
    c6p->c6_lun		= lun;
    c6p->c6_len		= len;
}

static __inline__
void
scsi_reqsense_setup(
    struct cdb_6	*c6p,
    int			lun,
    int			len
)
{
    c6p->c6_opcode	= C6OP_REQSENSE;
    c6p->c6_lun		= lun;
    c6p->c6_len		= len;
}

static __inline__
void
scsi_spinup_setup(
    struct cdb_6s	*c6p,
    int			lun
)
{
    c6p->c6s_opcode	= C6OP_STARTSTOP;
    c6p->c6s_lun	= lun;
    c6p->c6s_opt	= C6OPT_IMMED;

#if	__LITTLE_ENDIAN__
    c6p->c6s_len0	= C6S_SS_START;
#endif

#if	__BIG_ENDIAN__
#if	__NATURAL_ALIGNMENT__
    c6p->c6s_len[3]	= C6S_SS_START;
#else
    c6p->c6s_len	= C6S_SS_START;
#endif	__NATURAL_ALIGNMENT
#endif
}

static __inline__
void
scsi_eject_setup(
    struct cdb_6s	*c6p,
    int			lun
)
{
    c6p->c6s_opcode	= C6OP_STARTSTOP;
    c6p->c6s_lun	= lun;
#if	__LITTLE_ENDIAN__
    c6p->c6s_len0	= C6S_SS_EJECT;
#endif

#if	__BIG_ENDIAN__
#if	__NATURAL_ALIGNMENT__
    c6p->c6s_len[3]	= C6S_SS_EJECT;
#else
    c6p->c6s_len	= C6S_SS_EJECT;
#endif
#endif
}

static __inline__
void
scsi_readcapacity_setup(
    struct cdb_10	*c10p,
    int			lun
)
{
    c10p->c10_opcode	= C10OP_READCAPACITY;
    c10p->c10_lun	= lun;
}

static __inline__
void
scsi_readextended_setup(
    struct cdb_10	*c10p,
    int			lun,
    unsigned int	blkno,
    unsigned int	nblk
)
{
#if	__LITTLE_ENDIAN__
    _conv_t		_conv;
#endif __LITTLE_ENDIAN__

    c10p->c10_opcode	= C10OP_READEXTENDED;
    c10p->c10_lun	= lun;

#if	__LITTLE_ENDIAN__

     _conv.word		= blkno;
    c10p->c10_lba3	= _conv.bytes.byte3;
    c10p->c10_lba2	= _conv.bytes.byte2;
    c10p->c10_lba1	= _conv.bytes.byte1;
    c10p->c10_lba0	= _conv.bytes.byte0;
    
    _conv.word		= nblk;
    c10p->c10_len1	= _conv.bytes.byte1;
    c10p->c10_len0	= _conv.bytes.byte0;
#endif

#if	__BIG_ENDIAN__
#if __NATURAL_ALIGNMENT__
    bcopy(&blkno, &c10p->c10_lba, sizeof(blkno));
    c10p->c10_len[0] = (nblk >> 8) & 0xff;
    c10p->c10_len[1] = nblk & 0xff;
#else
    c10p->c10_lba = blkno;
    c10p->c10_len = nblk;
#endif __NATURAL_ALIGNMENT__
#endif __BIG_ENDIAN__
}

static __inline__
void
scsi_writeextended_setup(
    struct cdb_10	*c10p,
    int			lun,
    unsigned int	blkno,
    unsigned int	nblk
)
{
#if	__LITTLE_ENDIAN__
    _conv_t		_conv;
#endif __LITTLE_ENDIAN__

    c10p->c10_opcode	= C10OP_WRITEEXTENDED;
    c10p->c10_lun	= lun;

#if	__LITTLE_ENDIAN__
    
    _conv.word		= blkno;
    c10p->c10_lba3	= _conv.bytes.byte3;
    c10p->c10_lba2	= _conv.bytes.byte2;
    c10p->c10_lba1	= _conv.bytes.byte1;
    c10p->c10_lba0	= _conv.bytes.byte0;
    
    _conv.word		= nblk;
    c10p->c10_len1	= _conv.bytes.byte1;
    c10p->c10_len0	= _conv.bytes.byte0;
#endif

#if	__BIG_ENDIAN__
#if __NATURAL_ALIGNMENT__
    bcopy(&blkno, &c10p->c10_lba, sizeof(blkno));
    c10p->c10_len[0] = (nblk >> 8) & 0xff;
    c10p->c10_len[1] = nblk & 0xff;
#else
    c10p->c10_lba = blkno;
    c10p->c10_len = nblk;
#endif __NATURAL_ALIGNMENT__
#endif __BIG_ENDIAN__
}

/*
 * Access fields in returned SCSI
 * data structures.
 */

static __inline__
unsigned int
scsi_blklen(
    capacity_reply_t	*crp
)
{
#if	__LITTLE_ENDIAN__
    _conv_t		_conv;
    
    _conv.bytes.byte3	= crp->cr_blklen3;
    _conv.bytes.byte2	= crp->cr_blklen2;
    _conv.bytes.byte1	= crp->cr_blklen1;
    _conv.bytes.byte0	= crp->cr_blklen0;
    
    return (_conv.word);
#endif

#if	__BIG_ENDIAN__
    return (crp->cr_blklen);
#endif
}

static __inline__
unsigned int
scsi_lastlba(
    capacity_reply_t	*crp
)
{
#if	__LITTLE_ENDIAN__
    _conv_t		_conv;
    
    _conv.bytes.byte3	= crp->cr_lastlba3;
    _conv.bytes.byte2	= crp->cr_lastlba2;
    _conv.bytes.byte1	= crp->cr_lastlba1;
    _conv.bytes.byte0	= crp->cr_lastlba0;
    
    return (_conv.word);
#endif

#if	__BIG_ENDIAN__
    return (crp->cr_lastlba);
#endif
}

static __inline__
unsigned int
scsi_error_info(
    esense_reply_t	*erp
)
{
#if	__LITTLE_ENDIAN__
    _conv_t		_conv;
    
    _conv.bytes.byte3	= erp->er_info3;
    _conv.bytes.byte2	= erp->er_info2;
    _conv.bytes.byte1	= erp->er_info1;
    _conv.bytes.byte0	= erp->er_info0;
    
    return (_conv.word);
#endif

#if	__BIG_ENDIAN__
    return ((erp->er_infomsb << 24) | erp->er_info);
#endif
}

static __inline__
void
scsi_crp_setup(
    capacity_reply_t	*crp,
    unsigned int	cr_blklen,
    unsigned int	cr_lastlba
)
{
#if	__LITTLE_ENDIAN__
    _conv_t		_conv;
    
    _conv.word = cr_blklen;
    crp->cr_blklen3 = _conv.bytes.byte3;
    crp->cr_blklen2 = _conv.bytes.byte2;
    crp->cr_blklen1 = _conv.bytes.byte1;
    crp->cr_blklen0 = _conv.bytes.byte0;
   
    _conv.word = cr_lastlba;
    crp->cr_lastlba3 = _conv.bytes.byte3;
    crp->cr_lastlba2 = _conv.bytes.byte2;
    crp->cr_lastlba1 = _conv.bytes.byte1;
    crp->cr_lastlba0 = _conv.bytes.byte0;
#endif

#if	__BIG_ENDIAN__
    crp->cr_blklen	= cr_blklen;
    crp->cr_lastlba	= cr_lastlba;
#endif
}
