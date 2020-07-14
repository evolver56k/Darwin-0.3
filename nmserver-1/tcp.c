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
   * Mach Operating System
   * Copyright (c) 1989 Carnegie-Mellon University
   * Copyright (c) 1988 Carnegie-Mellon University
   * Copyright (c) 1987 Carnegie-Mellon University
   * All rights reserved.  The CMU software License Agreement specifies
   * the terms and conditions for use and redistribution.
   */

/*
   * TCP transport module.
   *
   * This is a simple interface to the TCP facility in the kernel. It uses
   * normal Unix sockets to communicate with the kernel.
   */


#ifdef WIN32

#include	<winnt-pdo.h>
#ifdef NEXT_CRT
#include	<sys/types.h>
#endif
#include	<winsock.h>
#include	"nm_extra.h"

#define ERRNO WSAGetLastError()

#else

#ifdef hpux
#include	<dce/cma.h>
#endif

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netinet/tcp.h>
#include	<sys/signal.h>
#include	<sys/uio.h>
#include	<arpa/inet.h>

#ifdef NeXT_PDO
#include <unistd.h>
#endif

#define ERRNO errno

#endif WIN32

#include	"netmsg.h"
#include	"timer.h"
#include	<servers/nm_defs.h>
#include 	<mach/mach.h>
#include	<mach/cthreads.h>
#include	<errno.h>


#include	"sbuf.h"
#include	"sys_queue.h"
#include	"transport.h"
#include	"disp_hdr.h"
#include	"dispatcher.h"
#include	"tcp_defs.h"

#ifdef WIN32
#undef MAX
#undef MIN
#endif

#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))


#ifdef WIN32

/*
   * Implement our own version of the writev() function.  Actually, sockets and
   * fds are not interchangable on NT, so we can't use write() on a socket.
   * Gonna call this sendv() then to differentiate between sockets and fds.
   */

struct iovec {
    void *iov_base;
    int iov_len;
};

#define writev(fd,vect,count) sendv(fd,vect,count,0)

int sendv (int fd, struct iovec *vectors, int vector_count, int flags);

static int safeSend(int fd, void *data, int len, int flags)
{
    int sent = 0, cc = 0;
    unsigned char *cp = data;
    
    for (sent = 0; sent < len; sent += cc)
    {
	cc = send(fd, &cp[sent], len - sent, flags);
	if (cc < 0)
	    return cc;
	else if (cc != len)
	    Sleep(0);	/* Release timeslice to somebody else */
    }
    return len;
}

#define LOC_BUF_SIZE (8 * 1024)
#define HIGH_WATER ((LOC_BUF_SIZE * 3) / 4)

int sendv(int fd, struct iovec *vec, int vector_count, int flags)
{
    int count, total;
    int i, filledBuf, len;
    unsigned char locBuf[LOC_BUF_SIZE];

    filledBuf = 0;
    total = 0;
    for (i = 0; i < vector_count; i++)
    {
	len = vec[i].iov_len;
	if (filledBuf + len < LOC_BUF_SIZE)
	{
	    memcpy(&locBuf[filledBuf], vec[i].iov_base, len);
	    filledBuf += len;
	    continue;	/* Continue filling the buffer */
	}

	if (filledBuf)
	{
	    count = safeSend(fd, locBuf, filledBuf, flags);
	    if (-1 == count) goto sendFailed;
	    total += count;
	    filledBuf = 0;
	}

	if (len < HIGH_WATER)
	{
	    i--; continue;	// Stuff this vector into the buffer
	}

	count = safeSend(fd, vec[i].iov_base, len, flags);
	if (-1 == count) goto sendFailed;
	total += count;
    }
    if (filledBuf)
    {
	count = safeSend(fd, locBuf, filledBuf, flags);
	if (-1 == count) goto sendFailed;
	total += count;
    }
    return total;

sendFailed:
    ERROR((msg,"sendv send failed: errno=%d", ERRNO));
    panic("tcp");
    return -1;
}

#endif WIN32

/*
   * Forward declarations.
   */
static void	tcp_conn_handler();


/*
   * Size for a small (inline) receive buffer.
   *
   * Must be a multiple of 4.
   */
#define	TCP_BUFSZ		8192


/*
   * Size of iovec for data transmission.
   */
#define	TCP_IOVLEN		16

/*
   * TCP port to be used by the Mach network service.
   */
#define	TCP_NETMSG_PORT	2453


/*
   * Debugging flags.
   */
#define	TCP_DBG_MAJOR	(debug.tcp & 0x1)	/* major events */
#define	TCP_DBG_CRASH	(debug.tcp & 0x2)	/* host crashes */
#define	TCP_DBG_VERBOSE	(debug.tcp & 0x4)	/* verbose output */
#define	TCP_DBG_SOCKET	(debug.tcp & 0x8)	/* kernel SO_DEBUG */


/*
   * Control messages.
   */
#define	TCP_CTL_REQUEST		1
#define	TCP_CTL_REPLY		2
#define	TCP_CTL_CLOSEREQ	3
#define	TCP_CTL_CLOSEREP	4

typedef struct tcp_ctl {
    int		ctl;
    unsigned long	trid;
    int		code;
    unsigned long	size;
    int		crypt_level;
} tcp_ctl_t, *tcp_ctl_ptr_t;

/*
   * Connection records.
   */
typedef	struct tcp_conn {
    int state;					/* see defines below */
    int sock;					/* socket descriptor */
    cthread_t th;				/* service thread */
    int	stay_alive;				/* set by access, decremented by scavenger */
    struct mutex lock;			/* lock for this record */
    struct condition cond;		/* to wake up the service thread */
    netaddr_t peer;				/* peer for current connection */
    netaddr_t to;				/* destination for active-side conn */
    sys_queue_head_t trans;		/* list of pending/waiting transactions */
    int count;					/* number of pending/waiting trans */
    sys_queue_chain_t connq;	/* list of records */
    unsigned long incarn;		/* incarnation number */
    struct iovec iov[TCP_IOVLEN];/* for writev() */
        tcp_ctl_t ctlbuf;			/* for xmit control header */
} tcp_conn_t, *tcp_conn_ptr_t;

