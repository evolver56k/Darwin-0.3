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
 * dx - menu based device exerciser tool.
 *
 * HISTORY
 * 19-Mar-91    Doug Mitchell at NeXT
 *      Created.
 */
 
#import <bsd/sys/types.h>
#import <bsd/libc.h>
#import <driverkit/IOClient.h>
#import <driverkit/SCSIDiskClient.h>
#import <bsd/dev/SCSITypes.h>
#import <mach/mach_error.h>
#import "defaults.h"
#import "buflib.h"

/*
 * prototypes for private functions.
 */
static void usage(char **argv);
static void print_menu();
static void setupCDB();
static void setupScsiReq(scsiReq_t *scsiReq, SCSIDmaDir_t dir);

/*
 * Menu stuff.
 */
struct menu_entry {
	char 	menu_c;
	char	*name;
	void	(*men_fcn)();
};

/*
 * User commands.
 */
static void dxRead();
static void dxWrite();
static void cdbRead();
static void cdbWrite();
static void setTarget();
static void setLun();
static void setCount();
static void setBlock();
static void dxQuery();
static void dxCheckReady();
static void setByteCount();
static void setLoopCount();
static void getForm();
static void setForm();
static void getRemove();
static void fillWbuf();
static void dumpRbuf();
static void quit();
static void getDname();
static void dxEject();

struct menu_entry me_array[] = {

    {'r', "Read         " , dxRead	}, {'w', "Write        " , dxWrite	},
    {'R', "cdbRead      " , cdbRead	}, {'W', "cdbWrite     " , cdbWrite	},
    {'c', "Set Count    " , setCount	}, {'b', "Set Block    " , setBlock	},
    {'T', "Set Target   " , setTarget	}, {'L', "Set lun      " , setLun	},
    {'Q', "Query        " , dxQuery	}, {'u', "Check Ready  " , dxCheckReady	},
    {'y', "Max ByteCount" , setByteCount}, {'l', "Loop Count   " , setLoopCount	},
    {'f', "Get Formatted" , getForm	}, {'F', "Set Formatted" , setForm	},
    {'v', "Get Removable" , getRemove	}, {'C', "Set up CDB   " , setupCDB	},
    {'D', "GetDriveName " , getDname  }, {'e', "Eject        " , dxEject      },
    {'1', "Fill buffer  " , fillWbuf	}, {'2', "Dump Buffer  " , dumpRbuf	},
    {'x', "Exit         " , quit	}, {'h', "Help         " , print_menu	},
    {0,	NULL		 , NULL		}
};

#define BUFSIZE		(64 * 1024)

/*
 * Static variables.
 */
int ioTimeout = 10;
int target = 0;
int lun = 0;
id clientId;
int count = 1024;
int block = 0;
int byteCountMax = 4096;
int loopCount = 1;
u_char cdb[12];
char *hostname=HOST_DEFAULT;
char *devname=DEVICE_DEFAULT;
char *rbuf, *wbuf;
IOErrorString errorstr;

int main(int argc, char **argv)
{
	int arg;
	char instr[80];
	struct	menu_entry *mep;
	int ok;
	IOReturn rtn;
	kern_return_t krtn;
	
	/*
	 * Get standard defaults from environment or defaults.h
	 */
	get_default_t("hostname", &hostname, HOST_DEFAULT);
	get_default_t("devname", &devname, DEVICE_DEFAULT);

	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'h':
		    	hostname = &argv[arg][2];
			break;
		    case 'd':
		    	devname = &argv[arg][2];
			break;
		    default:
		    	usage(argv);
		}
	}

	/*
	 * Instantiate and init the client object. Use SCSIDiskClient as
	 * default.
	 */
	rtn = [SCSIDiskClient clientOpen:devname
		hostName:hostname
		intentions:IO_INT_READ|IO_INT_WRITE
		idp:&clientId];
	if(rtn) {
		/*
		 * try opening read only.
		 */
		rtn = [SCSIDiskClient clientOpen:devname
			hostName:hostname
			intentions:IO_INT_READ
			idp:&clientId];
		if(rtn) {
			[SCSIDiskClient ioError:rtn
				logString:"Open"
				outString:errorstr];
			printf("%s...aborting\n", errorstr);
			exit(1);
		}
		else
			printf("...Warning: Device is Read-Only\n");
	}
	
	/*
	 * Allocate and init read and write buffers.
	 */
	krtn = vm_allocate(task_self(),
		(vm_address_t *)&rbuf,
		(vm_size_t)BUFSIZE,
		TRUE);
	if(krtn) {
		mach_error("vm_allocate", krtn);
		exit(1);
	}
	krtn = vm_allocate(task_self(),
		(vm_address_t *)&wbuf,
		(vm_size_t)BUFSIZE,
		TRUE);
	if(krtn) {
		mach_error("vm_allocate", krtn);
		exit(1);
	}
	bzero(rbuf, BUFSIZE);
	bzero(wbuf, BUFSIZE);
	
	/*
	 * Enter main loop.
	 */
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
	printf("usage: %s d[h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}

static void print_menu() {
	
	struct menu_entry *mep;
	
	printf("\n");
	mep = me_array;
	while(mep->menu_c) {
		printf(" %c: %s  ",mep->menu_c,mep->name);
		mep++;
		if(mep->menu_c) {
			printf(" %c: %s  ",mep->menu_c,mep->name);
			mep++;
		}
		printf("\n");
	}
	printf("\n");
} /* print_menu() */

static void cdbRead()
{
	scsiReq_t scsiReq;
	scStatus rtn;
	
	setupScsiReq(&scsiReq, SCSI_DMA_READ);
	rtn = [clientId sdCdbRead:&scsiReq buf:rbuf];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"sdCdbRead"
			outString:errorstr];
		printf("%s", errorstr);
	}
	else if(scsiReq.ioStatus)
		printf("...sdCdbRead: ioStatus = %d\n", scsiReq.ioStatus);
	else
		printf("...OK\n");
}

