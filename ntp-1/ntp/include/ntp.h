/*
 * ntp.h - NTP definitions for the masses
 */

#include "ntp_types.h"

/*
 * How to get signed characters.  On machines where signed char works,
 * use it.  On machines where signed char doesn't work, char had better
 * be signed.
 */
#ifdef NEED_S_CHAR_TYPEDEF
# if SIZEOF_SIGNED_CHAR
typedef signed char s_char;
# else
typedef char s_char;
# endif
  /* XXX: Why is this sequent bit INSIDE this test? */
# ifdef sequent
#  undef SO_RCVBUF
#  undef SO_SNDBUF
# endif
#endif
#ifndef TRUE
# define TRUE 1
#endif /* TRUE */
#ifndef FALSE
# define FALSE 0
#endif /* FALSE */

/*
 * NTP protocol parameters.  See section 3.2.6 of the specification.
 */
#define	NTP_VERSION	((u_char)4) /* current version number */
#define	NTP_OLDVERSION	((u_char)1) /* oldest credible version */
#define	NTP_PORT	123	/* included for sake of non-unix machines */
#define	NTP_MAXSTRATUM	((u_char)15) /* max stratum, infinity a la Bellman-Ford */
#define	NTP_MAXAGE	86400	/* one day in seconds */
#define NTP_UNREACH	16	/* poll interval backoff count */
#define NTP_MINDPOLL	6	/* log2 default min poll interval (64 s) */
#define NTP_MAXDPOLL	10	/* log2 default max poll interval (~17 m) */
#define	NTP_MINPOLL	4	/* log2 min poll interval (16 s) */
#define	NTP_MAXPOLL	17	/* log2 max poll interval (~4.5 h) */
#define	NTP_MINCLOCK	3	/* minimum survivors */
#define NTP_CANCLOCK	6	/* minimum candidates */
#define	NTP_MAXCLOCK	10	/* maximum candidates */
#define	NTP_WINDOW	8	/* reachability register size */
#define	NTP_SHIFT	8	/* 8 suitable for crystal time base */
#define	NTP_MAXKEY	65535	/* maximum authentication key number */
#define NTP_MAXSESSION	100	/* maximum entries on session key list */
#define NTP_AUTOMAX	12	/* log2 default max session key lifetime */
#define KEY_REVOKE	16	/* log2 default key revoke timeout */
#define NTP_FWEIGHT	.5	/* clock filter weight */
#define NTP_SWEIGHT	.75	/* select weight */
#define CLOCK_SGATE	10.	/* popcorn spike gate */
#define BURST_INTERVAL	1	/* interval between packets in burst */
#define MAXEPSIL	.125	/* max RMS error */
 
/*
 * Operations for RMS dispersion calculations (these use doubles)
 */
#define SQUARE(x) ((x) * (x))
#define SQRT(x) (sqrt(x))
#define DIFF(x, y) (SQUARE((x) - (y)))
#define LOGTOD(a)	((a) < 0 ? 1. / (1L << -(a)) : \
			    1L << (int)(a)) /* log2 to double */
#define ULOGTOD(a)	(1L << (int)(a)) /* ulog2 to double */
#define MAXDISPERSE	16.	/* max dispersion */
#define MINDISPERSE	.01	/* min dispersion */
#define MAXDISTANCE	1.	/* max root distance */

/*
 * Loop filter parameters.  See section 5.1 of the specification.
 *
 * Note that these are appropriate for a crystal time base.  If your
 * system clock is line frequency controlled you should read the
 * specification for appropriate modifications.  Note that the
 * loop filter code will have to change if you change CLOCK_MAX
 * to be greater than or equal to 500 ms.
 */
#define CLOCK_MAX	.128	/* max clock offset */
#define CLOCK_PHI	15e-6	/* max frequency wander */

#define	EVENT_TIMEOUT 0			/* one second, that is */

