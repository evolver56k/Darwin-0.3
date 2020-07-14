/*
 * This software was developed by the Software and Component Technologies
 * group of Trimble Navigation, Ltd.
 *
 * Copyright (c) 1997 Trimble Navigation Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Trimble Navigation, Ltd.
 * 4. The name of Trimble Navigation Ltd. may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TRIMBLE NAVIGATION LTD. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL TRIMBLE NAVIGATION LTD. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * refclock_palisade - clock driver for the Trimble Palisade GPS
 * timing receiver
 *
 * For detailed information on this program, please refer to the html 
 * Refclock 29 page accompanying the NTP distribution.
 *
 * for questions / bugs / comments, contact:
 * sven_dietrich@trimble.com
 *
 * Sven-Thorsten Dietrich
 * 645 North Mary Avenue
 * Post Office Box 3642
 * Sunnyvale, CA 94088-3642
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && defined(CLOCK_PALISADE)

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef SYS_HPUX
#include <sys/modem.h>
#define TIOCMSET MCSETA
#define TIOCMGET MCGETA
#define TIOCM_RTS MRTS
#endif

#ifdef _M_SYSV
#define _SVID3			/* hack for SCO OpenServer 5.0.x */
#include <sys/termio.h>
#undef _SVID3
#endif

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_control.h"
#include "ntp_refclock.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"




/*
 * GPS Definitions
 */
#define	DESCRIPTION	"Trimble Palisade GPS" /* Long name */
#define	PRECISION	(-17)	/* precision assumed (about 10 us) */
#define	REFID		"GPS\0"	/* reference ID */
#define TRMB_MINPOLL    6	/* 64 seconds */
#define TRMB_MAXPOLL	9	/* 512 seconds */

/* 
 * I/O Definitions
 */
#define	DEVICE		"/dev/palisade%d" 	/* device name and unit */
#define	SPEED232	B9600		  	/* uart speed (9600 baud) */

#define MIN_HW_DEL	(1 << 17) /* min. time to hold HW event signal */
#define MAX_HW_DEL	(1 << 20) /* hardcoded upper bound valid code delay */
#define POLL_AVG	10	  /* number of samples to average code delay */

/*
 * TSIP Report Definitions
 */
#define LENCODE		73	/* Length of TSIP 8F-0B Packet & header */

#define DLE 0x10
#define ETX 0x03

/* parse states */
#define TSIP_PARSED_EMPTY       0	
#define TSIP_PARSED_FULL        1
#define TSIP_PARSED_DLE_1       2
#define TSIP_PARSED_DATA        3
#define TSIP_PARSED_DLE_2       4

#define mb(_X_) (up->rpt_buf[(_X_)]) /* shortcut for buffer access */

/* Table to get from month to day of the year */
static int days_of_year [12] = {
	0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334
};

/*
 * Imported from ntp_timer module
 */
extern u_long current_time;	/* current time (s) */

/*
 * Imported from ntpd module
 */
#ifdef DEBUG
extern int debug;		/* global debug flag */
#endif

/*
 * mapping union for ints & doubles for decoding the 8F 0B output
 * packets from the Palisade receiver
 */

typedef union {
	u_char  bd[8];
	int     iv;
	double  dv;
} upacket;

/*
 * Palisade unit control structure.
 */
struct palisade_unit {
	short		unit;		/* NTP refclock unit number */
	int 		polled;		/* flag to detect noreplies */
	unsigned int 	eventcnt;	/* Palisade event counter */
	l_fp 		codedelay;	/* time to hold HW signal for event */
	char		rpt_status;	/* TSIP Parser State */
	short 		rpt_code;	/* TSIP packet ID */
	short 		rpt_cnt;	/* TSIP packet length so far */
	unsigned char	rpt_buf[LENCODE+1]; /* assembly buffer */
};

/*
 * Function prototypes
 */
static	int	palisade_start		P((int, struct peer *));
static	void	palisade_shutdown	P((int, struct peer *));
static	void	palisade_receive	P((struct recvbuf *));
static	void	palisade_poll		P((int, struct peer *));