static void cdbWrite()
{
	scsiReq_t scsiReq;
	scStatus rtn;
	
	setupScsiReq(&scsiReq, SCSI_DMA_WRITE);
	rtn = [clientId sdCdbWrite:&scsiReq buf:wbuf];
	if(rtn) 
	if(rtn) {
		[IOClient ioError:rtn
			logString:"sdCdbWrite"
			outString:errorstr];
		printf("%s", errorstr);
	}
	else if(scsiReq.ioStatus)
		printf("...sdCdbWrite: ioStatus = %d\n", scsiReq.ioStatus);
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

static void setCount()
{
	printf("Enter new count (CR = %d): ", count);
	count = get_num(count, DEC);
}

static void setBlock()
{
	printf("Enter new block (CR = %d): ", block);
	block = get_num(block, DEC);
}

static void dxQuery()
{
	u_int queryData;
	IOReturn rtn;
	
	rtn = [clientId ioQuery:&queryData];
	if(rtn) {
		[IOClient ioError:rtn 
			  logString:"ioQuery"
			  outString:errorstr];
		printf(errorstr);
		return;
	}
	printf("   queryData = 0x%X\n", queryData);
	if(queryData & DQF_READABLE)
		printf("\tDQF_READABLE\n");
	if(queryData & DQF_WRITEABLE)
		printf("\tDQF_WRITABLE\n");
	if(queryData & DQF_RAND_ACC)
		printf("\tDQF_RAND_ACC\n");
        if(queryData & DQF_CAN_QUEUE)
		printf("\tDQF_CAN_QUEUE\n");
        if(queryData & DQF_CAN_RD_LOCK)
		printf("\tDQF_CAN_RD_LOCK\n");
        if(queryData & DQF_CAN_WRT_LOCK)
		printf("\tDQF_CAN_WRT_LOCK\n");
        if(queryData & DQF_IS_RD_LOCK)
 		printf("\tDQF_IS_RD_LOCK\n");
        if(queryData & DQF_IS_WRT_LOCK)
		printf("\tDQF_IS_WRT_LOCK\n");
        if(queryData & DQF_EXCLUSIVE)
		printf("\tDQF_EXCLUSIVE\n");
#ifdef	notdef
	printf("   block_size = 0x%X   dev_size = %d\n", 
		queryData.block_size, queryData.dev_size);
#endif	notdef
}

static void dxRead()
{
	u_int bytesXfr;
	IOReturn rtn;
		
	rtn = [clientId ioRead:block
		bytesReq:count
		buf:rbuf
		bytesXfr:&bytesXfr];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"ioRead"
			outString:errorstr];
		printf("%s", errorstr);
		return;
	}
	if(bytesXfr != count) {
		printf("bytesXfr = 0x%x, expected 0x%x\n", 
			bytesXfr, count);
	}
}

static void dxWrite()
{
	u_int bytesXfr;
	IOReturn rtn;
	
	rtn = [clientId ioWrite:block
		bytesReq:count
		buf:wbuf
		bytesXfr:&bytesXfr];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"ioWrite"
			outString:errorstr];
		printf("%s", errorstr);
		return;
	}
	if(bytesXfr != count) {
		printf("bytesXfr = 0x%x, expected 0x%x\n", 
			bytesXfr, count);
	}
}