#define	TCP_INVALID	0
#define	TCP_FREE	1
#define	TCP_CONNECTED	2
#define	TCP_OPENING	3
#define	TCP_CLOSING	4
#define	TCP_CLOSED	5

#define TCP_STAY_ALIVE 5
#define TCP_SCAVENGER_INTERVAL 30
/*
   * Static declarations.
   */
/* The following number (ie., 128) must be == param.tcp_conn_max */
PRIVATE tcp_conn_t conn_vec[128];	/* connection records */

PRIVATE sys_queue_head_t conn_lru;	/* LRU list of active conn */
PRIVATE int conn_num;				/* number of active conn */
PRIVATE sys_queue_head_t conn_free;	/* list of free conn */
PRIVATE struct condition conn_cond;	/* to wake up listener */
PRIVATE int conn_closing;			/* number of conn in TCP_CLOSING */
PRIVATE struct mutex conn_lock;		/* lock for conn_lru & conn_free */


/*
   * Transport IDs are composed of 16 bits for the client side and 16 bits
   * for the server side. The client side is just a counter, to be matched
   * between the message and the transaction record. The server side is composed
   * of 8 bits of index of the connection record in the conn_vec array and
   * 8 bits of incarnation number for this connection record.
   *
   * We can afford not to protect the counter for client-side IDs with a lock,
   * because transaction records for one connection are protected by the lock
   * that connection, and they never move from one connection to another.
   *
   * XXX This is not completely foolproof if there is A LOT of traffic,
   * but it's cheap.
   */
PRIVATE unsigned long			trid_counter;

/*
   * Function for transmission of data.
   *
   * cp->lock must be held throughout.
   */
static int tcp_xmit_data(
                         tcp_conn_ptr_t cp, int ctlcode, unsigned long a_trid,
                         int a_code, sbuf_t a_data, int a_crypt)
{
    int ret;
    unsigned long	_size;
    sbuf_seg_ptr_t	_sp;
    sbuf_seg_ptr_t	_end;
    int		_iovc;

    cp->ctlbuf.ctl = htonl(ctlcode);
    cp->ctlbuf.trid = htonl(a_trid);
    cp->ctlbuf.code = htonl(a_code);
    SBUF_GET_SIZE(a_data,_size);
    cp->ctlbuf.size = htonl(_size);
    cp->ctlbuf.crypt_level = htonl(a_crypt);
    /*
       * XXX Worry about data encryption.
       */

    /*
       * Fill in the iovec and send all the data possible.
       */
    cp->iov[0].iov_base = (caddr_t)&(cp->ctlbuf);
    cp->iov[0].iov_len = sizeof(tcp_ctl_t);
    _sp = a_data.segs;
    _end = a_data.end;
    _iovc = 1;
    while ((_sp != _end) && (_iovc < TCP_IOVLEN)) {
        if (_sp->s != 0) {
            cp->iov[_iovc].iov_base = (caddr_t)_sp->p;
            cp->iov[_iovc].iov_len = _sp->s;
            _iovc++;
        }
        _sp++;
    }
    ret = writev(cp->sock,cp->iov,_iovc);
    INCSTAT(tcp_send);

    /*
       * Send the remaining segments one by one.
       */
    while ((ret >= 0) && (_sp != _end)) {
        if (_sp->s != 0) {
            ret = send(cp->sock,(void *)_sp->p,_sp->s,0);
            INCSTAT(tcp_send);
        }
        _sp++;
    }
    cp->stay_alive = TCP_STAY_ALIVE;
    return ret;
} /* tcp_xmit_data */

/*
 * Function for transmission of a simple control message.
 *
 * cp->lock must be held throughout.
 */
static int tcp_xmit_control(
                            tcp_conn_ptr_t cp, int ctlcode, unsigned long a_trid, int a_code)
{
    int ret;
    cp->ctlbuf.ctl = htonl(ctlcode);
    cp->ctlbuf.trid = htonl(a_trid);
    cp->ctlbuf.code = htonl(a_code);
    cp->ctlbuf.size = 0;
    cp->ctlbuf.crypt_level = 0;
    ret = send(cp->sock,(char *)&(cp->ctlbuf),sizeof(tcp_ctl_t),0);
    INCSTAT(tcp_send);
    cp->stay_alive = TCP_STAY_ALIVE;
    return ret;
}

/*
   * Memory management definitions.
   */
PUBLIC mem_objrec_t	MEM_TCPTRANS;

/*
   * tcp_init_conn --
   *
   * Allocate and initialize a new TCP connection record.
   *
   * Parameters:
   *
   * Results:
   *
   * pointer to the new record.
   *
   * Side effects:
   *
   * Starts a new thread to handle the connection.
   *
   * Note:
   *
   * conn_lock must be acquired before calling this routine.
   * It is held throughout its execution.
   */
PRIVATE tcp_conn_ptr_t tcp_init_conn()
{
    tcp_conn_ptr_t	cp;
    int		i;
    cthread_t	th;
    char		name[40];

    /*
       * Find an unused connection record in the conn_vec array.
       * We could have used the global memory allocator for that,
       * but since there are few connection records, why bother...
       *
       * conn_lock guarantees mutual exclusion.
       */
    cp = NULL;
    for (i = 0; i < param.tcp_conn_max; i++) {
        if (conn_vec[i].state == TCP_INVALID) {
            cp = &conn_vec[i];
            break;
        }
    }
    if (cp == NULL) {
        panic("The TCP module cannot allocate a new connection record");
    }

    cp->stay_alive = TCP_STAY_ALIVE;
    cp->state = TCP_FREE;
    cp->sock = 0;
    cp->count = 0;
    cp->peer = 0;
    cp->to = 0;
    mutex_init(&cp->lock);
    mutex_lock(&cp->lock);
    condition_init(&cp->cond);
    sys_queue_init(&cp->trans);
    th = cthread_fork((cthread_fn_t)tcp_conn_handler,cp);
    cp->th = th;
    sprintf(name,"tcp_conn_handler(0x%x)",(unsigned)cp);
    cthread_set_name(th,name);
    cthread_detach(th);


    mutex_unlock(&cp->lock);

    RETURN(cp);
}



