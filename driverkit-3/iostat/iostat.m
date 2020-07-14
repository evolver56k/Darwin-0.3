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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * 	iostat, driverkit style.
 *
 * HISTORY
 * 23-June-92    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/IODevice.h>
#import <driverkit/IODisk.h>
#import <driverkit/generalFuncs.h>
#import <sys/types.h>
#import <sys/printf.h>
#import <mach/mach_types.h>
#import <driverkit/IODeviceMaster.h>
#import <driverkit/IOSCSIController.h>

#define	NUM_DEVICES	16	/* max number of devices to measure */


/*
 * Types of devices this program knows about.
 */
typedef enum {
	DK_NONE,
	DK_DISK,
	DK_CONTROLLER,
	DK_TTY,
} device_kind_t;

/*
 * Map device_kind_t to an IOParameterName which tells us if a particular unit
 * is of that type.
 */
typedef struct {
	device_kind_t	dev_kind;
	IOParameterName	parameterName;
} dev_kind_map_t;

/*
 * Internal representation of one device whose stats we are logging.
 */
typedef struct {
	device_kind_t	dev_kind;
	IOObjectNumber 	unit;
	char 		name[IO_STRING_LENGTH];		// e.g., sd0, fd1
	union {
	    unsigned	disk_stats[IO_DISK_STAT_ARRAY_SIZE];
	    unsigned	tty_stats[1];			// TBD 
	} stats;
} dev_info_t;

/* 
 * Known device types.
 */
static dev_kind_map_t dev_kind_map_array[] = {
	{ DK_DISK,	IO_IS_A_DISK	 	},
	{ DK_CONTROLLER,IO_IS_A_SCSI_CONTROLLER	},
	{ DK_NONE,	NULL			},	/* null-terminated */
};

static void usage(char **argv);
static int size_devices(id deviceMaster, 
			dev_info_t *dev_info_array,
			unsigned dev_info_array_size);
static void print_header(dev_info_t *dev_info_array, boolean_t long_form);
static int display_dev_info(id deviceMaster, 
			dev_info_t *dev_info_array,
			unsigned elapsed_time,
			boolean_t long_form);

boolean_t raw = FALSE;


int main(int argc, char **argv) {
	
	int 		arg;
	boolean_t 	long_form = FALSE;
	int	 	interval = 0;
	dev_info_t 	dev_info_array[NUM_DEVICES];
	id		deviceMaster;
	unsigned	interval_ms;
	unsigned	elapsed_time_ms; 
	ns_time_t	elapsed_time_ns;
	ns_time_t	last_time;
	ns_time_t	this_time;
	int 		loop_num = 0;
	
	for(arg=1; arg<argc; arg++) { 
		switch(argv[arg][0]) {
		    case '-':
		    	switch(argv[arg][1]) {
			    case 'l':
			    	long_form = TRUE;
				break;
			    case 'r':
			    	raw = TRUE;
				break;
			    default:
			    	usage(argv);
			}
			break;
		    default:
			interval = atoi(argv[arg]);
			if(interval == 0) {
				usage(argv);
			}
			break;
		}
	}
	IOInitGeneralFuncs();
	deviceMaster = [IODeviceMaster new];
	if(deviceMaster == nil) {
		printf("Can't get IODeviceMaster object\n");
		exit(1);
	}
	if(size_devices(deviceMaster, dev_info_array, NUM_DEVICES) == 0) {
		printf("No I/O devices found\n");
		exit(1);
	}
	if(display_dev_info(deviceMaster, 
	    dev_info_array,
	    0,				// elapsed_time = 0 --> init stats
	    long_form)) {
		exit(1);		
	}
	IOGetTimestamp(&last_time);
	do {
		if((loop_num++) % 20 == 0) {
			print_header(dev_info_array, long_form);
		}
		if(interval) {
			IOSleep(interval * 1000);
		}
		else {
			/*
			 * Take a second to gather some stats...
			 */
			IOSleep(1);
		}
		IOGetTimestamp(&this_time);
		elapsed_time_ns = this_time - last_time;
		last_time = this_time;
		if(display_dev_info(deviceMaster, 
		    dev_info_array,
		    elapsed_time_ns / (1000 * 1000),
		    long_form)) {
			exit(1);		
		}
	} while(interval);
}

