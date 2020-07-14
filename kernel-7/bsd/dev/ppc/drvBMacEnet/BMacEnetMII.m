/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER@
 */

/*
 * Copyright (c) 1998-1999 by Apple Computer, Inc., All rights reserved.
 *
 * MII/PHY (National Semiconductor DP83840/DP83840A) support methods.
 * It is general enough to work with most MII/PHYs.
 *
 * HISTORY
 *
 */
#import "BMacEnetPrivate.h"

@implementation BMacEnet(MII)

/*
 * Read from MII/PHY registers.
 */
- (BOOL)miiReadWord:(unsigned short *)dataPtr reg:(unsigned short)reg
	phy:(unsigned char)phy
{
    int					i;
    miiFrameUnion 		frame;
    unsigned short		phyreg;
    BOOL			ret = YES;

    do
    {
        // Write preamble
        //
        [self miiWrite:MII_FRAME_PREAMBLE size:MII_FRAME_SIZE];

	
        if ([self miiCheckZeroBit] == YES) 
        {
//          IOLog("Ethernet(BMac): MII not floating before read\n\r");
	    ret = NO;
            break;
        }

        // Prepare command frame
        //
        frame.data = MII_FRAME_READ;
        frame.bit.regad = reg;
        frame.bit.phyad = phy;
	
        // write ST, OP, PHYAD, REGAD in the MII command frame
        //
        [self miiWrite:frame.data size:14];
	
        // Hi-Z state
        // Make sure the PHY generated a zero bit after the 2nd Hi-Z bit
        //

        [self miiOutThreeState];

        if ([self miiCheckZeroBit] == NO) 
        {
//          IOLog("Ethernet(BMac): MII not driven after turnaround\n\r");
	    ret = NO;
            break;
        }

        // read 16-bit data
        //
        phyreg = 0;
        for (i = 0; i < 16; i++) 
        {
	    phyreg = [self miiReadBit] | (phyreg << 1);
        }
        if (dataPtr)
	    *dataPtr = phyreg;

        // Hi-Z state
        [self miiOutThreeState];
	
        if ([self miiCheckZeroBit] == YES) 
        {
//          IOLog("Ethernet(BMac): MII not floating after read\n\r");
	    ret = NO;
            break;
        }
    }
    while ( 0 );

    return ret;
}

/*
 * Write to MII/PHY registers.
 */
- (BOOL)miiWriteWord:(unsigned short)data reg:(unsigned short)reg
	phy:(unsigned char)phy
{
    miiFrameUnion 		frame;
    BOOL			ret = YES;
	
    do
    {
        // Write preamble
        //
        [self miiWrite:MII_FRAME_PREAMBLE size:MII_FRAME_SIZE];

        if ([self miiCheckZeroBit] == YES) 
        {
	    ret = NO;
            break;
        }

        // Prepare command frame
        //
        frame.data = MII_FRAME_WRITE;
        frame.bit.regad = reg;
        frame.bit.phyad = phy;
        frame.bit.data  = data;
	
        // Write command frame
        //
        [self miiWrite:frame.data size:MII_FRAME_SIZE];

        // Hi-Z state
        [self miiOutThreeState];

        if ([self miiCheckZeroBit] == YES) 
        {
	    ret = NO;
            break;
        }
    }
    while ( 0 );

    return ret;
}

/* 
 * Write 'dataSize' number of bits to the MII management interface,
 * starting with the most significant bit of 'miiData'.
 *
 */
- (void)miiWrite:(unsigned int)miiData size:(unsigned int)dataSize
{
    int i;
    u_int16_t	regValue;
	
    regValue = kMIFCSR_DataOutEnable;
		
    for (i = dataSize; i > 0; i--) 
    {
        int bit = ((miiData & 0x80000000) ? kMIFCSR_DataOut : 0);
		
        regValue &= ~(kMIFCSR_Clock | kMIFCSR_DataOut) ;
        regValue |=  bit;
	WriteBigMacRegister(ioBaseEnet, kMIFCSR, regValue);
	IODelay(phyMIIDelay);
		
	regValue |= kMIFCSR_Clock;
	WriteBigMacRegister(ioBaseEnet, kMIFCSR, regValue );
	IODelay(phyMIIDelay);

	miiData = miiData << 1;
    }
}

/*
 * Read one bit from the MII management interface.
 */
- (int)miiReadBit
{
    u_int16_t		regValue;
    u_int16_t		regValueRead;

    regValue = 0;	

    WriteBigMacRegister(ioBaseEnet, kMIFCSR, regValue);
    IODelay(phyMIIDelay);

    regValue |= kMIFCSR_Clock;
    WriteBigMacRegister(ioBaseEnet, kMIFCSR, regValue);
    IODelay(phyMIIDelay);
	
    regValueRead = ReadBigMacRegister(ioBaseEnet, kMIFCSR);
    IODelay(phyMIIDelay);	// delay next invocation of this routine
	
    return ( (regValueRead & kMIFCSR_DataIn) ? 1 : 0 );
}

/*
 * Read the zero bit on the second clock of the turn-around (TA)
 * when reading a PHY register.
 */
- (BOOL)miiCheckZeroBit
{
    u_int16_t	regValue;
	
    regValue = ReadBigMacRegister(ioBaseEnet, kMIFCSR);
    
    return (((regValue & kMIFCSR_DataIn) == 0) ? YES : NO );
}

