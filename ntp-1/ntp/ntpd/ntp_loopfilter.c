/*
 * ntp_loopfilter.c - implements the NTP loop filter algorithm
 *
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>


#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"

#if defined(VMS) && defined(VMS_LOCALUNIT)	/*wjm*/
#include "ntp_refclock.h"
#endif /* VMS */

#ifdef KERNEL_PLL
#ifdef HAVE_SYS_TIMEX_H
#include <sys/timex.h>
#endif
#ifdef NTP_SYSCALLS_STD
#define ntp_gettime(t)  syscall(SYS_ntp_gettime, (t))
#define ntp_adjtime(t)  syscall(SYS_ntp_adjtime, (t))
#else /* NOT NTP_SYSCALLS_STD */
#ifdef HAVE___NTP_GETTIME
#define ntp_gettime(t)  __ntp_gettime((t))
#endif /* HAVE___NTP_GETTIME */
#ifdef HAVE___ADJTIMEX
#define ntp_adjtime(t)  __adjtimex((t))
#endif
#endif /* NOT NTP_SYSCALLS_STD */
#endif /* KERNEL_PLL */

/*
 * This is an implementation of the clock discipline algorithm described
 * in UDel TR 97-4-3. It operates as an adaptive parameter, hybrid
 * phase/frequency-lock oscillator. A number of sanity checks are
 * included to protect against timewarps, timespikes and general mayhem.
 * All units are in s and s/s, unless noted otherwise.
 */
#define CLOCK_WAYTOOBIG 1000.	/* insanity check */
#define NTP_MAXFREQ 500e-6	/* max frequency offset */
#define MAXADJ 10e-6		/* max frequency adjustment */
#define CLOCK_PHASE 4.		/* 2^2 phase factor */
#define CLOCK_FREQ 256.		/* 2^8 frequency factor */
#define NTP_AVG 4.		/* RMS averaging constant */
#define NTP_MAXFLL 13		/* max FLL averaging constant (log2) */
#define CLOCK_MINSTEP 900.	/* step-change timeout */
#define CLOCK_DAY 86400.	/* one day of seconds */
#define CLOCK_LIMIT 30		/* poll-adjust threshold */
#define CLOCK_PGATE 4.		/* poll-adjust gate */
#define CLOCK_ALLAN 3000.	/* Allan intercept */

/*
 * Clock discipline state machine. This is used to control the
 * synchronization behavior during initialization and following a
 * timewarp. 
 */
#define S_NSET	0		/* clock never set */
#define S_FREQ	1		/* frequency correction */
#define S_FSET	2		/* frequency set */
#define S_TSET	3		/* time set */
#define S_SYNC	4		/* clock synchronized */
#define S_SPIK	5		/* spike detected */

/*
 * Kernel PLL/PPS state machine. This is used with the kernel PLL
 * modifications described in the README.kernel file.
 *
 * Each update to a prefer peer sets pps_update true if it survives the
 * intersection algorithm and its time is within range. The PPS time
 * discipline is enabled (STA_PPSTIME bit set in the status word) when
 * pps_update is true and the PPS frequency discipline is enabled. If
 * the PPS time discipline is enabled and the kernel reports a PPS
 * signal is present, the pps_control variable is set to the current
 * time. If the current time is later than pps_control by PPS_MAXAGE
 * (120 s), this variable is set to zero.
 *
 * The ntp_enable and kern_enable switches can be set at configuration time 
 * or run time using ntpdc. If ntp_enable is false, the discipline loop
 * is unlocked and no correctios of any kind are made. If kern_enable is
 * true, the kernel modifications are active as described above; if false,
 * the kernel is bypassed entirely and the daemon PLL used instead.
 */
#define PPS_MAXAGE 120		/* kernel pps signal timeout (s) */

/*
 * Program variables
 */