static void usage(char **argv)
{
	printf("usage: %s [-l(ongform] [interval]\n", argv[0]);
	exit(1);
}

static void print_header(dev_info_t *dev_info_array, boolean_t long_form)
{
	dev_info_t *dev_info = dev_info_array;
	
	for(dev_info=dev_info_array;
	    dev_info->dev_kind != DK_NONE;
	    dev_info++) {
	    	switch(dev_info->dev_kind) {
		    case DK_TTY:
		    	printf("      tty");
			break;
		    case DK_DISK:
		    	if(raw) {
				printf("%42.3s", dev_info->name);
			}
		    	else if(long_form) {
				printf("%30.3s", dev_info->name);
			}
			else  {
				printf("%16.3s", dev_info->name);
			}
			break;

		    case DK_CONTROLLER:
			printf("%10.3s", dev_info->name);
			break;

		    default:
		    	break;
		}
	}
	printf("\n");
	for(dev_info=dev_info_array;
	    dev_info->dev_kind != DK_NONE;
	    dev_info++) {
	    	switch(dev_info->dev_kind) {
		    case DK_TTY:
			printf(" tin tout");
			break;
		    case DK_CONTROLLER:
			printf("   avg max");
			break;
		    case DK_DISK:
			if(raw) {
			/*      aaaaaabbbbbbeeee.eccccccddddddeeee.effff.f */
			printf("  read  time  ms/r write  time  ms/w ms/op");
			}
			else if(long_form) {
				/*      aaaaaabbbbbbccccccddddddeeee.e */
				printf(" kbr/s  rt/s kbw/s  wt/s  ms/s");
			}
			else {
				/*      aaaaabbbbbcccc.c */
				printf(" kb/s  t/s  ms/s");
			}
			break;
		    default:
		    	break;
		}
	}
	printf("\n");
}

/*
 * Determine which devices are present. Initialize dev_info_array. Returns
 * number of devices found.
 */
#define CLASS_NAME_SIZE	128