int 	palisade_configure	P((int, struct peer *));
void 	palisade_io		P((int, int, unsigned char *, l_fp *, 
				 struct palisade_unit *));
long	HW_poll			P((int, struct refclockproc *));
void 	clear_polled		P((struct palisade_unit *));
double 	getdbl 			P((u_char *));
int    	getint 			P((u_char *));

/*
 * Transfer vector
 */
struct refclock refclock_palisade = {
	palisade_start,		/* start up driver */
	palisade_shutdown,	/* shut down driver */
	palisade_poll,		/* transmit poll message */
	noentry,		/* not used  */
	noentry,		/* initialize driver (not used) */
	noentry,		/* not used */
	NOFLAGS			/* not used */
};

/*
 * palisade_start - open the devices and initialize data for processing
 */
static int
palisade_start (
	int unit,
	struct peer *peer
	)
{
	register struct palisade_unit *up;
	struct refclockproc *pp;
	int fd;
	char gpsdev[20];
	struct termios tio;
	
	(void) sprintf(gpsdev, DEVICE, unit);

	/*
	 * Open serial port. 
	 */
	fd = open(gpsdev, O_RDWR 
#ifdef O_NONBLOCK
		  | O_NONBLOCK
#endif
		  );

	msyslog(LOG_NOTICE, "Palisade #%d: fd: %d dev: %s", unit, fd,
		gpsdev);

	if (fd == -1) {
		msyslog(LOG_ERR,"Palisade #%d: parse_start: open %s failed: %m",
			unit, gpsdev);
		return 0;
	}

	tio.c_cflag = (CS8|CLOCAL|CREAD|PARENB|PARODD);
	tio.c_iflag = (IGNBRK|INPCK);
	tio.c_oflag = (0);
	tio.c_lflag = (0);

	if (cfsetispeed(&tio, SPEED232) == -1) {
		msyslog(LOG_ERR,"Palisade #%d: cfsetispeed(fd, &tio): %m",unit);
		return 0;
	}
	if (cfsetospeed(&tio, SPEED232) == -1) {
		msyslog(LOG_ERR,"Palisade #%d: cfsetospeed(fd, &tio): %m",unit);
		return 0;
	}
	if (tcsetattr(fd, TCSANOW, &tio) == -1) {
		msyslog(LOG_ERR, "Palisade #%d: tcsetattr(fd, &tio): %m",unit);
		return 0;
	}

	/*
	 * Allocate and initialize unit structure
	 */
	if (!(up = (struct palisade_unit *)
	      emalloc(sizeof(struct palisade_unit)))) {
		(void) close(fd);
		return (0);
	}
	memset((char *)up, 0, sizeof(struct palisade_unit));

	pp = peer->procptr;
	pp->io.clock_recv = palisade_receive;
	pp->io.srcclock = (caddr_t)peer;
	pp->io.datalen = 0;
	pp->io.fd = fd;

	if (!io_addclock(&pp->io)) {
		(void) close(fd);
		free(up);
		return (0);
	}

	/*
	 * Initialize miscellaneous variables
	 */
	pp->unitptr = (caddr_t)up;
	pp->clockdesc = DESCRIPTION;

	peer->precision = PRECISION;
	peer->sstclktype = CTL_SST_TS_UHF;
	peer->minpoll = TRMB_MINPOLL;
	peer->maxpoll = TRMB_MAXPOLL;
	memcpy((char *)&pp->refid, REFID, 4);

	up->unit = unit;
	up->eventcnt = 0;
	return palisade_configure(unit, peer); /* Find code delay */
}


/*
 * palisade_shutdown - shut down the clock
 */
static void
palisade_shutdown (
	int unit,
	struct peer *peer
	)
{
	register struct palisade_unit *up;
	struct refclockproc *pp;

	pp = peer->procptr;
	up = (struct palisade_unit *)pp->unitptr;
	io_closeclock(&pp->io);
	free(up);
}


/*
 * palisade__receive - receive data from the serial interface
 */