u_long	last_time;		/* time of last clock update (s) */
double	clock_stability;	/* clock stability (ppm) */
double	allan_xpt;		/* Allan intercept */
static double clock_frequency;	/* clock frequency adjustment (ppm) */
double	drift_comp;		/* clock frequency (ppm) */
double	last_offset;		/* last clock offset */
static double clock_offset;	/* current clock offset */
u_char	sys_poll;		/* log2 of system poll interval */
int	tc_counter;		/* poll-adjust counter */
int	pll_status;		/* status bits for kernel pll */
volatile int pll_control;	/* true if working kernel pll */
int	kern_enable;		/* true if pll enabled */
int	ntp_enable;		/* true if clock discipline enabled */
u_long	pps_control;		/* last pps sample time */
int	pps_update;		/* pps update valid */
int	state;			/* clock discipline state */
static double flladj;		/* last FLL frequency adjustment */
static double plladj;		/* last PLL frequency adjustment */
double	sys_error;		/* RMS jitter estimate */
static double fllerror;		/* FLL frequency error estimate */
static double pllerror;		/* PLL frequency error estimate */
static void rstclock P((void));	/* reset clock parameters */
int allow_set_backward;		/* if TRUE, use step */
int correct_any;		/* if TRUE, correct offset > 1000 s */

/*
 * Imported from ntp_proto.c module
 */
extern double sys_rootdelay;	/* root delay */
extern double sys_rootdispersion; /* root dispersion */
extern s_char sys_precision;	/* local clock precision */
extern struct peer *sys_peer;	/* system peer pointer */
extern u_char sys_leap;		/* system leap bits */
extern l_fp sys_reftime;	/* time at last update */

/*
 * Imported from the library
 */
extern double sys_maxfreq;	/* max frequency correction */

/*
 * Imported from ntp_io.c module
 */
extern struct interface *loopback_interface;

/*
 * Imported from ntpd.c module
 */
extern int debug;		/* global debug flag */

/*
 * Imported from ntp_io.c module
 */
extern u_long current_time;	/* like it says, in seconds */

#if defined(KERNEL_PLL)
/* Emacs cc-mode goes nuts if we split the next line... */
#define MOD_BITS (MOD_OFFSET | MOD_MAXERROR | MOD_ESTERROR | MOD_STATUS | MOD_TIMECONST)
#ifdef NTP_SYSCALLS_STD
#ifdef DECL_SYSCALL
extern int syscall	P((int, void *, ...));
#endif /* DECL_SYSCALL */
#endif /* NTP_SYSCALLS_STD */
void pll_trap		P((int));
#ifdef SIGSYS
static struct sigaction sigsys;	/* current sigaction status */
static struct sigaction newsigsys; /* new sigaction status */
static sigjmp_buf env;		/* environment var. for pll_trap() */
#endif /* SIGSYS */
#endif /* KERNEL_PLL */

/*
 * init_loopfilter - initialize loop filter data
 */
void
init_loopfilter(void)
{
	/*
	 * Initialize clockworks and wind clockspring
	 */
	drift_comp = 0;
	pps_update = pps_control = 0;
	rstclock();
	state = S_NSET;
	allow_set_backward = TRUE;
	correct_any = FALSE;
}

/*
 * local_clock - the NTP logical clock loop filter.  Returns 1 if the
 * clock was stepped, 0 if it was slewed and -1 if it is hopeless.
 */