/*
 * The interface structure is used to hold the addresses and socket
 * numbers of each of the interfaces we are using.
 */
struct interface {
	int fd;			/* socket this is opened on */
	int bfd;		/* socket for receiving broadcasts */
	struct sockaddr_in sin;	/* interface address */
	struct sockaddr_in bcast;	/* broadcast address */
	struct sockaddr_in mask;	/* interface mask */
	char name[8];		/* name of interface */
	int flags;		/* interface flags */
	int last_ttl;		/* last TTL specified */
	volatile long received;	/* number of incoming packets */
	long sent;		/* number of outgoing packets */
	long notsent;		/* number of send failures */
};

/*
 * Flags for interfaces
 */
#define	INT_BROADCAST	1	/* can broadcast out this interface */
#define	INT_BCASTOPEN	2	/* broadcast socket is open */
#define	INT_LOOPBACK	4	/* the loopback interface */
#define INT_MULTICAST	8	/* multicasting enabled */

/*
 * Define flasher bits (tests 1 through 8 in packet procedure)
 * These reveal the state at the last grumble from the peer and are
 * most handy for diagnosing problems, even if not strictly a state
 * variable in the spec. These are recorded in the peer structure.
 */
#define TEST1		0x0001	/* duplicate packet received */
#define TEST2		0x0002	/* bogus packet received */
#define TEST3		0x0004	/* protocol unsynchronized */
#define TEST4		0x0008	/* peer delay/dispersion bounds check */
#define TEST5		0x0010	/* peer authentication failed */
#define TEST6		0x0020	/* peer clock unsynchronized */
#define TEST7		0x0040	/* peer stratum out of bounds */
#define TEST8		0x0080	/* root delay/dispersion bounds check */
#define TEST9		0x0100	/* peer not authenticated */
#define TEST10		0x0200	/* access denied */

/*
 * The peer structure.  Holds state information relating to the guys
 * we are peering with.  Most of this stuff is from section 3.2 of the
 * spec.
 */