static void
palisade_receive (
	struct recvbuf *rbufp
	)
{
	register struct palisade_unit *up;
	struct refclockproc *pp;
	struct peer *peer;

	long   secint;
	double secs;
	double secfrac;
	int event, utc;
#ifdef DEBUG
	int sv;
#endif 
	/*
	 * Initialize pointers and read the timecode and timestamp.
	 */
	peer = (struct peer *)rbufp->recv_srcclock;
	pp = peer->procptr;
	up = (struct palisade_unit *)pp->unitptr;

	if (pp->sloppyclockflag & CLK_FLAG2) 
	    palisade_io(1, rbufp->recv_length, (unsigned char *)
			&rbufp->recv_space, &pp->lastrec, up);
	else 
	    palisade_io(0, rbufp->recv_length, (unsigned char *)
			&rbufp->recv_space, &pp->lastrec, up);

	/* check whether packet is complete */
	if (up->rpt_status != TSIP_PARSED_FULL) return; 

	/* clear for next packet */
	up->rpt_status = TSIP_PARSED_EMPTY;

	if (up->polled <= 0) 
	    return; /* no poll pending, already received or timeout */

	if (up->rpt_cnt != LENCODE) {
		clear_polled(up);
		refclock_report(peer, CEVNT_BADREPLY);
#ifdef DEBUG
		if (debug)
		    printf("palisade_receive: unit %d: packet length %2d\n",
			   up->unit, up->rpt_cnt);
#endif
		return;
	} 

	pp->lencode = 0; /* clear time code */

	/*
	 * Check the time packet, decode its contents. 
	 * If the timecode has invalid length or is not in
	 * proper format, we declare bad format and exit.
	 */
	if ((up->rpt_code != 0x8f) || (mb(0) != (char) 0x0b)) {
		clear_polled(up);
		refclock_report(peer, CEVNT_BADREPLY);
#ifdef DEBUG
		if (debug)
		    printf("palisade_receive: unit %d: invalid packet id:%2x%2x\n", 
			   up->unit, up->rpt_code, mb(0));
#endif
		return;
	} 

	/*
	 * Events are numbered sequentially 1 - 65535 then they roll over. 
	 * PPS packets and event packets look the same, but PPS packets are
	 * always #0.  Check to make sure they stay sequential, but  
	 * accept any packet if not using events.
	 */
	event = getint(&mb(1));
	if (!(pp->sloppyclockflag & CLK_FLAG2) && (event != up->eventcnt)) {
		if (up->eventcnt > 32767) { /* rolled over */	
#ifdef DEBUG
			if (debug)
			    printf("palisade_receive: unit %d: event rollover to %d\n", 
				   up->unit, event);
#endif
		} else if (up->eventcnt == 0) { /* initializing */
#ifdef DEBUG
			if (debug)
			    printf("palisade_receive: unit %d: initial event count %d\n", 
				   up->unit, event); 
#endif
		} else {
#ifdef DEBUG
			if (debug)
			    printf("palisade_receive: unit %d: event #%d expected #%d\n", 
				   up->unit, event, up->eventcnt);
#endif
			refclock_report(peer, CEVNT_TIMEOUT);
			up->eventcnt = event + 1; /* re-synchronize */
			up->polled = 0; /* mayhem out there-close the
					   door */
			return;
		}
		up->eventcnt = event; /* re-synchronize */
	}

	/* We have receive the packet we are looking for */
	up->eventcnt ++; /* Increment for next event */
	up->polled = 0;  /* Poll reply received */

	/* 
	 * Process Data
	 */
	utc = getint(&mb(16));
	if (utc < 12) /* Check UTC offset */ { 
		refclock_report(peer, CEVNT_BADTIME);
#ifdef DEBUG
		if (debug)
		    printf("palisade_receive: unit %d: UTC offset: %d invalid\n",
			   up->unit, utc);
#endif
		return;
	}

	pp->day = mb(11) + days_of_year[mb(12)-1];

	secs = getdbl(&mb(3));
	secint = secs;
	secfrac = secs - secint; /* 0.0 <= secfrac < 1.0 */

	/* Check month for array bounds */
	if ((mb(12) < 1) || (mb(12) > 12)) { 
		refclock_report(peer, CEVNT_BADTIME);
#ifdef DEBUG
		if (debug)
		    printf("palisade_receive: unit %d: invalid month %d\n",
			   up->unit, mb(12));
#endif
		clear_polled(up);
		return;
	}

	pp->day = mb(11) + days_of_year[mb(12)-1];
	pp->year = getint(&mb(13)); 

	if ( !(pp->year % 4)
	     && ((pp->year % 100)
		 || (!(pp->year % 100) && !(pp->year % 400))) && (mb(12) > 2))
	    pp->day ++; /* leap year and March or later */
		
	secint %= 86400;    /* Only care about today */

	pp->hour = secint / 3600;
	secint %= 3600;
	pp->minute = secint / 60;
	pp->second = secint % 60;
	pp->usec = secfrac * 1000000; 

#ifdef DEBUG
	if (debug) {
		printf(
			"palisade_receive: %4d %03d %02d:%02d:%02d.%06ld event #%d\n",
			pp->year, pp->day, pp->hour, pp->minute, pp->second,
			pp->usec, event);
		printf("palisade_receive: unit %d: tracking: ", up->unit);
		for (sv = 66; sv <= 73; sv++)
		    printf("sv%d ", mb(sv));
		printf("\n");
	}
#endif

	/*
	 * Process the sample
	 *
	 * Generate timecode: YYYY DoY HH:MM:SS.microsec 
	 * report and process 
	 */

	(void) sprintf(pp->a_lastcode,"%4d %03d %02d:%02d:%02d.%06ld",
		       pp->year,pp->day,pp->hour,pp->minute, pp->second,pp->usec); 
	pp->lencode = 24;
	
	if (!refclock_process(pp)) {
		refclock_report(peer, CEVNT_BADTIME);
		clear_polled(up);
		return;
	}

	record_clock_stats(&peer->srcadr, pp->a_lastcode); 
#ifdef DEBUG
	if (debug)
	    printf("palisade_receive: ->%s, %f\n",
		   prettydate(&pp->lastrec), pp->offset);
#endif
	refclock_receive(peer);
}