/*
   * tcp_close_conn --
   *
   * Arrange to close down one TCP connection as soon as possible.
   *
   * Parameters:
   *
   * Results:
   *
   * Side effects:
   *
   * Note:
   *
   * conn_lock must be acquired before calling this routine.
   * It is held throughout its execution.
   */
PRIVATE void tcp_close_conn()
{
    tcp_conn_ptr_t first;
    tcp_conn_ptr_t cp;
    int ret;

    /*
     * Look for an old connection to recycle.
     */
    first = (tcp_conn_ptr_t)sys_queue_first(&conn_lru);
    cp = (tcp_conn_ptr_t)sys_queue_last(&conn_lru);
    while (cp != first) {
        if ((cp->count == 0) && (cp->state == TCP_CONNECTED)) {
            /*
             * Close this unused connection.
             */
	    mutex_lock(&cp->lock);
	    if ((cp->count == 0) && (cp->state == TCP_CONNECTED)) {
		cp->state = TCP_CLOSING;
		conn_closing++;
		ret = tcp_xmit_control(cp, TCP_CTL_CLOSEREQ, 0, 0);
		mutex_unlock(&cp->lock);
		break;
	    }
	    mutex_unlock(&cp->lock);
        }
        cp = (tcp_conn_ptr_t)sys_queue_prev(&cp->connq);
    }

    /*
     * We are over-committed. We will try again
     * to close something at the next request or
     * reply.
     */

    RET;
}



/*
   * tcp_sendrequest --
   *
   * Send a request through the TCP interface.
   *
   * Parameters:
   *
   *	client_id	: an identifier assigned by the client to this transaction
   *	data		: the data to be sent
   *	to		: the destination of the request
   *	crypt_level	: whether the data should be encrypted
   *	reply_proc	: a function to be called to handle the response
   *
   * Results:
   *
   *	TR_SUCCESS or a specific failure code.
   *
   * Side effects:
   *
   * Design:
   *
   * Note:
   *
   */
EXPORT int tcp_sendrequest(client_id,data,to,crypt_level,reply_proc)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	to;
int		crypt_level;
int		(*reply_proc)();
{
    tcp_conn_ptr_t		first;
    tcp_conn_ptr_t		cp;
    tcp_trans_ptr_t		tp;
    int			ret;

    mutex_lock(&conn_lock);
    INCSTAT(tcp_requests_sent);

    /*
       * Find an open connection to the destination.
       */
    first = (tcp_conn_ptr_t)sys_queue_first(&conn_lru);
    cp = first;
    while (!sys_queue_end(&conn_lru,(sys_queue_entry_t)cp)) {
        if (cp->to == to) {
            mutex_lock(&cp->lock);
            cp->count++;
            mutex_unlock(&cp->lock);
            break;
        }
        cp = (tcp_conn_ptr_t)sys_queue_next(&cp->connq);
    }

    if (sys_queue_end(&conn_lru,(sys_queue_entry_t)cp)) {
        /*
           * Could not find an open connection.
           */
        if (conn_num < param.tcp_conn_opening) {
            /*
               * Immediately start a new connection.
               */
            if (sys_queue_empty(&conn_free)) {
                /*
                   * Initialize a new connection record.
                   */
                cp = tcp_init_conn();
            } else {
                cp = (tcp_conn_ptr_t)sys_queue_first(&conn_free);
                sys_queue_remove(&conn_free,cp,
                                 tcp_conn_ptr_t,connq);
            }
            mutex_lock(&cp->lock);
            sys_queue_enter_first(&conn_lru,cp,tcp_conn_ptr_t,connq);
            conn_num++;
            cp->peer = to;
            cp->to = to;
            cp->count = 1;
            cp->state = TCP_OPENING;
            cp->stay_alive = TCP_STAY_ALIVE;
            condition_signal(&cp->cond);
            mutex_unlock(&cp->lock);
            if ((conn_num - conn_closing) > param.tcp_conn_steady) {
                tcp_close_conn();
            }
            mutex_unlock(&conn_lock);
        } else {
            /*
               * We are over-committed. Tell the caller to wait.
               */
            if ((conn_num - conn_closing) > param.tcp_conn_steady) {
                tcp_close_conn();
            }
            mutex_unlock(&conn_lock);
            RETURN(TR_OVERLOAD);
        }
    } else {
        /*
           * Found an open connection. Use it!
           */
        if (cp != first) {
            /*
               * Place the record at the head of the queue.
               */
            sys_queue_remove(&conn_lru,cp,tcp_conn_ptr_t,connq);
            sys_queue_enter_first(&conn_lru,cp,tcp_conn_ptr_t,connq);
        }
        if ((conn_num - conn_closing) > param.tcp_conn_steady) {
            tcp_close_conn();
        }
        mutex_lock(&cp->lock);
        mutex_unlock(&conn_lock);
    }

    /*
       * At this point, we have a lock on a connection record for the
       * right destination. See if we can transmit the data.
       */

    /*
       * Link the transaction record in the connection record.
       */
    MEM_ALLOCOBJ(tp,tcp_trans_ptr_t,MEM_TCPTRANS);
    tp->client_id = client_id;
    tp->reply_proc = reply_proc;
    tp->trid = (trid_counter++) & 0xffff;


    if (cp->state == TCP_FREE) {
        panic("TCP module trying to transmit on a free connection");
    }

    if (cp->state == TCP_CONNECTED) {
        /*
           * Send all the data on the socket.
           */
        tp->state = TCP_TR_PENDING;
        ret = tcp_xmit_data(
                            cp, TCP_CTL_REQUEST, tp->trid, 0, (*data), crypt_level);
        if (ret < 0) {
            /*
               * Something went wrong. Most probably, the client is dead.
               */
            cp->count--;
            mutex_unlock(&cp->lock);
            MEM_DEALLOCOBJ(tp,MEM_TCPTRANS);
            RETURN(TR_FAILURE);
        }
    } else {
        tp->state = TCP_TR_WAITING;
        tp->data = data;
        tp->crypt_level = crypt_level;
    }
    sys_queue_enter(&cp->trans,tp,tcp_trans_ptr_t,transq);
    mutex_unlock(&cp->lock);

    RETURN(TR_SUCCESS);
}