int
local_clock(
	struct peer *peer,	/* synch source peer structure */
	double fp_offset,	/* clock offset */
	double epsil		/* jittter (square) */
	)
{
	double adjust;		/* frequency adjustment */
	double mu;		/* interval since last update (s) */
	double tau;		/* system poll interval (s) */
	double osys_error;	/* previous RMS jitter estimate */
	double dtemp, etemp;	/* double temps */
#if defined(KERNEL_PLL)
	struct timex ntv;
	int retval;
#if defined(MOD_CANSCALE)
	static int kernel_can_scale;
#endif /* MOD_CANSCALE */
#endif /* KERNEL_PLL */

#ifdef DEBUG
	if (debug)
		printf(
		    "local_clock: offset %.6f freq %.3f error %.6f state %d\n",
		    fp_offset, drift_comp * 1e6, SQRT(epsil), state);
#endif
	if (!ntp_enable)
		return(0);

	/*
	 * If the clock is way off, don't tempt fate by correcting it.
	 */
	if (fabs(fp_offset) >= CLOCK_WAYTOOBIG && !correct_any) {
		msyslog(LOG_ERR,
		    "time error %.0f over 1000 seconds; set clock manually)",
		    fp_offset);
		return (-1);
	}

	/*
	 * If the clock has never been set, set it and  initialize the
	 * discipline parameters.
	 */
	if (state == S_NSET) {
		step_systime(fp_offset);
		NLOG(NLOG_SYNCEVENT|NLOG_SYSEVENT)
		    msyslog(LOG_NOTICE, "time set %.6f s", fp_offset);
		rstclock();
		return (1);
	}

	/*
	 * If the offset exceeds CLOCK_MAX and this is the first update
	 * after setting the clock, switch to frequency mode. This could
	 * happen either because the frequency error was too large or
	 * the kernel ignored the fractional second, as some do.
	 * Otherwise, unlock the loop and allow it to coast up to
	 * CLOCK_MINSTEP. If the offset on a subsequent update is less
	 * than this, resume normal updates; if not, allow one
	 * additional spike, then step the clock to the indicated time
	 * and transition to S_TSET state.
	 */
	mu = current_time - last_time;
	tau = ULOGTOD(sys_poll);
	if (fabs(fp_offset) > CLOCK_MAX) {
		if (state == S_TSET || !allow_set_backward) {
			state = S_FREQ;
			allan_xpt = tau;
		} else if (state == S_SPIK || state == S_FSET) {
			step_systime(fp_offset);
			NLOG(NLOG_SYNCEVENT|NLOG_SYSEVENT)
			    msyslog(LOG_NOTICE, "time reset %.6f s",
		   	    fp_offset);
			rstclock();
			return (1);
		} else if (mu >= CLOCK_MINSTEP) {
			state = S_SPIK;
			allan_xpt = tau;
		}
		if (state != S_FREQ)
			return (0);
	}

	/*
	 * The offset is less than CLOCK_MAX. If this is the first
	 * update, initialize the discipline parameters and pretend we
	 * had just set the clock. 
	 */
	if (state == S_FSET) {
		rstclock();
		last_offset = clock_offset = fp_offset;
		return (0);
	}

	/*
	 * Update phase error estimate.
	 */
	osys_error = sys_error;
	dtemp = SQUARE(sys_error);
	sys_error = SQRT(dtemp + (epsil - dtemp) / NTP_AVG);

	/*
	 * If the offset exceeds the previous time error estimate by
	 * CLOCK_SGATE and the interval since the last update is less
	 * than twice the poll interval, consider the update a popcorn
	 * spike and ignore it.
	 */
	if (fabs(clock_offset - last_offset) > CLOCK_SGATE *
	    osys_error && mu < tau * 2 && state == S_SYNC)
		return (0);
	if (mu < tau / 2)
		return (0);
	
	/*
	 * Compute frequency error estimates. These are computed as the
	 * RMS errors over the last several updates. The old time error
	 * estimate is saved for the popcorn test later.
	 */
	adjust = fp_offset - clock_offset;
	clock_offset = fp_offset;
	dtemp = SQUARE(fllerror);
	etemp = SQUARE(adjust - flladj * mu);
	fllerror = SQRT(dtemp + (etemp - dtemp) / NTP_AVG);
	dtemp = SQUARE(pllerror);
	etemp = SQUARE(adjust - plladj * mu);
	pllerror = SQRT(dtemp + (etemp - dtemp) / NTP_AVG);

	/*
	 * Compute the FLL and PLL frequency adjustments. If the noise 
	 * exceeds MAXEPSIL, just set the phase and leave the frequency
	 * alone. Otherwise, adjust the frequency using the FLL and PLL
	 * predictions weighted by the respective error estimates.
	 */
	dtemp = LOGTOD(NTP_MAXFLL - sys_poll);
	if (dtemp < 2.)
		dtemp = 2.;
	flladj = clock_offset / (mu * dtemp);
	plladj = clock_offset * mu / (CLOCK_FREQ * tau * tau);
	if (state == S_FREQ)
		clock_frequency = flladj;
	else if (sys_error > MAXEPSIL && state == S_SYNC)
		clock_frequency = 0;
	else
		clock_frequency = (flladj * pllerror + plladj * fllerror) /
		    (fllerror + pllerror);
 
	/*
	 * In SYNC state, adjust the poll interval.
	 */
	if (fabs(clock_frequency) < MAXADJ && fabs(clock_offset) < CLOCK_MAX) {
		state = S_SYNC;
		allan_xpt = CLOCK_ALLAN;
		if (fabs(clock_offset) > CLOCK_PGATE * sys_error) {
			tc_counter -= sys_poll << 1;
			if (tc_counter < -CLOCK_LIMIT) {
				tc_counter = -CLOCK_LIMIT;
				if (sys_poll > peer->minpoll) {
					tc_counter = 0;
					sys_poll--;
				}
			}
		} else {
			tc_counter += sys_poll;
			if (tc_counter > CLOCK_LIMIT) {
				tc_counter = CLOCK_LIMIT;
				if (sys_poll < peer->maxpoll) {
					tc_counter = 0;
					sys_poll++;
				}
			}
		}
	}

	/*
	 * Adjust the clock frequency if allowed.
	 */
	if ( !(sys_peer->refclktype == REFCLK_LOCALCLOCK &&
	    sys_peer->flags & FLAG_PREFER)) {
		drift_comp += clock_frequency;
		if (drift_comp > sys_maxfreq)
			drift_comp = sys_maxfreq;
		else if (drift_comp <= -sys_maxfreq)
			drift_comp = -sys_maxfreq;
	}

	/*
	 * Calculate the Allan variance from the frequency adjustments.
	 * This is useful for performance monitoring, but does not
	 * affect the loop variables.
	 */
	dtemp = SQUARE(clock_stability);
	etemp = SQUARE(clock_frequency);
	clock_stability = SQRT(dtemp + (etemp - dtemp) / NTP_AVG);

	/*
	 * Time to warp the clock. Update the system variables here so
	 * the kernel API gets the right values.
	 */
	last_time = current_time;
	last_offset = clock_offset;
	dtemp = peer->dispersion + sys_error;
	if ((peer->flags & FLAG_REFCLOCK) == 0 && dtemp < MINDISPERSE)
		dtemp = MINDISPERSE;
	sys_rootdispersion = peer->rootdispersion + dtemp;
#ifdef DEBUG
        if (debug)
                printf(
                    "local_clock: mu %.0f padj %.6f fadj %.3f fll %.3f pll %.3f\n",
		    mu, adjust, clock_frequency * 1e6, flladj * 1e6,
		    plladj * 1e6);
#endif

	/*
	 * This code segment works when the clock-adjustment code is
	 * implemented in the kernel, which at present is only in the
	 * (modified) HP 9, SunOS 4, Ultrix 4 and OSF/1 kernels. In the
	 * case of the DECstation 5000/240 and Alpha AXP, additional
	 * kernel modifications provide a true microsecond clock.
	 */
#if defined(KERNEL_PLL)
	if (pll_control && kern_enable && state) {

		/*
		 * We initialize the structure for the ntp_adjtime()
		 * system call. We have to convert everything to
		 * microseconds first. Afterwards, remember the
		 * frequency offset for the drift file.
		 */
		memset((char *)&ntv,  0, sizeof ntv);
		ntv.modes = MOD_BITS;

		if (clock_offset < 0)
			dtemp = -.5;
		else
		    dtemp = .5;
#ifdef MOD_CANSCALE
		if (kernel_can_scale) {
			ntv.modes |= MOD_DOSCALE;
			ntv.offset = (int32)
			    (clock_offset * 1e6 * (1 << SHIFT_UPDATE) + dtemp);
		} else {
			ntv.offset = (int32)(clock_offset * 1e6 + dtemp);
		}
#else
		ntv.offset = (int32)(clock_offset * 1e6 + dtemp);
#endif

		ntv.esterror = (u_int32)(sys_error * 1e6);
		ntv.maxerror = (u_int32)((sys_rootdelay / 2 +
		    sys_rootdispersion) * 1e6);
		ntv.constant = sys_poll - 4;
		ntv.status = STA_PLL;
		if (pll_status & STA_PPSSIGNAL)
			ntv.status |= STA_PPSFREQ;
		if (pll_status & STA_PPSFREQ && pps_update)
			ntv.status |= STA_PPSTIME;

		/*
		 * Set the leap bits in the status word.
		 */
		if (sys_leap == LEAP_NOTINSYNC)
			ntv.status |= STA_UNSYNC;
		else if (calleapwhen(sys_reftime.l_ui) < CLOCK_DAY) {
			if (sys_leap & LEAP_ADDSECOND)
				ntv.status |= STA_INS;
			else if (sys_leap & LEAP_DELSECOND)
				ntv.status |= STA_DEL;
		}

		/*
		 * This astonishingly intricate wonder juggles the
		 * status bits so that the kernel loop behaves as the
		 * daemon loop; viz., selects the FLL when necessary,
		 * etc.
		 */
		if (sys_poll > NTP_MAXDPOLL)
			ntv.status |= STA_FLL;
		retval = ntp_adjtime(&ntv);
		if (retval < 0)
			msyslog(LOG_ERR, "local_clock: ntp_adjtime failed: %m");
		else if (retval == TIME_ERROR)
			if (ntv.status != pll_status)
				msyslog(LOG_ERR, "kernel pll status change %x",
				    ntv.status);
		pll_status = ntv.status;
		/*
		 * If we're close to perfect, don't ruin the integration:
		 * only pick up the kernel value if it would make a difference
		 */
		if (ntv.freq != (int32)(drift_comp * 65536e6))
			drift_comp = ntv.freq / 65536e6;

#ifdef MOD_CANSCALE
		kernel_can_scale = ntv.modes & MOD_CANSCALE;
#endif

		/*
		 * If the kernel pps discipline is working, monitor its
		 * performance.
		 */
		if (pll_status & STA_PPSTIME && pll_status & STA_PPSSIGNAL) {
			if (!pps_control)
				NLOG(NLOG_SYSEVENT)msyslog(LOG_INFO,
				    "pps sync enabled");
			pps_control = current_time;
			dtemp = ntv.offset / 1e6;
#ifdef MOD_CANSCALE
			if (ntv.modes & MOD_DOSCALE)
				dtemp /= (1 << SHIFT_UPDATE);
#endif
			record_peer_stats(&loopback_interface->sin,
					  ctlsysstatus(), dtemp, 0.,
					  ntv.jitter / 1e6, 0.);
		}
	}
#endif /* KERNEL_PLL */
#ifdef DEBUG
	if (debug)
		printf(
		    "local_clock: noise %.6f fll %.6f pll %.6f poll %d count %d\n",
		    sys_error, fllerror, pllerror, sys_poll, tc_counter);
#endif /* DEBUG */
	(void)record_loop_stats();
	return (0);
}


