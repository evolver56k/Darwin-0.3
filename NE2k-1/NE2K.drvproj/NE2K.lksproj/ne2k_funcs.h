
#import <driverkit/i386/ioPorts.h>
#import "ioPortsExt.h"

#import "ne2k_reg.h"

/* access commands */

static __inline__ void NICSelectPage(IOEISAPortAddress base, unsigned char page)
{
    RCNV	ncr;
    ncr.data =  inb(base + NIC_CR_OFFSET);
    ncr.cr.psel = page;
    outb(base + NIC_CR_OFFSET, ncr.data);
}

static __inline__ unsigned char NICGetRegister(IOEISAPortAddress base, unsigned char offset, unsigned char page)
{
    NICSelectPage(base, page);

    return inb(base + offset);
}

static __inline__ void NICSetRegister(IOEISAPortAddress base, unsigned char offset, unsigned char page, unsigned char value)
{
    NICSelectPage(base, page);

    outb(base + offset, value);
}








static __inline__ char NICGetCharData(IOEISAPortAddress base) {
    return inb(base + NE_DATAPORT);
}

static __inline__ void NICSetCharData(IOEISAPortAddress base, char value) {
    outb(base + NE_DATAPORT, value);
}

static __inline__ short NICGetShortData(IOEISAPortAddress base) {
    return inw(base + NE_DATAPORT);
}

static __inline__ void NICSetShortData(IOEISAPortAddress base, short value) {
    outw(base + NE_DATAPORT, value);
}


static __inline__ long NICGetLongData(IOEISAPortAddress base) {
    return inl(base + NE_DATAPORT);
}

static __inline__ void NICSetLongData(IOEISAPortAddress base, long value) {
    outl(base + NE_DATAPORT, value);
}




static __inline__ void NICGetCharDatas(IOEISAPortAddress base, void *addr, int size) {
    insb(base + NE_DATAPORT, addr, size);
}


static __inline__ void NICSetCharDatas(IOEISAPortAddress base, void *addr, int size) {
    outsb(base + NE_DATAPORT, addr, size);
}


static __inline__ void NICGetShortDatas(IOEISAPortAddress base, void *addr, int size) {
    insw(base + NE_DATAPORT, addr, size);
}


static __inline__ void NICSetShortDatas(IOEISAPortAddress base, void *addr, int size) {
    outsw(base + NE_DATAPORT, addr, size);
}



static __inline__ void NICGetLongDatas(IOEISAPortAddress base, void *addr, int size) {
    insl(base + NE_DATAPORT, addr, size);
}


static __inline__ void NICSetLongDatas(IOEISAPortAddress base, void *addr, int size) {
    outsl(base + NE_DATAPORT, addr, size);
}









/* reg get/set commands */

static __inline__ unsigned char NICGetCR(IOEISAPortAddress base)
{
    return inb(base + NIC_CR_OFFSET);
}

static __inline__ void NICSetCR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_CR_OFFSET, value);
}


static __inline__ unsigned char NICGetISR(IOEISAPortAddress base)
{
    return inb(base + NIC_ISR_OFFSET);
}

static __inline__ void NICSetISR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_ISR_OFFSET, value);
}


static __inline__ unsigned char NICGetIMR(IOEISAPortAddress base)
{
    return inb(base + NIC_IMR_OFFSET);
}

static __inline__ void NICSetIMR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_IMR_OFFSET, value);
}


static __inline__ unsigned char NICGetDCR(IOEISAPortAddress base)
{
    return inb(base + NIC_DCR_OFFSET);
}

static __inline__ void NICSetDCR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_DCR_OFFSET, value);
}


static __inline__ unsigned char NICGetTCR(IOEISAPortAddress base)
{
    return inb(base + NIC_TCR_OFFSET);
}

static __inline__ void NICSetTCR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_TCR_OFFSET, value);
}


static __inline__ unsigned char NICGetTSR(IOEISAPortAddress base)
{
    return inb(base + NIC_TSR_OFFSET);
}


static __inline__ unsigned char NICGetRCR(IOEISAPortAddress base)
{
    return inb(base + NIC_RCR_OFFSET);
}

static __inline__ void NICSetRCR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_RCR_OFFSET, value);
}


static __inline__ unsigned char NICGetRSR(IOEISAPortAddress base)
{
    return inb(base + NIC_RSR_OFFSET);
}


static __inline__ unsigned int NICGetCLDA(IOEISAPortAddress base)
{
    return (unsigned int)(inb(base + NIC_CLDA0_OFFSET) | (inb(base + NIC_CLDA1_OFFSET) << 8));
}


static __inline__ unsigned char NICGetPSTART(IOEISAPortAddress base)
{
    return inb(base + NIC_PSTART_OFFSET);
}

static __inline__ void NICSetPSTART(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_PSTART_OFFSET, value);
}


static __inline__ unsigned char NICGetPSTOP(IOEISAPortAddress base)
{
    return inb(base + NIC_PSTOP_OFFSET);
}

