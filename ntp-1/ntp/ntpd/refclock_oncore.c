/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * refclock_oncore.c
 *
 * Driver for the Motorola UT OnCore GPS receiver.
 *
 * Very green.  Contact phk@freebsd.org before even trying to get
 * this to work.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && defined(CLOCK_ONCORE)

#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_unixtime.h"
#include "ntp_refclock.h"
#include "ntp_stdlib.h"

struct ppsclockev {
	struct timeval tv;
	u_int serial;
};


struct instance;

static	int	oncore_start	(int, struct peer *);
static	void	oncore_consume	(struct instance *);
static	void	oncore_poll	(int, struct peer *);
static	void	oncore_receive	(struct recvbuf *);
static	void	oncore_shutdown	(int, struct peer *);
static	void	oncore_sendmsg	(int fd, u_char *, int len);

static	void	oncore_msg_any	(struct instance *, unsigned char *buf, int length);
static	void	oncore_msg_Cf	(struct instance *, unsigned char *buf, int length);
static	void	oncore_msg_Cj	(struct instance *, unsigned char *buf, int length);
static	void	oncore_msg_Fa	(struct instance *, unsigned char *buf, int length);
static	void	oncore_msg_En	(struct instance *, unsigned char *buf, int length);
static	void	oncore_msg_Ea	(struct instance *, unsigned char *buf, int length);
static	void	oncore_msg_Aa	(struct instance *, unsigned char *buf, int length);
static	void	oncore_msg_Aw	(struct instance *, unsigned char *buf, int length);

struct	refclock refclock_oncore = {
	oncore_start,		/* start up driver */
	oncore_shutdown,	/* shut down driver */
	oncore_poll,		/* transmit poll message */
	noentry,		/* not used */
	noentry,		/* not used */
	noentry,		/* not used */
	NOFLAGS			/* not used */
};

struct instance {
	int	ttyfd;		/* TTY filedescriptor */
	int	ppsfd;		/* PPS filedescriptor */
	enum	{
		ONCORE_NO_IDEA,
		ONCORE_RESET_SENT,
		ONCORE_TEST_SENT,
		ONCORE_ID_SENT,
		ONCORE_UTC,
		ONCORE_RUN
		} state;	/* Receive state */
	struct	refclockproc *pp;
	struct	peer *peer;
	int	pollcnt;
	int	polled;
	u_char	Ea[128];
	u_char	En[128];
	u_char	GPS_Aa[128];
	u_char	Aw[128];
};

/*
 * Understanding the next bit here is not easy unless you have a manual
 * for the the UT Oncore.
 */

static struct {
	char    flag[3];
	int     len;
	void    (*handler)(struct instance *, unsigned char *, int);
} oncore_messages[] = {
	{ "Aa",  10,	oncore_msg_Aa },
	{ "Ab",  10,	0 },
	{ "Ac",  11,	0 },
	{ "Ad",  11,	0 },
	{ "Ae",  11,	0 },
	{ "Af",  15,	0 },
	{ "As",  20,	0 },
	{ "At",   8,	0 },
	{ "Aw",   8,	oncore_msg_Aw },
	{ "Az",  11,	0 },
	{ "Bj",   8,	0 },
	{ "Cb",  33,	0 },
	{ "Cf",   7,	oncore_msg_Cf },
	{ "Ch",   9,	0 },
	{ "Cj", 294,	oncore_msg_Cj },
	{ "Ea",  76,	oncore_msg_Ea },
	{ "En",  69,	oncore_msg_En },
	{ "Fa",   9,	oncore_msg_Fa },
	{ "Sz",   8,	0 },
	{ 0,      7,	0	    }
};

