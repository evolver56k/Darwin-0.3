// Brutally ripped from SCSI.i and some other stuff added...
//	interfacer -asm -o SecondaryLoaderStructs.a SecondaryLoaderStructs.i

#pragma once

#pragma push
#pragma comments on

/* Signatures */
enum {
	sbSIGWord	= 0x4552,		/* signature word for Block 0 ('ER') */
	sbMac		= 1,			/* system type for Mac */
	pMapSIG		= 0x504D,		/* partition map signature ('PM') */
	pdSigWord	= 0x5453
};

enum {
	oldPMSigWord = pdSigWord,	
	newPMSigWord = pMapSIG
};

/* Partition Map Entry */

packed struct Partition {
	unsigned short			pmSig;				/* unique value for map entry blk */
	unsigned short			pmSigPad;			/* currently unused */
	unsigned long			pmMapBlkCnt;		/* # of blks in partition map */
	unsigned long			pmPyPartStart;		/* physical start blk of partition */
	unsigned long			pmPartBlkCnt;		/* # of blks in this partition */
	packed unsigned char	pmPartName[32];		/* ASCII partition name */
	packed unsigned char	pmParType[32];		/* ASCII partition type */
	unsigned long			pmLgDataStart;		/* log. # of partition's 1st data blk */
	unsigned long			pmDataCnt;			/* # of blks in partition's data area */
	unsigned long			pmPartStatus;		/* bit field for partition status */
	unsigned long			pmLgBootStart;		/* log. blk of partition's boot code */
	unsigned long			pmBootSize;			/* number of bytes in boot code */
	unsigned long			pmBootAddr;			/* memory load address of boot code */
	unsigned long			pmBootAddr2;		/* currently unused */
	unsigned long			pmBootEntry;		/* entry point of boot code */
	unsigned long			pmBootEntry2;		/* currently unused */
	unsigned long			pmBootCksum;		/* checksum of boot code */
	packed unsigned char	pmProcessor[16];	/* ASCII for the processor type */
	unsigned short			pmPad[188];			/* ARRAY[0..187] OF INTEGER; not used */
};

struct Extent {
	unsigned short start;
	unsigned short length;
};

struct MDB {
	unsigned short signature;			/* always $4244 for HFS volumes */
	unsigned long createTime;			/* date/time volume created */
	unsigned long modifyTime;			/* date/time volume last modified */
	unsigned short attributes;
	unsigned short rootValence;			/* number of files in the root directory */
	unsigned short bitmapStart;			/* first block of allocation bitmap */
	unsigned short reserved;			/* used by Mac for allocation ptr */
	unsigned short size;				/* number of allocation blocks on the volume */
	unsigned long allocSize;			/* size of an allocation block */
	unsigned long clumpSize;			/* minimum bytes per allocation */
	unsigned short allocStart;			/* 1st 512 byte block mapped in bitmap (bitmap is is au's though).*/
	unsigned long nextFID;				/* next available file ID */
	unsigned short freeCount;			/* number of free blocks on the volume */
	unsigned char name[28];				// Volume name as pascal string
	unsigned long backupTime;			/* date/time volume last backed-up */
	unsigned short sequenceNum;			/* volume sequence number */
	unsigned long writeCount;			/* number of writes to the volume */
	unsigned long extClumpSize;		/* clump size of extents B-tree file */
	unsigned long dirClumpSize;		/* clump size of directory B-tree file */
	unsigned short dirsInRoot;			/* number of directories in the root directory */
	unsigned long fileCount;			/* number of files on the volume */
	unsigned long dirCount;			/* number of directories on the volume */
	unsigned char finderInfo[32];		/* Finder information */
	unsigned char reserved2[6];			/* used internally */
	unsigned long extFileSize;			/* LEOF and PEOF of extents B-tree file */
	Extent extentsExtents[3];     /* first three extents of extents B-tree file */
	unsigned long dirFileSize;			/* LEOF and PEOF of directory B-tree file */
	Extent catalogExtents[3];     /* first three extents of directory B-tree file */
	unsigned char filler [350];
};

#pragma pop

