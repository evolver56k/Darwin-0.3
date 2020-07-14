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
 * sctest - test of SCSIController object. Linked with libSCSIController.o.
 */
 
#define FAKE_HARDWARE	1

#import <bsd/sys/types.h>
#import <bsd/libc.h>
#import <SCSIController/SCSIController.h>
#import <SCSIController/SCSIControllerPrivate.h>
#import <architecture/nrw/SCSI_channel.h>
#import <driverkit/return.h>
#import <driverkit/align.h>

/*
 * prototypes for private functions.
 */
static void usage(char **argv);
static void print_menu();
static void setupScsiReq(scsiReq_t *scsiReq);
void dump_buf(u_char *buf, int size);

/*
 * Menu stuff.
 */
struct menu_entry {
	char 	menu_c;
	char	*name;
	void	(*men_fcn)();
};

static void scReset();
static void cdbRead();
static void cdbWrite();
static void setTarget();
static void setLun();
static void quit();
static void setupCDB();
static void setDmaMax();
static void setDmaDir();
static void dumpBuf();

struct menu_entry me_array[] = {

    	{'t', 	"Reset" 	, scReset	},
	{'r',	"cdbRead" 	, cdbRead	},
	{'w',	"cdbWrite" 	, cdbWrite	},
	{'T',	"Set Target"	, setTarget	},
	{'L',	"Set lun"	, setLun	},
	{'c',	"Setup CDB"	, setupCDB	},
	{'d',	"Set DMA Size"	, setDmaMax	},
	{'D',	"Set DMA Dir"	, setDmaDir	},
	{'b',	"Dump r/w buf"  , dumpBuf	},
	{'h',	"Help"		, print_menu	},
	{'x',	"Exit"		, quit		},
	{'q',	NULL		, quit		},
	{0,	NULL		, NULL		}
};

#define MAX_DMA_SIZE	(8 * 512)

/*
 * Static variables.
 */
int ioTimeout = 10;
int target = 0;
int lun = 0;
id controllerId;
u_char cdb_bytes[12];
u_int dmaMax = MAX_DMA_SIZE;
SCSIDmaDir_t dmaDir = SCSI_DMA_READ;
char *rwbuf;
char *rwbuf_unalign;

/*
 * User-specified variables.
 */
int verbose = 0;		/* used by libIODevice */
int ioStub = 0;

int main(int argc, char **argv)
{
	int arg;
	char instr[80];
	struct	menu_entry *mep;
	int ok;
	
	if(argc < 1)
		usage(argv);
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'v':
		    	verbose++;
			break;
		    case 's':
		    	ioStub++;
			break;
		    default:
		    	usage(argv);
		}
	}

	rwbuf_unalign = malloc(MAX_DMA_SIZE + 2 * SCSI_BUFSIZE);
	rwbuf = IOAlign(char *, rwbuf_unalign, SCSI_BUFSIZE);
	
        IOInitGeneralFuncs();
#ifdef	DDM_DEBUG
	IOInitDDM(200, "scxpr");
#endif	DDM_DEBUG

	/*
	 * Instantiate and init the controller object.
	 */
	controllerId = [SCSIController probe:0 deviceMaster:PORT_NULL];
	if(controllerId == nil) {
		printf("SCSIController probe FAILED\n");
		exit(-1);
	}
	
	print_menu();
	while(1) {
		printf("Enter Selection: ");
		gets(instr);
		mep = me_array;
		ok = 0;
		while(mep->menu_c) {
			if(mep->menu_c == instr[0]) {
				ok = 1;
				(*mep->men_fcn)();
				break;
			}
			else
				mep++;
		}
		if(!ok)
			printf("***Illegal Selection\n");
	}
	return(0);
}

static void usage(char **argv)
{
	printf("usage: %s [options]\n", argv[0]);
	printf("Options:\n");
	printf("\tv  verbose mode\n");
	printf("\ts  Use I/O stub\n");
	exit(1);
}

static void print_menu() {
	
	struct menu_entry *mep;
	
	printf("\n");
	mep = me_array;
	while(mep->menu_c) {
		if(mep->name)
			printf(" %c: %s\n", mep->menu_c, mep->name);
		mep++;
	}
	printf("\n");
} /* print_menu() */

static void scReset()
{
	scStatus rtn;
	
	rtn = [controllerId scsiReset];
	if(rtn) {
		printf("...scsiReset returned %d\n", rtn);
	}
}