/*
   * tcp_sendreply --
   *
   * Send a response through the TCP interface.
   *
   * Parameters:
   *
   *	trid		: transport-level ID for a previous operation on this
   *			  transaction
   *	code		: a return code to be passed to the client.
   *	data		: the data to be sent
   *	crypt_level	: whether the data should be encrypted
   *
   * Results:
   *
   *	TR_SUCCESS or a specific failure code.
   *
   * Side effects:
   *
   * Design:
   *
   * Note:
   *
   */
EXPORT int tcp_sendreply(trid,code,data,crypt_level)
int		trid;
int		code;
sbuf_ptr_t	data;
int		crypt_level;
{
    tcp_conn_ptr_t	cp;
    int		ret;

    cp = (tcp_conn_ptr_t)&(conn_vec[trid >> 24]);
    if (((trid >> 16) & 0xff) != cp->incarn) cp = NULL;

    /*
       * If the client has died, the connection record may
       * already have been reused, and we may be sending this reply
       * to the wrong machine. This should be detected by the
       * incarnation number in the trid.
       */
    if (cp == NULL) {
        RETURN(TR_FAILURE);
    }

    mutex_lock(&cp->lock);

    INCSTAT(tcp_replies_sent);

    if (cp->state != TCP_CONNECTED) {
        /*
           * The client has died or the connection has just
           * been dropped. Drop the reply.
           */
        mutex_unlock(&cp->lock);
        RETURN(TR_FAILURE);
    }

    cp->count--;
    if (data)
        ret = tcp_xmit_data(
                            cp, TCP_CTL_REPLY, trid, code, (*data), crypt_level);
    else
        ret = tcp_xmit_control(cp, TCP_CTL_REPLY, trid, code);

    if (ret < 0) {
        /*
           * Something went wrong. Most probably, the client is dead.
           */
        mutex_unlock(&cp->lock);
        RETURN(TR_FAILURE);
    }

    mutex_unlock(&cp->lock);

    /*
       * Update the LRU list of active connections and check for
       * excess connections.
       */
    mutex_lock(&conn_lock);
    if (cp != (tcp_conn_ptr_t)sys_queue_first(&conn_lru)) {
        /*
           * Place the record at the head of the queue.
           */
        sys_queue_remove(&conn_lru,cp,tcp_conn_ptr_t,connq);
        sys_queue_enter_first(&conn_lru,cp,tcp_conn_ptr_t,connq);
    }
    if ((conn_num - conn_closing) > param.tcp_conn_steady) {
        tcp_close_conn();
    }
    mutex_unlock(&conn_lock);

    RETURN(TR_SUCCESS);
}



/*
   * tcp_conn_handler_open --
   *
   * Handler for one connection - opening phase.
   *
   * Parameters:
   *
   * cp: pointer to the connection record.
   *
   * Results:
   *
   * TRUE if the connection was successfully opened, FALSE otherwise.
   *
   * Side effects:
   *
   * Transactions waiting in the connection record are initiated.
   *
   * Note:
   *
   * cp->lock must be locked on entry. It is also locked on exit, but
   * it may be unlocked during the execution of this procedure.
   */
PRIVATE boolean_t tcp_conn_handler_open(cp)
tcp_conn_ptr_t	cp;
{
    tcp_trans_ptr_t		tp;
    int			cs;
    struct sockaddr_in	sname;
    netaddr_t		peeraddr;
    int			ret;

    sname.sin_family = AF_INET;
    sname.sin_port = htons(TCP_NETMSG_PORT);
    sname.sin_addr.s_addr = (u_long)(cp->peer);
    peeraddr = cp->peer;

    /*
     * Unlock the record while we are waiting for the connection
     * to be established.
     */
    mutex_unlock(&cp->lock);

    mutex_lock(&conn_lock);
    cs = socket(AF_INET,SOCK_STREAM,0);
    mutex_unlock(&conn_lock);
    if (cs < 0) {
        ERROR((msg,"tcp_conn_handler.socket failed: errno=%d", ERRNO));
        panic("tcp");
    }

#ifdef WIN32
    {
	int xxx = 1;
	if (setsockopt(cs, IPPROTO_TCP, TCP_NODELAY, &xxx, sizeof(xxx)) < 0) {
	    ERROR((msg,"tcp_conn_handler.setsockopt(NODELAY): %d",ERRNO));
	}
    }
#endif

    if (TCP_DBG_SOCKET) {
        int	optval;

        optval = 1;
        setsockopt(cs,SOL_SOCKET,SO_DEBUG,(char *)&optval,sizeof(int));
    }

    ret = connect(cs,(struct sockaddr *)&sname,sizeof(struct sockaddr_in));
    if (ret < 0) {
        mutex_lock(&cp->lock);
        RETURN(FALSE);
    }
    INCSTAT(tcp_connect);

    mutex_lock(&cp->lock);
    cp->sock = cs;
    cp->state = TCP_CONNECTED;

    /*
       * Look for transactions waiting to be transmitted.
       */
    tp = (tcp_trans_ptr_t)sys_queue_first(&cp->trans);
    while (!sys_queue_end(&cp->trans,(sys_queue_entry_t)tp)) {
        if (tp->state == TCP_TR_WAITING) {
            tp->state = TCP_TR_PENDING;
            ret = tcp_xmit_data(
                                cp, TCP_CTL_REQUEST, tp->trid, 0,
                                (*(tp->data)), tp->crypt_level);
            if (ret < 0) {
                RETURN(FALSE);
            }
        }
        tp = (tcp_trans_ptr_t)sys_queue_next(&tp->transq);
    }

    RETURN(TRUE);
}