/*
 * palisade_poll - called by the transmit procedure
 *
 */
static void
palisade_poll (
	int unit,
	struct peer *peer
	)
{
	register struct palisade_unit *up;
	struct refclockproc *pp;
	
	pp = peer->procptr;
	up = (struct palisade_unit *)pp->unitptr;

	pp->polls++;
	if (up->polled) /* last reply never arrived or error */
	    refclock_report(peer, CEVNT_TIMEOUT);
	up->polled = 2; /* synchronous packet + 1 event worst case */
	
#ifdef DEBUG
	if (debug)
	    printf("palisade_poll: unit #%d: HW poll: %d\n", unit,
		   !(pp->sloppyclockflag & CLK_FLAG2));
#endif 

	if (pp->sloppyclockflag & CLK_FLAG2) 
	    return;  /* using synchronous packet input */

	if (HW_poll(MIN_HW_DEL, pp) < 0) 
	    refclock_report(peer, CEVNT_FAULT); 
	if (!(pp->sloppyclockflag & CLK_FLAG3))
	    /* Add delay estimate */
	    L_ADD (&pp->lastrec, &up->codedelay); 	
}


int 
palisade_configure (
	int unit,
	struct peer *peer
	)
{
	register struct palisade_unit *up;
	struct refclockproc *pp;
	int rp, ec;			/* misc. reusable vars */
	long rc, delay;

	pp = peer->procptr;
	up = (struct palisade_unit *)pp->unitptr;
	
	up->codedelay.l_i = 0;
	up->codedelay.l_f = 0;
	
	delay = 0;
	rc = POLL_AVG - 1;
	for (ec = 0; ((rc >= 0) && (ec < POLL_AVG << 2)); ec ++) {
		rp = HW_poll(0, pp);
		if (rp < 0) {
#ifdef DEBUG
			if (debug)
			    printf("palisade_configure: unit %d: no h/w poll available\n",
				   unit);
#endif 
			msyslog(LOG_ERR, "Palisade #%d: palisade_configure: event poll unavailable\n", unit);
			/* fallback to synchronous mode */
			pp->sloppyclockflag |= CLK_FLAG2; 
			up->codedelay.l_f = 0;
			return 1;
		}
		
		if (rp < MAX_HW_DEL) {
			delay += rp / POLL_AVG;
			rc--;
		}
#ifdef DEBUG
		else if (debug) 
		    printf("Delay %d exceeds max %d. Sample discarded\n",
			   rp, MAX_HW_DEL);
#endif 
	}

	if (ec < POLL_AVG << 2) {
		up->codedelay.l_f = delay >> 1;
		TSFTOTVU(up->codedelay.l_f, rc);
		msyslog(LOG_NOTICE, "Palisade #%d: code delay: %d us %d polls",
			unit, rc, POLL_AVG); 
	} else {
		up->codedelay.l_f = 0;
		msyslog(LOG_NOTICE,"Palisade #%d: poll unable to check delays",
			unit); 
	}
	up->polled = 0;
	return 1;
} 


