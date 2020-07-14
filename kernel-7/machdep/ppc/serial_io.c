/*
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
 */
/*
 *	File: scc_8530_hdw.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	6/91
 *
 *	Hardware-level operations for the SCC Serial Line Driver
 */

#define	NSCC	1	/* Number of serial chips, two ports per chip. */
#if	NSCC > 0

/* #include <mach_kgdb.h> */

/* #include <platforms.h> */

/* #include <mach_kdb.h> */
/* #include <kgdb/gdb_defs.h> */
/* #include <kgdb/kgdb_defs.h> */    /* For kgdb_printf */

#include <kernserv/machine/spl.h>
#include <mach/std_types.h>
/* #include <types.h> */
/* #include <sys/syslog.h> */
/* #include <device/io_req.h> */
/* #include <device/tty.h> */
/* #include <chips/busses.h> */
/* #include <ppc/misc_protos.h> */
#include <machdep/ppc/proc_reg.h>
#include <mach/machine/exception.h>
#include <machdep/ppc/serial_io.h>
#include <machdep/ppc/powermac.h>
#include <machdep/ppc/interrupts.h>
#include <machdep/ppc/scc_8530.h>

#define	kdebug_state()	(1)
#define	scc_delay(x)	{ volatile int _d_; for (_d_ = 0; _d_ < (10000*x); _d_++) ; }

#define	NSCC_LINE	2	/* 2 ttys per chip */

#if	!MACH_KGDB
#define	SCC_DMA_TRANSFERS	1
#else
/* 
 * Don't deal with DMA because of the way KGDB needs to handle
 * the system and serial ports.
 */
#define	SCC_DMA_TRANSFERS	0
#endif
  
/*struct tty scc_tty[NSCC_LINE];*/

#define scc_dev_no(chan)	(chan)
/* #define scc_tty_for(chan)	(&scc_tty[chan]) */
#define scc_unit(dev_no)	(dev_no)
#define scc_chan(dev_no)	(dev_no)

int	serial_initted = 0;

static struct scc_byte {
	unsigned char	reg;
	unsigned char	val;
} scc_init_hw[] = {
	9, 0x40,	/* SCC_WR9_RESET_CHA_B */
	4, 0x44,	/* SCC_WR4_CLK_x16| SCC_WR4_1_STOP, */
	3, 0xC0,	/* SCC_WR3_RX_8_BITS| */
	5, 0xE2,	/* SCC_WR5_DTR| SCC_WR5_TX_8_BITS| SCC_WR5_RTS, */
	2, 0x00,
	10, 0x00,	/* SCC_WR10_NRZ| SCC_WR10_8BIT_SYNCH, */
	11, 0x50,	/* SCC_WR11_RCLK_BAUDR| SCC_WR11_XTLK_BAUDR|
			   SCC_WR11_XTLK_RTc_PIN| SCC_WR11_TRcOUT_XTAL, */
	12, 0x01,
	13, 0x00,
	3, 0xC1,	/* SCC_WR3_RX_8_BITS| SCC_WR3_RX_ENABLE, */
	5, 0xEA,	/* SCC_WR5_DTR| SCC_WR5_TX_8_BITS|
			   SCC_WR5_TX_ENABLE| SCC_WR5_RTS, */
	14, 0x01,	/* SCC_WR14_BAUDR_ENABLE, */
	15, 0x00,
	0, 0x10,	/* SCC_RESET_EXT_IP, */
	0, 0x10,	/* SCC_RESET_EXT_IP, */
	1, 0x12,	/* SCC_WR1_RXI_ALL_CHAR|SCC_WR1_TX_IE, */
	9, 0x0A		/* SCC_WR9_MASTER_IE| SCC_WR9_NV, */

};

static int	scc_init_hw_count = sizeof(scc_init_hw)/sizeof(scc_init_hw[0]);

enum scc_error {SCC_ERR_NONE, SCC_ERR_PARITY, SCC_ERR_BREAK, SCC_ERR_OVERRUN};


/*
 * BRG formula is:
 *				ClockFrequency (115200 for Power Mac)
 *	BRGconstant = 	---------------------------  -  2
 *			      BaudRate
 */

#define SERIAL_CLOCK_FREQUENCY (115200*2) /* Power Mac value */
#define	convert_baud_rate(rate)	((((SERIAL_CLOCK_FREQUENCY) + (rate)) / (2 * (rate))) - 2)

#define DEFAULT_SPEED 9600
#define DEFAULT_FLAGS (TF_LITOUT|TF_ECHO)

#ifdef notdef_next
void	scc_attach(struct bus_device *ui );
void	scc_set_modem_control(scc_softc_t scc, boolean_t on);
int	scc_pollc(int unit, boolean_t on);
int	scc_param(struct tty *tp);
int	scc_mctl(struct tty* tp, int bits, int how);
int	scc_cd_scan(void);
void	scc_start(struct tty *tp);
void	scc_intr(int device, struct ppc_saved_state *);
int	scc_simple_tint(dev_t dev, boolean_t all_sent);
void	scc_input(dev_t dev, int c, enum scc_error err);
void	scc_stop(struct tty *tp, int flags);
void	scc_update_modem(struct tty *tp);
void	scc_waitforempty(struct tty *tp);
#endif /* notdef_next */