/*
   * tcp_conn_handler_active --
   *
   * Handler for one connection - active phase.
   *
   * Parameters:
   *
   * cp: pointer to the connection record.
   *
   * Results:
   *
   * Exits when the connection should be closed.
   *
   * Note:
   *
   * For now, the data received on the connection is only kept until the
   * higher-level handler procedure (disp_in_request or reply_proc) returns.
   * This allows the use of a data buffer on the stack.
   *
   */
PRIVATE void tcp_conn_handler_active(cp)
tcp_conn_ptr_t	cp;
{
    int			cs;
    netaddr_t		peeraddr;
    tcp_trans_ptr_t		tp;
    int			ret;
    pointer_t		bufp=0;		/* current location in data */
    tcp_ctl_ptr_t		ctlbufp;	/* tcp control header in buf */
    int			buf_count;	/* data available in buf */
    int			buf_free=0;	/* free space in buf */
    pointer_t		bigbufp;	/* large (variable) data buffer */
    unsigned long		bigbuf_size=0;	/* size of bigbuf */
    unsigned long		bigbuf_count;	/* data in bigbuf */
    sbuf_t			sb;		/* sbuf for data received */
    unsigned long		data_size;	/* size of user data */
    unsigned long		seg_size;
    unsigned long		trid;
    int			disp_ret;
    /*
       * The following buffers are used to receive small amounts
       * of data inline, and to realign that data on a long word
       * boundary if needed. They are declared as int in order
       * to have the correct alignment.
       */
    int			buf[TCP_BUFSZ / 4];
    int			abuf[TCP_BUFSZ / 4];
    int			curbuf=0;	/* 0 = buf, 1 = abuf */

    /*
       * Prepare the sbuf for the handoff to the higher-level.
       * There will always be at most two segments, from buf and bigbuf.
       */
    SBUF_INIT(sb,2);

    peeraddr = cp->peer;	/* OK not to lock at this point */
    cs = cp->sock;
    buf_count = 0;

    /*
     * Enter the recv loop.
     */
    for (;;) {
        /*
           * Recycle the data buffer whenever needed.
           */
        if (buf_count > 0) {
            ctlbufp = (tcp_ctl_ptr_t)bufp;
            bufp += buf_count;
        } else {
            bufp = (pointer_t)buf;
            ctlbufp = (tcp_ctl_ptr_t)bufp;
            curbuf = 0;
            buf_count = 0;
            buf_free = TCP_BUFSZ - sizeof(tcp_ctl_t);
        }

        /*
           * Get at least a tcp control header in the
           * buffer. We always keep some space at the
           * end of buf in case we had received part of
           * the header in a previous pass. If we receive
           * the header in that extra space, we do not
           * receive more than that, so we can always
           * recycle the buffer after that.
           */
        while (buf_count < sizeof(tcp_ctl_t)) {
            ret = recv(cs,(void *)bufp,
                       MAX(buf_free,sizeof(tcp_ctl_t)),0);
            if (ret <= 0) {
                RET;
            }
            INCSTAT(tcp_recv);
            cp->stay_alive = TCP_STAY_ALIVE;
            buf_count += ret;
            buf_free -= ret;
            bufp += ret;
        }

        /*
           * Realign the data if needed.
           */
        if (((int)ctlbufp) & 0x3) {
            if (curbuf) {
                memmove(buf, ctlbufp, buf_count);
                curbuf = 0;
                ctlbufp = (tcp_ctl_ptr_t)buf;
                bufp = ((pointer_t)buf) + buf_count;
            } else {
                memmove(abuf, ctlbufp, buf_count);
                curbuf = 1;
                ctlbufp = (tcp_ctl_ptr_t)abuf;
                bufp = ((pointer_t)abuf) + buf_count;
            }
        }

        /*
           * Do all the required byte-swapping (Sigh!).
           */
        ctlbufp->ctl		= ntohl(ctlbufp->ctl);
        ctlbufp->trid		= ntohl(ctlbufp->trid);
        ctlbufp->code		= ntohl(ctlbufp->code);
        ctlbufp->size		= ntohl(ctlbufp->size);
        ctlbufp->crypt_level	= ntohl(ctlbufp->crypt_level);

        /* make sure the packet is valid before we attempt to
           allocate any memory */

        /* If checking the ctl field turns out not to provide
           * enough protection against rogue connections, should
           * change the code to read into the small buffer not
           * just the control header but also a dispatch header
           * and then check the disp_type field as is already done
           * in each of the dispatch routines. As the code stands
           * now, that check isn't done until after malloc'ing
           * bigbuf, which can be a big enough malloc to crash
           * the system if the size field is bogus.
           */

        switch (ctlbufp->ctl) {
          case TCP_CTL_REQUEST:
          case TCP_CTL_REPLY:
          case TCP_CTL_CLOSEREQ:
          case TCP_CTL_CLOSEREP:
            break;
          default:	
            ERROR((msg,"tcp_conn_handler_active: "
                   "bogus packet received from host "
                   "%s", inet_ntoa(*((struct in_addr*)&cp->peer))));
            
			    //continue;

            /* There's no real hope of recovery once we've
               * gotten bad data on a connection. Best to
               * return immediately so the connection gets
               * shut down
               */

            RET;
        }

        /*
           * Read any user data from the small buffer
           * and put it in the sbuf.
           * Advance the current data pointer.
           */
        bufp = (pointer_t)(ctlbufp + 1);
        buf_count -= sizeof(tcp_ctl_t);
        data_size = ctlbufp->size;
        if (data_size > 0) {
            SBUF_REINIT(sb);
            seg_size = MIN(data_size,buf_count);
            if (seg_size != 0) {
                SBUF_APPEND(sb,bufp,seg_size);
                buf_count -= seg_size;
                bufp += seg_size;
            }
        } else {
            seg_size = 0;
        }

        /*
           * Get more data in a large buffer if needed.
           */
        if (data_size > seg_size) {
            bigbuf_size = data_size - seg_size;
            MEM_ALLOC(bigbufp,pointer_t,bigbuf_size,FALSE);
            bigbuf_count = 0;
            bufp = bigbufp;
            while (bigbuf_count < bigbuf_size) {
                ret = recv(cs,(void *)bufp,
                           (bigbuf_size - bigbuf_count),0);
                if (ret <= 0) {
                    MEM_DEALLOC(bigbufp,bigbuf_size);
                    RET;
                }
                INCSTAT(tcp_recv);
                cp->stay_alive = TCP_STAY_ALIVE;
                bigbuf_count += ret;
                bufp += ret;
            }
            SBUF_APPEND(sb,bigbufp,bigbuf_size);
        } else {
            bigbufp = (pointer_t)NULL;
        }

        /*
           * XXX Worry about encryption.
           */

        /*
           * Now process the message.
           */
        switch(ctlbufp->ctl) {
          case TCP_CTL_REQUEST:
            INCSTAT(tcp_requests_rcvd);
            mutex_lock(&cp->lock);
            cp->count++;
            if (cp->state == TCP_CLOSING) {
                /*
                   * XXX BUG: Should retry
                   * waiting transactions.
                   *
                   * Not critical, since this
                   * situation can never happen,
                   * because each side initiates
                   * transactions on different
                   * connections.
                   */
                cp->state = TCP_CONNECTED;
                mutex_unlock(&cp->lock);
                mutex_lock(&conn_lock);
                conn_closing--;
                mutex_unlock(&conn_lock);
            } else {
                mutex_unlock(&cp->lock);
            }
            trid = ctlbufp->trid;
            trid |= (((((unsigned long)cp) -
                       ((unsigned long)conn_vec)) /
                      sizeof(tcp_conn_t)) << 24) | (cp->incarn << 16);

            disp_ret = disp_in_request(TR_TCP_ENTRY,trid,
                                       &sb,peeraddr,
                                       ctlbufp->crypt_level,FALSE);
            /*
               * Clean up the big buffer if there is one.
               */
            if (bigbufp != (pointer_t)NULL) {
                MEM_DEALLOC(bigbufp,bigbuf_size);
                bigbufp = (pointer_t)NULL;
            }
            if (disp_ret != DISP_WILL_REPLY) {
                mutex_lock(&cp->lock);
                ret = tcp_xmit_control(
                                       cp, TCP_CTL_REPLY, trid, disp_ret);
                cp->count--;
                mutex_unlock(&cp->lock);
                if (ret < 0) {
                    RET;
                }
            }
            break;

          case TCP_CTL_REPLY:
            INCSTAT(tcp_replies_rcvd);
            mutex_lock(&cp->lock);
            if (cp->state == TCP_CLOSING) {
                /*
                   * XXX BUG: Should retry
                   * waiting transactions.
                   *
                   * Not critical, since this
                   * situation can never happen,
                   * because we cannot not be closing
                   * a connection while awaiting
                   * a reply.
                   */
                cp->state = TCP_CONNECTED;
                mutex_unlock(&cp->lock);
                mutex_lock(&conn_lock);
                conn_closing--;
                mutex_unlock(&conn_lock);
                mutex_lock(&cp->lock);
            }
            /*
               * Find the transaction record.
               */
            trid = (ctlbufp->trid) & 0xffff;

            tp = (tcp_trans_ptr_t)sys_queue_first(&cp->trans);
            while (!sys_queue_end(&cp->trans,(sys_queue_entry_t)tp)) {
                if (tp->trid == trid) {
                    break;
                }
                tp = (tcp_trans_ptr_t)sys_queue_next(&tp->transq);
            }
            if (sys_queue_end(&cp->trans,(sys_queue_entry_t)tp)) {
                ERROR((msg, "tcp_conn_handler_active: cannot find the transaction record for a reply (trid = 0x%lx)", trid));
                mutex_unlock(&cp->lock);
            } else {
                sys_queue_remove(&cp->trans,tp,
                                 tcp_trans_ptr_t,transq);
                cp->count--;
                mutex_unlock(&cp->lock);
                if (data_size) {
                    (*(tp->reply_proc))(tp->client_id,
                                        ctlbufp->code,&sb);
                } else {
                    (*(tp->reply_proc))(tp->client_id,
                                        ctlbufp->code,0);
                }
                MEM_DEALLOCOBJ(tp,MEM_TCPTRANS);
            }
            /*
               * Clean up the big buffer if there is one.
               */
            if (bigbufp != (pointer_t)NULL) {
                MEM_DEALLOC(bigbufp,bigbuf_size);
                bigbufp = (pointer_t)NULL;
            }
            break;

          case TCP_CTL_CLOSEREQ:
            mutex_lock(&cp->lock);
            if (cp->count == 0) {
                /*
                   * Send CLOSEREP.
                   */
                ret = tcp_xmit_control(
                                       cp, TCP_CTL_CLOSEREP, 0, 0);
                if (cp->state != TCP_CLOSING) {
                    cp->state = TCP_CLOSED;
                }
                mutex_unlock(&cp->lock);
                RET;
            } else {
                /*
                   * We have some data in
                   * transit. Nothing more
                   * should be needed.
                   */
                cp->state = TCP_CONNECTED;
                mutex_unlock(&cp->lock);
            }
            break;

          case TCP_CTL_CLOSEREP:
            mutex_lock(&cp->lock);
            /*
               * cp->state can only be TCP_CLOSING:
               *
               * We have sent a CLOSEREQ, and set the
               * state to TCP_CLOSING then. If the state
               * has changed since then, it must be because
               * we have received data. But this data can only
               * be a request, because we had nothing going on
               * when we sent the CLOSEREQ. This CLOSEREQ must
               * arrive at the other end before our reply
               * because TCP does not reorder messages. But
               * then the CLOSEREQ will be rejected because
               * of the pending transaction.
               */
            mutex_unlock(&cp->lock);
            RET;

          default:
            ERROR((msg,
                   "tcp_conn_handler_active: received an unknown ctl code: %d",
                   ctlbufp->ctl));
            break;
        }
    }

}