static void cdbRead()
{
	scsiReq_t scsiReq;
	scStatus rtn;
	
	setupScsiReq(&scsiReq);
	rtn = [controllerId executeCdbRead:&scsiReq buffer:rwbuf];
	if(rtn) {
		printf("...executeCdbRead returned %s\n", 
			IOFindNameForValue(rtn, scStatusValues));
		printf("	ioStatus = %s\n", 
			IOFindNameForValue(scsiReq.ioStatus, scStatusValues));
	}
	else if(scsiReq.ioStatus) {
		printf("...executeCdbRead: ioStatus = %d\n", 
			IOFindNameForValue(scsiReq.ioStatus, scStatusValues));
	}
	else if(scsiReq.dmaXfr)
		printf("dmaXfr = %d\n", scsiReq.dmaXfr);
	else
		printf("...OK\n");
}

static void cdbWrite()
{
	scsiReq_t scsiReq;
	scStatus rtn;
	
	setupScsiReq(&scsiReq);
	rtn = [controllerId executeCdbWrite:&scsiReq buffer:rwbuf];
	if(rtn) {
		printf("...executeCdbWrite returned %s\n", 
			IOFindNameForValue(rtn, scStatusValues));
		printf("	ioStatus = %s\n", 
			IOFindNameForValue(scsiReq.ioStatus, scStatusValues));
	}
	else if(scsiReq.ioStatus) {
		printf("...executeCdbWrite: ioStatus = %d\n", 
			IOFindNameForValue(scsiReq.ioStatus, scStatusValues));
	}
	else if(scsiReq.dmaXfr)
		printf("dmaXfr = %d\n", scsiReq.dmaXfr);
	else
		printf("...OK\n");
}

static void setTarget()
{
	char instr[100];
	
	printf("Enter new target (CR = %d): ", target);
	gets(instr);
	if(instr[0]) 
		target = atoi(instr);
}

static void setLun()
{
	char instr[100];
	
	printf("Enter new lun (CR = %d): ", lun);
	gets(instr);
	if(instr[0]) 
		lun = atoi(instr);
}

static void setDmaMax()
{
	char instr[100];
	int ok;
	
	do {
		printf("Enter new dmaMax (CR = %d): ", dmaMax);
		gets(instr);
		if(instr[0]) 
			dmaMax = atoi(instr);
		if(dmaMax > MAX_DMA_SIZE) {
			printf("Maximum DMA size is %d\n", MAX_DMA_SIZE);
			ok = 0;
		}
		else {
			ok = 1;
		}
	} while(!ok);
}

static void setDmaDir()
{
	char instr[100];
	
	printf("Enter new dmaDir (r/w, CR = %s): ", 
		dmaDir == SCSI_DMA_READ ? "Read" : "Write");
	gets(instr);
	switch(instr[0]) {
	    case 'r':
	    	dmaDir = SCSI_DMA_READ;
		break;
	    case 'w':
	    	dmaDir = SCSI_DMA_WRITE;
		break;
	    default:
	    	break;
	}
}


static void setupCDB()
{
	int i;
	char instr[20];
	
	printf("Enter <ESC> at any prompt to quit\n");
	for(i=0; i<12; i++) {
		printf("  CDB byte %d (CR = 0x%02x): ", i, cdb_bytes[i]);
		gets(instr);
		switch(instr[0]) {
		    case '\0':
		    	break;			/* take current value */
		    case 0x1b:			/* Escape */
		    	goto outOfHere;
		    default:
		    	cdb_bytes[i] = atoh(instr);
			break;
		}
	}
outOfHere:
	printf("CDB bytes: ");
	for(i=0; i<12; i++) 
		printf("0x%02x ", cdb_bytes[i]);
	printf("\n");
}

static void setupScsiReq(scsiReq_t *scsiReq)
{
	bzero(scsiReq, sizeof(scsiReq_t));
	scsiReq->target = target;
	scsiReq->lun = lun;
	bcopy(cdb_bytes, &scsiReq->cdb, 12);
	scsiReq->dmaDir = dmaDir;
	scsiReq->dmaMax = dmaMax;
	scsiReq->ioTimeout = ioTimeout;
}

static void dumpBuf()
{
	dump_buf((u_char *)rwbuf, MAX_DMA_SIZE);
}

static void quit()
{
	exit(0);
}

#ifdef	notdef
void dump_buf(u_char *buf, int size) {

	int i;
	char c[100];
	
	printf("\n");
	for(i=0; i<size; i++) {
		if((i>0) && (i%0x100 == 0)) {
			printf("\n...More? (y/anything) ");
			gets(c);
			if(c[0] != 'y')
				return;
		}
		if(i%0x10 == 0)
			printf("\n %03X: ",i);
		else if(i%8 == 0)
			printf("  ");
		printf("%02X ",buf[i]);
	}
	printf("\n");
} /* dump_buf() */ 
#endif	notdef