/*
 * adj_host_clock - Called once every second to update the local clock.
 */
void
adj_host_clock(void)
{
	double adjustment;

	/*
	 * Update the dispersion since the last update. In contrast to
	 * NTPv3, NTPv4 does not declare unsynchronized after one day,
	 * since the dispersion check serves this function. Also,
	 * since the poll interval can exceed one day, the old test
	 * would be counterproductive.
	 */
	sys_rootdispersion += CLOCK_PHI;

	/*
	 * Declare PPS kernel unsync if the pps signal has not been
	 * heard for a few minutes.
	 */
	if (pps_control && current_time - pps_control > PPS_MAXAGE) {
		if (pps_control)
			NLOG(NLOG_SYSEVENT) /* conditional if clause */
			    msyslog(LOG_INFO, "pps sync disabled");
		pps_control = 0;
	}
	if (!ntp_enable)
		return;

	/*
	 * If the phase-lock loop is implemented in the kernel, we
	 * have no business going further.
	 */
	if (pll_control && kern_enable)
		return;

	/*
	 * Intricate wrinkle. If the local clock driver is in use and
	 * selected for synchronization, somebody else may come tinker the
	 * adjtime() syscall. If this is the case, the driver is marked
	 * prefer and we have to avoid calling adjtime(), since that may
	 * truncate the other guy's requests.
	 */
	if (sys_peer != 0) {
		if (sys_peer->refclktype == REFCLK_LOCALCLOCK &&
		    sys_peer->flags & FLAG_PREFER)
			return;
	}
	adjustment = clock_offset / (CLOCK_PHASE * ULOGTOD(sys_poll));
	clock_offset -= adjustment;
	adj_systime(adjustment + drift_comp);
}


