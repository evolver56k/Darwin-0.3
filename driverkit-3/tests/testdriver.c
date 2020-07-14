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
 * sample driver. Exec'd by Config as devr_0999_0000.
 */
 
#import <bsd/sys/types.h>
#import <mach/mach.h>
#import <servers/bootstrap.h>
#import <servers/netname.h>
#import <bsd/libc.h>
#import <mach/mach_error.h>
#import <bsd/syslog.h>
#import <driverkit/driverServer.h>
#import <driverkit/userConfigServer.h>
#import <bsd/sys/signal.h>

int get_devr_port(port_t *port_array, int max_devices);
#ifdef	notdef
void sigint(int foo);
#endif	notdef
void sighup(int foo);

#define NUM_DEVICES	10
#define DEV_TYPE	0x0999

port_t config_port;
port_t driver_sig_port;
port_t dev_ports[NUM_DEVICES];
port_t driver_port;
char *driver_name;

#define print(x,a,b,c,d,e)	syslog(LOG_ERR, x,a,b,c,d,e)

int main(int argc, char **argv)
{
	IOConfigReturn crtn;
	kern_return_t krtn;
	
	print("driver %s: starting\n", argv[0], 2,3,4,5);
	driver_name = argv[0];
	
	/*
	 * sigint (signal 2) causes a device_destroy().
	 * sighup (1) causes a driver_destroy() and clean exit.
	 */
#ifdef	notdef
	signal(SIGINT, sigint);
#endif	notdef
	signal(SIGHUP, sighup);
	
	/*
	 * Get some ports.
	 */
	krtn = netname_look_up(name_server_port,
		"",					// hostname
		CONFIG_SERVER_NAME,
		&config_port);
	if(krtn) {
		print("%s: can't find %s: %s\n",
			argv[0], CONFIG_SERVER_NAME, mach_error_string(krtn),
			4,5);
		exit(1);
	}
	krtn = bootstrap_look_up(bootstrap_port, 
		SIG_PORT_NAME,
		&driver_sig_port);
	if(krtn) {
		print("%s: can't find %s: %s\n",
			argv[0], SIG_PORT_NAME, mach_error_string(krtn), 4,5);
		exit(1);
	}
	port_allocate(task_self(), &driver_port);
	if(get_devr_port(dev_ports, NUM_DEVICES)) {
		/*
		 * Register with Config.
		 */
		crtn = IORegisterDriver(config_port,
			driver_sig_port,
			driver_port);	
		if(crtn) {
			print("%s: IORegisterDriver: crtn = %d\n",
				argv[0], crtn, 3,4,5);
			exit(1);
		}
	}
	
	/*
	 * Sleep until killed.
	 */
	while(1)
		sleep(1);
	exit(0);
}

/*
 * look up all of our dev_ports. returns # of ports found.
 */
int get_devr_port(port_t *dev_ports, int max_devices)
{
	int i;
	name_array_t service_names;
	unsigned int service_cnt;
	name_array_t server_names;
	unsigned int server_cnt;
	bool_array_t service_active;
	unsigned int service_active_cnt;
	kern_return_t krtn;
	int port_index = 0;
	
	krtn = bootstrap_info(bootstrap_port, 
		&service_names, 
		&service_cnt,
		&server_names, 
		&server_cnt, 
		&service_active, 
		&service_active_cnt);
	if (krtn != BOOTSTRAP_SUCCESS) {
		print("%s: bootstrap_info: %s", 
			driver_name, mach_error_string(krtn), 3,4,5);
		return(PORT_NULL);
	}

	/*
	 * Search for devr_XXXX_XXXX. Later - versions and dev_index via
	 * dev_port_to_type().
	 */
	print("%s: service_cnt %d\n", driver_name, service_cnt, 3,4,5);
	for (i = 0; i < service_cnt; i++) {
#ifdef	notdef
		print("%s: service_name %s\n", driver_name, service_names[i],
			4,5);
#endif	notdef
		if(strncmp(service_names[i], 
		    "dev_port_", strlen("dev_port_")) == 0) {
			print("%s: port %s found\n", 
				driver_name, service_names[i], 3,4,5);
			
			/*
			 * Get the dev_port. 
			 */
			krtn = bootstrap_look_up(bootstrap_port,
				service_names[i],
				&dev_ports[port_index]);
			if(krtn) {
				print("%s: bootstrap_look_up: %s",
					driver_name, mach_error_string(krtn),
					3,4,5);
				return(0);
			}
			else {
				if(++port_index >= max_devices) {
					print("%s: num_devices exceeded\n",
						driver_name, 2,3,4,5);
					return(port_index);
				}
			}
		}
	}
	print("%s: %d dev_ports found\n", driver_name, port_index, 3,4,5);
	return(port_index);
}

#ifdef	notdef
/*
 * Destroy a dev_port.
 *
 * NOT SUPPORTED....
 */
static int delete_dev = 0; 

void sigint(int foo)
{
	config_return_t crtn;
	
	crtn = IODeleteDevice(config_port,
		driver_sig_port,
		dev_ports[delete_dev],
		&crtn);
	if(crtn) {
		print("%s: crtn %d", driver_name, crtn, 3,4,5);
		return;
	}
	dev_ports[delete_dev++] = PORT_NULL;
}
#endif	notdef

/*
 * Destroy driver (without re-exec).
 */
void sighup(int foo)
{
	IOConfigReturn crtn;
	
	crtn = IODeleteDriver(config_port, driver_sig_port);
	if(crtn) {
		print("%s IODeleteDriver: crtn %d", driver_name, crtn, 3,4,5);
		return;
	}
	exit(0);
}