/*
 * Tri-state the STA's MDIO pin.
 */
- (void)miiOutThreeState
{
    u_int16_t		regValue;

    regValue = 0;	
    WriteBigMacRegister(ioBaseEnet, kMIFCSR, regValue);
    IODelay(phyMIIDelay);
	
    regValue |= kMIFCSR_Clock;
    WriteBigMacRegister(ioBaseEnet, kMIFCSR, regValue);
    IODelay(phyMIIDelay);
}

- (BOOL)miiResetPHY:(unsigned char)phy
{
    int i = MII_RESET_TIMEOUT;
    unsigned short mii_control;

    // Set the reset bit
    //
    [self miiWriteWord:MII_CONTROL_RESET reg:MII_CONTROL phy:phy];
	
    IOSleep(MII_RESET_DELAY);

    // Wait till reset process is complete (MII_CONTROL_RESET returns to zero)
    //
    while (i > 0) 
    {
	if ([self miiReadWord:&mii_control reg:MII_CONTROL phy:phy] == NO)
		return NO;

	if (!(mii_control & MII_CONTROL_RESET))
        {
            [self miiReadWord:&mii_control reg:MII_CONTROL phy:phy];
            mii_control &= ~MII_CONTROL_ISOLATE;
            [self miiWriteWord:mii_control reg:MII_CONTROL phy:phy];
            return YES;
        }

	IOSleep(MII_RESET_DELAY);
	i -= MII_RESET_DELAY;
    }
    return NO;
}

- (BOOL)miiWaitForLink:(unsigned char)phy
{
    int i = MII_LINK_TIMEOUT;
    unsigned short mii_status;
	
    while (i > 0) 
    {
	if ([self miiReadWord:&mii_status reg:MII_STATUS phy:phy] == NO)
		return NO;
		
	if (mii_status & MII_STATUS_LINK_STATUS)
		return YES;
		
	IOSleep(MII_LINK_DELAY);
	i -= MII_LINK_DELAY;
    }
    return NO;
}

- (BOOL)miiWaitForAutoNegotiation:(unsigned char)phy
{
    int i = MII_LINK_TIMEOUT;
    unsigned short mii_status;
	
    while (i > 0) 
    {
	if ([self miiReadWord:&mii_status reg:MII_STATUS phy:phy] == NO)
		return NO;
		
	if (mii_status & MII_STATUS_NEGOTIATION_COMPLETE)
		return YES;
		
	IOSleep(MII_LINK_DELAY);
	i -= MII_LINK_DELAY;
    }
    return NO;
}

- (void)miiRestartAutoNegotiation:(unsigned char)phy
{
    unsigned short mii_control;

    [self miiReadWord:&mii_control reg:MII_CONTROL phy:phy];
    mii_control |= MII_CONTROL_RESTART_NEGOTIATION;
    [self miiWriteWord:mii_control reg:MII_CONTROL phy:phy];

    /*
     * If the system is not connected to the network, then auto-negotiation
     * never completes and we hang in this loop!
     */
#if 0
    while (1) 
    {
	[self miiReadWord:&mii_control reg:MII_CONTROL phy:phy];
	if ((mii_control & MII_CONTROL_RESTART_NEGOTIATION) == 0)
		break;
    }
#endif
}

/*
 * Find the first PHY device on the MII interface.
 *
 * Return
 *	YES		PHY found 
 *  	NO		PHY not found
 */
- (BOOL)miiFindPHY:(unsigned char *)phy
{
    int i;
	
    *phy = -1;

    // The first two PHY registers are required.
    //
    for (i = 0; i < MII_MAX_PHY; i++) 
    {
	if ([self miiReadWord:NULL reg:MII_STATUS phy:i] &&
		[self miiReadWord:NULL reg:MII_CONTROL phy:i])
		break;
    }
	
    if (i >= MII_MAX_PHY)
	return NO;

    *phy = i;

    return YES;
}

/*
 *
 *
 */
- (BOOL)miiInitializePHY:(unsigned char)phy
{
    u_int16_t		phyWord; 

    // Clear enable auto-negotiation bit
    //
    [self miiReadWord:&phyWord reg:MII_CONTROL phy:phy];
    phyWord &= ~MII_CONTROL_AUTONEGOTIATION;
    [self miiWriteWord:phyWord reg:MII_CONTROL phy:phy];

    // Advertise 10/100 Half/Full duplex capable to link partner
    //
    [self miiReadWord:&phyWord reg:MII_ADVERTISEMENT phy:phy];
    phyWord |= (MII_ANAR_100BASETX_FD | MII_ANAR_100BASETX |
                MII_ANAR_10BASET_FD   | MII_ANAR_10BASET );
    [self miiWriteWord:phyWord reg:MII_ADVERTISEMENT phy:phy];

    // Set enable auto-negotiation bit
    //
    [self miiReadWord:&phyWord reg:MII_CONTROL phy:phy];
    phyWord |= MII_CONTROL_AUTONEGOTIATION;
    [self miiWriteWord:phyWord reg:MII_CONTROL phy:phy];

    [self miiRestartAutoNegotiation:phy];

    return YES;
}        

@end