static void dxCheckReady()
{
	DiskReadyState ready;
	IOReturn rtn;
	
	rtn = [clientId updateReadyState:&ready];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"CheckReady"
			outString:errorstr];
		printf("%s", errorstr);
		return;
	}
	switch(ready) {
	    case IO_Ready:
	    	printf("...Disk Ready\n");
		break;
	    case IO_NotReady:
	    	printf("...Disk Not Ready\n");
		break;
	    case IO_NoDisk:
	    	printf("...No Disk\n");
		break;
	    case IO_Ejecting:
	    	printf("...Ejecting\n");
		break;
	}
	return;
}

static void setByteCount()
{
	printf("Enter new max byte count (CR = %d): ", byteCountMax);
	byteCountMax = get_num(byteCountMax, DEC);

}

static void setLoopCount()
{
	printf("Enter new loop count (CR = %d): ", loopCount);
	loopCount = get_num(loopCount, DEC);
}

static void setupCDB()
{
	char instr[80];
	int i;
	
	printf("Hit ESC anytime to quit.\n");
	for(i=0; i<12; i++) {
		printf("Enter CDB byte %d (CR = 0x%x): ", i, cdb[i]);
		gets(instr);
		switch(instr[0]) {
		    case '\0':
		    	break;			/* take default */
		    case 0x1b:			
		    	return;			/* ESC - done */
		    default:
		    	cdb[i] = atoh(instr);
			break;
		}
	}
}

static void getForm()
{
	u_int formFlag;
	IOReturn rtn;
	
	rtn = [clientId getFormatted:&formFlag];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"getFormatted"
			outString:errorstr];
		printf("%s", errorstr);
		return;
	}
	if(formFlag)
		printf("...Disk Formatted\n");
	else
		printf("...Disk Unformatted\n");
}

static void setForm()
{
	u_int formFlag;
	IOReturn rtn;
	char instr[80];
	
	printf("New formatted value (0/1): ");
	gets(instr);
	if(instr[0] == '1')
		formFlag = 1;
	else
		formFlag = 0;		
	rtn = [clientId setFormatted:formFlag];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"setFormatted"
			outString:errorstr];
		printf("%s", errorstr);
		return;
	}
	else
		printf("...OK\n");
}

static void getRemove()
{
	u_int removeFlag;
	IOReturn rtn;
	
	rtn = [clientId getRemovable:&removeFlag];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"getRemovable"
			outString:errorstr];
		printf("%s", errorstr);
		return;
	}
	if(removeFlag)
		printf("...Removable Disk\n");
	else
		printf("...Fixed Disk\n");
}

static void fillWbuf()
{
	char instr[80];
	int i;
	
	printf("z = zero\ni = incrementing\nd = decrementing\n");
	printf("Enter pattern: ");
	gets(instr);
	switch(instr[0]) {
	    case 'z':
	    	bzero(wbuf, BUFSIZE);
		break;
	    case 'i':
		for(i=0; i<BUFSIZE; i++)
			wbuf[i] = i;
		break;
	    case 'd':
		for(i=0; i<BUFSIZE; i++)
			wbuf[i] = BUFSIZE - i;
		break;
	    default:
	    	printf("***Illegal selection***\n");
		break;
	}
}

static void dumpRbuf()
{
	dump_buf((u_char *)rbuf, BUFSIZE);
}

static void getDname()
{
/*	const char *name; */
	
	printf("...getDriveName not supported\n");
	return;
#ifdef	notdef
	name = [clientId getDriveName];
	if(name == NULL) {
		printf("NULL drivename\n");
		return;
	}
	printf("Drive Name = %s\n", name);
	return;
#endif	notdef
}

static void dxEject()
{
	IOReturn rtn;
	
	rtn = [clientId eject];
	if(rtn) {
		[IOClient ioError:rtn
			logString:"eject"
			outString:errorstr];
		printf("%s", errorstr);
		return;
	}
	printf("...OK\n");
}

/*
 * Internal functions.
 */
static void setupScsiReq(scsiReq_t *scsiReq, SCSIDmaDir_t dir)
{
	bzero(scsiReq, sizeof(scsiReq_t));
	scsiReq->target = target;
	scsiReq->lun	= lun;
	scsiReq->cdb    = *(cdb_t *)cdb;
	scsiReq->dmaDir = dir;
	scsiReq->dmaMax = byteCountMax;
	scsiReq->ioTimeout = ioTimeout;
	scsiReq->disconnect = 1;
}

static void quit()
{
	exit(0);
}