static int size_devices(id deviceMaster, 
	dev_info_t *dev_info_array,
	unsigned dev_info_array_size)		// in dev_info_t's
{
	int 		dex;
	dev_info_t 	*dev_info;
	IOReturn 	drtn;
	IOObjectNumber 	unit;
	unsigned 	returned_cnt;
	IOString 	device_kind;
	IOString 	unit_name;
	dev_kind_map_t 	*dev_kind_map;
	unsigned	param;
	unsigned	param_count;
	char		class_name[CLASS_NAME_SIZE];
	unsigned	num_devices = 0;
	
	for(dex=0; dex<dev_info_array_size; dex++) {
		bzero(&dev_info_array[dex].stats, sizeof(dev_info[dex].stats));
		dev_info_array[dex].dev_kind = DK_NONE;
	}	
	unit = 0;
	dev_info = dev_info_array;
	
	/*
	 * One loop for every device in the kernel. 
	 */
	for(unit=0; ; unit++) {
		drtn = [deviceMaster lookUpByObjectNumber:unit
				deviceKind:&device_kind
				deviceName:&unit_name];
		switch(drtn) {
		    case IO_R_SUCCESS:
		    	break;			// normal case
		    case IO_R_NO_DEVICE:
			return num_devices;	// no more devices
		    case IO_R_NOT_ATTACHED:
		    	continue;		// nothing here, keep going
		    default:
		    	printf("iostat: lookUpByObjectNumber: %s\n", 
				[IODevice stringFromReturn:drtn]);
			return num_devices;
		}
		
		/*
		 * See if this device is one which we know about by doing a 
		 * device-specific -getIntValues for each known 
		 * device type.
		 */
		for(dev_kind_map=dev_kind_map_array; 
		    dev_kind_map->dev_kind != DK_NONE;
		    dev_kind_map++) {
		        param_count = 1;
		 	drtn = [deviceMaster getIntValues:&param
				forParameter:dev_kind_map->parameterName
				objectNumber:unit
				count:&param_count];
			if(drtn == IO_R_SUCCESS) {
				break;
			}
		}
		if(dev_kind_map->dev_kind == DK_NONE) {
			continue;
		}
		
		/*
		 * Get class name. (Not used yet, but we might want 
		 * it for other devices...)
		 * Note MIG bogosity - have to pass max array size in
		 * param_count, which is a mig-generated out parameter!
		 */
		param_count = CLASS_NAME_SIZE;
		drtn = [deviceMaster getCharValues:class_name
			forParameter:IO_CLASS_NAME
			objectNumber:unit
			count:&param_count];
		if(drtn) {
			/*
			 * Should never happen.
			 */
			continue;
		}
		
		/*
		 * Device-specific filtering here.
		 */
		switch(dev_kind_map->dev_kind) {
		    case DK_DISK:
			/*
			 * Must be a physical device, i.e., not
			 * a logical disk.
			 */	
			param_count = 1;
		 	drtn = [deviceMaster getIntValues:&param
				forParameter:IO_IS_A_PHYSICAL_DISK
				objectNumber:unit
				count:&param_count];
			if(drtn || (param == 0)) {
				continue;
			}
			break;
			
		    /*
		     * Other types have no filtering (if they got this far,
		     * they're OK).
		     */
		    default:
			break;
		}
		
		/*
		 * Success. Record it.
		 */
		dev_info->dev_kind = dev_kind_map->dev_kind;
		dev_info->unit = unit;
		strcpy(dev_info->name, unit_name);
		dev_info++;
		num_devices++;
	}	
}

/*
 * Display delta stats for array of dev_info_t's, update running stats for
 * each device. Elapsed_time of 0 means just gather initial data, no display.
 */