/*
   * tcp_conn_handler_close --
   *
   * Handler for one connection - closing phase.
   *
   * Parameters:
   *
   * cp: pointer to the connection record.
   *
   * Results:
   *
   * none.
   *
   * Note:
   *
   */
PRIVATE void tcp_conn_handler_close(cp)
tcp_conn_ptr_t	cp;
{
    tcp_trans_ptr_t		tp;

    /*
       * Some transactions might be initiated after the active phase exits
       * and before this phase starts. Hopefully, they will be stopped by
       * the TCP_CLOSING or TCP_CLOSED states, or the send will fail.
       */
    mutex_lock(&conn_lock);
    mutex_lock(&cp->lock);
#ifdef WIN32
    closesocket(cp->sock);
#else
    close(cp->sock);
#endif
    INCSTAT(tcp_close);
    if (cp->state == TCP_CLOSING) {
        conn_closing--;
    }
    cp->state = TCP_FREE;

    /*
       * Go down the list of waiting/pending transactions
       * and abort them.
       * The client is of course free to retry them later.
       */
    while (!sys_queue_empty(&cp->trans)) {
        tp = (tcp_trans_ptr_t)sys_queue_first(&cp->trans);
        if (tp->state == TCP_TR_WAITING) {
            (*tp->reply_proc)
                (tp->client_id,TR_SEND_FAILURE,0,0);
        } else {
            (*tp->reply_proc)
                (tp->client_id,TR_FAILURE,0,0);
        }
        sys_queue_remove(&cp->trans,tp,tcp_trans_ptr_t,transq);
        MEM_DEALLOCOBJ(tp,MEM_TCPTRANS);
    }
    sys_queue_init(&cp->trans);
    cp->count = 0;
    cp->to = 0;
    sys_queue_remove(&conn_lru,cp,tcp_conn_ptr_t,connq);
    sys_queue_enter(&conn_free,cp,tcp_conn_ptr_t,connq);
    mutex_unlock(&cp->lock);
    conn_num--;
    if (conn_num == (param.tcp_conn_max - 1)) {
        /*
           * OK to start accepting connections again.
           */
        condition_signal(&conn_cond);
    }
    mutex_unlock(&conn_lock);

    RET;
}