struct scc_softc	scc_softc[NSCC];
caddr_t	scc_std[NSCC] = { (caddr_t) 0};

#ifdef notdef_next
/*
 * Definition of the driver for the auto-configuration program.
 */

struct	bus_device *scc_info[NSCC];

struct	bus_driver scc_driver =
        {scc_probe,
	 0,
	scc_attach,
	 0,
	 scc_std,
	"scc",
	scc_info,
	0,
	0,
	0
};
#endif /* notdef_next */

#if	SCC_DMA_TRANSFERS

extern struct scc_dma_ops	scc_amic_ops /*, scc_db_ops*/;
#endif

/*
 * Adapt/Probe/Attach functions
 */
boolean_t	scc_uses_modem_control = FALSE;/* patch this with adb */

/* This is called VERY early on in the init and therefore has to have
 * hardcoded addresses of the serial hardware control registers. The
 * serial line may be needed for console and debugging output before
 * anything else takes place
 */

void
initialize_serial()
{
	int i, chan, bits;
	scc_regmap_t	regs;
	/* static struct bus_device d; */ 

	if (serial_initted) 
		return;

	// If this machine has PMU then turn on the serial ports.
	if (HasPMU()) {
	  volatile unsigned long *ohareFeatureCntl;
	  
	  ohareFeatureCntl = powermac_io_info.io_base_phys + 0x38;
	  
	  *ohareFeatureCntl &= ~(1 << 24);
	  eieio();
	  *ohareFeatureCntl |= (1 << 17) | ( 1 << 22) | (1 << 23);
	  eieio();
	}

	scc_softc[0].full_modem = TRUE;

	scc_std[0] = (caddr_t) PCI_SCC_BASE_PHYS;

	regs = scc_softc[0].regs = (scc_regmap_t)scc_std[0];

	scc_write_reg(regs, 0, 9, 0xc0); /* hard reset */
	scc_delay(100);
	for (chan = 0; chan < NSCC_LINE; chan++) {
		if (chan == 1)
			scc_init_hw[0].val = 0x80;

		for (i = 0; i < scc_init_hw_count; i++) {
			if (scc_init_hw[i].reg == 0xff) {
				scc_delay(100);
			} else
			scc_write_reg(regs, chan,
				      scc_init_hw[i].reg, scc_init_hw[i].val);
		}
	}

#ifdef notdef_next
	/* Call probe so we are ready very early for remote gdb and for serial
	   console output if appropriate.  */
	/* d.unit = 0; */
	if (scc_probe(0, (void *) &d)) {
		for (i = 0; i < NSCC_LINE; i++) {
			scc_softc[0].softr[i].wr5 = SCC_WR5_DTR | SCC_WR5_RTS;
			scc_param(scc_tty_for(i));
	/* Enable SCC interrupts (how many interrupts are to this thing?!?) */
			scc_write_reg(regs,  i,  9, SCC_WR9_NV);

			scc_read_reg_zero(regs, 0, bits);/* Clear the status */
		}
	}
#endif /* notdef_next */

	serial_initted = TRUE;
	return;
}

#ifdef notdef_next
int
scc_probe(caddr_t  xxx, void *param)
{
	struct bus_device *ui = (struct bus_device *) param;
	scc_softc_t     scc;
	register int	val, i;
	register scc_regmap_t	regs;
	spl_t	s;

	/* Readjust the I/O address to handling 
	 * new memory mappings.
	 */

	scc_std[0] = POWERMAC_IO(scc_std[0]);

	regs = (scc_regmap_t)scc_std[0];

	if (regs == (scc_regmap_t) 0) {
		return 0;
	}

	scc = &scc_softc[0];
	scc->regs = regs;

	if (scc->probed_once++){
		/* Second time in means called from system */

		switch (powermac_info.class) {
		case	POWERMAC_CLASS_PDM:
#if	SCC_DMA_TRANSFERS
			scc_softc[0].dma_ops = &scc_amic_ops;
#endif
			pmac_register_int(PMAC_DEV_SCC, SPLTTY,
					  (void (*)(int, void *))scc_intr);
			break;

		case	POWERMAC_CLASS_PCI:
#if	SCC_DMA_TRANSFERS
			/*scc_softc[0].dma_ops = &scc_db_ops;*/
#endif
			pmac_register_int(PMAC_DEV_SCC_A, SPLTTY,
					  (void (*)(int, void *))scc_intr);
			pmac_register_int(PMAC_DEV_SCC_B, SPLTTY,
					  (void (*)(int, void *))scc_intr);
			break;
		default:
			panic("unsupported class for serial code\n");
		}

		return 1;
	}

	s = spltty();

	for (i = 0; i < NSCC_LINE; i++) {
		register struct tty	*tp;
		tp = scc_tty_for(i);
		simple_lock_init(&tp->t_lock);
		tp->t_addr = (char*)(0x80000000L + (i&1));
		/* Set default values.  These will be overridden on
		   open but are needed if the port will be used
		   independently of the Mach interfaces, e.g., for
		   gdb or for a serial console.  */
		tp->t_ispeed = DEFAULT_SPEED;
		tp->t_ospeed = DEFAULT_SPEED;
		tp->t_flags = DEFAULT_FLAGS;
		scc->softr[i].speed = -1;

		/* do min buffering */
		tp->t_state |= TS_MIN;

		tp->t_dev = scc_dev_no(i);
	}

	splx(s);

	return 1;
}

