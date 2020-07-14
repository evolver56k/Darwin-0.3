/* internal definitions for binary extendible hashing package */

#define _INTERNAL_H_

#include <stdlib.h>
#include <errno.h>
#include <architecture/byte_order.h>

#ifdef 	NeXT_PDO
#include <string.h>
#include <rpc/types.h>
#include <sys/stat.h>
#include <unistd.h>
#else	NeXT_PDO
#include <libc.h>
#endif 	NeXT_PDO


/*
 * Abosolutely brilliant.  Assume that sizeof(long) == 4.  The author of this
 * code should give classes on how to write non-portable code.  Other bitches:
 *
 *    (1) assuming that all architectures are big endian
 *    (2) storing/accessing shorts on unaligned boundaries
 *    (3) assuming all machines align things to 4 byte boundaries
 *
 * Define a 2-byte word type and a 4-byte word type.
 */

typedef unsigned short uword2;
typedef short word2;
typedef unsigned int uword4;
typedef int word4;

#define MAGIC	0x00d50001

/* 
 * the maximum depth, MAX_DEPTH, is non-portable.  It must be chosen
 * such that (2^MAX_DEPTH)*SL can be expressed in an unsigned long integer.
 * On the NeXT 68030 machine, a long is 32 bits so SL is 4;
 * 	the maximum directory size is therefore 0x80000000, or (2^29)*4  
 */
#define MAX_DEPTH	29

#define SL sizeof(uword4)
#define SS sizeof(uword2)

#define SDIR		sizeof(dhead)
#define SLHD		sizeof(lhead)
#define	MINOVERHEAD	(SLHD + (3 * SS)) /* amount of overhead in a leaf */

#define _MaxLeafBuf	128	/* maximum number of internal leaves */
#define _LeafHash	(_MaxLeafBuf + 13)
#define	dbMinLeafBuf	2	/* minimum no. of leaves needed for split */

				/* in-core directory, block-cached */
#define DBLOCK	4096		/* directory block size, number of bytes */
#define LPDB 	(DBLOCK/SL)	/* how many 4-byte words in such a block */
#define MAXOPEN	30		/* maximum number of open databases */

/* how to copy data into a Data buffer */
typedef enum	{
	dbCopyLength, 		/* copy length of key and length of data */
	dbCopyRecord, 		/* copy lengths and content of key and data */
	dbCopyPointer, 		/* copy lengths and pointer to key and data */
	dbCopyKeyOnly		/* copy length and content of key */
} dbCopyType;

/* leaf is sorted list of Data */
typedef struct	{	/* leaf header (in .L file) */
	uword2	flag;	/* leaf flag word */
	uword2	ldepth;	/* hash prefix length for this record */
	uword2	lsize;	/* size of this leaf when compressed */
	uword2	lrcnt;	/* how many records in the leaf */
	uword2	reserved;
	uword2	ldata;	/* byte count to beginning of data */
} lhead;		/* next (on disk) is array of record pointers */
			/* followed by the 'lrcnt' records */

/* a leaf in core */
typedef struct Leaf	{
	uword2	flag;	/* leaf descriptor flags */
	uword4	lp;	/* leaf address on disk */
	union	{		/* map buffer into leaf */
		char	*ulbuf;	/* start of a whole buffer */
		lhead	*ulhd;	/* leaf header always first */
	} leafp;
	uword2	*list;	/* pointer to list of rec pointers */
struct Leaf	*lnext;	/* open leaves chained on in-core hash table */
			/* free leaves chained on free chain */
} Leaf;

#define lhd	leafp.ulhd
#define lbuf	leafp.ulbuf

/* directory header in .D file */

typedef struct	{
	uword4	magic;	/* magic number */
	uword2	flag;	/* directory flag word */
	uword2	depth;	/* hash prefix length; 2^depth = number of entries */
	uword2	reserved;
	uword2	llen;	/* leaf size in bytes */
	uword4	dlcnt;	/* number of leaves in database */
} dhead;

#if defined(PROFILE) || defined(DEBUG)

/* leaf fetching statistics */
typedef struct bstat	{
	uword4	fetch; 	/* number of calls to _dbFetchLeaf */
	uword4	cleaf; 	/* number of current leaves reused */
	uword4	nopen; 	/* number of open leaves reused */
} bstat;

/* leaf accquisition statistics */
typedef struct astat	{
	uword4	total; 	/* number of calls to _dbLeafDescriptor */
	uword4	alloc;	/* allocations made by current database */
	uword4	cfree; 	/* free leaves reused; current database */
	uword4	ccycle; /* open leaves cycled; current database */
	uword4	sfree; 	/* free leaves reused; smaller leaf databases */
	uword4	scycle; /* open leaves cycled; smaller leaf databases */
	uword4	lfree; 	/* free leaves reused; larger leaf databases */
	uword4	lcycle; /* open leaves cycled; larger leaf databases */
} astat;

#endif