struct peer {
	struct peer *next;
	struct peer *ass_next;		/* link pointer in associd hash */
	struct sockaddr_in srcadr;	/* address of remote host */
	struct interface *dstadr;	/* pointer to address on local host */
	struct refclockproc *procptr;	/* pointer to reference clock stuff */
	u_char leap;			/* leap indicator */
	u_char hmode;			/* association mode with this peer */
	u_char pmode;			/* peer's association mode */
	u_char stratum;			/* stratum of remote peer */
	s_char precision;		/* peer's clock precision */
	u_char ppoll;			/* peer poll interval */
	u_char hpoll;			/* local host poll interval */
	u_char minpoll;			/* min local host poll interval */
	u_char maxpoll;			/* max local host poll interval */
	u_char burst;			/* packets remaining in burst */
	u_char version;			/* version number */
	int flags;			/* peer flags */
	u_char cast_flags;		/* flags MDF_?CAST */
	u_int flash;			/* protocol error tally bits */
	u_char refclktype;		/* reference clock type */
	u_char refclkunit;		/* reference clock unit number */
	u_char sstclktype;		/* clock type for system status word */
	u_int32 refid;			/* peer reference ID */
	l_fp reftime;			/* time of peer's last update */
	u_long keyid;			/* current key ID */
	u_long pkeyid;			/* previous key ID (autokey) */
	u_long *keylist;		/* session key identifier list */
	int keynumber;			/* session key identifier number */
	u_short associd;		/* association ID, a unique integer */
	u_char ttl;			/* time to live (multicast) */

/* **Start of clear-to-zero area.*** */
/* Everything that is cleared to zero goes below here */
	u_char valid;			/* valid counter */
#define	clear_to_zero valid
	double estbdelay;		/* broadcast offset */
	u_char status;			/* peer status */
	u_char pollsw;			/* what it says */
	u_char reach;			/* reachability, NTP_WINDOW bits */
	u_char unreach;			/* unreachable count */
	u_short filter_nextpt;		/* index into filter shift register */
	double filter_delay[NTP_SHIFT];	/* delay part of shift register */
	double filter_offset[NTP_SHIFT]; /* offset part of shift register */
	double filter_error[NTP_SHIFT];	/* error part of shift register */
	u_char filter_order[NTP_SHIFT];	/* we keep the filter sorted here */
	l_fp org;			/* originate time stamp */
	l_fp rec;			/* receive time stamp */
	l_fp xmt;			/* transmit time stamp */
	double age;			/* last sample age */
	double offset;			/* clock offset */
	double delay;			/* roundtrip delay */
	double dispersion;		/* error bound */
	double skew;			/* filter error */
	double rootdelay;		/* distance from primary clock */
	double rootdispersion;		/* peer clock dispersion */

/* ***End of clear-to-zero area.*** */
/* Everything that is cleared to zero goes above here */
	u_long update;			/* receive time last packet */
#define end_clear_to_zero update
	u_long outdate;			/* send time last packet */
	u_long nextdate;		/* send time next packet */
        u_long nextaction;	        /* peer local activity timeout (refclocks mainly) */
        void   (*action) P((struct peer *));/* action timeout function */
	/*
	 * statistic counters
	 */
	u_long timereset;		/* time stat counters were reset */
	u_long sent;			/* number of updates sent */
	u_long received;		/* number of frames received */
	u_long timereceived;		/* last time a frame received */
	u_long timereachable;		/* last reachable/unreachable event */
	u_long processed;		/* processed by the protocol */
	u_long badauth;			/* bad credentials detected */
	u_long bogusorg;		/* rejected due to bogus origin */
	u_long oldpkt;			/* rejected as duplicate packet */
	u_long seldisptoolarge;		/* too much dispersion for selection */
	u_long selbroken;		/* broken NTP detected in selection */
	u_long seltooold;		/* too long since sync in selection */
	u_char last_event;		/* set to code for last peer error */
	u_char num_events;		/* num. of events which have occurred */
};

/*
 * Values for peer.leap, sys_leap
 */
#define	LEAP_NOWARNING	0x0	/* normal, no leap second warning */
#define	LEAP_ADDSECOND	0x1	/* last minute of day has 61 seconds */
#define	LEAP_DELSECOND	0x2	/* last minute of day has 59 seconds */
#define	LEAP_NOTINSYNC	0x3	/* overload, clock is free running */

/*
 * Values for peer.mode
 */
#define	MODE_UNSPEC	0	/* unspecified (probably old NTP version) */
#define	MODE_ACTIVE	1	/* symmetric active */
#define	MODE_PASSIVE	2	/* symmetric passive */
#define	MODE_CLIENT	3	/* client mode */
#define	MODE_SERVER	4	/* server mode */
#define	MODE_BROADCAST	5	/* broadcast mode */
#define	MODE_CONTROL	6	/* control mode packet */
#define	MODE_PRIVATE	7	/* implementation defined function */

#define	MODE_BCLIENT	8	/* a pseudo mode, used internally */
#define MODE_MCLIENT	9	/* multicast mode, used internally */

/*
 * Values for peer.stratum, sys_stratum
 */
#define	STRATUM_REFCLOCK ((u_char)0) /* stratum claimed by primary clock */
#define	STRATUM_PRIMARY	((u_char)1) /* host has a primary clock */
#define	STRATUM_INFIN ((u_char)NTP_MAXSTRATUM) /* infinity a la Bellman-Ford */
/* A stratum of 0 in the packet is mapped to 16 internally */
#define	STRATUM_PKT_UNSPEC ((u_char)0) /* unspecified in packet */
#define	STRATUM_UNSPEC	((u_char)(NTP_MAXSTRATUM+(u_char)1)) /* unspecified */

/*
 * Values for peer.flags
 */
