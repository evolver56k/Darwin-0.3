/*---------------------------------------------------------------------------*
 |                                                                           |
 |                            <<< TraceBack.h >>>                            |
 |                                                                           |
 |                Debugging TraceBack Table Layout Definitions               |
 |                                                                           |
 |                               Ira L. Ruben                                |
 |                                 06/28/93                                  |
 |                                                                           |
 |                  Copyright Apple Computer, Inc. 1993, 1998                |
 |                           All rights reserved.                            |
 |                                                                           |
 *---------------------------------------------------------------------------*
 
 This is a compiler independent representation of the AIX Version 3 traceback table (in
 sys/debug.h), which occurs, usually, one per procedure (routine).  The table is marked by
 a multiple of 4 32-bit word of zeroes in the instruction space.  The traceback table is
 also referred to as "procedure-end table".
 
 The AIX traceback table representation on which this header is based is defined as a
 series of bit field struct specifications.  Bit fields are compiler dependent!  Thus,
 the definitions presented here follow the original header and the existing documentation
 (such as it is), but define the fields as BIT MASKS and other macros.  The mask names,
 however, where chosen as the original field names to give some compatibility with the 
 original header and to agree with the documentation.
*/

#ifndef __TRACEBACK__
#define __TRACEBACK__
													/*------------------------------*
													 | TracebackTbl (fixed portion) |
													 *------------------------------*/

struct TracebackTbl {											/* Traceback table layout (fixed portion):		*/
	unsigned char version;									/* 		traceback format version								*/
	unsigned char lang;											/* 		language indicators:										*/
		#define TB_C		 				 0U						/*				C																		*/
		#define TB_FORTRAN			 1U						/*				FORTRAN															*/
		#define TB_PASCAL	 			 2U						/*				Pascal															*/
		#define TB_ADA		 			 3U						/*				ADA																	*/
		#define TB_PL1		 	 		 4U						/*				PL1																	*/
		#define TB_BASIC	 			 5U						/*				Basic																*/
		#define TB_LISP		 	 		 6U						/*				Lisp																*/
		#define TB_COBOL	 	 		 7U						/*				Cobol																*/
		#define TB_MODULA2			 8U						/*				Modula2															*/
		#define TB_CPLUSPLUS		 9U						/*				C++																	*/
		#define TB_RPG					10U						/*				RPG (you got to be kidding!)				*/
		#define TB_PL8					11U						/*				PL8																	*/
		#define TB_ASM					12U						/*				Asm																	*/
	
	unsigned char flags1;										/*		flag bits #1:														*/
		#define globallink 		0x80U						/*				routine is Global Linkage						*/
		#define is_eprol	 		0x40U						/*				out-of-line prolog or epilog routine*/
		#define has_tboff  		0x20U						/*				tb_offset set 		(extension field) */
		#define int_proc	 		0x10U						/*				internal leaf routine								*/
		#define has_ctl		 		0x08U						/*				has controlled automatic storage 		*/
		#define tocless		 		0x04U						/*				routine has no TOC									*/
		#define fp_present 		0x02U						/*				routine has floating point ops			*/			
		#define log_abort	 		0x01U						/*				fp_present && log/abort compiler opt*/
	
	unsigned char flags2;										/*		flag bits #2:														*/
		#define int_hndl			0x80U						/* 				routine is an interrupt handler    	*/
		#define name_present	0x40U						/* 				name_len/name set	(extension field)	*/
		#define uses_alloca		0x20U						/* 				uses alloca() to allocate storage 	*/
		#define cl_dis_inv		0x1CU						/* 				on-condition directives (see below) */
		#define saves_cr			0x02U   				/* 				routine saves the CR 								*/
		#define saves_lr			0x01U    				/* 				routine saves the LR 								*/
		
																					/*				cl_dis_inv "on condition" settings:	*/
		#define WALK_ONCOND		 	 0U						/* 					walk stack without restoring state*/
		#define DISCARD_ONCOND 	 1U						/* 					walk stack and discard 						*/
		#define INVOKE_ONCOND	 	 2U						/* 					invoke a specific system routine 	*/
	
		#define CL_DIS_INV(x) (((x) & cl_dis_inv) >> 2U)
		
	unsigned char flags3;										/*		flag bits #3:														*/
		#define stores_bc			0x80U				   	/* 				routine saves frame ptr of caller		*/
		#define spare2				0x40U      			/* 				spare bit 													*/
		#define fpr_saved			0x3FU   				/* 				number of FPRs saved (max of 32) 		*/
																					/*				(last reg saved is ALWAYS fpr31)		*/
		#define FPR_SAVED(x) ((x) & fpr_saved)
		
