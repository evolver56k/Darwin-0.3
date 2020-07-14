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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOVGADisplayPrivate.h - Private definitions used by the IOVGADisplay
 * implementation.
 *
 * HISTORY
 * 28 Sep 92	Gary Crum
 *      Created. 
 */

// Color palette constants

#define WHITE_INDEX 3
#define LIGHT_GRAY_INDEX 2
#define DARK_GRAY_INDEX 1
#define BLACK_INDEX 0
// These values were computed given a gamma of 1.5.
// For most CRT displays, gamma should be 2.2, but this driver is
// used for flat panel LCD displays, so this 1.5 setting is a compromise.
#define WHITE_PALETTE_VALUE 63
#define LIGHT_GRAY_PALETTE_VALUE 48
#define DARK_GRAY_PALETTE_VALUE 30
#define BLACK_PALETTE_VALUE 0

// Frame buffer size
#define ET4000_VIDEO_W		640
#define ET4000_VIDEO_MW		640
#define ET4000_VIDEO_H		480
#define ET4000_VIDEO_MH		480
#define ET4000_VIDEO_NBPL	ET4000_VIDEO_MW / 8
#define ET4000_NPPW		32

// Address and size of the frame buffer
#define ET4000_PHYS_BASE	0xa0000
#define ET4000_BASE_ADR		(VM_MIN_KERNEL_ADDRESS + ET4000_PHYS_BASE)
#define ET4000_BASE_SIZ		0x10000

#define INB(addr)		{ inb(addr); }
#define INDB(addr,data)		{ data=inb(addr); }
#define OUTB(addr,data)		{ outb(addr,data); }