boolean_t scc_timer_started = FALSE;

void
scc_attach( register struct bus_device *ui )
{
	extern int tty_inq_size, tty_outq_size;
	int i;
	struct tty *tp;

#if	SCC_DMA_TRANSFERS
	/* DMA Serial can send a lot... ;-) */
	tty_inq_size = 16384;
	tty_outq_size = 16384;

	for (i = 0; i < NSCC_LINE; i++) {
		if (scc_softc[0].dma_ops) {
			scc_softc[0].dma_ops->scc_dma_init(i);
			scc_softc[0].dma_initted |= (1<<i);
		}
	}
#endif

	if (!scc_timer_started) {
		/* do all of them, before we call scc_scan() */
		/* harmless if done already */
		for (i = 0; i < NSCC_LINE; i++)  {
			tp = scc_tty_for(i);
			ttychars(tp);
			/* hack MEB 1/5/96 */
			tp->t_state |= TS_CARR_ON;
			scc_softc[0].modem[i] = 0;
		}

		scc_timer_started = TRUE;
		scc_cd_scan();
	}

	printf("\n sl0: ( alternate console )\n sl1:");
	return;
}

/*
 * Would you like to make a phone call ?
 */

void
scc_set_modem_control(scc, on)
	scc_softc_t      scc;
	boolean_t	on;
{
	scc->full_modem = on;
	/* user should do an scc_param() ifchanged */
}

/*
 * Polled I/O (debugger)
 */

int
scc_pollc(int unit, boolean_t on)
{
	scc_softc_t		scc;

	scc = &scc_softc[unit];
	if (on) {
		scc->polling_mode++;
	} else
		scc->polling_mode--;

	return 0;
}

/*
 * Interrupt routine
 */
int scc_intr_count;

void
scc_intr(int device, struct ppc_saved_state *ssp)
{
	int			chan;
	scc_softc_t		scc = &scc_softc[0];
	register scc_regmap_t	regs = scc->regs;
	register int		rr1, rr2, status;
	register int		c;

scc_intr_count++;

	scc_read_reg_zero(regs, 0, status);/* Clear the status */

	scc_read_reg(regs, SCC_CHANNEL_B, SCC_RR2, rr2);

	rr2 = SCC_RR2_STATUS(rr2);

	/*printf("{INTR %x}", rr2);*/
	if ((rr2 == SCC_RR2_A_XMIT_DONE) || (rr2 == SCC_RR2_B_XMIT_DONE)) {

		chan = (rr2 == SCC_RR2_A_XMIT_DONE) ?
					SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_TX_IP);

		c = scc_simple_tint(scc_dev_no(chan), FALSE);

		if (c == -1) {
			/* no more data for this line */
			c = scc->softr[chan].wr1 & ~SCC_WR1_TX_IE;
			scc_write_reg(regs, chan, SCC_WR1, c);
			scc->softr[chan].wr1 = c;

			c = scc_simple_tint(scc_dev_no(chan), TRUE);
			if (c != -1) {
				/* funny race, scc_start has been called
				   already */
				scc_write_data(regs, chan, c);
			}
		} else {

			scc_write_data(regs, chan, c);
			/* and leave it enabled */
		}
	}

	else if (rr2 == SCC_RR2_A_RECV_DONE || rr2 == SCC_RR2_B_RECV_DONE) {
		int	err = 0;
		chan = (rr2 == SCC_RR2_A_RECV_DONE) ?
					SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0,
			      SCC_RESET_HIGHEST_IUS);

#if MACH_KGDB
		if (chan == KGDB_PORT) {
			/* 11/10/95 MEB
			 * Drop into the debugger.. scc_getc() will
			 * pick up the character
			 */

			call_kgdb_with_ctx(EXC_INTERRUPT, 0, ssp);
			goto next_intr;
		}
#endif

		scc_read_data(regs, chan, c);

		scc_input(scc_dev_no(chan), c, SCC_ERR_NONE);
	}

	else if ((rr2 == SCC_RR2_A_EXT_STATUS) ||
		 (rr2 == SCC_RR2_B_EXT_STATUS)) {
		chan = (rr2 == SCC_RR2_A_EXT_STATUS) ?
			SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_read_reg(regs, chan, SCC_RR0, status);
		if (status & SCC_RR0_TX_UNDERRUN)
			scc_write_reg(regs, chan, SCC_RR0,
				      SCC_RESET_TXURUN_LATCH);
		if (status & SCC_RR0_BREAK)
			scc_input(scc_dev_no(chan), 0, SCC_ERR_BREAK);

		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_EXT_IP);
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);

		/* Update the modem lines */
		scc_update_modem(&scc_tty[chan]);
	}

	else if ((rr2 == SCC_RR2_A_RECV_SPECIAL) ||
		 (rr2 == SCC_RR2_B_RECV_SPECIAL)) {
		chan = (rr2 == SCC_RR2_A_RECV_SPECIAL) ?
			SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_read_reg(regs, chan, SCC_RR1, rr1);
#if SCC_DMA_TRANSFERS
		if (scc->dma_initted & (chan<<1)) {
			scc->dma_ops->scc_dma_reset_rx(chan);
			scc->dma_ops->scc_dma_start_rx(chan);
		}
#endif
		if (rr1 & (SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
			enum scc_error err;
			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);
			if (rr1 & SCC_RR1_FRAME_ERR)
				err = SCC_ERR_BREAK;
			else if (rr1 & SCC_RR1_PARITY_ERR)
				err = SCC_ERR_OVERRUN;
			else {
				assert(rr1 & SCC_RR1_RX_OVERRUN);
				err = SCC_ERR_OVERRUN;
			}
#if SCC_DMA_TRANSFERS
			if ((scc->dma_initted & (chan<<1)) == 0)
#endif
				scc_input(scc_dev_no(chan), 0, err);
		}
		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	}