static __inline__ void NICSetPSTOP(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_PSTOP_OFFSET, value);
}


static __inline__ unsigned char NICGetBNRY(IOEISAPortAddress base)
{
    return inb(base + NIC_BNRY_OFFSET);
}

static __inline__ void NICSetBNRY(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_BNRY_OFFSET, value);
}





static __inline__ void NICSetTBCR(IOEISAPortAddress base, unsigned int value)
{
    outb(base + NIC_TBCR0_OFFSET, (value & 0xff));
    outb(base + NIC_TBCR1_OFFSET, ((value >> 8) & 0xff));
}




static __inline__ void NICSetTPSR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_TPSR_OFFSET, value);
}

static __inline__ unsigned char NICGetNCR(IOEISAPortAddress base)
{
    return inb(base + NIC_NCR_OFFSET);
}


static __inline__ unsigned char NICGetFIFO(IOEISAPortAddress base)
{
    return inb(base + NIC_FIFO_OFFSET);
}


static __inline__ unsigned int NICGetCRDA(IOEISAPortAddress base)
{
    return (unsigned int)(inb(base + NIC_CRDA0_OFFSET) | (inb(base + NIC_CRDA1_OFFSET) << 8));
}


static __inline__ void NICSetRSAR(IOEISAPortAddress base, unsigned int value)
{
    outb(base + NIC_RSAR0_OFFSET, (value & 0xff));
    outb(base + NIC_RSAR1_OFFSET, ((value>>8) & 0xff));
}


static __inline__ void NICSetRBCR(IOEISAPortAddress base, unsigned int value)
{
    outb(base + NIC_RBCR0_OFFSET, (value & 0xff));
    outb(base + NIC_RBCR1_OFFSET, ((value>>8) & 0xff));
}


static __inline__ unsigned char NICGetCNTR0(IOEISAPortAddress base)
{
    return inb(base + NIC_CNTR0_OFFSET);
}


static __inline__ unsigned char NICGetCNTR1(IOEISAPortAddress base)
{
    return inb(base + NIC_CNTR1_OFFSET);
}


static __inline__ unsigned char NICGetCNTR2(IOEISAPortAddress base)
{
    return inb(base + NIC_CNTR2_OFFSET);
}




static __inline__ void NICGetPAR(IOEISAPortAddress base, unsigned char *paddr)
{
    paddr[0] = inb(base + NIC_PAR0_OFFSET);
    paddr[1] = inb(base + NIC_PAR1_OFFSET);
    paddr[2] = inb(base + NIC_PAR2_OFFSET);
    paddr[3] = inb(base + NIC_PAR3_OFFSET);
    paddr[4] = inb(base + NIC_PAR4_OFFSET);
    paddr[5] = inb(base + NIC_PAR5_OFFSET);
}


static __inline__ void NICSetPAR(IOEISAPortAddress base, unsigned char *paddr)
{
    outb(base + NIC_PAR0_OFFSET, paddr[0]);
    outb(base + NIC_PAR1_OFFSET, paddr[1]);
    outb(base + NIC_PAR2_OFFSET, paddr[2]);
    outb(base + NIC_PAR3_OFFSET, paddr[3]);
    outb(base + NIC_PAR4_OFFSET, paddr[4]);
    outb(base + NIC_PAR5_OFFSET, paddr[5]);
}




static __inline__ unsigned char NICGetCURR(IOEISAPortAddress base)
{
    return inb(base + NIC_CURR_OFFSET);
}

static __inline__ void NICSetCURR(IOEISAPortAddress base, unsigned char value)
{
    outb(base + NIC_CURR_OFFSET, value);
}







static __inline__ void NICGetMAR(IOEISAPortAddress base, unsigned char *maddr)
{
    maddr[0] = inb(base + NIC_MAR0_OFFSET);
    maddr[1] = inb(base + NIC_MAR1_OFFSET);
    maddr[2] = inb(base + NIC_MAR2_OFFSET);
    maddr[3] = inb(base + NIC_MAR3_OFFSET);
    maddr[4] = inb(base + NIC_MAR4_OFFSET);
    maddr[5] = inb(base + NIC_MAR5_OFFSET);
    maddr[6] = inb(base + NIC_MAR6_OFFSET);
    maddr[7] = inb(base + NIC_MAR7_OFFSET);
}


static __inline__ void NICSetMAR(IOEISAPortAddress base, unsigned char *maddr)
{
    outb(base + NIC_MAR0_OFFSET, maddr[0]);
    outb(base + NIC_MAR1_OFFSET, maddr[1]);
    outb(base + NIC_MAR2_OFFSET, maddr[2]);
    outb(base + NIC_MAR3_OFFSET, maddr[3]);
    outb(base + NIC_MAR4_OFFSET, maddr[4]);
    outb(base + NIC_MAR5_OFFSET, maddr[5]);
    outb(base + NIC_MAR6_OFFSET, maddr[6]);
    outb(base + NIC_MAR7_OFFSET, maddr[7]);
}

