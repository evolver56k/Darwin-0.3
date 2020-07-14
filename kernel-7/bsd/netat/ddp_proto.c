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
 *	Copyright (c) 1988, 1989, 1997, 1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* ddp_proto.c: 2.0, 1.23; 10/18/93; Apple Computer, Inc. */

#include <sysglue.h>
#ifdef _AIX
#include <sys/sysmacros.h>
#endif
#include <sys/buf.h>
#include <at/appletalk.h>
#include <atlog.h>
#include <lap.h>
#include <at/ddp.h>
#include <at/zip.h>
#include <at/at_lap.h>
#include <at_ddp.h>
#include <at/elap.h>
#include <h/at_elap.h>

extern atlock_t ddpall_lock;
extern atlock_t ddpinp_lock;
extern int ot_protoCnt;

#define DDPCLONEMIN	1
 
/* Pointer to LAP interface which DDP addresses are tied to */
extern	at_if_t	  *ifID_table[];

ddp_dev_t	  ddp_devs[DDP_SOCKET_LAST+1];

static void ddp_stop_ack();
char ddp_off_flag;
static gbuf_t *ddp_ack_m;
static gref_t *ddp_ack_gref;

int
lap_open(gref)
	gref_t *gref;
{
	int rc;

	if ((rc = ddp_open(gref)) == 0)
		((ddp_dev_t *)gref->info)->pid = 0; 
	return rc;
}

int
ddp_open(gref)
	gref_t *gref;
{
	int s;
	int i;

	if ((ot_protoCnt >= 3) && !ifID_table[IFID_HOME]) {
		return(ENETDOWN);
	}

	/*
	 *	If not ready, return error
	 */
	if (ddp_off_flag)
		return ENOTREADY;

	/* Look for an unused minor number */
	ATDISABLE(s, ddpall_lock);
	for (i=DDPCLONEMIN; i <= DDP_SOCKET_LAST; i++) {
		if (!ddp_devs[i].flags) {
			break;	/* Either ALLOCATED or SHUTDOWN */
		}
	}

	if (i > DDP_SOCKET_LAST) {
		ATENABLE(s, ddpall_lock);
		return(EADDRNOTAVAIL);
	}

	/* If we arrive here, i is the minor number to be
	 * allocated for this stream.
	 */
	ddp_devs[i].flags |= DDPF_ALLOCATED;
	ddp_devs[i].proto = 0;
	ddp_devs[i].sock_entry = NULL;
	ddp_devs[i].pid = gref->pid;
	gref->info = (void *)&ddp_devs[i];
	ATENABLE(s, ddpall_lock);

	return(0);
}

int
ddp_close(gref)
	gref_t *gref;
{
	int s;
	int rc = 0;
 
	/*
	 * Free the entry for this queue in the ddp_devs table.
	 * Can clear both DDPF_ALLOCATED, DDPF_SHUTDOWN since this is the
	 *  "last close" for this device.
	 */
	ATDISABLE(s, ddpall_lock);
	((ddp_dev_t *)gref->info)->flags = 0;

	/* protect against the case where close is done on socket that's
	 * not bound
	 */
	if (DDP_SOCKENTRY(gref))
 		rc = ddp_close_socket(DDP_SOCKENTRY(gref));
	ATENABLE(s, ddpall_lock);
	return rc;
}