#define	FLAG_CONFIG		0x1	/* association was configured */
#define	FLAG_AUTHENABLE		0x2	/* this guy needs authentication */
#define	FLAG_MCAST1		0x4	/* multicast client/server mode */
#define	FLAG_MCAST2		0x8	/* multicast client mode */
#define	FLAG_AUTHENTIC		0x10	/* last message was authentic */
#define	FLAG_REFCLOCK		0x20	/* this is actually a reference clock */
#define	FLAG_SYSPEER		0x40	/* this is one of the selected peers */
#define FLAG_PREFER		0x80	/* this is the preferred peer */
#define FLAG_BURST		0x100	/* burst mode */
#define FLAG_SKEY		0x200	/* autokey authentication */

/*
 * Definitions for the clear() routine.  We use memset() to clear
 * the parts of the peer structure which go to zero.  These are
 * used to calculate the start address and length of the area.
 */
#define	CLEAR_TO_ZERO(p)	((char *)&((p)->clear_to_zero))
#define	END_CLEAR_TO_ZERO(p)	((char *)&((p)->end_clear_to_zero))
#define	LEN_CLEAR_TO_ZERO	(END_CLEAR_TO_ZERO((struct peer *)0) \
				    - CLEAR_TO_ZERO((struct peer *)0))
/*
 * Reference clock identifiers (for pps signal)
 */
#define PPSREFID (u_int32)"PPS "	/* used when pps controls stratum>1 */

/*
 * Reference clock types.  Added as necessary.
 */
#define	REFCLK_NONE		0	/* unknown or missing */
#define	REFCLK_LOCALCLOCK	1	/* external (e.g., lockclock) */
#define	REFCLK_GPS_TRAK		2	/* TRAK 8810 GPS Receiver */
#define	REFCLK_WWV_PST		3	/* PST/Traconex 1020 WWV/H */
#define	REFCLK_WWVB_SPECTRACOM	4	/* Spectracom 8170/Netclock WWVB */
#define	REFCLK_TRUETIME		5	/* TrueTime (generic) Receivers */
#define REFCLK_IRIG_AUDIO	6       /* IRIG-B audio decoder */
#define	REFCLK_CHU		7	/* scratchbuilt CHU (Canada) */
#define REFCLK_PARSE		8	/* generic driver (usually DCF77,GPS,MSF) */
#define	REFCLK_GPS_MX4200	9	/* Magnavox MX4200 GPS */
#define REFCLK_GPS_AS2201	10	/* Austron 2201A GPS */
#define	REFCLK_GPS_ARBITER	11	/* Arbiter 1088A/B/ GPS */
#define REFCLK_IRIG_TPRO	12	/* KSI/Odetics TPRO-S IRIG */
#define REFCLK_ATOM_LEITCH	13	/* Leitch CSD 5300 Master Clock */
#define REFCLK_MSF_EES		14	/* EES M201 MSF Receiver */
#define	REFCLK_GPSTM_TRUE	15	/* OLD TrueTime GPS/TM-TMD Receiver */
#define REFCLK_IRIG_BANCOMM	16	/* Bancomm GPS/IRIG Interface */
#define REFCLK_GPS_DATUM	17	/* Datum Programmable Time System */
#define REFCLK_NIST_ACTS	18	/* NIST Auto Computer Time Service */
#define REFCLK_WWV_HEATH	19	/* Heath GC1000 WWV/WWVH Receiver */
#define REFCLK_GPS_NMEA		20	/* NMEA based GPS clock */
#define REFCLK_GPS_VME		21	/* TrueTime GPS-VME Interface */
#define REFCLK_ATOM_PPS		22	/* 1-PPS Clock Discipline */
#define REFCLK_PTB_ACTS		23	/* PTB Auto Computer Time Service */
#define REFCLK_USNO		24	/* Naval Observatory dialup */
#define REFCLK_GPS_HP		26	/* HP 58503A Time/Frequency Receiver */
#define REFCLK_ARCRON_MSF       27      /* ARCRON MSF radio clock. */
#define REFCLK_SHM		28	/* clock attached thru shared memory */
#define REFCLK_PALISADE		29	/* Trimble Navigation Palisade GPS */
#define REFCLK_ONCORE		30	/* Motorola UT Oncore GPS */
#define REFCLK_GPS_JUPITER	31	/* Rockwell Jupiter GPS receiver */
#define REFCLK_MAX		31	/* Grow as needed... */