/* open database descriptor */
typedef struct Database	{
	char	*name;	/* name of database; no extension */
	int	flag;	/* directory flags; see defines below */
	int	D;	/* file descriptor for directory */
	int	L;	/* file descriptor for leaf file */
	uword2	cache;	/* cache limit for this database */
	uword2	alloc;	/* number of leaves allocated by this database */
	uword2	nopen;	/* number of open leaves in cache */
	uword2 	recno;	/* current record within the current leaf */
	dhead	d;	/* disk resident directory header */
	word4	clidx;	/* the directory index of the current leaf */
	Leaf	*cleaf;	/* pointer to the current leaf */
	Leaf	*free;	/* list of free leaves for this database */
	Leaf	*table[_LeafHash];	/* open leaf hash table */
	uword4	*dir;	/* in core copy of directory */
struct Database	*dnext; /* chain of open dbs for cache sharing */
#if defined(PROFILE) || defined(DEBUG)
	astat	dastat;	/* leaf fetching statistics */
	bstat	dbstat;	/* leaf accquisition statistics */
#endif
} Database;

/* file name construction macros */
#define BSIZE 200	/* DO NOT INCREASE BSIZE W/O CHECKING dbGlobals.c */
#define	_dbExtFile(s, t)	\
	strncat(strncat(strcpy(_dbstrbuf, ""), s, BSIZE - 1), t, BSIZE - 1)
#define _dbDirFile(s)	_dbExtFile(s, dbDirExtension)
#define _dbLeafFile(s)	_dbExtFile(s, dbLeafExtension)
#define _dbLockFile(s)	_dbExtFile(s, dbLockExtension)

/* return code definitions */
#define SUCCESS		1
#define FAILURE		0

/* flag bits used in disk resident leaf header */
#define	dbFlagCoalesceable	(1 << 0)	/* leaf is coalesceable */

/* additional flag bits used in leaf descriptor */
#define	dbFlagLeafOpen		(1 << 0)	/* leaf is in cache */
#define	dbFlagLeafDirty		(1 << 1)	/* leaf modified */
#define	dbFlagLeafMatch		(1 << 2)	/* leaf contains current key */

/* private flag bits used in directory descriptor - see db.h for public bits */
#define	dbFlagDirDirty (1 << 11)	/* directory modified, not written */

/* Byte swapping macros */

#ifdef __LITTLE_ENDIAN__

#define _dbSwapInWord4(x) ((x) = NXSwapBigIntToHost(x))
#define _dbSwapOutWord4(x) ((x) = NXSwapHostIntToBig(x))
#define _dbSwapInWord2(x) ((x) = NXSwapBigShortToHost(x))
#define _dbSwapOutWord2(x) ((x) = NXSwapHostShortToBig(x))
#define _unalignedWord2Value(s) (((uword2)(*((char *)(s)+1)) << 8) \
                                | ((uword2)(*(char *)(s))))

static inline uword2
_unalignedWord2Swap (uword2 *s)
{
    char *ss, c;

    c = *(ss = (char *) s);
    *ss = *(ss+1);
    *(ss+1) = c;

    return _unalignedWord2Value(s);
}

#define _unalignedSwapInWord2(s) _unalignedWord2Swap((uword2 *)(s))
#define _unalignedSwapOutWord2(s) _unalignedWord2Swap((uword2 *)(s))

#else

#define _dbSwapInWord4(x) (x)
#define _dbSwapInWord2(x) (x)
#define _dbSwapOutWord4(x) (x)
#define _dbSwapOutWord2(x) (x)
#define _unalignedWord2Value(s) (((uword2)(*(char *)(s)) << 8) \
                                | ((uword2)(*((char *)(s)+1))))
#define _unalignedSwapInWord2(s) _unalignedWord2Value((uword2 *)(s))
#define _unalignedSwapOutWord2(s) _unalignedWord2Value((uword2 *)(s))

#endif /* LITTLE_ENDIAN */



#if m68k || i386
/* macros to move 16 bits into and out of char[2] */
#define _dbSetLen(x,y)	(*((uword2 *) (x)) = ((uword2) y), \
					((u_char *) (x)) += SS)
#define _dbGetLen(y,x)	(((uword2) (y)) = *((uword2 *) (x)), \
					((u_char *) (x)) += SS)
#else

/* (might need bytewise versions on some machines) */

/*
 * How nice of you to assume that all other machines are big endian.
 */

#ifdef __LITTLE_ENDIAN__
#define _dbSetLen(x,y) (*((u_char *) (x))++ = ((u_char) y), \
                        *((u_char *) (x))++ = (u_char) (((uword2) y) >> 8))
#define _dbGetLen(y,x) (((uword2) y) = (uword2) (*((u_char *)(x))++), \
                        ((uword2) y) |= (uword2) (*((u_char *)(x))++) << 8)
#else
#define _dbSetLen(x,y)	(*((u_char *) (x))++ = (u_char) (((uword2) y) >> 8), \
				*((u_char *) (x))++ = ((u_char) y))
