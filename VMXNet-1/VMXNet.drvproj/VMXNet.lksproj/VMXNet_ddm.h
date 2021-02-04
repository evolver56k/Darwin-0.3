/* 	Copyright (c) 1994 NeXT Computer, Inc.  All rights reserved. 
 *
 *
 * DDM macros for AMD SCSI driver.
 */
 
#import <driverkit/debugging.h>

 /*
 * The index into IODDMMasks[].
 */
#define VMX_DDM_INDEX	2

#define DDM_EXPORTED	0x00000001	// exported methods
#define DDM_IOTHREAD	0x00000002	// I/O thread methods
#define DDM_INIT	0x00000004	// Initialization
#define DDM_INTR 	0x00000008	// Interrupt

#define DDM_CONSOLE_LOG		0		/* really hosed...*/

#if	DDM_CONSOLE_LOG

#undef	IODEBUG
#define IODEBUG(index, mask, x, a, b, c, d, e) { 			\
	if(IODDMMasks[index] & mask) {					\
		IOLog(x, a, b, c,d, e); 				\
	}								\
}

#endif	DDM_CONSOLE_LOG


/*
 * catch both I/O thread events and interrupt events.
 */
#define ddm_intr(x, a, b, c, d, e) 					\
	IODEBUG(VMX_DDM_INDEX, (DDM_IOTHREAD | DDM_INTR), x, a, b, c, d, e)