/*
 * We tell reference clocks from real peers by giving the reference
 * clocks an address of the form 127.127.t.u, where t is the type and
 * u is the unit number.  We define some of this here since we will need
 * some sanity checks to make sure this address isn't interpretted as
 * that of a normal peer.
 */
#define	REFCLOCK_ADDR	0x7f7f0000	/* 127.127.0.0 */
#define	REFCLOCK_MASK	0xffff0000	/* 255.255.0.0 */

#define	ISREFCLOCKADR(srcadr)	((SRCADR(srcadr) & REFCLOCK_MASK) \
					== REFCLOCK_ADDR)

/*
 * Macro for checking for invalid addresses.  This is really, really
 * gross, but is needed so no one configures a host on net 127 now that
 * we're encouraging it the the configuration file.
 */
#define	LOOPBACKADR	0x7f000001
#define	LOOPNETMASK	0xff000000

#define	ISBADADR(srcadr)	(((SRCADR(srcadr) & LOOPNETMASK) \
				    == (LOOPBACKADR & LOOPNETMASK)) \
				    && (SRCADR(srcadr) != LOOPBACKADR))

/*
 * Utilities for manipulating addresses and port numbers
 */
#define	NSRCADR(src)	((src)->sin_addr.s_addr) /* address in net byte order */
#define	NSRCPORT(src)	((src)->sin_port)	/* port in net byte order */
#define	SRCADR(src)	(ntohl(NSRCADR((src))))	/* address in host byte order */
#define	SRCPORT(src)	(ntohs(NSRCPORT((src))))	/* host port */

/*
 * NTP packet format.  The mac field is optional.  It isn't really
 * an l_fp either, but for now declaring it that way is convenient.
 * See Appendix A in the specification.
 *
 * Note that all u_fp and l_fp values arrive in network byte order
 * and must be converted (except the mac, which isn't, really).
 */
struct pkt {
	u_char li_vn_mode;	/* contains leap indicator, version and mode */
	u_char stratum;		/* peer's stratum */
	u_char ppoll;		/* the peer polling interval */
	s_char precision;	/* peer clock precision */
	u_fp rootdelay;		/* distance to primary clock */
	u_fp rootdispersion;	/* clock dispersion */
	u_int32 refid;		/* reference clock ID */
	l_fp reftime;		/* time peer clock was last updated */
	l_fp org;		/* originate time stamp */
	l_fp rec;		/* receive time stamp */
	l_fp xmt;		/* transmit time stamp */

#define MIN_MAC_LEN	(sizeof(u_int32) + 8)		/* DES */
#define MAX_MAC_LEN	(sizeof(u_int32) + 16)		/* MD5 */

	/*
	 * The length of the packet less MAC must be a multiple of 64
	 * bits. For normal private-key cryptography, the cryptosum
	 * covers only the raw NTP header. For autokey cryptography,
	 * the heade is incresed by 64 bits to contain the field length
	 * and private value.
	 */
	u_int32 keyid1;		/* key identifier 1 */
	u_int32 keyid2;		/* key identifier 2 */
	u_int32	keyid3;		/* key identifier 3 */
	u_char mac[MAX_MAC_LEN]; /* mac */
};

/*
 * Packets can come in two flavours, one with a mac and one without.
 */
#define LEN_PKT_NOMAC	(sizeof(struct pkt) - MAX_MAC_LEN - 3 * sizeof(u_int32))