static int display_dev_info(id deviceMaster, 
	dev_info_t *dev_info_array,
	unsigned elapsed_time,			// in milliseconds
	boolean_t long_form)
{
	dev_info_t 	*dev_info;
	unsigned 	disk_stats[IO_DISK_STAT_ARRAY_SIZE];
	unsigned 	controller_stats[IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE];
	unsigned 	net_stats[1];
	IOReturn 	drtn;
	unsigned 	returnedCnt;
	unsigned 	*statp;

	for(dev_info=dev_info_array;
	    dev_info->dev_kind != DK_NONE;
	    dev_info++) {
		switch(dev_info->dev_kind) {

		    case DK_CONTROLLER:
			returnedCnt = IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE;
		 	drtn = [deviceMaster getIntValues:controller_stats
				forParameter:IO_SCSI_CONTROLLER_STATS
				objectNumber:dev_info->unit
				count:&returnedCnt];
			if(drtn) {
				printf("getIntValues returned %d\n", 
					drtn);
				return drtn;
			}
			if(returnedCnt != IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE) {
				printf("getIntValues only returned %d "
					"words, expected %d\n", returnedCnt,
					IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE);
				return -1;
			}
			if (elapsed_time) {
			    float averageLen;
			    unsigned int total,count;
			    count = controller_stats[
			    	IO_SCSI_CONTROLLER_QUEUE_SAMPLES];
			    total = controller_stats[
			    	IO_SCSI_CONTROLLER_QUEUE_TOTAL];
			    if (count)
			    	averageLen = (float) total / (float) count;
			    printf("%6.3f%4d", averageLen, controller_stats[
				IO_SCSI_CONTROLLER_MAX_QUEUE_LENGTH]);
			}
			/*
			 * Reset controller statistics
			 * FIXME - zero them here? This is hypothetical
			 * code anyway.
			 */
		 	drtn = [deviceMaster setIntValues:controller_stats
				forParameter:IO_SCSI_CONTROLLER_STATS
				objectNumber:dev_info->unit
				count:IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE];
			if(drtn) {
				printf("setIntValues returned %d\n", 
					drtn);
				return drtn;
			}
			break;
		
		    case DK_DISK:
			statp = dev_info->stats.disk_stats;
			returnedCnt = IO_DISK_STAT_ARRAY_SIZE;
		 	drtn = [deviceMaster getIntValues:disk_stats
				forParameter:IO_DISK_STATS
				objectNumber:dev_info->unit
				count:&returnedCnt];
			if(drtn) {
				printf("getIntValues returned %d\n", 
					drtn);
				return drtn;
			}
			if(returnedCnt != IO_DISK_STAT_ARRAY_SIZE) {
				printf("getIntValues only returned %d "
					"words, expected %d\n", returnedCnt,
					IO_DISK_STAT_ARRAY_SIZE);
				return -1;
			}
			if(elapsed_time) {
			
			    unsigned 	bytes_read;
			    unsigned	read_ops;
			    unsigned	bytes_wrt;
			    unsigned	write_ops;
			    float 	ms_per_seek;
			    float 	latency;
			    unsigned 	read_time;
			    unsigned 	write_time;

			    bytes_read = disk_stats[IO_BytesRead] - 
			    		      statp[IO_BytesRead];
			    bytes_wrt  = disk_stats[IO_BytesWritten] - 
			    		      statp[IO_BytesWritten];
			    read_ops   = disk_stats[IO_Reads] - 
			    		      statp[IO_Reads];
			    write_ops  = disk_stats[IO_Writes] - 
			    		      statp[IO_Writes];
			    read_time =
			    	disk_stats[IO_TotalReadTime] -
			        statp[IO_TotalReadTime];
			    write_time =
			    	disk_stats[IO_TotalWriteTime] -
			        statp[IO_TotalWriteTime];
			    latency = 
			       	disk_stats[IO_LatentReadTime] +
				disk_stats[IO_LatentWriteTime] -
			        statp[IO_LatentReadTime] -
			        statp[IO_LatentWriteTime];
			    if((read_ops + write_ops) == 0) {
			    	ms_per_seek = 0;
			    }
			    else {
				    ms_per_seek = 
				    	latency / (read_ops + write_ops);
			    }
			    if(raw) {
			    	float per_read = 0, per_write = 0, per_op = 0;
				if (read_ops > 0)
					per_read = read_time / read_ops;
				if (write_ops > 0)
					per_write = write_time / write_ops;
				if (read_ops > 0 || write_ops > 0)
					per_op = (read_time + write_time) /
						 (read_ops + write_ops);
				printf("%6d%6d%6.1f%6d%6d%6.1f%6.1f",
				    read_ops, read_time, per_read,
				    write_ops, write_time, per_write,
				    per_op);
			    }
			    else if(long_form) {
			    	printf("%6d%6d%6d%6d%6.1f",
				    bytes_read       / (elapsed_time),
				    read_ops * 1000  / (elapsed_time),
				    bytes_wrt        / (elapsed_time),
				    write_ops * 1000 / (elapsed_time),
				    ms_per_seek);
			    }
			    else {
				printf("%5d%5d%6.1f", 
				    (bytes_read + bytes_wrt) / elapsed_time, 
				    (read_ops   + write_ops) * 1000 /
				    	elapsed_time,
				    ms_per_seek);
		 	    }
			}
			
			/*
			 * Copy current (cumulative) stats into dev_info.
			 */
			bcopy(disk_stats, statp, IO_DISK_STAT_ARRAY_SIZE * 4);
			break;
			
		    default:
		    	printf("Can\'t handle dev_kind %d yet\n", 
				dev_info->dev_kind);
			exit(1);
		}
	}
	printf("\n");
	return 0;
}