/*
   * tcp_conn_handler --
   *
   * Handler for one connection.
   *
   * Parameters:
   *
   * cp: pointer to the connection record.
   *
   * Results:
   *
   *	Should never exit.
   *
   * Note:
   *
   * For clarity, this code is split into three different procedures handling
   * the opening, active and closing phases of the life of the connection.
   *
   */
PRIVATE void tcp_conn_handler(cp)
tcp_conn_ptr_t	cp;
{
    boolean_t	active;

    for (;;) {
        /*
           * First wait to be activated.
           */
        mutex_lock(&cp->lock);
        while(cp->state == TCP_FREE) {
            condition_wait(&cp->cond,&cp->lock);
        }

        /*
           * At this point, the state is either TCP_OPENING (local open)
           * or TCP_CONNECTED (remote open).
           */

        if (cp->state == TCP_OPENING) {
            /*
               * Open a new connection.
               */
            active = tcp_conn_handler_open(cp);
        } else {
            active = TRUE;
        }
        cp->incarn = (cp->incarn++) & 0xff;
        mutex_unlock(&cp->lock);

        if (active) {
            tcp_conn_handler_active(cp);
        }

        /*
           * Close the connection.
           */
        tcp_conn_handler_close(cp);

        LOGCHECK;
    }
}



/*
   * tcp_listener --
   *
   * Handler for the listener socket.
   *
   * Parameters:
   *
   * Results:
   *
   *	Should never exit.
   *
   * Note:
   *
   */
PRIVATE void tcp_listener()
{
    int			s;
    int			ret;
    struct sockaddr_in	sname;
    int			snamelen;
    tcp_conn_ptr_t		cp;

    /*
       * First create the listener socket.
       */
    mutex_lock(&conn_lock);
    s = socket(AF_INET,SOCK_STREAM,0);
    mutex_unlock(&conn_lock);
    if (s < 0) {
        ERROR((msg,"tcp_listener.socket failed: errno=%d", ERRNO));
        panic("tcp");
    }

#if defined(NeXT_PDO) && !defined(WIN32)
#warning questionable debugging code...
    {
        int allow = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&allow, sizeof(allow)) < 0) {
            ERROR((msg, "tcp_listener.setsockopt failed errno=%d", ERRNO));
        }
    }
#endif

    sname.sin_family = AF_INET;
    sname.sin_port = htons(TCP_NETMSG_PORT);
    sname.sin_addr.s_addr = INADDR_ANY;
    ret = bind(s,(struct sockaddr *)&sname,sizeof(struct sockaddr_in));
    if (ret < 0) {
        ERROR((msg,"tcp_listener.bind failed: errno=%d", ERRNO));
        panic("tcp");
    }
    ret = listen(s,2);
    if (ret < 0) {
        ERROR((msg,"tcp_listener.listen failed: errno=%d", ERRNO));
        panic("tcp");
    }

    /*
       * Loop forever accepting connections.
       */
    for (;;) {
        mutex_lock(&conn_lock);
        while (conn_num >= param.tcp_conn_max) {
            condition_wait(&conn_cond,&conn_lock);
        }

        mutex_unlock(&conn_lock);
        snamelen = sizeof(struct sockaddr_in);
        ret = accept(s,(struct sockaddr *)&sname,&snamelen);
        if (ret < 0) {
            ERROR((msg, "tcp_listener.accept failed: errno=%d", ERRNO));
            continue;
        }
        INCSTAT(tcp_accept);

#ifdef WIN32
    {
	int xxx = 1;
	if (setsockopt(ret, IPPROTO_TCP, TCP_NODELAY, &xxx, sizeof(xxx)) < 0) {
	    ERROR((msg,"tcp_conn_handler.setsockopt(NODELAY): %d",ERRNO));
	}
    }
#endif
        if (TCP_DBG_SOCKET) {
            int	optval;

            optval = 1;
            setsockopt(ret,SOL_SOCKET,SO_DEBUG,(char *)&optval,sizeof(int));
        }

        mutex_lock(&conn_lock);
        if (sys_queue_empty(&conn_free)) {
            /*
               * Initialize a new connection record.
               */
            cp = tcp_init_conn();
        } else {
            cp = (tcp_conn_ptr_t)sys_queue_first(&conn_free);
            sys_queue_remove(&conn_free,cp,tcp_conn_ptr_t,connq);
        }
        mutex_lock(&cp->lock);
        sys_queue_enter_first(&conn_lru,cp,tcp_conn_ptr_t,connq);
        conn_num++;
        cp->sock = ret;
        cp->peer = (netaddr_t)(sname.sin_addr.s_addr);
        cp->to = 0;
        cp->stay_alive = TCP_STAY_ALIVE;
        cp->state = TCP_CONNECTED;
        cp->count = 0;
        condition_signal(&cp->cond);
        mutex_unlock(&cp->lock);
        if ((conn_num - conn_closing) > param.tcp_conn_steady) {
            tcp_close_conn();
        }
        mutex_unlock(&conn_lock);
        LOGCHECK;
    }

}