next_intr:
	return;
}


/*
 * Start output on a line
 */

void
scc_start(tp)
	struct tty *tp;
{
	spl_t			s;
	int			cc;
	scc_regmap_t	regs;
	int			chan = scc_chan(tp->t_dev), temp;
	struct scc_softreg	*sr = &scc_softc[0].softr[chan];
	scc_softc_t		scc = &scc_softc[0];

	s = spltty();

	/* Start up the DMA channel if it was paused */
	if ((tp->t_state & TS_TTSTOP) == 0 && sr->dma_flags & SCC_FLAGS_DMA_PAUSED) {
		/*printf("{DMA RESUME}");*/
		scc->dma_ops->scc_dma_continue_tx(chan);
		splx(s);
		return;
	}

	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) 
		goto out;


#if	SCC_DMA_TRANSFERS
	if (scc_softc[0].dma_initted & (1<<chan)) {
 	 	/* Don't worry about low water marks...
	  	 * The DMA operation should be able to pull off most
	  	 * if not all of the TTY output queue
	  	*/
	
		tt_write_wakeup(tp);

		if (tp->t_outq.c_cc <= 0) 
			goto out;

		tp->t_state |= TS_BUSY;

		scc_softc[0].dma_ops->scc_dma_start_tx(chan, tp);
	} else 
#endif
	{
		cc = tp->t_outq.c_cc;
		if (cc <= TTLOWAT(tp)) {
			tt_write_wakeup(tp);
		}
		if (cc <= 0)
			goto out;
		tp->t_state |= TS_BUSY;
	
		regs = scc_softc[0].regs;
		sr   = &scc_softc[0].softr[chan];
	
		scc_read_reg(regs, chan, SCC_RR15, temp);
		temp |= SCC_WR15_TX_UNDERRUN_IE;
		scc_write_reg(regs, chan, SCC_WR15, temp);
	
		temp = sr->wr1 | SCC_WR1_TX_IE;
		scc_write_reg(regs, chan, SCC_WR1, temp);
		sr->wr1 = temp;
	
		/* but we need a first char out or no cookie */
		scc_read_reg(regs, chan, SCC_RR0, temp);
		if (temp & SCC_RR0_TX_EMPTY)
		{
			register char	c;
	
			c = getc(&tp->t_outq);
			scc_write_data(regs, chan, c);
		}
	}
out:
	splx(s);
}

#define	u_min(a,b)	((a) < (b) ? (a) : (b))
#endif /* notdef_next */

/*
 * Get a char from a specific SCC line
 * [this is only used for console&screen purposes]
 */