unsigned char oncore_cmd_Aa[] = { 'A', 'a', 0xff, 0xff, 0xff};
unsigned char oncore_cmd_As[] = { 'A', 's', 0x0b, 0xe3, 0xdd, 0x1a, 0x02, 0x6e, 0xb6, 0x10, 0x00, 0x00, 0x20, 0x48, 0 };
unsigned char oncore_cmd_At[] = { 'A', 't', 1 };
unsigned char oncore_cmd_Az0[] = { 'A', 'z', 0, 0, 0, 0 };
unsigned char oncore_cmd_Az20u[] = { 'A', 'z', 0, 0, 0x4e, 0x20 };
unsigned char oncore_cmd_Be[] = { 'B', 'e', 1 };
unsigned char oncore_cmd_Bj[] = { 'B', 'j', 0 };
unsigned char oncore_cmd_Cf[] = { 'C', 'f' };
unsigned char oncore_cmd_Cj[] = { 'C', 'j' };
unsigned char oncore_cmd_Ea[] = { 'E', 'a', 1 };
unsigned char oncore_cmd_En[] = { 'E', 'n', 1, 1, 0,10, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char oncore_cmd_Fa[] = { 'F', 'a' };
unsigned char oncore_cmd_Aw0[] = { 'A', 'w', 0 };
unsigned char oncore_cmd_Aw1[] = { 'A', 'w', 1 };

static u_char	rcvbuf[300];
static int	rcvptr;

/*
 * oncore_start - initialize data for processing
 */
static int
oncore_start(
	int unit,
	struct peer *peer
	)
{
	register struct instance *up;
	struct refclockproc *pp;
	int flags, fd1, fd2;

	flags = 0;

#if 0
{
#include "ntp_calendar.h"
	u_long l,l1,i;
	l = 852497761;
	for(i=0;i<20;i++) {
	l1 = calyearstart(l);
	printf("%d %s", l, ctime(&l));
	printf("                                            %d %s", l1, ctime(&l1));
	l -= 365*24*3600;
	}
	printf("MAR1988 %d\n", MAR1988);
	exit(0);
}
#endif
	fd1 = refclock_open("/dev/cuaa0", B9600, LDISC_RAW);
	if (!fd1) {
perror("barf0");
exit(1);
		return (0);
	}

	fd2 = open("/dev/pppps0", O_RDONLY);
	if (fd2 < 0) {
perror("barf0.5");
exit(1);
		return (0);
	}

	if (!(up = (struct instance *)emalloc(sizeof *up))) {
perror("barf1");
exit(1);
		close(fd1);
		return (0);
	}
	memset((char *)up, 0, sizeof *up);
	pp = peer->procptr;
	pp->nstages = MAXSTAGE;
        pp->nskeep = MAXSTAGE * 3 / 5;

	pp->unitptr = (caddr_t)up;
	up->ttyfd = fd1;
	up->ppsfd = fd2;
	up->state = ONCORE_NO_IDEA;
	up->pp = pp;
	up->peer = peer;
  
	/*
	 * Initialize miscellaneous variables
	 */
	peer->precision = -19;
	peer->minpoll = 4;
	pp->clockdesc = "Motorola UT OnCore GPS Receiver";
	memcpy((char *)&pp->refid, "GPS\0", 4);
#if 0
	pp->nstages = MAXSTAGE;
	pp->nskeep = MAXSTAGE * 3 / 5;
#endif

	pp->io.clock_recv = oncore_receive;
	pp->io.srcclock = (caddr_t)peer;
	pp->io.datalen = 0;
	pp->io.fd = fd1;
	if (!io_addclock(&pp->io)) {
perror("barf2");
exit(1);
		(void) close(fd1);
		free(up);
		return (0);
	}
	pp->unitptr = (caddr_t)up;
	if (up->state == ONCORE_NO_IDEA) {
		oncore_sendmsg(fd1, oncore_cmd_Cf, sizeof oncore_cmd_Cf);
		up->state = ONCORE_RESET_SENT;
	}

	up->pollcnt = 2;

	return (1);
}


/*
 * oncore_shutdown - shut down the clock
 */
static void
oncore_shutdown(
	int unit,
	struct peer *peer
	)
{
	register struct instance *up;
	struct refclockproc *pp;

	pp = peer->procptr;
	up = (struct instance *)pp->unitptr;
	free(up);
}

/*
 * oncore_poll - called by the transmit procedure
 */
static void
oncore_poll(
	int unit,
	struct peer *peer
	)
{
	struct instance *instance;

	instance = (struct instance *)peer->procptr->unitptr;
	if (!instance->pollcnt)
		refclock_report(peer, CEVNT_TIMEOUT);
	else
		instance->pollcnt--;
	peer->procptr->polls++;
	instance->polled = 1;
}

static void
oncore_receive(
	struct recvbuf *rbufp
	)
{
	u_char *p;
	struct peer *peer;
	struct instance *up;

	peer = (struct peer *)rbufp->recv_srcclock;
	up = (struct instance *) peer->procptr->unitptr;
	p = (u_char *)&rbufp->recv_space;
#if 0
	{
	int i;
	printf(">>>");
	for(i=0;i<rbufp->recv_length;i++) {
		printf("%02x", p[i]);
	}
	printf("\n");
	}
#endif

	memcpy(rcvbuf+rcvptr, p, rbufp->recv_length);
	rcvptr += rbufp->recv_length;
	oncore_consume(up);
}


/* Deal with any complete messages */
static void
oncore_consume(
	struct instance *up
	)
{
	int i, l, j, m;

	while (rcvptr >= 7) {
		if (rcvbuf[0] != '@' || rcvbuf[1] != '@') {
			/* We're not in sync, lets try to get there */
			for (i=1; i < rcvptr; i++) {
				if (rcvbuf[i] == '@')
					break;
			}
printf(">>> skipping %d chars\n", i);
			if (i != rcvptr)
				memcpy(rcvbuf, rcvbuf+i, rcvptr - i);
			rcvptr -= i;
			continue;
		}
		/* Ok, we probably have a header now */
		for(m=0; *oncore_messages[m].flag; m++) {
			if (!strncmp(oncore_messages[m].flag, rcvbuf+2, 2))
				break;
		}
		l = oncore_messages[m].len;
#if 0
printf("GOT: %c%c  %d of %d entry %d\n", rcvbuf[2], rcvbuf[3], rcvptr, l, m);
#endif
		if (rcvptr < l)
			return;
		j = 0;
		for (i = 2; i < l-3; i++) 
			j ^= rcvbuf[i];
		if (j == rcvbuf[l-3]) {
			oncore_msg_any(up, rcvbuf, l-3);
			if (oncore_messages[m].handler)
				oncore_messages[m].handler(up, rcvbuf, l - 3);
		} else {
printf("Checksum mismatch!\n");	
		}

		if (l != rcvptr)
			memcpy(rcvbuf, rcvbuf+l, rcvptr - l);
		rcvptr -= l;
	}

}

static void
oncore_sendmsg(
	int	fd,
	u_char *ptr,
	int	len
	)
{
	u_char cs = 0;

printf("OnCore: Send @@%c%c %d\n", ptr[0], ptr[1], len);
	write(fd, "@@", 2);
	write(fd, ptr, len);
	while (len--)
		cs ^= *ptr++;	
	write(fd, &cs, 1);
	write(fd, "\r\n", 2);
}

static void
oncore_msg_any(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
        int i;
	struct timeval tv;

	if (buf[2] == 'E')
		return;

	gettimeofday(&tv, 0);
	printf("%d.%06d", tv.tv_sec, tv.tv_usec);

	printf(">>@@%c%c ", buf[2], buf[3]);
        for(i=2; i < len && i < 24 ; i++)
                printf("%02x", buf[i]);
        printf("\n");
}

static void
oncore_msg_Cf(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
	oncore_sendmsg(instance->ttyfd, oncore_cmd_Fa, sizeof oncore_cmd_Fa);
	instance->state = ONCORE_TEST_SENT;
}

static void
oncore_msg_Fa(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
	oncore_sendmsg(instance->ttyfd, oncore_cmd_Cj, sizeof oncore_cmd_Cj);
	instance->state = ONCORE_ID_SENT;
}

static void
oncore_msg_Cj(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
	oncore_sendmsg(instance->ttyfd, oncore_cmd_Be, sizeof oncore_cmd_Be);
	oncore_sendmsg(instance->ttyfd, oncore_cmd_Ea, sizeof oncore_cmd_Ea);
	oncore_sendmsg(instance->ttyfd, oncore_cmd_En, sizeof oncore_cmd_En);
	oncore_sendmsg(instance->ttyfd, oncore_cmd_Az0, sizeof oncore_cmd_Az0);
	oncore_sendmsg(instance->ttyfd, oncore_cmd_As, sizeof oncore_cmd_As);
	oncore_sendmsg(instance->ttyfd, oncore_cmd_At, sizeof oncore_cmd_At);
	oncore_sendmsg(instance->ttyfd, oncore_cmd_Aw1, sizeof oncore_cmd_Aw1);
	instance->state = ONCORE_UTC;
}


static void
oncore_msg_En(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
	static struct ppsclockev ev;
	int i, j;
	l_fp ts;

	if (instance->state != ONCORE_RUN && instance->state != ONCORE_UTC) return;
	memcpy(instance->En, buf, len);

	j = ev.serial;
	i = read(instance->ppsfd, &ev, sizeof ev);
	if (i != sizeof ev) return;
	if (ev.serial == j) return;
	/* ev.tv.tv_usec += 20; */
	if (ev.tv.tv_usec >= 1000000) {
		ev.tv.tv_usec -= 1000000;
		ev.tv.tv_sec ++;
	}
	TVTOTS(&ev.tv, &ts);
	ts.l_ui += JAN_1970;
	instance->pp->lastrec = ts;
	instance->pp->msec = 0;
	sprintf(instance->pp->a_lastcode, 
	    "%d.%06d %d %d %2d %2d %2d %2d rstat %02x dop %d nsat %d raim %d sigma %d sat %d%d%d%d%d%d%d%d",
	    ev.tv.tv_sec, ev.tv.tv_usec,
	    instance->pp->year, instance->pp->day,
	    instance->pp->hour, instance->pp->minute, instance->pp->second,
	    ev.tv.tv_sec % 60,
	    instance->Ea[72], instance->Ea[37], instance->Ea[39], instance->En[21],
	    instance->En[23]*256+instance->En[24],
	    instance->Ea[41], instance->Ea[45], instance->Ea[49], instance->Ea[53],
	    instance->Ea[57], instance->Ea[61], instance->Ea[65], instance->Ea[69]
	    );
	printf("ONCORE: %s\n", instance->pp->a_lastcode);
	if (instance->En[21]) return;
	if (instance->state == ONCORE_UTC) {
		if (!(ev.tv.tv_sec % 15)) {
			oncore_sendmsg(instance->ttyfd, oncore_cmd_Aw0, sizeof oncore_cmd_Aw0);
			oncore_sendmsg(instance->ttyfd, oncore_cmd_Aa, sizeof oncore_cmd_Aa);
			oncore_sendmsg(instance->ttyfd, oncore_cmd_Aw1, sizeof oncore_cmd_Aw1);
		}
		return;
	}
	if (!refclock_process(instance->pp)) {
		refclock_report(instance->peer, CEVNT_BADTIME);
		return;
	}
	record_clock_stats(&(instance->peer->srcadr), instance->pp->a_lastcode);
	instance->pollcnt = 2;
	if (!instance->polled)
		return;
	instance->polled = 0;
	instance->pp->dispersion = instance->pp->skew = 0;
	refclock_receive(instance->peer);
}

static void
oncore_msg_Ea(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
	if (instance->state != ONCORE_RUN && instance->state != ONCORE_UTC) return;
	memcpy(instance->Ea, buf, len);
	instance->pp->year = buf[6]*256+buf[7];
	instance->pp->day = ymd2yd(buf[6]*256+buf[7], buf[4], buf[5]);
	instance->pp->hour = buf[8];
	instance->pp->minute = buf[9];
	instance->pp->second = buf[10];
	memcpy(instance->Ea, buf, len);
}

static void
oncore_msg_Aw(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
	memcpy(instance->Aw, buf, len);
}

static void
oncore_msg_Aa(
	struct instance *instance,
	unsigned char *buf,
	int len
	)
{
	int gps, utc;

	if (instance->Aw[4] != 0) return;
	memcpy(instance->GPS_Aa, buf, len);
	if (instance->state != ONCORE_UTC) return;
	gps = instance->GPS_Aa[6];
	utc = instance->Ea[10];
	printf("GPS-UTC: %d %d\n", gps, utc);
	if (gps == utc) return;
	instance->state = ONCORE_RUN;
}

#else
int refclock_oncore_bs;
#endif /* REFCLOCK */