/*
   * tcp_scavenger --
   *	Called by timer module to scavenge idle tcp connections.
   *
   * Parameters:
   *	timer - the timer that went off.
   *
   * Results:
   *	none
   *
   */
PRIVATE void tcp_scavenger(nmtimer_t timer)
{
    tcp_conn_ptr_t cp, first;
    mutex_lock(&conn_lock);
    /*
       * Look for an old connection to recycle.
       */
    first = (tcp_conn_ptr_t)sys_queue_first(&conn_lru);
    cp = (tcp_conn_ptr_t)sys_queue_last(&conn_lru);

    while (cp != first)
      {
        mutex_lock(&cp->lock);
        if ((cp->count == 0)
            && (cp->state == TCP_CONNECTED)
            && (cp->stay_alive-- <= 0))
          {
            conn_closing++;
            cp->state = TCP_CLOSING;
            (void)tcp_xmit_control(cp, TCP_CTL_CLOSEREQ, 0, 0);
#ifndef WIN32
            if (param.syslog)
                syslog(LOG_INFO, "Idle tcp connection closed");
#endif WIN32
          }
        /*
           * We should be able to unlock this connection and still
           * be able to use it connection queue. That should be
           * protected by the more global conn_lock.
           */
        mutex_unlock(&cp->lock);
        cp = (tcp_conn_ptr_t)sys_queue_prev(&cp->connq);
      }

    mutex_unlock(&conn_lock);
    timer->interval.tv_sec = TCP_SCAVENGER_INTERVAL;
    timer->interval.tv_usec = 0;
    timer->action = tcp_scavenger;
    timer->info = NULL;
    timer_restart(timer);  /* just do it again, and again, and ... */
}

/*
   * tcp_init --
   *
   * Initialises the TCP transport protocol.
   *
   * Parameters:
   *
   * Results:
   *
   *	FALSE : we failed to initialise the TCP transport protocol.
   *	TRUE  : we were successful.
   *
   * Side effects:
   *
   *	Initialises the TCP protocol entry point in the switch array.
   *	Allocates the listener port and creates a thread to listen to the network.
   *
   */
EXPORT boolean_t tcp_init()
{
    int i;
    nmtimer_t	timer;
    tcp_conn_ptr_t cp;
    cthread_t thread;
#ifndef NeXT_PDO
    struct sigvec svec, osvec;
#endif

    /*
       * Initialize the memory management facilities.
       */
    mem_initobj(&MEM_TCPTRANS,"TCP transaction record",sizeof(tcp_trans_t),
                FALSE,120,50);

    /*
       * Initialize the set of connection records and the lists.
       */
    for (i = 0; i < param.tcp_conn_max; i++) {
        conn_vec[i].state = TCP_INVALID;
        conn_vec[i].incarn = 0;
    }
    mutex_init(&conn_lock);
    mutex_lock(&conn_lock);
    condition_init(&conn_cond);
    sys_queue_init(&conn_lru);
    sys_queue_init(&conn_free);
    conn_num = 0;
    conn_closing = 0;
    trid_counter = 10;

    /*
       * Create a first connection record (just a test).
       */
    cp = tcp_init_conn();
    sys_queue_enter(&conn_free,cp,tcp_conn_ptr_t,connq);

    /*
       * Set up the entry in the transport switch.
       */
    transport_switch[TR_TCP_ENTRY].sendrequest = tcp_sendrequest;
    transport_switch[TR_TCP_ENTRY].sendreply = tcp_sendreply;

    /*
       * Start the listener.
       */
    thread = cthread_fork((cthread_fn_t)tcp_listener, 0);
    cthread_set_name(thread,"tcp_listener");
    cthread_detach(thread);

    /*
       * Start the scavenger based on a timer.
       */
    if ((timer = timer_alloc()) == (nmtimer_t)0)
        panic("tcp_init.timer_alloc");

    timer->interval.tv_sec = TCP_SCAVENGER_INTERVAL;
    timer->interval.tv_usec = 0;
    timer->action = tcp_scavenger;
    timer->info = NULL;
    timer_start(timer);

#ifndef NeXT_PDO
    /*
       * Under certain conditions, the kernel may raise a
       * SIGPIPE when a send() fails. We want none of that...
       */
    svec.sv_handler = (void (*)()) SIG_IGN;
    svec.sv_mask = 0;
    svec.sv_flags = 0;
    sigvec(SIGPIPE,&svec,&osvec);
#else
#ifndef WIN32
#warning probably should install signal handlers on PDO
#endif
#endif !NeXT_PDO

    /*
       * Get the show on the road...
       */
    mutex_unlock(&conn_lock);
    RETURN(TRUE);

}