	unsigned char flags4;										/*		flag bits #4:														*/
		#define has_vec_info	0x80U						/*				routine uses vectors								*/
		#define spare3				0x40U      			/*				spare bit 													*/
		#define gpr_saved			0x3FU   				/*				number of GPRs saved (max of 32) 		*/
																					/*				(last reg saved is ALWAYS gpr31)		*/
		#define GPR_SAVED(x) ((x) & gpr_saved)
		
	unsigned char fixedparms;								/*		number of fixed point parameters				*/
	
	unsigned char flags5;										/*		flag bits #5:														*/
		#define floatparms 		0xFEU						/*				number of floating point parameters	*/
		#define parmsonstk		0x01U						/*				all parameters are on the stack			*/
	
		#define FLOATPARMS(x) (((x) & floatparms) >> 1U)
};
typedef struct TracebackTbl TracebackTbl, *TracebackTblPtr;

typedef TracebackTbl tbtable_short;				/* for compatibility (what little there is)		*/
#define TBTBLFIXSZ sizeof(TracebackTbl);


												 /*------------------------------------*
													| TracebackTbl (optional) extensions |
													*------------------------------------*/
													
/* Optional portions exist independently in the order presented below, not as a 				*/
/* structure or a union.  Whether or not portions exist is determinable from bit-fields */
/* within the fixed portion above.																											*/

/* The following is present only if fixedparms or floatparms are non zero and it				*/
/* immediately follows the fixed portion of the traceback table...											*/

unsigned long parminfo;  									/* Order and type encoding of parameters:			*/
																					/* 		Left-justified bit-encoding as follows:	*/
		#define FIXED_PARAM		0								/* 			'0'  ==> fixed param (1 gpr or word)	*/
		#define SPFP_PARAM		2								/* 			'10' ==> single-precision float param	*/
		#define DPFP_PARAM		3								/* 			'11' ==> double-precision float param */
		
		#define PARAM_ENCODING(x, bit) 				/*		yields xxx_PARAM as a function of "bit"	*/\
			((((x)&(1UL<<(31UL-(bit++))))==0UL) /*			values 0:31 (left-to-right). "bit" is	*/\
			  ? FIXED_PARAM 										/*			an L-value that's left incremented to	*/\
				: ((((x)&(1UL<<(31UL-(bit++))))==0)/*			the next bit position for the next		*/\
					? SPFP_PARAM 										/*			parameter.  This will be 1 or 2 bit 	*/\
					: DPFP_PARAM))									/*			positions later.											*/


/* The following is present only if has_tboff (in flags1) in fixed part is present...		*/

unsigned long tb_offset; 									/* Offset from start of code to TracebackTbl	*/


/* The following is present only if int_hndl (in flags2) in fixed part is present	...		*/

long hand_mask;														/* What interrupts are handled by the routine	*/


/* The following are present only if has_ctl (in flags1) in fixed part is present...		*/

struct AutoAnchors {											/* Controlled automatic storage info:					*/
	long ctl_info;													/* 		number of controlled automatic anchors	*/ 
	long ctl_info_disp[1];									/* 		array of stack displacements where each	*/
};																				/*		anchor is located (array STARTS here)		*/
typedef struct AutoAnchors AutoAnchors, *AutoAnchorsPtr;


/* The following are present only if name_present (in flags2) in fixed part is 					*/
/* present...																																						*/

struct RoutineName {											/* Routine name:															*/
	short name_len;													/* 		length of name that follows							*/
	char name[1];														/* 		name starts here (NOT null terminated)	*/
};
typedef struct RoutineName RoutineName, *RoutineNamePtr;


/* The following are present only if uses_alloca (in flags2) in fixed part is present...*/

char alloca_reg;													/* Register auto storage when alloca() is used*/


/* The following are present only if has_vec_info (in flags4) in fixed part is 					*/
/* present...																																						*/

struct VecInfo {													/* Vector info:																*/
	unsigned char vec_flags1;								/*		vec info bits #1:												*/
		#define vr_saved			0xFCU						/*				number of saved vector registers		*/
		#define saves_vrsave	0x02U						/*				saves VRsave												*/
		#define has_varargs		0x01U						/*				routine has a variable argument list*/

		#define VR_SAVED(x) (((x) & vr_saved) >> 2U)
	
	unsigned char vec_flags2;								/*		vec info bits #2:												*/
		#define vectorparms		0xFEU						/*				number of vector parameters					*/
		#define vec_present		0x01U						/*				routine uses at least one vec instr.*/
		
		#define VECPARMS(x) (((x) & vectorparms) >> 1U)
};
typedef struct VecInfo VecInfo, *VecInfoPtr;

#endif