/*
 * Minimum size of packet with a MAC: has to include at least a key number.
 */
#define LEN_PKT_MAC	(LEN_PKT_NOMAC + sizeof(u_int32))

/*
 * Stuff for extracting things from li_vn_mode
 */
#define	PKT_MODE(li_vn_mode)	((u_char)((li_vn_mode) & 0x7))
#define	PKT_VERSION(li_vn_mode)	((u_char)(((li_vn_mode) >> 3) & 0x7))
#define	PKT_LEAP(li_vn_mode)	((u_char)(((li_vn_mode) >> 6) & 0x3))

/*
 * Stuff for putting things back into li_vn_mode
 */
#define	PKT_LI_VN_MODE(li, vn, md) \
	((u_char)((((li) << 6) & 0xc0) | (((vn) << 3) & 0x38) | ((md) & 0x7)))


/*
 * Dealing with stratum.  0 gets mapped to 16 incoming, and back to 0
 * on output.
 */
#define	PKT_TO_STRATUM(s)	((u_char)(((s) == (STRATUM_PKT_UNSPEC)) ?\
				(STRATUM_UNSPEC) : (s)))

#define	STRATUM_TO_PKT(s)	((u_char)(((s) == (STRATUM_UNSPEC)) ?\
				(STRATUM_PKT_UNSPEC) : (s)))

/*
 * Format of a recvbuf.  These are used by the asynchronous receive
 * routine to store incoming packets and related information.
 */

/*
 *  the maximum length NTP packet is a full length NTP control message with
 *  the maximum length message authenticator.  I hate to hard-code 468 and 12,
 *  but only a few modules include ntp_control.h...
 */   
#define	RX_BUFF_SIZE	(468+12+MAX_MAC_LEN)

struct recvbuf {
	struct recvbuf *next;		/* next buffer in chain */
	union {
		struct sockaddr_in X_recv_srcadr;
		caddr_t X_recv_srcclock;
	} X_from_where;
#define recv_srcadr	X_from_where.X_recv_srcadr
#define	recv_srcclock	X_from_where.X_recv_srcclock
	struct sockaddr_in srcadr;	/* where packet came from */
	struct interface *dstadr;	/* interface datagram arrived thru */
	int fd;				/* fd on which it was received */
	l_fp recv_time;			/* time of arrival */
	void (*receiver) P((struct recvbuf *)); /* routine to receive buffer */
	int recv_length;		/* number of octets received */
	union {
		struct pkt X_recv_pkt;
		u_char X_recv_buffer[RX_BUFF_SIZE];
	} recv_space;
#define	recv_pkt	recv_space.X_recv_pkt
#define	recv_buffer	recv_space.X_recv_buffer
};


/*
 * Event codes.  Used for reporting errors/events to the control module
 */
#define	PEER_EVENT	0x80		/* this is a peer event */

#define	EVNT_UNSPEC	0
#define	EVNT_SYSRESTART	1
#define	EVNT_SYSFAULT	2
#define	EVNT_SYNCCHG	3
#define	EVNT_PEERSTCHG	4
#define	EVNT_CLOCKRESET	5
#define	EVNT_BADDATETIM	6
#define	EVNT_CLOCKEXCPT	7

#define	EVNT_PEERIPERR	(1|PEER_EVENT)
#define	EVNT_PEERAUTH	(2|PEER_EVENT)
#define	EVNT_UNREACH	(3|PEER_EVENT)
#define	EVNT_REACH	(4|PEER_EVENT)
#define	EVNT_PEERCLOCK	(5|PEER_EVENT)

/*
 * Clock event codes
 */
#define	CEVNT_NOMINAL	0
#define	CEVNT_TIMEOUT	1
#define	CEVNT_BADREPLY	2
#define	CEVNT_FAULT	3
#define	CEVNT_PROP	4
#define	CEVNT_BADDATE	5
#define	CEVNT_BADTIME	6
#define CEVNT_MAX	CEVNT_BADTIME