void
palisade_io (
	int noevents,			/* if noevents is set, no HW poll */
	int buflen,			/* bytes in buffer to process */
	unsigned char *bufp,		/* receive buffer */
	l_fp* t_in,			/* receive time stamp */
	struct palisade_unit *up	/* pointer to unit data structure   */
	)
{
	unsigned char *dp, *dpend;
	dp = bufp;	
	dpend = dp + buflen;

	/* Build time packet */
	while ((up->rpt_status != TSIP_PARSED_FULL) && (dp < dpend)) {

		/* (unsigned char)(inbyte & 0x00FF); */

		switch (up->rpt_status) {

		    case TSIP_PARSED_DLE_1:
			switch (*dp)
			{
			    case 0:
			    case DLE:
			    case ETX:
				up->rpt_status = TSIP_PARSED_EMPTY;
				break;

			    default:
				up->rpt_status = TSIP_PARSED_DATA;
				up->rpt_code = *dp;
				break;
			}
			break;

		    case TSIP_PARSED_DATA:
			if (*dp == DLE)
			    up->rpt_status = TSIP_PARSED_DLE_2;
			else 
			    up->rpt_buf[up->rpt_cnt++] = *dp;
			break;

		    case TSIP_PARSED_DLE_2:
			if (*dp == DLE) {
				up->rpt_status = TSIP_PARSED_DATA;
				up->rpt_buf[up->rpt_cnt++] = *dp;
			}       
			else if (*dp == ETX) {
 				/* check whether we want it */
				if (mb(1) || mb(2) || noevents)
				    up->rpt_status = TSIP_PARSED_FULL;
				else 
				    up->rpt_status = TSIP_PARSED_EMPTY;
			}
			else 	{
                        	/* error: start new report packet */
				up->rpt_status = TSIP_PARSED_DLE_1;
				up->rpt_code = *dp;
			}
			break;

		    case TSIP_PARSED_FULL:
		    case TSIP_PARSED_EMPTY:
		    default:
			up->rpt_status = (*dp==DLE)
			    ? TSIP_PARSED_DLE_1
			    : TSIP_PARSED_EMPTY;
			if (noevents && (up->polled > 0))
				/* stamp it */
			    get_systime(t_in);
			break;
		}

		if (up->rpt_cnt > LENCODE) 
		    /* error: start new report packet */
		    up->rpt_status = TSIP_PARSED_EMPTY;
           
		if (up->rpt_status == TSIP_PARSED_EMPTY ||
		    up->rpt_status == TSIP_PARSED_DLE_1) 
		    up->rpt_cnt = 0;
		dp++;
	} /* while chars in buffer */
}


/*
 * Trigger the Palisade's event input, which is driven off the RTS
 *
 * The pulse needs to be held high long enough for the Palisade
 * to recognize it.  Furthermore, we need a system time stamp
 * to match the GPS time stamp.
 *
 *  Taking the time twice gives us:
 *    1. A delay to hold the pulse high
 *    2. An average to approximate time the pulse was sent
 *    3. A way to detect OS preemption
 */