#define _dbGetLen(y,x)	(((uword2) y) = (uword2) (*((u_char *) (x))++) << 8, \
			 ((uword2) y) |= (uword2) (*((u_char *) (x))++))
#endif __LITTLE_ENDIAN__
#endif

/* functions made macros for speed */

#define _dbAllocLeaf(l)	(((l) = (Leaf *) calloc(1, sizeof(Leaf))) \
			? (((l)->lbuf = calloc(1, db->d.llen)) \
			? (l) : (free(l), ((l) = (Leaf *) 0))) : (l))

#define _dbLeafAddress(db, idx)	((db)->dir[(idx)])

#define	_dbFreeToHeap(l)	(free((l)->lbuf), free(l))

#define _dbFreeToList(db, l)	(((l)->lnext = (db)->free), ((db)->free = (l)))

#define	_dbGetFreeLeaf(db, l)	((db)->free ? (((l) = (db)->free), \
				((db)->free = (l)->lnext), (l)) : (Leaf *) 0)

#define	_dbSwapLeaf(l, tl)	(((l)->lbuf = (tl)->lbuf), \
				((tl)->lbuf = ((char *) (l)->list) - SLHD), \
				((l)->list = (tl)->list), \
				((tl)->list = (uword2 *) ((tl)->lbuf + SLHD)))

#define	__dbHashLeaf(db, lp)	(((lp) / (db)->d.llen) % _LeafHash)

#define _dbHashLeaf(db, lp)	((db)->table[__dbHashLeaf(db, lp)])

#define _dbOpenLeaf(db, l)	(++(db)->nopen, \
			((l)->flag |= dbFlagLeafOpen), \
		((l)->lnext = (db)->table[__dbHashLeaf(db, (l)->lp)]), \
			((db)->table[__dbHashLeaf(db, (l)->lp)] = (l)))

#define _dbReadLeaf(db, l)	\
	(((l) && ((l)->lp == lseek((db)->L, (l)->lp, 0))) && \
	(read((db)->L, (l)->lbuf, (db)->d.llen) >= \
		((l)->lhd->lsize ? NXSwapBigShortToHost((l)->lhd->lsize) : (db)->d.llen)))

#define	_dbVerifyWrite(fd, buf, len)	(write(fd, buf, len) == len)
			
#define _dbWriteLeaf(db, l)	(((db)->flag & dbFlagBuffered) ? \
				((l)->flag |= dbFlagLeafDirty) : \
						_dbFlushLeaf(db, l))

#define	_dbNewBuffer(db, l, p)	(((p) = malloc((db)->d.llen)) ? \
				(free((l)->lbuf), ((l)->lbuf = (p)), SUCCESS) \
							: FAILURE)

#define	_dbSetUpUserFlag(db)	((dbErrorNo = dbErrorNoError), \
			((db)->flag = ((db)->d.flag & ~((1 << 8) - 1)) \
					| ((db)->flag & ((1 << 8) - 1))))

#define _dbWriteDir(db)		(((db)->flag & dbFlagBuffered) ? \
				((db)->d.flag |= dbFlagDirDirty) : \
						_dbFlushDir(db))

#define _dbCreateFile(s)	(open(s, O_CREAT | O_TRUNC | O_RDWR, \
					dbMode & 0666))

/* externs */

extern char _dbstrbuf[];
extern int _dbLeafCount;	/* not static: used by dbClose */
extern Database *_DB;		/* not used previously -- commandeered for 
				chain of open dbs for buffer sharing */

#include "db.h"

/* internal functions are usually static, visible in debuggable library */

#ifdef SHLIB

#define visibility static

#else

#define	visibility extern

#endif

visibility int _dbDoubleDir(Database *db);
visibility int _dbReadDir(Database *db);
visibility int _dbFlushDir(Database *db);
visibility uword4 _dbHashKey(Data *d, uword2 depth);
visibility int 
_dbCopyData(Database *db, Leaf *l, uword2 k, Data *b, dbCopyType mode);
visibility int _dbFlushLeaf(Database *db, Leaf *l);
visibility int _dbSearchLeaf(Leaf *l, Data *d);
visibility int _dbPackLeaf(Database *db, Leaf *l);
visibility int _dbFreeLeaf(Database *db, Leaf *l, int freeFlag);
visibility Leaf *_dbCloseLeaf(Database *db, Leaf *l);
visibility Leaf *_dbCycleLeaf(Database *db);
visibility Leaf *_dbLeafDescriptor(Database *db);
visibility Leaf *_dbNewLeaf(Database *db);
visibility Leaf *_dbFetchLeaf(Database *db, word4 clidx);
visibility int _dbRemoveRecord(Database *db, Leaf *l, uword2 recno);
visibility int _dbPutRecord(Database *db, Leaf *l, uword2 recno, Data *d);
visibility int _dbSplitLeaf(Database *db);
visibility int
_dbFindRecord(Database *db, Data *d, word4 clidx, dbCopyType mode);
visibility int _dbGetRecord(Database *db, Data *d, dbCopyType mode);