/*
 * Very misplaced value.  Default port through which we send traps.
 */
#define	TRAPPORT	18447


/*
 * To speed lookups, peers are hashed by the low order bits of the remote
 * IP address.  These definitions relate to that.
 */
#define	HASH_SIZE	32
#define	HASH_MASK	(HASH_SIZE-1)
#define	HASH_ADDR(src)	((SRCADR((src))^(SRCADR((src))>>8)) & HASH_MASK)

/*
 * How we randomize polls.  The poll interval is a power of two.
 * We chose a random value which is between 1/4 and 3/4 of the
 * poll interval we would normally use and which is an even multiple
 * of the EVENT_TIMEOUT.  The random number routine, given an argument
 * spread value of n, returns an integer between 0 and (1<<n)-1.  This
 * is shifted by EVENT_TIMEOUT and added to the base value.
 */
#if defined(HAVE_MRAND48)
#define RANDOM		(mrand48())
#define SRANDOM(x)	(srand48(x))
#elif defined(HAVE_RANDOM)
#define RANDOM		(random())
#define SRANDOM(x)	(srandom(x))
#else
#define RANDOM		(0)
#define SRANDOM(x)	(0)
#endif

#define RANDPOLL(x)	((1 << (x)) - (1 << ((x) - 2)) + (RANDOM & \
			    ((1 << ((x) - 1)) - 1)))
#define	RANDOM_SPREAD(poll)	((poll) - (EVENT_TIMEOUT+1))
#define	RANDOM_POLL(poll, rval)	((((rval)+1)<<EVENT_TIMEOUT) + (1<<((poll)-2)))

/*
 * min, min3 and max.  Makes it easier to transliterate the spec without
 * thinking about it.
 */
#define	min(a,b)	(((a) < (b)) ? (a) : (b))
#define	max(a,b)	(((a) > (b)) ? (a) : (b))
#define	min3(a,b,c)	min(min((a),(b)), (c))


/*
 * Configuration items.  These are for the protocol module (proto_config())
 */
#define	PROTO_BROADCLIENT	1
#define	PROTO_PRECISION		2	/* (not used) */
#define	PROTO_AUTHENTICATE	3
#define	PROTO_BROADDELAY	4
#define	PROTO_AUTHDELAY		5	/* (not used) */
#define PROTO_MULTICAST_ADD	6
#define PROTO_MULTICAST_DEL	7
#define PROTO_NTP		8
#define PROTO_KERNEL		9
#define PROTO_MONITOR		10
#define PROTO_FILEGEN		11

/*
 * Configuration items for the loop filter
 */
#define	LOOP_DRIFTINIT		1	/* set initial frequency offset */
#define LOOP_DRIFTCOMP		2	/* set frequency offset */
#define LOOP_PPSDELAY		3	/* set pps delay */
#define LOOP_PPSBAUD		4	/* set pps baud rate */

/*
 * Configuration items for the stats printer
 */
#define	STATS_FREQ_FILE		1	/* configure drift file */
#define STATS_STATSDIR		2	/* directory prefix for stats files */
#define	STATS_PID_FILE		3	/* configure ntpd PID file */

#define MJD_1970		40587	/* MJD for 1 Jan 1970 */

/*
 * Default parameters.  We use these in the absence of something better.
 */
#define	DEFBROADDELAY	4e-3		/* default broadcast offset */
#define INADDR_NTP	0xe0000101	/* NTP multicast address 224.0.1.1 */
/*
 * Structure used optionally for monitoring when this is turned on.
 */