long
HW_poll (
	int delay,
	struct refclockproc * pp 	/* pointer to unit structure */
	)
{	
	long sl;		/* state before & after RTS set */
	l_fp postpoll;		
	struct palisade_unit *up;
	up = (struct palisade_unit *)pp->unitptr;

	if (ioctl(pp->io.fd, TIOCMGET, &sl) == -1) {
		msyslog(LOG_ERR, "Palisade #%d: HW_poll: ioctl(fd,GET): %m", 
			up->unit);
		return -1;
	}
  
	sl |= TIOCM_RTS;        /* turn on RTS  */

	get_systime(&pp->lastrec);
	if (ioctl(pp->io.fd, TIOCMSET, &sl) == -1) { 
		msyslog(LOG_ERR,
			"Palisade #%d: HW_poll: ioctl(fd, SET, RTS_on): %m", 
			up->unit);
		return -1;
	}

	/* Loop to hold pulse high for several us */
	do {
		get_systime(&postpoll);
		L_SUB (&postpoll, &pp->lastrec);
	} while ((postpoll.l_i == 0) && (postpoll.l_f < delay));

	sl &= ~TIOCM_RTS;        /* turn off RTS  */
	if (ioctl(pp->io.fd, TIOCMSET, &sl) == -1) {
		msyslog(LOG_ERR,"Palisade #%d: HW_poll: ioctl(fd, SET, RTS_off): %m", 
			up->unit);
		return -1;
	}

#ifdef DEBUG
	if (debug) 
	    TSFTOTVU(postpoll.l_f, sl);
	printf("Palisade #%d: poll: completion time %d.%06lu s\n",
	       up->unit, postpoll.l_i, sl);
#endif

	if (((postpoll.l_f > (up->codedelay.l_f + delay))
	     ||(postpoll.l_i != 0))
	    && (delay > 0)) {
		TSFTOTVU(postpoll.l_f, sl);
		msyslog(LOG_NOTICE, 
			"Palisade #%d: poll: max. delay exceeded: %d.%06lds\n",
			up->unit, postpoll.l_i, sl);
		return 0;
	}
	return postpoll.l_f;
}


/*
 * Prepare TSIP report buffer for next packet 
 * and clear parse status
 */
void
clear_polled (
	struct palisade_unit *up
	)
{
	if (up->polled > 0) {
		up->polled--;
		if (up->polled == 0)  /* special state 0 = received ok */
		    up->polled = -1;
	}
}

/*
 * this 'casts' a character array into a double
 */
double
getdbl (
	u_char *bp
	)
{
	upacket uval;
#ifdef XNTP_BIG_ENDIAN 
	uval.bd[0] = *bp++;
	uval.bd[1] = *bp++;
	uval.bd[2] = *bp++;
	uval.bd[3] = *bp++;
	uval.bd[4] = *bp++;
	uval.bd[5] = *bp++;
	uval.bd[6] = *bp++;
	uval.bd[7] = *bp;
#else /* ! XNTP_BIG_ENDIAN */
	uval.bd[7] = *bp++;
	uval.bd[6] = *bp++;
	uval.bd[5] = *bp++;
	uval.bd[4] = *bp++;
	uval.bd[3] = *bp++;
	uval.bd[2] = *bp++;
	uval.bd[1] = *bp++;
	uval.bd[0] = *bp;
#endif  /* ! XNTP_BIG_ENDIAN */ 
	return uval.dv;
}

/*
 * cast a 16 bit character array into an int
 */
int
getint (
	u_char *bp
	)
{
	if (*bp & 0x80)
	    return ((~0xffff) | (*bp<<8) | *(bp+1));
	else
	    return ((*bp<<8) | *(bp+1));
}

/* 
 * Revisions
 *
 * November 01, 1997:  First Draft
 * November 15, 1997:  Update TSIP parser; simplify error handling
 * November 17, 1997:  Addition of FLAG3 to eliminate code delay calc.
 * November 24, 1997:  Release v. 1.01
 */
#else
int refclock_palisade_bs;
#endif /* REFCLOCK */