int ddp_putmsg(gref, mp)
	gref_t *gref;
	gbuf_t *mp;
{
	extern char ot_protoT[];
	ddp_socket_t	socket;
	register ioc_t	*iocbp;
	register int	error;
	at_ddp_t *ddp;
	at_inet_t *addr;
	gbuf_t *ymp;
	union at_ddpopt *ddpopt;

	switch(gbuf_type(mp)) {
	case MSG_DATA :
		/* If this message is going out on a socket that's not bound, 
		 * nail it.
		 */
	ddp = (at_ddp_t *)gbuf_rptr(mp);
	if (ot_protoT[ddp->type]) {
		if ((gref == 0) || !DDP_SOCKENTRY(gref)) {
			int src_addr_included = ((ddp->type==3) && ddp->src_node)? 1 : 0;
			if (ddp_output(&mp, ddp->src_socket, src_addr_included) != 0) {
				do {
					ymp = gbuf_next(mp);
					gbuf_next(mp) = 0;
					gbuf_freem(mp);
					mp = ymp;
				} while (mp);
			}
			return;
		}
	}
		if (gref && !DDP_SOCKENTRY(gref)) {
			data_error(ENOTCONN, mp, gref);
			return;
		}

		if ((error = ddp_output(&mp, DDP_SOCKNUM(gref), 0)) != 0)
		  if (gref)
			data_error(error, mp, gref);
		  else
			gbuf_freem(mp);
		return;

	case MSG_IOCTL :
		iocbp = (ioc_t *)gbuf_rptr(mp);
		if (DDP_IOC_MYIOCTL(iocbp->ioc_cmd)) {
			switch(iocbp->ioc_cmd) {
			case DDP_IOC_GET_CFG :
				if (gbuf_cont(mp))
					gbuf_freem(gbuf_cont(mp));
				if ((gbuf_cont(mp) = gbuf_alloc(sizeof(at_ddp_cfg_t),
					PRI_MED)) == NULL) {
				  if (gref == 0) {
					iocbp->ioc_error = ENOBUFS;
					gbuf_set_type(mp, MSG_IOCNAK);
					if (iocbp->ioc_error == 3)
						atp_input(mp);
					else
						adsp_input(mp);
				  } else
					ioc_ack(ENOBUFS, mp, gref);
					break;
				}
			  if (gref == 0) {
				ddp_get_cfg((at_ddp_cfg_t *)gbuf_rptr(gbuf_cont(mp)),
					iocbp->ioc_count);
			  } else
				ddp_get_cfg((at_ddp_cfg_t *)gbuf_rptr(gbuf_cont(mp)),
					DDP_SOCKNUM(gref));
				gbuf_wset(gbuf_cont(mp),sizeof(at_ddp_cfg_t));
				iocbp->ioc_count = sizeof(at_ddp_cfg_t);
			  if (gref == 0) {
				gbuf_set_type(mp, MSG_IOCACK);
				error = iocbp->ioc_error;
				iocbp->ioc_error = 0;
				if (error == 3)
					atp_input(mp);
				else
					adsp_input(mp);
			  } else
				ioc_ack(0, mp, gref);
				break;
			case DDP_IOC_GET_STATS :
				if (gbuf_cont(mp))
					gbuf_freem(gbuf_cont(mp));
				if ((gbuf_cont(mp) = gbuf_alloc(sizeof(at_ddp_stats_t),
					PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, mp, gref);
					break;
				}
				ddp_get_stats(
					(at_ddp_stats_t *)gbuf_rptr(gbuf_cont(mp)));
				gbuf_wset(gbuf_cont(mp),sizeof(at_ddp_stats_t));
				iocbp->ioc_count = sizeof(at_ddp_stats_t);
				ioc_ack(0, mp, gref);
				break;
			case DDP_IOC_BIND_SOCK :
				if ((iocbp->ioc_count < sizeof(at_socket)) ||
					(gbuf_cont(mp) == NULL) ||
					(gbuf_len(gbuf_cont(mp)) < sizeof(at_socket))) {
					ioc_ack(EMSGSIZE, mp, gref);
					break;
				}
				socket.number= *(at_socket *)gbuf_rptr(gbuf_cont(mp));
				socket.flags = 0;
				socket.sock_u.gref = gref;
				socket.dev = (void *)gref->info;
				if ((error = ddp_bind_socket(&socket)) != 0) {
					ioc_ack(error, mp, gref);
					break;
				} else {
					DDP_SOCKENTRY(gref) = &ddp_socket[socket.number];
					DDP_SOCKENTRY(gref)->otChecksum = 0;
					DDP_SOCKENTRY(gref)->otType = 0;
					DDP_SOCKENTRY(gref)->otPeerAddr = 0;
					ddp_socket[socket.number].dev = (void *)gref->info;
					*(at_socket *)gbuf_rptr(gbuf_cont(mp)) = socket.number;
					ioc_ack(0, mp, gref);
				}
				break;
			case DDP_IOC_SET_PROTO :
				((ddp_dev_t *)gref->info)->proto = *(u_char *)gbuf_rptr(gbuf_cont(mp));
				ioc_ack(0, mp, gref);
				break;
			case DDP_IOC_SET_OPTS:
				if (DDP_SOCKENTRY(gref) == NULL) {
					ioc_ack(ENOTREADY, mp, gref);
					break;
				}
				if (gbuf_cont(mp) != NULL) {
					ddpopt = (union at_ddpopt *)gbuf_rptr(gbuf_cont(mp));
					DDP_SOCKENTRY(gref)->otChecksum = ddpopt->ct.checksum;
					DDP_SOCKENTRY(gref)->otType = ddpopt->ct.type;
				}
				ioc_ack(0, mp, gref);
				break;
			case DDP_IOC_GET_OPTS:
				if (DDP_SOCKENTRY(gref) == NULL) {
					ioc_ack(ENOTREADY, mp, gref);
					break;
				}
				if ((gbuf_cont(mp) == NULL)
						&& (gbuf_cont(mp) = gbuf_alloc(sizeof(union at_ddpopt),
					PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, mp, gref);
					break;
				}
				gbuf_wset(gbuf_cont(mp),sizeof(union at_ddpopt));
				ddpopt = (union at_ddpopt *)gbuf_rptr(gbuf_cont(mp));
				ddpopt->ct.checksum = DDP_SOCKENTRY(gref)->otChecksum;
				ddpopt->ct.type = DDP_SOCKENTRY(gref)->otType;
				ioc_ack(0, mp, gref);
				break;
			case DDP_IOC_GET_SOCK:
			case DDP_IOC_GET_PEER:
				if (DDP_SOCKENTRY(gref) == NULL) {
					ioc_ack(ENOTREADY, mp, gref);
					break;
				}
				if ((gbuf_cont(mp) == NULL)
						&& (gbuf_cont(mp) = gbuf_alloc(sizeof(at_inet_t),
					PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, mp, gref);
					break;
				}
				gbuf_wset(gbuf_cont(mp),sizeof(at_inet_t));
				addr = (at_inet_t *)gbuf_rptr(gbuf_cont(mp));
				if (iocbp->ioc_cmd == DDP_IOC_GET_SOCK) {
					bzero((char *)addr, sizeof(at_inet_t));
					addr->socket = DDP_SOCKENTRY(gref)->number;
					if (ifID_table[IFID_HOME]) {
						addr->node =
							ifID_table[IFID_HOME]->ifThisNode.atalk_node;
						*(unsigned short *)addr->net = *(unsigned short *)
							ifID_table[IFID_HOME]->ifThisNode.atalk_net;
					}
				} else
					*addr = *(at_inet_t *)&DDP_SOCKENTRY(gref)->otPeerAddr;
				ioc_ack(0, mp, gref);
				break;
			case DDP_IOC_SET_PEER:
				if (DDP_SOCKENTRY(gref) == NULL) {
					ioc_ack(ENOTREADY, mp, gref);
					break;
				}
				if (gbuf_cont(mp) == NULL) {
					ioc_ack(EINVAL, mp, gref);
					break;
				}
				*(at_inet_t *)&DDP_SOCKENTRY(gref)->otPeerAddr =
					*(at_inet_t *)gbuf_rptr(gbuf_cont(mp));
				ioc_ack(0, mp, gref);
				break;
			}
		} else if (ZIP_IOC_MYIOCTL(iocbp->ioc_cmd)) {
			zip_ioctl(gref, mp);
			break;
		} else {
			/* Unknown ioctl */
			ioc_ack(EINVAL, mp, gref);
		}
		break;
	case MSG_PROTO:
		if (((ddp_dev_t *)gref->info)->flags & DDPF_SI)
			si_ddp_proto_req(gref, mp);
		else
			gbuf_freem(mp);
		break;
	case MSG_CTL:
		/* *** this should never happen *** */
		kprintf("ddp_putmsg: MSG_CTL received/n");
		gbuf_freem(mp);
		break;
	case MSG_IOCACK:
		ddp_stop_ack(gref, mp);
		break;
	default :
		gbuf_freem(mp);
		break;
	}
	return;
} /* ddp_putmsg */

char	*
ddps_init()
{	register int i;
	register ddp_dev_t *d;

	for (i=0, d=ddp_devs; i < DDP_SOCKET_LAST;i++, d++)
		d->sock_entry = NULL;

	/* A non-zero value to keep ddp_init happy */
	return ((char *)1);
}


int ddps_shutdown(grefInput)
gref_t	*grefInput;
{
	/* Network is shutting down... send error messages up on each open
	 * socket
	 */
	register gbuf_t		*mp, *mp2;
	register gref_t	*gref;
	register at_socket	socket;
	register ddp_socket_t	*socketp;
	register int		do_timeout = 0;

	if ((mp = gbuf_alloc(1, PRI_HI)) == NULL) {
		goto later;
	}

	gbuf_wset(mp, 1);
	gbuf_set_type(mp, MSG_ERROR);
	*gbuf_rptr(mp) = ESHUTDOWN;

	for (socket = 1, socketp = &ddp_socket[1]; socket <= DDP_SOCKET_LAST;
		socket++, socketp++) {
		if (gref = socketp->sock_u.gref) {
			if (mp2 = (gbuf_t *)gbuf_copym(mp)) {
				if (socketp->flags & DDP_CALL_HANDLER)
					(*socketp->sock_u.handler)(mp2, 0);
				else
					atalk_putnext(gref, mp2);
			} else {
				/* Out of buffers, wait a while...
				 */
				do_timeout = 1;
				ddp_devs[socketp->number].flags
					|= DDPF_SHUTDOWN;
				continue;
			}
		}
	}

	gbuf_freem(mp);
	if (do_timeout)
		goto later;
	return;

later:
	atalk_timeout(ddps_shutdown, grefInput, SYS_HZ/10);
	return;
}

gbuf_t  *ddp_compress_msg(mp)
register gbuf_t	*mp;
{
        register gbuf_t   *tmp;

        while (gbuf_len(mp) == 0) {
                tmp = mp;
                mp = gbuf_cont(mp);
                gbuf_freeb(tmp);

		if (mp == NULL)
		        break;
	}
	return (mp);
}

int
si_ddp_proto_req(gref, mp)
    gref_t *gref;
    gbuf_t *mp;
{
    at_ddp_t *ddp;
    at_inet_t *remaddr;
    gbuf_t *xmp = gbuf_cont(mp);

    if ((DDP_SOCKENTRY(gref) == NULL) || (xmp == NULL)) {
        gbuf_freem(mp);
        return(-1);
    }

    if (gbuf_len(mp) == 1) {
        remaddr = (at_inet_t *)&DDP_SOCKENTRY(gref)->otPeerAddr;
        if (remaddr->socket == 0) {
            gbuf_freem(mp);
            return(-1);
        }
    } else
        remaddr = (at_inet_t *)gbuf_rptr(mp);

    gbuf_rinc(xmp,DDP_X_HDR_SIZE);
    ddp = (at_ddp_t *)gbuf_rptr(mp);
    *(unsigned short *)&ddp->checksum = DDP_SOCKENTRY(gref)->otChecksum;
    ddp->length = gbuf_len(xmp);
    ddp->dst_socket = remaddr->socket;
    ddp->dst_node = remaddr->node;
    ddp->type = DDP_SOCKENTRY(gref)->otType;
    *(unsigned short *)&ddp->dst_net = *(unsigned short *)&remaddr->net;
    gbuf_freeb(mp);
    if (ddp_output(&xmp, DDP_SOCKENTRY(gref)->number, 0) != 0) {
        gbuf_freem(xmp);
        return(-1);
    }

    return( 0);
}

int
si_ddp_input(gref, mp)
    gref_t *gref;
    gbuf_t  *mp;
{
    gbuf_t *xmp;
    at_ddp_t *ddp;
    at_inet_t *remaddr;

    /*
     * allocate a buffer for the M_PROTO
     */
    if ((xmp = gbuf_alloc(sizeof(at_inet_t), PRI_MED)) == NULL) {
        gbuf_freem(mp);
        return(ENOBUFS);
    }
    gbuf_cont(xmp) = mp;

    /*
     * construct the M_PROTO
     */
    ddp = (at_ddp_t *)gbuf_rptr(mp);
    gbuf_rinc(mp,DDP_X_HDR_SIZE);
    gbuf_wset(xmp,sizeof(at_inet_t));
    remaddr = (at_inet_t *)gbuf_rptr(xmp);
    remaddr->socket = ddp->src_socket;
    remaddr->node = ddp->src_node;
    *(unsigned short *)&remaddr->net = *(unsigned short *)&ddp->src_net;
    gbuf_set_type(xmp, MSG_PROTO);

    /*
     * send the M_PROTO upstream
     */
    atalk_putnext(gref, xmp);
    return(0);
}

/*
 * stop DDP now
 */
void
ddp_stop(mioc, gref)
	gbuf_t *mioc;
	gref_t *gref;
{
	unsigned char flag;
	int k, s, *x_wptr;
	ddp_dev_t *dev;
	gbuf_t *m = gbuf_cont(mioc);

	ATDISABLE(s, ddpinp_lock);
	if ((flag = *gbuf_rptr(m)) != 0)
		ddp_off_flag = 1;

	x_wptr = (int *)gbuf_wptr(m);
	for (k=0; k < (DDP_SOCKET_LAST+1); k++) {
		dev = &ddp_devs[k];
		if ((dev->flags == 0) || (dev->pid == 0))
			continue;
		*(int *)gbuf_wptr(m) = dev->pid;
		gbuf_winc(m,sizeof(int));
	}
	ATENABLE(s, ddpinp_lock);

	while (x_wptr != (int *)gbuf_wptr(m)) {
		dPrintf(D_M_DDP,D_L_TRACE, ("ddp_stop: pid=%d\n", *x_wptr));
		x_wptr++;
	}

	ddp_ack_m = mioc;
	ddp_ack_gref = gref;
	gbuf_rptr(m)[1] = 0;
	gbuf_set_type(m, MSG_IOCTL);
	atp_input(m);
}

static void
ddp_stop_ack(gref, m)
	gref_t *gref;
	gbuf_t *m;
{
	static int gotADSPinput = 0;

	if (!gotADSPinput) {
		dPrintf(D_M_DDP,D_L_INFO,
			("ddp_stop_ack: CHECK_STATE...1\n"));
		gotADSPinput = 1;
		gbuf_set_type(m, MSG_IOCTL);
		adsp_input(m);
	} else {
		dPrintf(D_M_DDP,D_L_INFO,
			("ddp_stop_ack: CHECK_STATE...2\n"));
		gotADSPinput = 0;
		gbuf_rinc(m,sizeof(int));
		gbuf_set_type(m, MSG_DATA);
		((ioc_t *)gbuf_rptr(ddp_ack_m))->ioc_count = gbuf_msgsize(m);
		ioc_ack(0, ddp_ack_m, ddp_ack_gref);
	}
}
