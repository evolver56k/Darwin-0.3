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

.include <${MAKETOP}bos/kernext/Kernext.mk>

KERNEL_EXT = atalk
ILIST = ${KERNEL_EXT}
IDIR = /usr/lib/drivers/pse/

atalk_ENTRYPOINT = config_atalk
atalk_EXPORTS = -bE:atalk.exp
atalk_IMPORTS = -bI:sockets.exp #-bI:atalk.imp

INCFLAGS = -I../h -I. -I..
#CFLAGS += -DUSE_MBLK
LIBS = ${KERNEL_LIBS} libsys.a libcsys.a libsrc.a

atalk_OFILES = \
	atp_alloc.o atp_misc.o atp_open.o atp_read.o atp_write.o asp_proto.o \
	ddp_aep.o ddp.o ddp_brt.o ddp_proto.o ddp_nbp.o ddp_rtmp.o \
	ddp_sip.o ddp_rtmptable.o ddp_r_rtmp.o ddp_r_zip.o ddp_aarp.o ddp_lap.o \
	aurp_aurpd.o aurp_cfg.o aurp_gdata.o \
	aurp_misc.o aurp_open.o aurp_rd.o aurp_ri.o aurp_rx.o \
	aurp_tickle.o aurp_tx.o aurp_zi.o \
	adsp_CLDeny.o adsp_Read.o adsp_Timer.o adsp_attention.o \
	adsp_CLListen.o adsp_NewCID.o adsp_TimerElem.o adsp_reset.o \
	adsp_Close.o adsp_Open.o adsp_RxAttn.o adsp_Write.o \
	adsp_Control.o adsp_RxData.o adsp.o adsp_Init.o adsp_Options.o \
	adsp_stream.o adsp_InitGlobals.o adsp_Packet.o adsp_Status.o \
	adsp_misc.o \
	sys_glue.o sys_dep.o drv_dep.o

.include <${RULES_MK}>