/*
 * Reset local clock variables and predictors.
 */
static void
rstclock(void)
{
	state = S_TSET;
	allan_xpt = CLOCK_ALLAN;
	tc_counter = 0;
	sys_poll = NTP_MINDPOLL;
	last_time = current_time;
	last_offset = clock_offset = 0;
	flladj = plladj = 0;
	sys_error = fllerror = pllerror = LOGTOD(sys_precision);
}


/*
 * loop_config - configure the loop filter
 */
void
loop_config(
	int item,
	double freq
	)
{
#if defined(KERNEL_PLL)
	struct timex ntv;
#endif /* KERNEL_PLL */

#ifdef DEBUG
	if (debug)
		printf("loop_config: state %d freq %.3f\n", item, freq * 1e6);
#endif
	if (item == LOOP_DRIFTCOMP)
		state = S_FSET;
	else
		state = S_NSET;
	switch (item) {

	    case LOOP_DRIFTINIT:
	    case LOOP_DRIFTCOMP:
		drift_comp = freq;
		if (drift_comp > sys_maxfreq)
			drift_comp = sys_maxfreq;
		if (drift_comp < -sys_maxfreq)
			drift_comp = -sys_maxfreq;

#ifdef KERNEL_PLL
		if (!kern_enable)
			return;

		/*
		 * If the phase-lock code is implemented in the kernel,
		 * give the time_constant and saved frequency offset to
		 * the kernel. If not, no harm is done.
		 */
		memset((char *)&ntv, 0, sizeof ntv);
		pll_control = 1;
		ntv.modes = MOD_BITS | MOD_FREQUENCY;
		ntv.freq = (int32)(drift_comp * 65536e6);
		ntv.maxerror = MAXDISPERSE;
		ntv.esterror = MAXDISPERSE;
		ntv.status = STA_UNSYNC | STA_PLL;
		ntv.constant = sys_poll - 4;
#ifdef SIGSYS
		newsigsys.sa_handler = pll_trap;
		newsigsys.sa_flags = 0;
		if ((sigaction(SIGSYS, &newsigsys, &sigsys))) {
			msyslog(LOG_ERR,
			    "sigaction() fails to save SIGSYS trap: %m");
			pll_control = 0;
			return;
		}

		/*
		 * Use sigsetjmp() to save state and then call
		 * ntp_adjtime(); if it fails, then siglongjmp() is used
		 * to return control
		 */
		if (sigsetjmp(env, 1) == 0)
			(void)ntp_adjtime(&ntv);
		if ((sigaction(SIGSYS, &sigsys, (struct sigaction *)NULL))) {
			msyslog(LOG_ERR,
			    "sigaction() fails to restore SIGSYS trap: %m");
			pll_control = 0;
			return;
		}
#else /* SIGSYS */
		if (ntp_adjtime(&ntv) < 0) {
			msyslog(LOG_ERR,
			    "loop_config: ntp_adjtime() failed: %m");
			pll_control = 0;
		}
#endif /* SIGSYS */
		if (pll_control)
			msyslog(LOG_NOTICE,
		 	    "using kernel phase-lock loop %04x", ntv.status);
#endif /* KERNEL_PLL */
	}
}


#if defined(KERNEL_PLL) && defined(SIGSYS)
/*
 * _trap - trap processor for undefined syscalls
 *
 * This nugget is called by the kernel when the SYS_ntp_adjtime()
 * syscall bombs because the silly thing has not been implemented in
 * the kernel. In this case the phase-lock loop is emulated by
 * the stock adjtime() syscall and a lot of indelicate abuse.
 */
RETSIGTYPE
pll_trap(
	int arg
	)
{
	pll_control = 0;
	siglongjmp(env, 1);
}
#endif /* KERNEL_PLL && SIGSYS */