int
scc_getc(int unit, int line, boolean_t wait, boolean_t raw)
{
	register scc_regmap_t	regs;
	unsigned char   c, value;
	int             rcvalue, from_line;
	spl_t		s = spltty();

	regs = scc_softc[0].regs;

	/*
	 * wait till something available
	 *
	 */
again:
	rcvalue = 0;
	while (1) {
		scc_read_reg_zero(regs, line, value);

		if (value & SCC_RR0_RX_AVAIL)
			break;

		if (!wait) {
			splx(s);
			return -1;
		}
	}

	/*
	 * if nothing found return -1
	 */

	scc_read_reg(regs, line, SCC_RR1, value);
	scc_read_data(regs, line, c);

	/*
	 * bad chars not ok
	 */
	if (value&(SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
		scc_write_reg(regs, line, SCC_RR0, SCC_RESET_ERROR);

		if (wait) {
			scc_write_reg(regs, line, SCC_RR0, SCC_RESET_HIGHEST_IUS);
			goto again;
		}
	}

	scc_write_reg(regs, line, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	splx(s);

	return c;
}

/*
 * Put a char on a specific SCC line
 */

int
scc_putc(int unit, int line, int c)
{
	scc_regmap_t	regs;
	spl_t            s = spltty();
	unsigned char	 value;

	regs = scc_softc[0].regs;

	do {
		scc_read_reg(regs, line, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
		scc_delay(100);
	} while (1);

	scc_write_data(regs, line, c);
/* wait for it to swallow the char ? */

	do {
		scc_read_reg(regs, line, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
	} while (1);
	scc_write_reg(regs, line, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	splx(s);

	return 0;
}

#ifdef notdef_next
int
scc_param(struct tty *tp)
{
	scc_regmap_t	regs;
	unsigned char	value;
	unsigned short	speed_value;
	int		bits, chan;
	spl_t		s;
	struct scc_softreg	*sr;
	scc_softc_t	scc;

	chan = scc_chan(tp->t_dev);
	scc = &scc_softc[0];
	regs = scc->regs;

	sr = &scc->softr[chan];

	/* Do a quick check to see if the hardware needs to change */
	if ((sr->flags & (TF_ODDP|TF_EVENP)) == (tp->t_flags & (TF_ODDP|TF_EVENP))
	&& sr->speed == tp->t_ispeed) 
		return 0;

	sr->flags = tp->t_flags;
	sr->speed = tp->t_ispeed;

	s = spltty();

	if (tp->t_ispeed == 0) {
		sr->wr5 &= ~SCC_WR5_DTR;
		scc_write_reg(regs,  chan, 5, sr->wr5);
		splx(s);

		return 0;
	}
	

#if	SCC_DMA_TRANSFERS
	if (scc->dma_initted & (1<<chan)) 
		scc->dma_ops->scc_dma_reset_rx(chan);
#endif

	value = SCC_WR4_1_STOP;

	/* 
	 * For 115K the clocking divide changes to 64.. to 230K will
	 * start at the normal clock divide 16.
	 *
	 * However, both speeds will pull from a different clocking
	 * source
	 */

	if (tp->t_ispeed == 115200)
		value |= SCC_WR4_CLK_x32;
	else	
		value |= SCC_WR4_CLK_x16 ;

	/* .. and parity */
	if ((tp->t_flags & (TF_ODDP | TF_EVENP)) == TF_EVENP)
		value |= (SCC_WR4_EVEN_PARITY |  SCC_WR4_PARITY_ENABLE);
	else if ((tp->t_flags & (TF_ODDP | TF_EVENP)) == TF_ODDP)
		value |= SCC_WR4_PARITY_ENABLE;

	/* set it now, remember it must be first after reset */
	sr->wr4 = value;

	/* Program Parity, and Stop bits */
	scc_write_reg(regs,  chan, 4, sr->wr4);

	/* Setup for 8 bits */
	scc_write_reg(regs,  chan, 3, SCC_WR3_RX_8_BITS);

	// Set DTR, RTS, and transmitter bits/character.
	sr->wr5 = SCC_WR5_TX_8_BITS | SCC_WR5_RTS | SCC_WR5_DTR;

	scc_write_reg(regs,  chan, 5, sr->wr5);
	
	scc_write_reg(regs, chan, 14, 0);	/* Disable baud rate */

	/* Setup baud rate 57.6Kbps, 115K, 230K should all yeild
	 * a converted baud rate of zero
	 */
	speed_value = convert_baud_rate(tp->t_ispeed);

	if (speed_value == 0xffff)
		speed_value = 0;

	scc_set_timing_base(regs, chan, speed_value);
	
	if (tp->t_ispeed == 115200 || tp->t_ispeed == 230400) {
		/* Special case here.. change the clock source*/
		scc_write_reg(regs, chan, 11, 0);
		/* Baud rate generator is disabled.. */
	} else {
		scc_write_reg(regs, chan, 11, SCC_WR11_RCLK_BAUDR|SCC_WR11_XTLK_BAUDR);
		/* Enable the baud rate generator */
		scc_write_reg(regs,  chan, 14, SCC_WR14_BAUDR_ENABLE);
	}


	scc_write_reg(regs,  chan,  3, SCC_WR3_RX_8_BITS|SCC_WR3_RX_ENABLE);


	sr->wr1 = SCC_WR1_RXI_ALL_CHAR | SCC_WR1_EXT_IE;
	scc_write_reg(regs,  chan,  1, sr->wr1);
	
	scc_write_reg(regs,  chan, 15, 0);

	/* Clear out any pending external or status interrupts */
	scc_write_reg(regs,  chan,  0, SCC_RESET_EXT_IP);
	scc_write_reg(regs,  chan,  0, SCC_RESET_EXT_IP);
	//scc_write_reg(regs,  chan,  0, SCC_RESET_ERROR);
	scc_write_reg(regs,  chan,  0, SCC_IE_NEXT_CHAR);

	/* Enable SCC interrupts (how many interrupts are to this thing?!?) */
	scc_write_reg(regs,  chan,  9, SCC_WR9_MASTER_IE|SCC_WR9_NV);

	scc_read_reg_zero(regs, 0, bits);/* Clear the status */

#if	SCC_DMA_TRANSFERS
	if (scc->dma_initted & (1<<chan))  {
		scc->dma_ops->scc_dma_start_rx(chan);
		scc->dma_ops->scc_dma_setup_8530(chan);
	} else
#endif
	{
		sr->wr1 = SCC_WR1_RXI_ALL_CHAR | SCC_WR1_EXT_IE;
		scc_write_reg(regs, chan, 1, sr->wr1);
	}

	sr->wr5 |= SCC_WR5_TX_ENABLE;
	scc_write_reg(regs,  chan,  5, sr->wr5);

	splx(s);

	return 0;

}

void
scc_update_modem(struct tty *tp)
{
	scc_softc_t scc = &scc_softc[0];
	int chan = scc_chan(tp->t_dev);
	scc_regmap_t	regs = scc->regs;
	unsigned char rr0, old_modem;

	old_modem = scc->modem[chan];
	scc->modem[chan] &= ~(TM_CTS|TM_CAR|TM_RNG|TM_DSR);
	scc->modem[chan] |= TM_DSR|TM_CTS;

	scc_read_reg_zero(regs, chan, rr0);

	if (rr0 & SCC_RR0_DCD) {
		scc->modem[chan] |= TM_CAR;
		if ((old_modem & TM_CAR) == 0) {
			/*printf("{DTR-ON %x/%x}", rr0, old_modem);*/
			/*
			 * The trick here is that
			 * the device_open does not hang
			 * waiting for DCD, but a message
			 * is sent to the process 
			 */

			if ((tp->t_state & (TS_ISOPEN|TS_WOPEN))
			&& tp->t_flags & TF_OUT_OF_BAND) {
				/*printf("{NOTIFY}");*/
				tp->t_outofband = TOOB_CARRIER;
				tp->t_outofbandarg = TRUE;
				tty_queue_completion(&tp->t_delayed_read);
			}
		}
	} else if (old_modem & TM_CAR) {
		if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
			/*printf("{DTR-OFF %x/%x}", rr0, old_modem);*/

			if (tp->t_flags & TF_OUT_OF_BAND) {
				tp->t_outofband = TOOB_CARRIER;
				tp->t_outofbandarg = FALSE;
				tty_queue_completion(&tp->t_delayed_read);
			} else
				ttymodem(tp, FALSE);
		}
	}
}

/*
 * Modem control functions
 */
int
scc_mctl(struct tty* tty, int bits, int how)
{
	register dev_t dev = tty->t_dev;
	int sccline;
	register int tcr, msr, brk, n_tcr, n_brk;
	int b;
	scc_softc_t      scc;
	int wr5;

	sccline = scc_chan(dev);

	if (bits == TM_HUP) {	/* close line (internal) */
	    bits = TM_DTR | TM_RTS;
	    how = DMBIC;
	}

	scc = &scc_softc[0];
 	wr5 = scc->softr[sccline].wr5;

	if (how == DMGET) {
	    scc_update_modem(tty);
	    return scc->modem[sccline];
	}

	switch (how) {
	case DMSET:
	    b = bits; break;
	case DMBIS:
	    b = scc->modem[sccline] | bits; break;
	case DMBIC:
	    b = scc->modem[sccline] & ~bits; break;
	default:
	    return 0;
	}

	if (scc->modem[sccline] == b)
	    return b;

	scc->modem[sccline] = b;

	if (bits & TM_BRK) {
	    ttydrain(tty);
	    scc_waitforempty(tty);
	}

	wr5 &= ~(SCC_WR5_SEND_BREAK|SCC_WR5_DTR);

	if (b & TM_BRK)
		wr5 |= SCC_WR5_SEND_BREAK;

	if (b & TM_DTR)
		wr5 |= SCC_WR5_DTR;

	wr5 |= SCC_WR5_RTS;

	scc_write_reg(scc->regs, sccline, 5, wr5);
	scc->softr[sccline].wr5 = wr5;

	return scc->modem[sccline];
}

/*
 * Periodically look at the CD signals:
 * they do generate interrupts but we
 * must fake them on channel A.  We might
 * also fake them on channel B.
 */

int
scc_cd_scan(void)
{
	spl_t s = spltty();
	scc_softc_t	scc;
	int		j;

	scc = &scc_softc[0];
	for (j = 0; j < NSCC_LINE; j++) {
		if (scc_tty[j].t_state & (TS_ISOPEN|TS_WOPEN))
			scc_update_modem(&scc_tty[j]);
	}
	splx(s);

	timeout((timeout_fcn_t)scc_cd_scan, (void *)0, hz/4);

	return 0;
}

#if MACH_KGDB
void no_spl_scc_putc(int chan, char c)
{
	register scc_regmap_t	regs;
	register unsigned char	value;

	if (!serial_initted)
		initialize_serial();

	regs = scc_softc[0].regs;

	do {
		scc_read_reg(regs, chan, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
		scc_delay(100);
	} while (1);

	scc_write_data(regs, chan, c);
/* wait for it to swallow the char ? */

	do {
		scc_read_reg(regs, chan, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
	} while (1);
	scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);

	if (c == '\n')
		no_spl_scc_putc(chan, '\r');
 
}

#define	SCC_KGDB_BUFFER	15

int no_spl_scc_getc(int chan, boolean_t timeout)
{
	register scc_regmap_t	regs;
	unsigned char   c, value, i;
	int             rcvalue, from_line;
	int	timeremaining = timeout ? 10000000 : 0;	/* XXX */
	static unsigned char	buffer[2][SCC_KGDB_BUFFER];
	static int		bufcnt[2], bufidx[2];

	/* This should be rewritten to produce a constant timeout
	   regardless of the processor speed.  */

	if (!serial_initted)
		initialize_serial();

	regs = scc_softc[0].regs;

get_char:
	if (bufcnt[chan]) {
		bufcnt[chan] --;
		return ((unsigned int) buffer[chan][bufidx[chan]++]);
	}

	/*
	 * wait till something available
	 *
	 */
	bufidx[chan] = 0;

	for (i = 0; i < SCC_KGDB_BUFFER; i++) {
		rcvalue = 0;

		while (1) {
			scc_read_reg_zero(regs, chan, value);
			if (value & SCC_RR0_RX_AVAIL)
				break;
			if (timeremaining && !--timeremaining) {
				if (i)
					goto get_char;
				else
					return KGDB_GETC_TIMEOUT;
			}
		}

		scc_read_reg(regs, chan, SCC_RR1, value);
		scc_read_data(regs, chan, c);
		buffer[chan][bufcnt[chan]] = c;
		bufcnt[chan]++;

		/*
	 	 * bad chars not ok
	 	 */


		if (value&(SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);

			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);
			bufcnt[chan] = 0;
			return KGDB_GETC_BAD_CHAR;
		}

			
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		
		for (timeremaining = 0; timeremaining < 1000; timeremaining++) {
			scc_read_reg_zero(regs, chan, value);

			if (value & SCC_RR0_RX_AVAIL)
				continue;
		}

		if (timeout == FALSE)
			break;

	}


	goto get_char;
}
#endif	/* MACH_KGDB */

/*
 * Open routine
 */

io_return_t
scc_open(
	dev_t		dev,
	dev_mode_t	flag,
	io_req_t	ior)
{
	register struct tty	*tp;
	spl_t			s;
	scc_softc_t		scc;
	int			chan;
	int			forcedcarrier;
	io_return_t		result;

	if (dev >= NSCC * NSCC_LINE)
		return D_NO_SUCH_DEVICE;

	chan = scc_chan(dev);
	tp = &scc_tty[chan];
	scc = &scc_softc[0];

	/* But was it there at probe time */
	if (tp->t_addr == 0)
		return D_NO_SUCH_DEVICE;

	s = spltty();
	simple_lock(&tp->t_lock);

	if (!(tp->t_state & (TS_ISOPEN|TS_WOPEN))) {
		tp->t_dev	= dev;
		tp->t_start	= scc_start;
		tp->t_stop	= scc_stop;
		tp->t_mctl	= scc_mctl;
		tp->t_getstat	= scc_get_status;
		tp->t_setstat	= scc_set_status;
		scc->modem[chan] = 0;	/* No assumptions on things.. */
		if (tp->t_ispeed == 0) {
			tp->t_ispeed = DEFAULT_SPEED;
			tp->t_ospeed = DEFAULT_SPEED;
			tp->t_flags = DEFAULT_FLAGS;
		}

		scc->softr[chan].speed = -1;	/* Force reset */
		scc->softr[chan].wr5 |= SCC_WR5_DTR;
		scc_param(tp);
	}

	scc_update_modem(tp);

	tp->t_state |= TS_CARR_ON;	/* Always.. */

	simple_unlock(&tp->t_lock);
	splx(s);
	result = char_open(dev, tp, flag, ior);

	if (tp->t_flags & CRTSCTS) {
		simple_lock(&tp->t_lock);
		if (!(scc->modem[chan] & TM_CTS)) 
			tp->t_state |= TS_TTSTOP;
		simple_unlock(&tp->t_lock);
	}

	return result;
}

/*
 * Close routine
 */
void
scc_close(
	dev_t	dev)
{
	register struct tty	*tp;
	spl_t			s;
	scc_softc_t		scc = &scc_softc[0];
	int			chan = scc_chan(dev);

	tp = &scc_tty[dev];

	s = spltty();
	simple_lock(&tp->t_lock);

	ttstart(tp);
	ttydrain(tp);
	scc_waitforempty(tp);

	/* Disable Receiver.. */
	scc_write_reg(scc->regs, chan, SCC_WR3, 0);
#if SCC_DMA_TRANSFERS
	if (scc->dma_initted & (chan <<1))
		scc->dma_ops->scc_dma_reset_rx(chan);
#endif

	ttyclose(tp);
	if (tp->t_state & TS_HUPCLS) {
		scc->softr[chan].wr5 &= ~(SCC_WR5_DTR);
		scc_write_reg(scc->regs, chan, SCC_WR5, scc->softr[chan].wr5);
		scc->modem[chan] &= ~(TM_DTR|TM_RTS);
	}


	tp->t_state &= ~TS_ISOPEN;

	simple_unlock(&tp->t_lock);
	splx(s);
}

io_return_t
scc_read(
	dev_t		dev,
	io_req_t	ior)
{
	return char_read(&scc_tty[dev], ior);
}

io_return_t
scc_write(
	dev_t		dev,
	io_req_t	ior)
{
	return char_write(&scc_tty[dev], ior);
}

/*
 * Stop output on a line.
 */
void
scc_stop(
	struct tty	*tp,
	int		flags)
{
	int chan = scc_chan(tp->t_dev);
	scc_softc_t scc = &scc_softc[0];
	struct scc_softreg *sr = &scc->softr[chan];

	spl_t s = spltty();

	if (tp->t_state & TS_BUSY) {
		if (sr->dma_flags & SCC_FLAGS_DMA_TX_BUSY) {
			/*printf("{DMA OFF}");*/
			scc->dma_ops->scc_dma_pause_tx(chan);
		} else if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}

	splx(s);
	/*printf("{STOP %x}", flags);*/
}

/*
 * Abnormal close
 */
boolean_t
scc_portdeath(
	dev_t		dev,
	ipc_port_t	port)
{
	return (tty_portdeath(&scc_tty[dev], port));
}

/*
 * Get/Set status rotuines
 */
io_return_t
scc_get_status(
	dev_t			dev,
	dev_flavor_t		flavor,
	dev_status_t		data,
	mach_msg_type_number_t	*status_count)
{
	register struct tty *tp;

	tp = &scc_tty[dev];

	switch (flavor) {
	case TTY_MODEM:
		scc_update_modem(tp);
		*data = scc_softc[0].modem[scc_chan(dev)];
		*status_count = 1;
		return (D_SUCCESS);
	default:
		return (tty_get_status(tp, flavor, data, status_count));
	}
}

io_return_t
scc_set_status(
	dev_t			dev,
	dev_flavor_t		flavor,
	dev_status_t		data,
	mach_msg_type_number_t	status_count)
{
	register struct tty *tp;
	spl_t s;
	io_return_t result = D_SUCCESS;
	scc_softc_t scc = &scc_softc[0];
	int chan = scc_chan(dev);

	tp = &scc_tty[dev];

	s = spltty();
	simple_lock(&tp->t_lock);

	switch (flavor) {
	case TTY_MODEM:
		(void) scc_mctl(tp, *data, DMSET);
		break;

	case TTY_NMODEM:
		break;

	case TTY_SET_BREAK:
		(void) scc_mctl(tp, TM_BRK, DMBIS);
		break;

	case TTY_CLEAR_BREAK:
		(void) scc_mctl(tp, TM_BRK, DMBIC);
		break;

	default:
		simple_unlock(&tp->t_lock);
		splx(s);
		result = tty_set_status(tp, flavor, data, status_count);
		s = spltty();
		simple_lock(&tp->t_lock);
		if (result == D_SUCCESS &&
		    (flavor== TTY_STATUS_NEW || flavor == TTY_STATUS_COMPAT)) {
			result = scc_param(tp);

			if (tp->t_flags & CRTSCTS) {
				if (scc->modem[chan] & TM_CTS) {
					tp->t_state &= ~TS_TTSTOP;
					ttstart(tp);
				} else
					tp->t_state |= TS_TTSTOP;
			}
		}
		break;
	}

	simple_unlock(&tp->t_lock);
	splx(s);

	return result;
}

void
scc_waitforempty(struct tty *tp)
{
	int chan = scc_chan(tp->t_dev);
	scc_softc_t scc = &scc_softc[0];
	int rr0;

	while (1) {
		scc_read_reg(scc->regs, chan, SCC_RR0, rr0);
		if (rr0 & SCC_RR0_TX_EMPTY)
			break;
		assert_wait(0, TRUE);
		thread_set_timeout(1);
		simple_unlock(&tp->t_lock);
		thread_block((void (*)(void)) 0);
		reset_timeout_check(&current_thread()->timer);
		simple_lock(&tp->t_lock);
	}
}

/*
 * Send along a character on a tty.  If we were waiting for
 * this char to complete the open procedure do so; check
 * for errors; if all is well proceed to ttyinput().
 */

void
scc_input(dev_t dev, int c, enum scc_error err)
{
	register struct tty *tp;

	tp = &scc_tty[dev];

	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (tp->t_state & TS_INIT)
			tt_open_wakeup(tp);
		return;
	}
	switch (err) {
	case SCC_ERR_NONE:
		ttyinput(c, tp);
		break;
	case SCC_ERR_OVERRUN:
		/*log(LOG_WARNING, "sl%d: silo overflow\n", dev);*/
		/* Currently the Mach interface doesn't define an out-of-band
		   event that we could use to signal this error to the user
		   task that has this device open.  */
		break;
	case SCC_ERR_PARITY:
		ttyinputbadparity(c, tp);
		break;
	case SCC_ERR_BREAK:
		ttybreak(c, tp);
		break;
	}
}

/*
 * Transmission of a character is complete.
 * Return the next character or -1 if none.
 */
int
scc_simple_tint(dev_t dev, boolean_t all_sent)
{
	register struct tty *tp;

	tp = &scc_tty[dev];
	if ((tp->t_addr == 0) || /* not probed --> stray */
	    (tp->t_state & TS_TTSTOP))
		return -1;

	if (all_sent) {
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;

		scc_start(tp);
	}

	if (tp->t_outq.c_cc == 0 || (tp->t_state&TS_BUSY)==0)
		return -1;

	return getc(&tp->t_outq);
}
#endif /* notdef_next */

void
powermac_scc_set_datum(scc_regmap_t regs, unsigned int offset, unsigned char value)
{
	volatile unsigned char *address = (unsigned char *) regs + offset;
  
	*address = value;
	eieio();
}
  
unsigned char
powermac_scc_get_datum(scc_regmap_t regs, unsigned int offset)
{
	volatile unsigned char *address = (unsigned char *) regs + offset;
	unsigned char	value;
  
	value = *address; eieio();
	return value;
}

/* modem port is 1, printer port is 0 */
#define LINE	1

#if got_console_now
int
kmtrygetc()
{
	return scc_getc(0 /* ignored */, LINE, 0 /*no_wait*/, 0);
}

int
cngetc()
{
	return scc_getc(0 /* ignored */, LINE, 1 /*wait*/, 0);
}

int
cnputc(char c)
{
	int a;

	a= scc_putc(0 /* ignored */, LINE, c);
	if (c == '\n')
		a = cnputc('\r');
	return a;
}
#endif /* got_console_now */
#endif	/* NSCC > 0 */