#define vga_reg_out(reg,indx,data)				\
	{							\
	char tmpi;						\
	INDB(READ_##reg##_ADDR, tmpi)				\
	tmpi &= ~reg##_MSK;					\
	tmpi |= (indx & reg##_MSK);				\
	OUTB(WRIT_##reg##_ADDR, tmpi)				\
	OUTB(WRIT_##reg##_DATA, data)				\
	}

#define vga_reg__in(reg,indx,data)				\
	{							\
	char tmpi;						\
	INDB(READ_##reg##_ADDR, tmpi)				\
	tmpi &= ~reg##_MSK;					\
	tmpi |= (indx & reg##_MSK);				\
	OUTB(WRIT_##reg##_ADDR, tmpi)				\
	INDB(READ_##reg##_DATA, data)				\
	}

#define vga_acr_out(reg,indx,data)				\
	{							\
	char tmpi;						\
	if (colr_mode)	INB(READ_COLR_GEN_IN_ST_1)		\
	else		INB(READ_MONO_GEN_IN_ST_1)	 	\
	INDB(READ_TOGL_ACR_ADDR, tmpi)				\
	tmpi &= ~ACR_MSK;					\
	tmpi |= (indx & ACR_MSK);				\
	OUTB(WRIT_TOGL_ACR_ADDR, tmpi)				\
	OUTB(WRIT_TOGL_ACR_DATA, data)				\
	}

#define vga_acr__in(reg,indx,data)				\
	{							\
	char tmpi;						\
	if (colr_mode)	INB(READ_COLR_GEN_IN_ST_1)		\
	else		INB(READ_MONO_GEN_IN_ST_1) 		\
	INDB(READ_TOGL_ACR_ADDR, tmpi)				\
	tmpi &= ~ACR_MSK;					\
	tmpi |= (indx & ACR_MSK);				\
	OUTB(WRIT_TOGL_ACR_ADDR, tmpi)				\
	INDB(READ_TOGL_ACR_DATA, data)				\
	}


/*
***********************************************************************************************
**				VGA :- General registers				     **
***********************************************************************************************
*/
#define	WRIT_MONO_GEN_MISC_OP	0x03C2
#define	WRIT_COLR_GEN_MISC_OP	0x03C2
#define	WRIT_EIDR_GEN_MISC_OP	0x03C2

#define	READ_MONO_GEN_MISC_OP	0x03CC
#define	READ_COLR_GEN_MISC_OP	0x03CC
#define	READ_EIDR_GEN_MISC_OP	0x03CC

#define READ_MONO_GEN_IN_ST_0	0x03C2
#define READ_COLR_GEN_IN_ST_0	0x03C2
#define READ_EIDR_GEN_IN_ST_0	0x03C2

#define READ_MONO_GEN_IN_ST_1	0x03BA
#define READ_COLR_GEN_IN_ST_1	0x03DA

#define WRIT_MONO_GEN_FEAT_CT	0x03BA
#define WRIT_COLR_GEN_FEAT_CT	0x03DA

#define READ_MONO_GEN_FEAT_CT	0x03CA
#define READ_COLR_GEN_FEAT_CT	0x03CA

/*
**	Index mask  - not necessary here
*/
#define MONO_GEN_MSK		0x00
#define COLR_GEN_MSK		0x00
#define EIDR_GEN_MSK		0x00

/*
**	List of index registers
*/

/*
**	Bitfields for misc op reg.
*/
#define GEN_AT_VSP		0x80
#define GEN_AT_HSP		0x40
#define GEN_AT__PB		0x00
#define GEN_AT_DVD		0x10
#define GEN_AT__CS		0x0C
#define GEN_AT__ER		0x02
#define GEN_AT_IOA		0x01

/*
**	Bitfields for feature control reg.
*/
#define	GEN_AT_VSS		0x08

/*
**	Bitfields for Input Status #0 register
*/
#define	GEN_AT__SS		0x10

/*
**	Bitfields for Input Status #1 register
*/
#define	GEN_AT__VR		0x08
#define	GEN_AT__DE		0x01

/*
***********************************************************************************************
**				VGA :- Sequencer registers				     **
***********************************************************************************************
*/
#define READ_MONO_SEQ_ADDR	0x03C4
#define READ_COLR_SEQ_ADDR	0x03C4
#define READ_EIDR_SEQ_ADDR	0x03C4

#define WRIT_MONO_SEQ_ADDR	0x03C4
#define WRIT_COLR_SEQ_ADDR	0x03C4
#define WRIT_EIDR_SEQ_ADDR	0x03C4

#define	READ_MONO_SEQ_DATA	0x03C5
#define	READ_COLR_SEQ_DATA	0x03C5
#define	READ_EIDR_SEQ_DATA	0x03C5

#define	WRIT_MONO_SEQ_DATA	0x03C5
#define	WRIT_COLR_SEQ_DATA	0x03C5
#define	WRIT_EIDR_SEQ_DATA	0x03C5

/*
**	Index mask
*/
#define MONO_SEQ_MSK		0x07
#define COLR_SEQ_MSK		0x07
#define EIDR_SEQ_MSK		0x07

/*
**	List of index registers
*/
#define SEQ_AT_RST		0x00
#define SEQ_AT_CLO		0x01
#define SEQ_AT_MPK		0x02
#define SEQ_AT_CRS		0x03
#define SEQ_AT_MMD		0x04

/*
**      Bitfield - Sequencer registers - Reset
*/
#define SEQ_AT__SR		0x02
#define SEQ_AT__AR		0x00

/*
**	Bitfields - Sequencer registers - Clocking Mode Register
*/
#define	SEQ_AT__SO		0x20
#define	SEQ_AT__S4		0x10
#define	SEQ_AT__DC		0x08
#define	SEQ_AT__SL		0x04
#define	SEQ_AT__89		0x01

/*
**      Bitfields - Sequencer registers - Map Mask Register
*/
#define SEQ_AT_EM3		0x08
#define SEQ_AT_EM2		0x04
#define SEQ_AT_EM1		0x02
#define SEQ_AT_EM0		0x01

/*
**	Bitfields - Sequencer registers - Character Map Select Registers
*/
#define	SEQ_AT_SAH		0x20
#define	SEQ_AT_SBH		0x10
#define	SEQ_AT__SA		0x0C
#define	SEQ_AT__SB		0x03

/*
**      Bitfields - Sequencer registers - Memory Mode Register
*/
#define SEQ_AT__C4		0x08
#define SEQ_AT__OE		0x04
#define SEQ_AT__EM		0x02

/*
***********************************************************************************************
**				VGA :- CRT Controller Registers				     **
***********************************************************************************************
*/
#define WRIT_MONO_CRT_ADDR	0x03B4
#define READ_MONO_CRT_ADDR	0x03B4

#define WRIT_MONO_CRT_DATA	0x03B5
#define READ_MONO_CRT_DATA	0x03D5

#define WRIT_COLR_CRT_ADDR	0x03D4
#define READ_COLR_CRT_ADDR	0x03D4

#define WRIT_COLR_CRT_DATA	0x03D5
#define READ_COLR_CRT_DATA	0x03D5

/*
**	Index mask
*/
#define MONO_CRT_MSK		0x3F
#define COLR_CRT_MSK		0x3F

/*
**	List of index registers
*/
#define CRT_AT_HORZ_TOT		0x00
#define CRT_AT_HORZ_DND		0x01
#define CRT_AT_HORZ_BST		0x02
#define CRT_AT_HORZ_BND		0x03
#define CRT_AT_HORZ_RST		0x04
#define CRT_AT_HORZ_RND		0x05
#define CRT_AT_VERT_TOT		0x06
#define CRT_AT_OVERFLOW		0x07
#define CRT_AT_PRE_ROWS		0x08
#define CRT_AT_MAX_SCAN		0x09
#define CRT_AT_CURSR_ST		0x0A
#define CRT_AT_CURSR_ND		0x0B
#define CRT_AT_ST_ADRHI		0x0C
#define CRT_AT_ST_ADRLO		0x0D
#define CRT_AT_CR_LOCHI		0x0E
#define CRT_AT_CR_LOCLO		0x0F
#define CRT_AT_VRT_RTST		0x10
#define CRT_AT_VRT_RTLO		0x11
#define CRT_AT_VRT_DSND		0x12
#define CRT_AT___OFFSET		0x13
#define CRT_AT_UNDR_LOC		0x14
#define CRT_AT_VBLNK_ST		0x15
#define CRT_AT_VBLNK_ND		0x16
#define CRT_AT_MOD_CTRL		0x17
#define CRT_AT_LINE_CMP		0x18

/*
**	Bitfields - CRT Controller registers - Horizontal total Register
*/
#define CRT_AT__HT		0xFF

/*
**	Bitfields - CRT Controller registers - Horizontal Display End Register
*/
#define	CRT_AT_HDE		0xFF

/*
**	Bitfields - CRT Controller registers - Start Horizontal Blanking register
*/
#define CRT_AT_SHB		0xFF

/*
**	Bitfields - CRT Controller registers - End Horizontal Blanking Register 
*/
#define	CRT_AT__CR		0x80
#define	CRT_AT_DES		0x60
#define	CRT_AT_EHB		0x1F

/*
**	Bitfields - CRT Controller registers - Start Horizontal Retrace Register
*/
#define	CRT_AT_SHR		0xFF

/*
**	Bitfields - CRT Controller registers - End Horizontal Retrace Register
*/
#define	CRT_AT_EHB_6		0x80
#define EGA_AT_CRT		0x80
#define	CRT_AT_HRD		0x60
#define	CRT_AT_EHR		0x1F

/*
**	Bitfields - CRT Controller registers - Vertical Total Register
*/
#define CRT_AT_VT		0xFF

/*
**	Bitfields - CRT Controller registers - Overflow Register
*/
#define CRT_AT_VRS_9		0x80
#define	CRT_AT_VDE_9		0x40
#define	EGA_AT_CRT_8		0x20
#define	CRT_AT_VT1_8		0x20
#define	CRT_AT__LC_8		0x10
#define	CRT_AT_VBS_8		0x08
#define	CRT_AT_VRS_8		0x04
#define	CRT_AT_VDE_8		0x02
#define	CRT_AT__VT_8		0x01

/*
**	Bitfields - CRT Controller registers - Preset Row Scan Register
*/
#define	CRT_AT__BP		0x60
#define	CRT_AT_PRS		0x1F

/*
**	Bitfields - CRT Controller registers - Maximum Scan Line Register
*/
#define CRT_AT_2T4		0x80
#define	CRT_AT__LC		0x40
#define	CRT_AT_VBS_9		0x20
#define	CRT_AT_MSL		0x1F

/*
**	Bitfields - CRT Controller registers - Cursor Start Register
*/
#define CRT_AT_COO		0x20
#define CRT_AT__CS		0x1F

/*
**	Bitfields - CRT Controller registers - Cursor End Register
*/
#define CRT_AT_CSK		0x60
#define	CRT_AT__CE		0x1F

/*
**	Bitfields - CRT Controller registers - Start Address High Register
*/
#define CRT_AT_SAH		0xFF

/*
**	Bitfields - CRT Controller registers - Start Address Low Register
*/
#define	CRT_AT_SAL		0xFF

/*
**	Bitfields - CRT Controller registers - Cursor Location High Register
*/
#define	CRT_AT_CLH		0xFF

/*
**	Bitfields - CRT Controller registers - Cursor Location Low Register
*/
#define	CRT_AT_CLL		0xFF

/*
**	Bitfields - CRT Controller registers - Vertical Retrace Start Register
*/
#define	CRT_AT_VRS		0xFF

/*
**	Bitfields - CRT Controller registers - Vertical Retrace End Register
*/
#define	CRT_AT__PR		0x80
#define	CRT_AT__BW		0x40
#define CRT_AT_DVI		0x20
#define	CRT_AT_CVI		0x10
#define	CRT_AT_EVR		0x0F

/*
**	Bitfields - CRT Controller registers - Light Pen High Register 
*/
#define	CRT_AT_LPH		0xFF

/*
**	Bitfields - CRT Controller registers - Light Pen Low Register
*/
#define CRT_AT_LPL		0xFF

/*
**	Bitfields - CRT Controller registers - Vertical Display End Register
*/
#define CRT_AT_VDE		0xFF

/*
**	Bitfields - CRT Controller registers - Offset Register
*/
#define	CRT_AT_OFF		0xFF

/*
**	Bitfields - CRT Controller registers - Underline Location Register
*/
#define	CRT_AT__DW		0x40
#define	CRT_AT_CB4		0x20
#define	CRT_AT__UL		0x1F

/*
**	Bitfields - CRT Controller registers - Start vertical Blank register
*/
#define CRT_AT_VBS		0xFF

/*
**	Bitfields - CRT Controller registers - End Vertical Blank register
*/
#define	CRT_AT_VBE_H		0x60
#define	CRT_AT_VBE_L		0x1F

/*
**	Bitfields - CRT Controller registers - Mode Control Register
*/
#define	CRT_AT__HR		0x80
#define	CRT_AT__WB		0x40
#define	CRT_AT__AW		0x20
#define	CRT_AT__OC		0x10
#define	CRT_AT_CBT		0x08
#define	CRT_AT_HRS		0x04
#define	CRT_AT_SRS		0x02
#define	CRT_AT_CMS		0x01

/*
**	Bitfields - CRT Controller registers - Line Compare Register
*/
#define	CRT_AT_LNC		0xFF

/*
***********************************************************************************************
**				VGA :- GCR Controller Registers				     **
***********************************************************************************************
*/
#define	READ_MONO_GCR_ADDR	0x03CE
#define	READ_COLR_GCR_ADDR	0x03CE
#define	READ_EIDR_GCR_ADDR	0x03CE

#define	WRIT_MONO_GCR_ADDR	0x03CE
#define	WRIT_COLR_GCR_ADDR	0x03CE
#define	WRIT_EIDR_GCR_ADDR	0x03CE

#define	READ_MONO_GCR_DATA	0x03CF
#define	READ_COLR_GCR_DATA	0x03CF
#define	READ_EIDR_GCR_DATA	0x03CF

#define	WRIT_MONO_GCR_DATA	0x03CF
#define	WRIT_COLR_GCR_DATA	0x03CF
#define	WRIT_EIDR_GCR_DATA	0x03CF

/*
**	Index mask
*/
#define MONO_GCR_MSK		0x0F
#define COLR_GCR_MSK		0x0F
#define EIDR_GCR_MSK		0x0F

/*
**	List of index registers
*/
#define	GCR_AT_SET_RESET	0x00
#define	GCR_AT_ENA_S_RST	0x01
#define	GCR_AT_COLR_COMP	0x02
#define	GCR_AT_DATA_ROTR	0x03
#define	GCR_AT_READ_MAPS	0x04
#define	GCR_AT_GCR__MODE	0x05
#define	GCR_AT_GCR__MISC	0x06
#define	GCR_AT_CLR_NOCAR	0x07
#define	GCR_AT_BIT__MASK	0x08

/*
**	Bitfields - Graphics Controller Registers - Graphics#1 Position Register
*/
#define	GCR_AT_GP1		0x03

/*
**	Bitfields - Graphics Controller Registers - Graphics#2 Position Register
*/
#define	GCR_AT_GP2		0x03

/*
**	Bitfields - Graphics Controller Registers - Set/Reset Register
*/
#define	GCR_AT_S_R		0x0F

/*
**	Bitfields - Graphics Controller Registers - Enable Set/Reset Register
*/
#define	GCR_AT_ESR		0x0F

/*
**	Bitfields - Graphics Controller Registers - Color Compare Register
*/
#define	GCR_AT__CC		0x0F

/*
**	Bitfields - Graphics Controller Registers - Data Rotate register
*/
#define	GCR_AT__FS		0x30
#define	GCR_AT__RC		0x0F

/*
**	Bitfields - Graphics Controller Registers - Read Map Select Register
*/
#define	GCR_AT_RMS		0x03

/*
**	Bitfields - Graphics Controller Registers - Mode Register
*/
#define	GCR_AT__SR		0x60
#define	GCR_AT__OE		0x10
#define	GCR_AT__RM		0x08
#define	GCR_AT__TC		0x04
#define	GCR_AT__WM		0x03

/*
**	Bitfields - Graphics Controller Registers - Miscellaneous register
*/
#define	GCR_AT__MM		0x0C
#define	GCR_AT_COE		0x02
#define	GCR_AT__GA		0x01

/*
**	Bitfields - Graphics Controller Registers - Color Dont Care register
*/
#define	GCR_AT_CDC		0x0F

/*
**	Bitfields - Graphics Controller Registers - Bit Mask
*/
#define	GCR_AT__BM		0xFF

/*
***********************************************************************************************
**				VGA :- ACR Controller Registers				     **
***********************************************************************************************
*/

#define	WRIT_TOGL_ACR_ADDR	0x03C0
#define	READ_TOGL_ACR_ADDR	0x03C0

#define	WRIT_TOGL_ACR_DATA	0x03C0
#define	READ_TOGL_ACR_DATA	0x03C1

/*
**	Index mask
*/
#define ACR_MSK			0x1F

/*
**	List of index registers
*/
#define	ACR_AT_PALETTE_0	0x00
#define	ACR_AT_PALETTE_1	0x01
#define	ACR_AT_PALETTE_2	0x02
#define	ACR_AT_PALETTE_3	0x03
#define	ACR_AT_PALETTE_4	0x04
#define	ACR_AT_PALETTE_5	0x05
#define	ACR_AT_PALETTE_6	0x06
#define	ACR_AT_PALETTE_7	0x07
#define	ACR_AT_PALETTE_8	0x08
#define	ACR_AT_PALETTE_9	0x09
#define	ACR_AT_PALETTE_A	0x0A
#define	ACR_AT_PALETTE_B	0x0B
#define	ACR_AT_PALETTE_C	0x0C
#define	ACR_AT_PALETTE_D	0x0D
#define	ACR_AT_PALETTE_E	0x0E
#define	ACR_AT_PALETTE_F	0x0F
#define ACR_AT_MODE_CNTL	0x10
#define	ACR_AT_OVERSCN_C	0x11
#define	ACR_AT_CLR_PL_EN	0x12
#define	ACR_AT_HORZ_PXPN	0x13
#define	ACR_AT_COLOR_SEL	0x14

/*
**	Bitfields - Attribute Controller registers - pallette registers
*/
#define	ACR_AT__SR		0x20
#define	ACR_AT__SG		0x10
#define	ACR_AT__SB		0x08
#define	ACR_AT_REG		0x04
#define	ACR_AT_GRN		0x02
#define	ACR_AT_BLU		0x01

/*
**	Bitfields - Attribute Controller registers -  Mode Control register
*/
#define	ACR_AT_IPS		0x80
#define	ACR_AT_PCS		0x40
#define	ACR_AT_PPC		0x20
#define	ACR_AT__BI		0x08
#define	ACR_AT_ELG		0x04
#define	ACR_AT__DT		0x02
#define	ACR_AT__GA		0x01

/*
**	Bitfields - Attribute Controller registers -  Overscan Color Register
*/

#define	ACR_AT_OSR		0x20
#define	ACR_AT_OSG		0x10
#define	ACR_AT_OSB		0x08
#define	ACR_AT_ORD		0x04
#define	ACR_AT_OGR		0x02
#define	ACR_AT_OBL		0x01
/*
**	Bitfields - Attribute Controller registers - Color Plane Enable Register
*/

#define	ACR_AT_VSM		0x30
#define	ACR_AT_CPE		0x0F
/*
**	Bitfields - Attribute Controller registers - Horizontal Pixel Panning Reg
*/
#define	ACR_AT_HPP		0x0F

/*
**	Bitfields - Attribute Controller registers - Color Select Register
*/
#define	ACR_AT_C67		0x0C
#define	ACR_AT_C45		0x03

/*
***********************************************************************************************
**				VGA :- PEL Controller Registers				     **
***********************************************************************************************
*/
#define	WRIT_MONO_PEL_AWMR	0x03C8
#define	WRIT_COLR_PEL_AWMR	0x03C8
#define	READ_MONO_PEL_AWMR	0x03C8
#define	READ_COLR_PEL_AWMR	0x03C8

#define	WRIT_MONO_PEL_ARMR	0x03C7
#define	WRIT_COLR_PEL_ARMR	0x03C7

#define	WRIT_MONO_PEL_DATA	0x03C9
#define	WRIT_COLR_PEL_DATA	0x03C9
#define	READ_MONO_PEL_DATA	0x03C9
#define	READ_COLR_PEL_DATA	0x03C9

#define	READ_MONO_PEL_DACS	0x03C7
#define	READ_COLR_PEL_DACS	0x03C7

#define	WRIT_MONO_PEL_MASK	0x03C6
#define	WRIT_COLR_PEL_MASK	0x03C6
#define	READ_MONO_PEL_MASK	0x03C6
#define	READ_COLR_PEL_MASK	0x03C6

/*
***********************************************************************************************
**				  VGA :- External Palette RAM
***********************************************************************************************
*/
#define	READ_MONO_PEL_MASK	0x03C6
#define	READ_COLR_PEL_MASK	0x03C6
#define	READ_EIDR_PEL_MASK	0x03C6

#define	WRIT_MONO_PEL_MASK	0x03C6
#define	WRIT_COLR_PEL_MASK	0x03C6
#define	WRIT_EIDR_PEL_MASK	0x03C6

#define	READ_MONO_PEL_WADR	0x03C8
#define	READ_COLR_PEL_WADR	0x03C8
#define	READ_EIDR_PEL_WADR	0x03C8

#define	WRIT_MONO_PEL_WADR	0x03C8
#define	WRIT_COLR_PEL_WADR	0x03C8
#define	WRIT_EIDR_PEL_WADR	0x03C8

#define	READ_MONO_PEL_DATA	0x03C9
#define	READ_COLR_PEL_DATA	0x03C9
#define	READ_EIDR_PEL_DATA	0x03C9

#define	WRIT_MONO_PEL_DATA	0x03C9
#define	WRIT_COLR_PEL_DATA	0x03C9
#define	WRIT_EIDR_PEL_DATA	0x03C9

#define	WRIT_MONO_PEL_RADR	0x03C7
#define	WRIT_COLR_PEL_RADR	0x03C7
#define	WRIT_EIDR_PEL_RADR	0x03C7

#define	WRIT_MONO_PEL_DACS	0x03C7
#define	WRIT_COLR_PEL_DACS	0x03C7
#define	WRIT_EIDR_PEL_DACS	0x03C7

/*
***********************************************************************************************
**				VGA :- MODES						     **
***********************************************************************************************
*/
#define	MAX_GEN_INDEX		0x01
#define	MAX_SEQ_INDEX		SEQ_AT_MMD
#define	MAX_CRT_INDEX		CRT_LINE_CMP
#define	MAX_GCR_INDEX		GCR_BIT__MASK
#define	MAX_ACR_INDEX		ACR_COLOUR_SEL

/* 
** There now follows a symbolic list of modes, these will be used as indeces into tables of
** initial values to be put into the various registers for a given mode.
*/

#define	MODE_VGA_AT_00		0x00
#define	MODE_VGA_AT_01		0x01
#define	MODE_VGA_AT_02		0x02
#define	MODE_VGA_AT_03		0x03
#define	MODE_VGA_AT_04		0x04
#define	MODE_VGA_AT_05		0x05
#define	MODE_VGA_AT_06		0x06
#define	MODE_VGA_AT_07		0x07
#define	MODE_VGA_AT_00_X	0x08
#define	MODE_VGA_AT_01_X	0x09
#define	MODE_VGA_AT_02_X	0x0A
#define	MODE_VGA_AT_03_X	0x0B
#define	MODE_VGA_AT_07_X	0x0C
#define	MODE_VGA_AT_0D		0x0D
#define	MODE_VGA_AT_0E		0x0E
#define	MODE_VGA_AT_0F		0x0F
#define	MODE_VGA_AT_10		0x10
#define	MODE_VGA_AT_11		0x11
#define	MODE_VGA_AT_12		0x12
#define	MODE_VGA_AT_13		0x13

#define MIN_VGA_AT_MODE	MODE_VGA_AT_00
#define MAX_VGA_AT_MODE	MODE_VGA_AT_13
#define NUM_VGA_AT_MODE MAX_VGA_AT_MODE-MIN_VGA_AT_MODE

#define IS_REG_MODE(a) ((a) <= MAX_VGA_AT_MODE)

struct mode_params	{
	char	*m_str;
	int	p_type;
	int	p_bpp;
	int	p_start;
	int	p_pages;
	int	p_alfa_w;
	int	p_alfa_h;
	int	p_char_w;
	int	p_char_h;
	int	p_disp_w;
	int	p_disp_h;
	int	p_font;
	};

/*
** Defines for the type of a particular mode ie - is it alphanummeric(A) or graphical(G),  
**	color(C) or mono(M), 
*/

#define	MODE_A_M 0x00
#define MODE_A_C 0x01
#define MODE_G_M 0x10
#define MODE_G_C 0x11

/*
***********************************************************************************************
**				VGA :- FONTS						     **
***********************************************************************************************
*/

#define FONT_09_BY_16	0x00
#define FONT_09_BY_15	0x01
#define FONT_09_BY_14	0x02
#define FONT_09_BY_13	0x03
#define FONT_08_BY_16	0x04
#define FONT_08_BY_15	0x05
#define FONT_08_BY_14	0x06
#define FONT_08_BY_08	0x07


#define VGA_AT_MAX_PLANES 0x04


// Start of defines from prototype vga_ts_defs.h file.

/*
***********************************************************************************************
**				  VGA :- General registers					  **
***********************************************************************************************
*/
#define WRIT_MONO_MODE_CT	0x03B8
#define WRIT_COLR_MODE_CT	0x03D8
#define WRIT_EIDR_MODE_CT	0x03D8

#define WRIT_MONO_GEN_VID_EN	0x46E8
#define WRIT_COLR_GEN_VID_EN	0x46E8
#define WRIT_EIDR_GEN_VID_EN	0x46E8

#define READ_MONO_GEN_VID_EN	0x03C3
#define READ_COLR_GEN_VID_EN	0x03C3
#define READ_EIDR_GEN_VID_EN	0x03C3

#define WRIT_MONO_ALT_VID_EN	0x46E8
#define WRIT_COLR_ALT_VID_EN	0x46E8
#define WRIT_EIDR_ALT_VID_EN	0x46E8

#define READ_MONO_ALT_VID_EN	0x46E8
#define READ_COLR_ALT_VID_EN	0x46E8
#define READ_EIDR_ALT_VID_EN	0x46E8

/*
**	Bitfields for Hercules compatibility register
*/
#define	GEN_TS_HER		0xFD
#define	GEN_TS_ESP		0x02

/*
**	Bitfields for Video Subsystem Register
*/
#define	GEN_TS_WRI		0x01
#define GEN_TS_REA		0x08
#define	GEN_TS_VSR		0xF6

/*
***********************************************************************************************
**				  VGA :- 6845 Compatibility registers				**
***********************************************************************************************
*/
#define WRIT_MONO_6845_CTL_REG	0x03B4
#define WRIT_COLR_6845_CTL_REG	0x03D4
#define READ_MONO_6845_CTL_REG	0x03B4
#define READ_COLR_6845_CTL_REG	0x03D4

#define WRIT_MONO_6845_DTA_REG	0x03B5
#define WRIT_COLR_6845_DTA_REG	0x03D5
#define READ_MONO_6845_DTA_REG	0x03B5
#define READ_COLR_6845_DTA_REG	0x03D5

#define WRIT_MONO_6845_DMC_REG	0x03B8
#define WRIT_COLR_6845_DMC_REG	0x03D8
#define READ_MONO_6845_DMC_REG	0x03B8
#define READ_COLR_6845_DMC_REG	0x03D8

#define WRIT_6845_DCC_REG	0x03D9

#define WRIT_MONO_6845_DSC_REG	0x03BA
#define WRIT_COLR_6845_DSC_REG	0x03DA
#define READ_MONO_6845_DSC_REG	0x03BA
#define READ_COLR_6845_DSC_REG	0x03DA

#define WRIT_6845_ATT_REG	0x03DE
#define WRIT_HERCULES_REG	0x03BF

/*
***********************************************************************************************
**				  VGA :- Sequencer registers					**
***********************************************************************************************
*/

/*
**      New Registers 
*/

/*
**      List of index registers
*/
#define	SEQ_TS_TSS		0x06
#define	SEQ_TS_TAM		0x07

/*
**	Bitfields - Sequencer Registers - TS state control 				Tseng Labs
*/
#define SEQ_TS_TSS		0x06

/*
**	Bitfields - Sequencer Registers - TS Auxiliary mode				Tseng Labs
*/
#define	SEQ_TS_VGM		0x80
#define	SEQ_TS_MC2		0x40
#define	SEQ_TS_RO2		0x20
#define	SEQ_TS_RO1		0x08
#define	SEQ_TS_SC2		0x02
#define	SEQ_TS_MC4		0x01
#define	SEQ_TS_MSK		0x14

/*
***********************************************************************************************
**				  VGA :- CRT Controller Registers					 **
***********************************************************************************************
*/

/*
**      New Registers 
*/

/*
**      List of index registers
*/
#define	CRT_TS_RAS		0x32
#define	CRT_TS_ESA		0x33
#define	CRT_TS_CTL		0x34
#define	CRT_TS_OVH		0x35
#define	CRT_TS_VS1		0x36
#define	CRT_TS_VS2		0x37

/*
**	Bitfields - CRT Controller registers - RAS/CAS Configuration Reggister	Tseng Labs
*/
#define	CRT_TS_SCM		0x80
#define	CRT_TS_RAL		0x40
#define	CRT_TS_RCD		0x20
#define	CRT_TS_RSP		0x18
#define	CRT_TS_CSP		0x04
#define	CRT_TS_CSW		0x03

/*
**	Bitfields - CRT Controller registers - Extended Start Register		Tseng Labs
*/
#define	CRT_TS_CSA		0x0C
#define	CRT_TS_LSA		0x03

/*
**	Bitfields - CRT Controller registers - 6845 Compatibility Control Register	Tseng Labs
*/
#define	CRT_TS_CMP		0x80
#define	CRT_TS_EBA		0x40
#define	CRT_TS_EXL		0x20
#define	CRT_TS_EXR		0x10
#define	CRT_TS_EVS		0x08
#define	CRT_TS_TRI		0x04
#define	CRT_TS_MCK		0x02
#define CRT_TS_EMK		0x01

/*
**	Bitfields - CRT Controller registers - Overflow High				Tseng Labs
*/
#define CRT_TS_VIM		0x80
#define	CRT_TS_ARW		0x40
#define	CRT_TS_ESR		0x20
#define	CRT_TS_LCA		0x10
#define	CRT_TS_VSA		0x08
#define	CRT_TS_VDA		0x04
#define	CRT_TS_VTA		0x02
#define	CRT_TS_VBA		0x01

/*
**	Bitfields - CRT Controller registers - Video System Configuration 1		Tseng Labs
*/
#define CRT_TS_RWF		0x80
#define	CRT_TS_DMF		0x40
#define	CRT_TS_AMD		0x20
#define	CRT_TS_SGL		0x10
#define	CRT_TS_FWF		0x08
#define	CRT_TS_REF		0x07

/*
**	Bitfields - CRT Controller registers - Video System Configuration 2		Tseng Labs
*/
#define CRT_TS_DRM		0x80
#define	CRT_TS_TST		0x40
#define	CRT_TS_PTC		0x20
#define	CRT_TS_RM8		0x10
#define	CRT_TS_DMD		0x0C
#define	CRT_TS_DBW		0x03

/*
***********************************************************************************************
**				  VGA :- GCR Controller Registers				  **
***********************************************************************************************
*/

/*
**      New Registers 
*/
#define	READ_MONO_GCR_SEGS	0x03CD
#define	READ_COLR_GCR_SEGS	0x03CD
#define	READ_EIDR_GCR_SEGS	0x03CD

#define	WRIT_MONO_GCR_SEGS	0x03CD
#define	WRIT_COLR_GCR_SEGS	0x03CD
#define	WRIT_EIDR_GCR_SEGS	0x03CD

/*
**      List of index registers
*/

/*
**	Bitfields - GCR Controller registers - Segment Select Register			Tseng Labs
*/
#define GCR_TS_GRD		0xF0
#define GCR_TS_GWR		0x0F

/*
***********************************************************************************************
**				  VGA :- ACR Controller Registers				  **
***********************************************************************************************
*/

/*
**      New Registers 
*/

/*
**      List of index registers
*/
#define	ACR_TS_MSC		0x16

/*
**	Bitfields - Attribute Controller Registers - Miscellaneous			Tseng Labs
*/
#define	ACR_TS_BYP		0x80
#define	ACR_TS_2BC		0x40
#define	ACR_TS_SHR		0x30
#define	ACR_TS_RES		0x0F

/*
***********************************************************************************************
**                              VGA :- MODES                                                 **
***********************************************************************************************
*/
#define MAX_TS_GEN_INDEX 	0x01
#define MAX_TS_SEQ_INDEX  	SEQ_TS_TAM
#define MAX_TS_CRT_INDEX  	CRT_TS_VS2
#define MAX_TS_GCR_INDEX  	GCR_AT_BIT__MASK
#define MAX_TS_ACR_INDEX  	ACR_TS_MISC

/*
** There now follows a symbolic list of modes, these will be used as indeces into tables of
** initial values to be put into the various registers for a given mode.
*/

#define	MODE_VGA_TS_18	(MAX_VGA_AT_MODE + 0x01)
#define	MODE_VGA_TS_19	(MAX_VGA_AT_MODE + 0x02)
#define	MODE_VGA_TS_1A	(MAX_VGA_AT_MODE + 0x03)
#define	MODE_VGA_TS_22	(MAX_VGA_AT_MODE + 0x04)
#define	MODE_VGA_TS_23	(MAX_VGA_AT_MODE + 0x05)
#define	MODE_VGA_TS_24	(MAX_VGA_AT_MODE + 0x06)
#define	MODE_VGA_TS_25	(MAX_VGA_AT_MODE + 0x07)
#define	MODE_VGA_TS_26	(MAX_VGA_AT_MODE + 0x08)
#define	MODE_VGA_TS_29	(MAX_VGA_AT_MODE + 0x09)
#define	MODE_VGA_TS_2A	(MAX_VGA_AT_MODE + 0x0A)
#define	MODE_VGA_TS_2D	(MAX_VGA_AT_MODE + 0x0B)
#define	MODE_VGA_TS_2E	(MAX_VGA_AT_MODE + 0x0C)
#define	MODE_VGA_TS_30	(MAX_VGA_AT_MODE + 0x0D)
#define	MODE_VGA_TS_37i	(MAX_VGA_AT_MODE + 0x0E)
#define	MODE_VGA_TS_37n	(MAX_VGA_AT_MODE + 0x0F)
#define	MODE_VGA_TS_2F	(MAX_VGA_AT_MODE + 0x10)
#define	MODE_VGA_TS_38i	(MAX_VGA_AT_MODE + 0x11)
#define	MODE_VGA_TS_38n	(MAX_VGA_AT_MODE + 0x12)

#define MAX_VGA_TS_MODE	MODE_VGA_TS_38n
#define MIN_VGA_TS_MODE	MODE_VGA_TS_18
#define NUM_VGA_TS_MODE (MAX_VGA_TS_MODE-MIN_VGA_TS_MODE)

/*
***********************************************************************************************
**                              Tseng Labs Specific                                                 **
***********************************************************************************************
*/

#define CHIP_IS_ET3000	0x10
#define CHIP_IS_ET4000	0x11

#define VGA_TS_MAX_SIZE 0x10000
