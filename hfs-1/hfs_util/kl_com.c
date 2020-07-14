/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*	@(#)kl_com.c		3.0
 *
 *	(c) 1998 Apple Computer, Inc.  All Rights Reserved
 *
 *	kl_com.c -- kern_loader communication module for hfs.util
 *
 *	HISTORY
 */

/*
 *	kwells
 *	I got rid of exit code was a global variable shared between
 *	fs_util and kl_com.
 *	
 *	kl_com_wait() pings the kern_loader and the ping 
 *	causes the ping routine in kl_com to be called which used
 *	to exit.  This meant that waiting on the kern_loader was the last thing 
 *	a program could do because of the exit call.
 *
 *	Got rid of the "exit" in the ping routine and instead it now sets
 *	a flag which kl_com_wait loops on.
 */


#include <bsd/libc.h>
#include <bsd/strings.h>
#include <bsd/sys/param.h>
#include <mach/mach_traps.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mig_errors.h>
#include <mach/cthreads.h>
#include <mach/notify.h>
#include <mach/message.h>

#include <servers/bootstrap.h>
#include <kernserv/kern_loader_error.h>
#include <kernserv/kern_loader_types.h>
#include <kernserv/kern_loader.h>
#include <kernserv/kern_loader_reply_handler.h>
#include <kernserv/loadable_fs.h>

#include "kl_com.h"

static port_name_t	kl_port;
static port_name_t	kernel_task_port;
static port_name_t	reply_port;
static boolean_t	kl_init_flag = FALSE;
static boolean_t	kl_wait_flag;


static kern_return_t print_string (
    void		*arg,
    printf_data_t	string,
    u_int		stringCnt,
    int			level)
{
    /* dprintf(("kl_com: print_string\n")); */
#if DEBUG
    if (stringCnt == 0 || !string)
	return KERN_SUCCESS;
    fputs(string, stdout);
    fflush(stdout);
#endif
    return KERN_SUCCESS;
}

static kern_return_t ping (
    void	*arg,
    int		id)
{
    kl_wait_flag = FALSE;
    dprintf(("kl_com: ping\n"));
    return KERN_SUCCESS;
}


static void kl_com_error(char *err_str, kern_return_t rtn)
{
#ifdef DEBUG
    char err_string[100];
    
    strcpy(err_string, "kl_com: ");
    strcat(err_string, err_str);
    mach_error(err_str, rtn);
#endif
}


kern_loader_reply_t kern_loader_reply = {
    0,
    0,
    print_string,
    ping
};

/*
 * wait for reply messages.
 */
static int kl_com_log(port_name_t port)
{
    char			msg_buf[kern_loader_replyMaxRequestSize];
    msg_header_t	*	msg = (msg_header_t *)msg_buf;
    port_name_t			notify_port;
    port_set_name_t		port_set;
    kern_return_t		r;

    dprintf(("ENTER: kl_com_log\n"));
    
    r = port_allocate(task_self(), &notify_port);
    if (r != KERN_SUCCESS) {
	kl_com_error("allocating notify port", r);
	return(FSUR_LOADERR);
    }

    r = task_set_notify_port(task_self(), notify_port);
    if (r != KERN_SUCCESS) {
	kl_com_error("cannot set notify port", r);
	return(FSUR_LOADERR);
    }

    r = port_set_allocate(task_self(), &port_set);
    if (r != KERN_SUCCESS) {
	kl_com_error("allocating port set", r);
	return(FSUR_LOADERR);
    }

    r = port_set_add(task_self(), port_set, notify_port);
    if (r != KERN_SUCCESS) {
	kl_com_error("adding notify port to port set", r);
	return(FSUR_LOADERR);
    }

    r = port_set_add(task_self(), port_set, port);
    if (r != KERN_SUCCESS) {
	kl_com_error("adding listener port to port set", r);
	return(FSUR_LOADERR);
    }

    while(1) {

        msg->msg_size = kern_loader_replyMaxRequestSize;
	msg->msg_local_port = port_set;
	
        r = msg_receive(msg, MSG_OPTION_NONE, 0);
	if (r != KERN_SUCCESS) {
	    kl_com_error("log_thread receive", r);
	    return(FSUR_LOADERR);
	}

        /* dprintf(("kl_com_log: msg received\n")); */
	
        if (msg->msg_local_port == notify_port) {
	    notification_t *n = (notification_t *)msg;
	    
	    if ((msg->msg_id == NOTIFY_PORT_DELETED) &&
		(n->notify_port == kl_port)) {
		printf("kl_com_wait: kern_loader port deleted\n");
		continue;
	    } else {
		printf("kl_com_wait: weird notification\n");
		continue;
	    }
	} else { 
	    kern_loader_reply_handler(msg, &kern_loader_reply);
	}

    } // while (1)

    return(0);
}


/*
 * Set up ports, etc. for kern_loader communication.
 */
static int kl_init() {
    kern_return_t r;
    boolean_t isactive;

    if(kl_init_flag)
	return(0);

    dprintf(("kl_init\n"));

    r = kern_loader_look_up(&kl_port);
    if (r != KERN_SUCCESS) {
	kl_com_error("can't find kernel loader", r);
	return(FSUR_LOADERR);
    }

#if DEBUG
    {
        int i;
        int count;
        server_name_array_t server_names;
        kern_return_t r;
        r = kern_loader_server_list(kl_port, &server_names, &count);
        if ( r != KERN_SUCCESS )
            kl_com_error("kern_loader_server_list failed",r);
        else
          {
            for (i = 0; i < count; i++)
              {
                dprintf(("Server %d: '%s'\n", i, server_names[i]));
              }
          }
    }
#endif
    
    r = task_by_unix_pid(task_self(), 0, &kernel_task_port);
    if (r != KERN_SUCCESS) {
	kl_com_error("cannot get kernel task port", r);
	return(FSUR_LOADERR);
    }

    r = port_allocate(task_self(), &reply_port);
    if (r != KERN_SUCCESS) {
	kl_com_error("can't allocate reply port", r);
	return(FSUR_LOADERR);
    }

    r = bootstrap_status(bootstrap_port, KERN_LOADER_NAME, &isactive);
    if (r != KERN_SUCCESS) {
	kl_com_error("can't find kernel loader status", r);
	return(FSUR_LOADERR);
    }
    /*
     * Create a thread to listen on reply_port
     * and print out anything that comes back.
     */
    cthread_init();
    cthread_set_name(cthread_self(), "command thread");
    cthread_detach(cthread_fork((cthread_fn_t)kl_com_log,
	(any_t)reply_port));

    if (!isactive)
	printf("kernel loader inactive, pausing\n");

    r = kern_loader_status_port(kl_port, reply_port);
    if(r) {
	kl_com_error("can't get status back", r);
	return(FSUR_LOADERR);
    }

    kl_init_flag = TRUE;

    return(0);

} /* kl_init */


int kl_com_add(char *path_name, char *server_name) {
    int rtn;
    kern_return_t r;
    
    dprintf(("kl_com_add('%s','%s')\n",path_name,server_name));

    if( (rtn = kl_init()) )
	return(rtn);

    switch(kl_com_get_state(server_name)) {
	case KSS_LOADED:
	case KSS_ALLOCATED:
	    dprintf(("kl_com_add: server already exists\n"));
	    return(0);	/* this function is a nop */
       default:
	    dprintf(("kl_com_add: server has not been added yet\n"));
	   break;
    }

    r = kern_loader_add_server(kl_port, kernel_task_port, path_name);
    if((r != KERN_SUCCESS) && 
       (r != KERN_LOADER_SERVER_EXISTS) &&
       /* bogus - covers kern_loader bug #7190 */
       (r != KERN_LOADER_SERVER_WONT_LOAD)) {
	kl_com_error("kern_loader_add_server", r);
	rtn = FSUR_LOADERR;
    }

    dprintf(("kl_com_add: returning %d\n", rtn));

    return(rtn);

} /* kl_com_add */

int kl_com_delete(char *server_name) {
    int rtn;
    kern_return_t r;

    dprintf(("kl_com_delete('%s')\n", server_name));

    if( (rtn = kl_init()) )
	return(rtn);

    r = kern_loader_delete_server(kl_port, kernel_task_port, server_name);
    if(r) {
	kl_com_error("kern_loader_delete_server", r);
	rtn = FSUR_LOADERR;
    }

    dprintf(("kl_com_delete: returning %d\n", rtn));

    return(rtn);

} /* kl_com_delete */

int kl_com_load(char *server_name) {
    int rtn;
    kern_return_t r;

    dprintf(("kl_com_load('%s')\n", server_name));

    if( (rtn = kl_init()) )
	return(rtn);

    switch(kl_com_get_state(server_name)) {
	case KSS_LOADED:
	    dprintf(("kl_com_load: server already loaded\n"));
	    return(0);	/* this function is a nop */
       default:
	    dprintf(("kl_com_load: server has not been loaded yet\n"));
	   break;
    }

    r = kern_loader_load_server(kl_port, server_name);
    if(r) {
	kl_com_error("kern_loader_load_server", r);
	rtn = FSUR_LOADERR;
    }

    dprintf(("kl_com_load: returning %d\n", rtn));

    return(rtn);

} /* kl_com_load */

int kl_com_unload(char *server_name) {
    int rtn = 0;
    kern_return_t r;

    dprintf(("kl_com_unload: %s\n", server_name));

    if( (rtn = kl_init()) )
	return(rtn);

    r = kern_loader_unload_server(kl_port, kernel_task_port, server_name);
    if(r) {
	kl_com_error("kern_loader_unload_server", r);
	rtn = FSUR_LOADERR;
    }

    dprintf(("kl_com_unload: returning %d\n", rtn));

    return(rtn);

} /* kl_com_unload */

klc_server_state kl_com_get_state(char *server_name)
{
    server_state_t state;
    /*
     * remainder are unused...
     */
    vm_address_t load_address;
    vm_size_t load_size;
    server_reloc_t reloc, loadable;
    port_name_t *ports;
    port_name_string_array_t port_names;
    u_int cnt;
    boolean_t *advertised;
    kern_return_t rtn;
    
    dprintf(("kl_com_get_state: %s\n", server_name));

    if( (rtn = kl_init()) )
	return(rtn);

    rtn = kern_loader_server_info(kl_port, PORT_NULL, server_name,
	&state, &load_address, &load_size, reloc, loadable,
	&ports, &cnt, &port_names, &cnt, &advertised, &cnt);
    if(rtn != KERN_SUCCESS) {
        return(KSS_UNKNOWN); /* this error is expected the first time we check for the non-existent server */
    }

    switch(state) {

        case Allocating:
	    dprintf(("kl_com_get_state: state: Allocating -> KSS_ALLOCATED\n"));
	    return(KSS_ALLOCATED);
	case Allocated:
	    dprintf(("kl_com_get_state: state: Allocated -> KSS_ALLOCATED\n"));
	    return(KSS_ALLOCATED);
	case Loading:
	    dprintf(("kl_com_get_state: state: Loading-> KSS_LOADED\n"));
	    return(KSS_LOADED);
	case Loaded:
	    dprintf(("kl_com_get_state: state: Loaded -> KSS_LOADED\n"));
	    return(KSS_LOADED);
	case Zombie:
	    dprintf(("kl_com_get_state: state: Zombie -> KSS_UNKNOWN\n"));
	    return(KSS_UNKNOWN);
	case Unloading:
	    dprintf(("kl_com_get_state: state: Unloading -> KSS_UNKNOWN\n"));
	    return(KSS_UNKNOWN);
	case Deallocated:
	    dprintf(("kl_com_get_state: state: Deallocated -> KSS_UNKNOWN\n"));
	    return(KSS_UNKNOWN);
	default:
	    dprintf(("kl_com_get_state: state: ??? -> KSS_UNKNOWN\n"));
	    return(KSS_UNKNOWN);

    } /* switch(state) */

} /* kl_com_get_state */

void kl_com_wait()
{
    dprintf(("kl_com_wait\n"));
    kl_wait_flag = TRUE;
    kern_loader_ping(kl_port, reply_port, 0);
    while (kl_wait_flag);
} /* kl_com_wait */