struct mon_data {
	struct mon_data *hash_next;	/* next structure in hash list */
	struct mon_data *mru_next;	/* next structure in MRU list */
	struct mon_data *mru_prev;	/* previous structure in MRU list */
	struct mon_data *fifo_next;	/* next structure in FIFO list */
	struct mon_data *fifo_prev;	/* previous structure in FIFO list */
	u_long lastdrop;		/* last time dropped due to RES_LIMIT*/
	u_long lasttime;		/* last time data updated */
	u_long firsttime;		/* time structure initialized */
	u_long count;			/* count we have seen */
	u_int32 rmtadr;			/* address of remote host */
	struct interface *interface;	/* interface on which this arrived */
	u_short rmtport;		/* remote port last came from */
	u_char mode;			/* mode of incoming packet */
	u_char version;			/* version of incoming packet */
	u_char cast_flags;		/* flags MDF_?CAST */
};

#define	MDF_UCAST	0x1		/* unicast packet */
#define	MDF_MCAST	0x2		/* multicast packet */
#define	MDF_BCAST	0x4		/* broadcast packet */
#define	MDF_LCAST	0x8		/* local packet */
#define MDF_ACAST	0x10		/* manycast packet */

/*
 * Values used with mon_enabled to indicate reason for enabling monitoring
 */
#define MON_OFF    0x00			/* no monitoring */
#define MON_ON     0x01			/* monitoring explicitly enabled */
#define MON_RES    0x02			/* implicit monitoring for RES_LIMITED */
/*
 * Structure used for restrictlist entries
 */
struct restrictlist {
	struct restrictlist *next;	/* link to next entry */
	u_int32 addr;			/* host address (host byte order) */
	u_int32 mask;			/* mask for address (host byte order) */
	u_long count;			/* number of packets matched */
	u_short flags;			/* accesslist flags */
	u_short mflags;			/* match flags */
};

/*
 * Access flags
 */
#define	RES_IGNORE		0x1	/* ignore if matched */
#define	RES_DONTSERVE		0x2	/* don't give him any time */
#define	RES_DONTTRUST		0x4	/* don't trust if matched */
#define	RES_NOQUERY		0x8	/* don't allow queries if matched */
#define	RES_NOMODIFY		0x10	/* don't allow him to modify server */
#define	RES_NOPEER		0x20	/* don't allocate memory resources */
#define	RES_NOTRAP		0x40	/* don't allow him to set traps */
#define	RES_LPTRAP		0x80	/* traps set by him are low priority */
#define RES_LIMITED		0x100   /* limit per net number of clients */

#define	RES_ALLFLAGS \
    (RES_IGNORE|RES_DONTSERVE|RES_DONTTRUST|RES_NOQUERY\
    |RES_NOMODIFY|RES_NOPEER|RES_NOTRAP|RES_LPTRAP|RES_LIMITED)

/*
 * Match flags
 */
#define	RESM_INTERFACE		0x1	/* this is an interface */
#define	RESM_NTPONLY		0x2	/* match ntp port only */

/*
 * Restriction configuration ops
 */
#define	RESTRICT_FLAGS		1	/* add flags to restrict entry */
#define	RESTRICT_UNFLAG		2	/* remove flags from restrict entry */
#define	RESTRICT_REMOVE		3	/* remove a restrict entry */


/*
 * Experimental alternate selection algorithm identifiers
 */
#define	SELECT_1	1
#define	SELECT_2	2
#define	SELECT_3	3
#define	SELECT_4	4
#define	SELECT_5	5

/*
 * Endpoint structure for the select algorithm
 */
struct endpoint {
	double	val;			/* offset of endpoint */
	int	type;			/* interval entry/exit */
};

/*
 * Defines for association matching 
 */
#define AM_MODES	10	/* total number of modes */
#define NO_PEER		0	/* action when no peer is found */

/*
 * Association matching AM[] return codes
 */
#define AM_ERR		-1
#define AM_NOMATCH	 0
#define AM_PROCPKT	 1
#define AM_FXMIT	 2
#define AM_MANYCAST	 3
#define AM_NEWPASS	 4
#define AM_NEWBCL	 5
#define AM_POSSBCL	 6
