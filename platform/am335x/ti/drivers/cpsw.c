/**
 *  \file   cpsw.c
 *
 *  \brief  CPSW device abstraction layer APIs.
 *
 *   This file contains the device abstraction layer APIs for CPSW RGMII.
 */

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "hw_types.h"
#include "cpsw.h"

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/
#define CPSW_CORE_OFFSET                   (0x10u)
#define CPSW_MAX_HEADER_DESC               (0x08u)
#define CPDMA_P0_DEF_TX_MAP                (0x76543210u)
#define ALE_ENTRY_WORDS                    (0x03u)
#define CPDMA_ERR_CHANNEL_POS              (0xFFu)
#define CPSW_PORT_DUAL_MAC_MODE            (0x01u <<                        \
                                            CPSW_PORT_P0_TX_IN_CTL_TX_IN_SEL_SHIFT)
#define CPSW_PORT_RATE_LIM_MODE            (0x02u <<                        \
                                            CPSW_PORT_P0_TX_IN_CTL_TX_IN_SEL_SHIFT)

/*******************************************************************************
*                        API FUNCTION DEFINITIONS
*******************************************************************************/
/**
 * \brief   Resets the CPSW Subsystem.
 *
 * \param   baseAddr    Base address of the CPSW Subsystem
 *
 * \return  None
 **/
void CPSWSSReset(unsigned int baseAddr)
{
    /* Reset the CPSW */
    HWREG(baseAddr + CPSW_SS_SOFT_RESET) = CPSW_SS_SOFT_RESET_SOFT_RESET;

    while(HWREG(baseAddr + CPSW_SS_SOFT_RESET) 
          & CPSW_SS_SOFT_RESET_SOFT_RESET);
}

/**
 * \brief   Enables the copying of RX short frames, which are of size less than
 *          64 bytes.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlRxShortFramesCopyEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SL_MACCONTROL) |= CPSW_SL_MACCONTROL_RX_CSF_EN;
}

/**
 * \brief   Disables the copying of RX short frames, which are of size less than
 *          64 bytes.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlRxShortFramesCopyDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SL_MACCONTROL) &= ~CPSW_SL_MACCONTROL_RX_CSF_EN;
}

/**
 * \brief   Enables limiting the number of short gap packets transmitted to
 *          100ppm. Each time a short gap packet is sent, a counter is loaded
 *          with 10,000 and decremented on each wireside clock. Another short 
 *          gap packet will not be sent out until the counter decrements to 
 *          zero. This mode is included to preclude the host from filling up 
 *          the FIFO and sending every packet out with short gap which would 
 *          violate the maximum number of packets per second allowed.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlTxShortGapLimitEnable(unsigned int baseAddr)
{
     HWREG(baseAddr + CPSW_SL_MACCONTROL) |=
                             CPSW_SL_MACCONTROL_TX_SHORT_GAP_LIM_EN;
}

/**
 * \brief   Disables limiting the number of short gap packets transmitted.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlTxShortGapLimitDisable(unsigned int baseAddr)
{
     HWREG(baseAddr + CPSW_SL_MACCONTROL) &=
                            ~CPSW_SL_MACCONTROL_TX_SHORT_GAP_LIM_EN;
}

/**
 * \brief   Force the CPGMAC_SL into gigabit mode if the input GMII_MTCLK has
 *          been stopped by the PHY
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlGigModeForceEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SL_MACCONTROL) |= CPSW_SL_MACCONTROL_GIG_FORCE;
}

/**
 * \brief   Enables the fullduplex and gigabit mode to be selected
 *          from the FULLDUPLEX_IN and GIG_IN input signals and not from the
 *          fullduplex and gig bits in this register. The FULLDUPLEX_MODE bit
 *          reflects the actual fullduplex mode selected
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlControlExtEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SL_MACCONTROL) |= CPSW_SL_MACCONTROL_EXT_EN;
}


/**
 * \brief   Disables the CPGMAC_SL gigabit mode if the input GMII_MTCLK has
 *          been stopped by the PHY
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlGigModeForceDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SL_MACCONTROL) &= ~CPSW_SL_MACCONTROL_GIG_FORCE;
} 

/**
 * \brief   Sets the Transfer mode, 10/100 or gigabit mode  and the duplex
 *          mode  for the sliver.  
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   mode        The transfer mode
 *    'mode' can take one of the below values. \n
 *         CPSW_SLIVER_NON_GIG_HALF_DUPLEX - 10/100 Mbps mode, half duplex. \n
 *         CPSW_SLIVER_NON_GIG_FULL_DUPLEX - 10/100 Mbps mode, full duplex. \n
 *         CPSW_SLIVER_GIG_FULL_DUPLEX - 1000 Mbps mode, full duplex. \n
 *
 * \return  None
 *
 **/
void CPSWSlTransferModeSet(unsigned int baseAddr, unsigned int mode)
{
    HWREG(baseAddr + CPSW_SL_MACCONTROL) &= ~(CPSW_SL_MACCONTROL_GIG
                                              | CPSW_SL_MACCONTROL_FULLDUPLEX);
      
    HWREG(baseAddr + CPSW_SL_MACCONTROL) |= mode;
}

/**
 * \brief   Returns the MAC Status. The value will be the contents of the MAC
 *          status register.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   statFlag    Status flags to be read.
 *            statFlag can take an OR combination of the below values. \n
 *              CPSW_SLIVER_STATE - The Sliver state. \n
 *              CPSW_SLIVER_EXT_GIG_INPUT_BIT - The EXT_GIG input bit mask.\n
 *              CPSW_SLIVER_EXT_FULL_DUPLEX_BIT - The EXT_FULLDUPLEX input
 *                                                  bit mask. \n
 *              CPSW_SLIVER_RX_FLOWCTRL - The receive flow control active. \n
 *              CPSW_SLIVER_TX_FLOWCTRL - The transmit flow control.
 *
 *
 * \return  MAC Status
 *            The MAC status register value returned can be compared against
 *            the below tokens. \n
 *              CPSW_SLIVER_STATE_IDLE - The Sliver is in idle state. \n
 *              CPSW_SLIVER_EXT_GIG_INPUT_HIGH - The EXT_GIG input 
 *                                                 bit is in HIGH state.\n
 *              CPSW_SLIVER_EXT_FULL_DUPLEX_HIGH - The EXT_FULLDUPLEX input 
 *                                                   bit is in HIGH state. \n
 *              CPSW_SLIVER_RX_FLOWCTRL_ACTIVE - The receive flow control is
 *                                                 active. \n
 *              CPSW_SLIVER_TX_FLOWCTRL_ACTIVE - The pause time period is 
 *                                                 observed for a received 
 *                                                 pause frame 
 *
 **/
unsigned int CPSWSlMACStatusGet(unsigned int baseAddr, unsigned int statFlag)
{
    /* Return the required status only */
    return (HWREG(baseAddr + CPSW_SL_MACSTATUS) & statFlag);
}

/**
 * \brief   Resets the CPSW Sliver Logic.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  None
 *
 **/
void CPSWSlReset(unsigned int baseAddr)
{
    /* Reset the sliver logic */
    HWREG(baseAddr + CPSW_SL_SOFT_RESET) = CPSW_SL_SOFT_RESET_SOFT_RESET;
    
    /* Wait till the reset completes */
    while(CPSW_SL_SOFT_RESET_SOFT_RESET == 
          ((HWREG(baseAddr + CPSW_SL_SOFT_RESET)) 
           & CPSW_SL_SOFT_RESET_SOFT_RESET));
}


/**
 * \brief   Sets the maximum length for received frame. 
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   rxMaxLen    Maximum length for a received frame
 *     The default value for 'rxMaxLen' is 1518. The maximum value 
 *     which can be set is 16383.
 *
 * \return  None
 *
 **/
void CPSWSlRxMaxLenSet(unsigned int baseAddr, unsigned int rxMaxLen)
{
    /* Set the desired maximum length */
    HWREG(baseAddr + CPSW_SL_RX_MAXLEN) = rxMaxLen;
}

/**
 * \brief   Sets the receive pause timer.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   rxPauseTmr  Receive pause timer value
 *
 * \return  None
 **/
void CPSWSlRxPauseTmrSet(unsigned int baseAddr, unsigned int rxPauseTmr)
{
    HWREG(baseAddr + CPSW_SL_RX_PAUSE) = rxPauseTmr;
}

/**
 * \brief   Reads the receive pause timer. The receive pause timer is
 *          decremented at slot time intervals. If the receive pause timer
 *          decrements to zero, then another outgoing pause frame will be
 *          sent and the load/decrement process will be repeated
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  Receive Pause Timer
 **/
unsigned int CPSWSlRxPauseTmrGet(unsigned int baseAddr)
{
    return (HWREG(baseAddr + CPSW_SL_RX_PAUSE));
}

/**
 * \brief   Sets the transmit pause timer.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   txPauseTmr  Transmit pause timer value
 *
 * \return  None
 **/
void CPSWSlTxPauseTmrSet(unsigned int baseAddr, unsigned int txPauseTmr)
{
    HWREG(baseAddr + CPSW_SL_TX_PAUSE) = txPauseTmr;
}

/**
 * \brief   Reads the transmit pause timer. The transmit pause timer is
 *          loaded by a received (incoming) pause frame, and then decremented,
 *          at slottime intervals, down to zero at which time the sliver
 *          transmit frames are again enabled.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 *
 * \return  Transmit Pause Timer
 **/
unsigned int CPSWSlTxPauseTmrGet(unsigned int baseAddr)
{
    return (HWREG(baseAddr + CPSW_SL_TX_PAUSE));
}

/**
 * \brief   Sets the transmitter inter-packet gap value. 
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   intrPktGap  The transmit inter-packet gap value.
 *     The maximum value 'intrPktGap' can take is 0x1FF.
 *
 * \return  None
 **/
void CPSWSlTxIntrPktGapSet(unsigned int baseAddr, unsigned int intrPktGap)
{
    HWREG(baseAddr + CPSW_SL_TX_GAP) = intrPktGap;
}

/**
 * \brief   Enables GMII for the sliver.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   intrPktGap  The transmit inter-packet gap value.
 *
 *            The maximum value 'intrPktGap' can take is 0x1FF.
 *
 * \return  None
 **/
void CPSWSlGMIIEnable(unsigned int baseAddr)
{
   HWREG(baseAddr + CPSW_SL_MACCONTROL) |= CPSW_SL_MACCONTROL_GMII_EN;
}

/**
 * \brief   Enables RGMII for the sliver.
 *
 * \param   baseAddr    Base address of the CPSW Sliver Module registers.
 * \param   intrPktGap  The transmit inter-packet gap value.
 *
 *            The maximum value 'intrPktGap' can take is 0x1FF.
 *
 * \return  None
 **/
void CPSWSlRGMIIEnable(unsigned int baseAddr)
{
   HWREG(baseAddr + CPSW_SL_MACCONTROL) |= (CPSW_SL_MACCONTROL_GMII_EN 
                                            | CPSW_SL_MACCONTROL_IFCTL_A 
                                            | CPSW_SL_MACCONTROL_IFCTL_B);
}

/**
 * \brief   Resets the CPSW Wrapper module.
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 *
 * \return  None
 **/
void CPSWWrReset(unsigned int baseAddr)
{
    /* Reset the CPSW Wrapper */
    HWREG(baseAddr + CPSW_WR_SOFT_RESET) = CPSW_WR_SOFT_RESET_SOFT_RESET;

    while(HWREG(baseAddr + CPSW_WR_SOFT_RESET) 
          & CPSW_WR_SOFT_RESET_SOFT_RESET);
}

/**
 * \brief   Resets the Control Register of CPSW Wrapper module
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 *
 * \return  None
 **/
void CPSWWrControlRegReset(unsigned int baseAddr)
{
    /* Reset the CPSW Wrapper control Register */
    HWREG(baseAddr + CPSW_WR_CONTROL) =  CPSW_WR_CONTROL_MMR_RESET;
}

/**
 * \brief   Enables an interrupt for the specified core.
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 * \param   core        Core number
 * \param   channel     Channel number
 * \param   intFlag     Interrupt to be enabled
 *    'intFlag' can take one of the below values. \n
 *          CPSW_CORE_INT_RX_THRESH - RX threshold interrupt \n
 *          CPSW_CORE_INT_RX_PULSE - RX pulse interrupt \n
 *          CPSW_CORE_INT_TX_PULSE - TX pulse interrupt \n
 *          CPSW_CORE_INT_MISC - Miscellaneous interrupt
 *
 * \return  None
 **/
void CPSWWrCoreIntEnable(unsigned int baseAddr, unsigned int core,
                         unsigned int channel, unsigned int intFlag)
{
    HWREG(baseAddr + CPSW_WR_C_RX_THRESH_EN(core) + intFlag) |= (1 << channel); 
}

/**
 * \brief   Disables an interrupt for the specified core.
 *
 * \param   baseAddr    Base address of thei CPSW Wrapper Module
 * \param   core        Core number
 * \param   channel     Channel number
 * \param   intFlag     Interrupt to be disabled
 *    'intFlag' can take one of the below values. \n
 *          CPSW_CORE_INT_RX_THRESH - RX threshold interrupt \n
 *          CPSW_CORE_INT_RX_PULSE - RX pulse interrupt \n
 *          CPSW_CORE_INT_TX_PULSE - TX pulse interrupt \n
 *          CPSW_CORE_INT_MISC - Miscellaneous interrupt
 *
 * \return  None
 **/
void CPSWWrCoreIntDisable(unsigned int baseAddr, unsigned int core, 
                          unsigned int channel, unsigned int intFlag)
{
    HWREG(baseAddr + CPSW_WR_C_RX_THRESH_EN(core) + intFlag) &= 
                                                            ~(1 << channel); 
}

/**
 * \brief   Returns the interrupt status of the core for the specified
 *          channel
 *
 * \param   baseAddr    Base address of thei CPSW Wrapper Module
 * \param   core        Core number
 * \param   channel     Channel number
 * \param   intFlag     Interrupt status to be read
 *    'intFlag' can take one of the below values. \n
 *          CPSW_CORE_INT_RX_THRESH - RX threshold interrupt \n
 *          CPSW_CORE_INT_RX_PULSE - RX pulse interrupt \n
 *          CPSW_CORE_INT_TX_PULSE - TX pulse interrupt \n
 *          CPSW_CORE_INT_MISC - Miscellaneous interrupt
 *
 * \return  same as intFlag if the status is set
 *          '0' if the status is cleared
 **/
unsigned int CPSWWrCoreIntStatusGet(unsigned int baseAddr, unsigned int core,
                                    unsigned int channel, unsigned int intFlag)
{
    return (HWREG(baseAddr + CPSW_WR_C_RX_THRESH_STAT(core) + intFlag)
            &  (1 << channel));
}

/**
 * \brief   Returns the RGMII status requested.
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 * \param   statFlag    Status to be checked
 *    'statFlag' can take a combination of the below values. \n
 *        CPSW_RGMII2_DUPLEX - Duplex of RGMII2 \n
 *        CPSW_RGMII2_SPEED - Speed of RGMII2 \n
 *        CPSW_RGMII2_LINK_STAT - Link Status of RGMII2 \n
 *        CPSW_RGMII1_DUPLEX - Duplex of RGMII1 \n
 *        CPSW_RGMII1_SPEED - Speed of RGMII1 \n
 *        CPSW_RGMII1_LINK_STAT - Link Status of RGMII1 \n
 *  
 *    The returned value can be compared agains the below values \n
 *        CPSW_RGMII2_DUPLEX_FULL - RGMII2 full duplex \n
 *        CPSW_RGMII2_DUPLEX_HALF - RGMII2 half duplex \n
 *        CPSW_RGMII2_SPEED_10M - Speed is 10 Mbps \n
 *        CPSW_RGMII2_SPEED_100M - Speed is 100 Mbps \n
 *        CPSW_RGMII2_SPEED_1000M - Speed is 1000 Mbps \n
 *        CPSW_RGMII2_LINK_UP - RGMII2 link is up\n
 *        CPSW_RGMII2_LINK_DOWN - RGMII2 link is down \n
 *        CPSW_RGMII1_DUPLEX_FULL - RGMII1 full duplex \n
 *        CPSW_RGMII1_DUPLEX_HALF - RGMII1 half duplex \n
 *        CPSW_RGMII1_SPEED_10M - Speed is 10 Mbps \n
 *        CPSW_RGMII1_SPEED_100M - Speed is 100 Mbps \n
 *        CPSW_RGMII1_SPEED_1000M - Speed is 1000 Mbps \n
 *        CPSW_RGMII1_LINK_UP - RGMII1 link is up\n
 *        CPSW_RGMII1_LINK_DOWN - RGMII1 link is down \n
 *
 * \return  Status of RGMII. Return value can be compared agains the same
 *          statFlag passed. 
 **/
unsigned int CPSWWrRGMIIStatusGet(unsigned int baseAddr, unsigned int statFlag)
{
    return (HWREG(baseAddr + CPSW_WR_RGMII_CTL) & statFlag);
}

/**
 * \brief   Enables pacing of interrupts.
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 * \param   pacFlag     The interrpupts for which pacing to be enabled
 *    'pacFlag' can take any combination of the below values. \n
 *         CPSW_INT_PACING_C0_RX_PULSE  - Core 0 RX pulse interrupt pacing \n 
 *         CPSW_INT_PACING_C0_TX_PULSE  - Core 0 TX pulse interrupt pacing \n 
 *         CPSW_INT_PACING_C1_RX_PULSE  - Core 1 RX pulse interrupt pacing \n 
 *         CPSW_INT_PACING_C1_TX_PULSE  - Core 1 TX pulse interrupt pacing \n 
 *         CPSW_INT_PACING_C2_RX_PULSE  - Core 2 RX pulse interrupt pacing \n 
 *         CPSW_INT_PACING_C2_TX_PULSE  - Core 2 TX pulse interrupt pacing \n 
 *
 * \return  None
 **/
void CPSWWrIntPacingEnable(unsigned int baseAddr, unsigned int pacFlag)
{
    HWREG(baseAddr + CPSW_WR_INT_CONTROL) &= ~CPSW_WR_INT_CONTROL_INT_PACE_EN;
    HWREG(baseAddr + CPSW_WR_INT_CONTROL) |= pacFlag;
}

/**
 * \brief   Sets the number of transmit interrupts per millisecond.
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 * \param   core        Core number
 * \param   txIntPerMs  The number of tx interrupts per millisecond
 *    The maximum value for 'txIntPerMs' is 0x3F.
 *
 * \return  None
 **/
void CPSWWrIntTxPerMsSet(unsigned int baseAddr, unsigned int core,
                         unsigned int txIntPerMs)
{
    HWREG(baseAddr + CPSW_WR_C_TX_IMAX(core)) = txIntPerMs;
}

/**
 * \brief   Sets the number of receive interrupts per millisecond.
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 * \param   core        Core number
 * \param   rxIntPerMs  The number of rx interrupts per millisecond
 *    The maximum value for 'rxIntPerMs' is 0x3F.
 *
 * \return  None
 **/
void CPSWWrIntRxPerMsSet(unsigned int baseAddr, unsigned int core,
                         unsigned int rxIntPerMs)
{
    HWREG(baseAddr + CPSW_WR_C_RX_IMAX(core)) = rxIntPerMs;
}

/**
 * \brief   Disables pacing of interrupts                           
 *
 * \param   baseAddr    Base address of the CPSW Wrapper Module
 * \param   pacFlag     The interrpupts for which pacing to be disabled
 *    'pacFlag' can take any combination of the below values. \n
 *         CPSW_INT_PACING_C0_RX_PULSE  - Core 0 RX pulse interrupt pacing \n
 *         CPSW_INT_PACING_C0_TX_PULSE  - Core 0 TX pulse interrupt pacing \n
 *         CPSW_INT_PACING_C1_RX_PULSE  - Core 1 RX pulse interrupt pacing \n
 *         CPSW_INT_PACING_C1_TX_PULSE  - Core 1 TX pulse interrupt pacing \n
 *         CPSW_INT_PACING_C2_RX_PULSE  - Core 2 RX pulse interrupt pacing \n
 *         CPSW_INT_PACING_C2_TX_PULSE  - Core 2 TX pulse interrupt pacing \n
 *
 * \return  None
 **/
void CPSWWrIntPacingDisable(unsigned int baseAddr, unsigned int pacFlag)
{
   HWREG(baseAddr + CPSW_WR_INT_CONTROL) &= ~(pacFlag);
}

/**
 * \brief   Initializes the ALE. The ALE logic is reset and the ALE table
 *          entries are cleared.
 *
 * \param   baseAddr    Base address of the ALE module
 *
 * \return  None
 **/
void CPSWALEInit(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_ALE_CONTROL) = (CPSW_ALE_CONTROL_CLEAR_TABLE 
                                          | CPSW_ALE_CONTROL_ENABLE_ALE);
}

/**
 * \brief   Sets the port state in the ALE for a given port
 *
 * \param   baseAddr    Base address of the ALE module
 * \param   portNum     The port number
 * \param   portState   The port state to be set
 *    'portState' can take one of the below values \n
 *        CPSW_ALE_PORT_STATE_FWD - ALE state is Forward \n
 *        CPSW_ALE_PORT_STATE_LEARN - ALE state is Learn \n
 *        CPSW_ALE_PORT_STATE_BLOCKED - ALE state is Blocked \n
 *        CPSW_ALE_PORT_STATE_DISABLED - ALE state is Disabled
 *
 * \return  None
 **/
void CPSWALEPortStateSet(unsigned int baseAddr, unsigned int portNum,
                         unsigned int portState)
{
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) &=
                                        ~CPSW_ALE_PORTCTL0_PORT_STATE;

    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) |= portState;
}

/**
 * \brief   Sets an ALE table entry
 *
 * \param   baseAddr    Base address of the ALE Module
 * \param   aleTblIdx   The Index of the table entry 
 * \param   aleEntryPtr The address of the entry to be set
 *
 * \return  None
 **/
void CPSWALETableEntrySet(unsigned int baseAddr, unsigned int aleTblIdx, 
                          unsigned int *aleEntryPtr)
{
    unsigned int cnt;

    for (cnt = 0; cnt < ALE_ENTRY_WORDS; cnt++)
    {
        HWREG(baseAddr +  CPSW_ALE_TBLW(cnt)) =  *(aleEntryPtr + cnt);
    }     
    
    HWREG(baseAddr +  CPSW_ALE_TBLCTL) =
                                aleTblIdx | CPSW_ALE_TBLCTL_WRITE_RDZ;
}

/**
 * \brief   Returns an ALE table entry
 *
 * \param   baseAddr    Base address of the ALE Module
 * \param   aleTblIdx   The Index of the table entry 
 * \param   aleEntryPtr The address where the ALE entry to be written
 *
 * \return  None
 **/
void CPSWALETableEntryGet(unsigned int baseAddr, unsigned int aleTblIdx,
                          unsigned int *aleEntryPtr)
{
    unsigned int cnt;

    HWREG(baseAddr + CPSW_ALE_TBLCTL) = aleTblIdx;

    for (cnt = 0; cnt < ALE_ENTRY_WORDS; cnt++)
    {
        *(aleEntryPtr + cnt) = HWREG(baseAddr + CPSW_ALE_TBLW(cnt));
    }
}

/**
 * \brief   Enables the ALE port to leaning an address
 *
 * \param   baseAddr      Base Address of the ALE module
 * \param   portNum       The port number 
 *
 * \return  None
 *
 **/
void CPSWALELearnModeEnable(unsigned int baseAddr, unsigned int  portNum)
{
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) &= 
                                         ~CPSW_ALE_PORTCTL0_NO_LEARN;
}

/**
 * \brief   Disables the ALE port from leaning an address
 *
 * \param   baseAddr      Base Address of the ALE module
 * \param   portNum       The port number 
 *
 * \return  None
 *
 **/
void CPSWALELearnModeDisable(unsigned int baseAddr, unsigned int  portNum)
{
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) |= 
                                         CPSW_ALE_PORTCTL0_NO_LEARN;
}

/**
 * \brief   Sets the rate limit for broadcast packets. Each prescale pulse 
 *          loads the rate limit into the port counter, which gets decremented
 *          with each packet received or transmitted. If the counters
 *          decrement to zero, then further packets are rate limited until the
 *          next prescale pulse. 
 *
 * \param   baseAddr      Base Address of the ALE module
 * \param   portNum       The port number
 * \param   rateLtVal     The rate limit value
 *
 * \return  None
 *
 **/
void CPSWALEBCastPktRateLimitSet(unsigned int baseAddr, unsigned int  portNum,
                                 unsigned int rateLtVal)
{
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) &= ~CPSW_ALE_PORTCTL0_BCAST_LIMIT;
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) |= 
                               (rateLtVal << CPSW_ALE_PORTCTL0_BCAST_LIMIT_SHIFT);
}

/**
 * \brief   Sets the rate limit for multicast packets. Each prescale pulse 
 *          loads the rate limit into the port counter, which gets decremented
 *          with each packet received or transmitted. If the counters
 *          decrement to zero, then further packets are rate limited until the
 *          next prescale pulse. 
 *
 * \param   baseAddr      Base Address of the ALE module
 * \param   portNum       The port number
 * \param   rateLtVal     The rate limit value
 *
 * \return  None
 *
 **/
void CPSWALEMCastPktRateLimitSet(unsigned int baseAddr, unsigned int  portNum,
                                 unsigned int rateLtVal)
{
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) &= ~CPSW_ALE_PORTCTL0_MCAST_LIMIT;
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) |= 
                               (rateLtVal << CPSW_ALE_PORTCTL0_MCAST_LIMIT_SHIFT);

}

/**
 * \brief   Enables the port for updating the source port number in the ALE 
 *          table entry.
 *
 * \param   baseAddr      Base Address of the ALE module
 * \param   portNum       The port number 
 *
 * \return  None
 *
 **/
void CPSWALESrcPortNumUpdateEnable(unsigned int baseAddr, unsigned int portNum)
{
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) &= 
                                           ~CPSW_ALE_PORTCTL0_NO_SA_UPDATE;
}

/**
 * \brief   Disables the port from updating the source port number in the ALE 
 *          table entry.
 *
 * \param   baseAddr      Base Address of the ALE module
 * \param   portNum       The port number 
 *
 * \return  None
 *
 **/
void CPSWALESrcPortNumUpdateDisable(unsigned int baseAddr, unsigned int portNum)
{
    HWREG(baseAddr + CPSW_ALE_PORTCTL(portNum)) |= 
                                           CPSW_ALE_PORTCTL0_NO_SA_UPDATE;
}

/**
 * \brief   Returns the prescale value for ALE. The input clock is divided
 *          by this value to use in the broadcast/multicast rate limiters.
 *
 * \param   baseAddr      Base Address of the ALE module
 *
 * \return  Prescale value
 *
 **/
unsigned int CPSWALEPrescaleGet(unsigned int baseAddr)
{
    return (HWREG(baseAddr + CPSW_ALE_PRESCALE) 
            & CPSW_ALE_PRESCALE_ALE_PRESCALE);   
}

/**
 * \brief   Sets the prescale value for ALE. The input clock is divided
 *          by this value to use in the broadcast/multicast rate limiters.
 *
 * \param   baseAddr      Base Address of the ALE module
 * \param   psVal         The prescale value
 *
 * \return  None
 *
 **/
void CPSWALEPrescaleSet(unsigned int baseAddr, unsigned int psVal)
{
    HWREG(baseAddr + CPSW_ALE_PRESCALE) |= psVal 
                                   & CPSW_ALE_PRESCALE_ALE_PRESCALE ;
}

/**
 * \brief   By calling this APIs, The ALE hardware removes (free up) any
 *          ageable table entry that does not have a set touch bit 
 *
 * \param   baseAddr      Base Address of the ALE module
 *
 * \return  None
 *
 **/
void CPSWALEAddrTblAgeOut(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_ALE_CONTROL) |= CPSW_ALE_CONTROL_AGE_OUT_NOW ;
   
    /* Wait till ageout completes */
    while(HWREG(baseAddr +  CPSW_ALE_CONTROL) & CPSW_ALE_CONTROL_AGE_OUT_NOW);
}

/**
 * \brief   Enables flooding of unknown unicast packets to port 0.
 *
 * \param   baseAddr      Base Address of the ALE module
 *
 * \return  None
 *
 **/
void CPSWALEP0UcastFloodEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_ALE_CONTROL) |= CPSW_ALE_CONTROL_EN_P0_UNI_FLOOD;
}

/**
 * \brief   Disables flooding of unknown unicast packets to port 0.
 *
 * \param   baseAddr      Base Address of the ALE module
 *
 * \return  None
 *
 **/
void CPSWALEP0UcastFloodDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_ALE_CONTROL) &= ~CPSW_ALE_CONTROL_EN_P0_UNI_FLOOD;
}

/**
 * \brief   Enables the bypassing of the ALE logic
 *
 * \param   baseAddr      Base Address of the ALE module
 *
 * \return  None
 *
 **/
void CPSWALEBypassEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_ALE_CONTROL) |= CPSW_ALE_CONTROL_ALE_BYPASS;
}

/**
 * \brief   Disables the bypassing of the ALE logic
 *
 * \param   baseAddr      Base Address of the ALE module
 *
 * \return  None
 *
 **/
void CPSWALEBypassDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_ALE_CONTROL) &= ~CPSW_ALE_CONTROL_ALE_BYPASS;
}

/**
 * \brief   Enables the receive flow control for CPSW for a given port
 *
 * \param   baseAddr      Base Address of the CPSW subsystem
 * \param   portNum       The port number
 *
 * \return  None
 *
 **/
void CPSWRxFlowControlEnable(unsigned int baseAddr, unsigned int portNum)
{
    HWREG(baseAddr + CPSW_SS_FLOW_CONTROL) |= (1 << portNum);
}

/**
 * \brief   Disables the receive flow control for CPSW for a given port
 *
 * \param   baseAddr      Base Address of the CPSW subsystem
 * \param   portNum       The port number
 *
 * \return  None
 *
 **/
void CPSWRxFlowControlDisable(unsigned int baseAddr, unsigned int portNum)
{
    HWREG(baseAddr + CPSW_SS_FLOW_CONTROL) &= ~(1 << portNum);
}

/**
 * \brief   Enables the software idle mode, causing the switch fabric to stop
 *          forward packets at the next start of packet.
 *
 * \param   baseAddr      Base Address of the CPSW subsystem
 *
 * \return  None
 *
 **/
void CPSWSoftwareIdleEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SS_SOFT_IDLE) |= CPSW_SS_SOFT_IDLE_SOFT_IDLE; 
}

/**
 * \brief   Disables the software idle mode, causing the switch fabric to 
 *          forward packets at the next start of packet.
 *
 * \param   baseAddr      Base Address of the CPSW subsystem
 *
 * \return  None
 *
 **/
void CPSWSoftwareIdleDisable(unsigned int baseAddr, unsigned int portNum)
{
    HWREG(baseAddr + CPSW_SS_SOFT_IDLE) &= ~CPSW_SS_SOFT_IDLE_SOFT_IDLE;
}

/**
 * \brief   Enables the CPSW statistics for the given port
 *
 * \param   baseAddr      Base Address of the CPSW subsystem
 * \param   portNum       The port number
 *
 * \return  None
 *
 **/
void CPSWStatisticsEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SS_STAT_PORT_EN) = CPSW_SS_STAT_PORT_EN_P0_STAT_EN
                                             | CPSW_SS_STAT_PORT_EN_P1_STAT_EN
                                             | CPSW_SS_STAT_PORT_EN_P2_STAT_EN;
}

/**
 * \brief   Disables the VLAN aware mode for CPSW
 *
 * \param   baseAddr      Base Address of the CPSW subsystem
 *
 * \return  None
 *
 **/
void CPSWVLANAwareDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_SS_CONTROL) &= ~CPSW_SS_CONTROL_VLAN_AWARE;
}

/**
 * \brief   Sets the ethernet address at the CPSW port
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers
 * \param   ethAddr       Start address of the 6 byte ethernet address
 *
 * \return  None
 *
 **/
void CPSWPortSrcAddrSet(unsigned int baseAddr, unsigned char *ethAddr)
{
 
    HWREG(baseAddr + CPSW_PORT_SA_HI) = 
                   ethAddr[0] 
                   | (ethAddr[1] << CPSW_PORT_P1_SA_HI_MACSRCADDR_39_32_SHIFT)
                   | (ethAddr[2] << CPSW_PORT_P1_SA_HI_MACSRCADDR_31_24_SHIFT) 
                   | (ethAddr[3] << CPSW_PORT_P1_SA_HI_MACSRCADDR_23_16_SHIFT);
    HWREG(baseAddr + CPSW_PORT_SA_LO) = 
                   ethAddr[4]  
                   | (ethAddr[5] << CPSW_PORT_P1_SA_LO_MACSRCADDR_7_0_SHIFT);
}

/**
 * \brief   Sets the Tx Header Priority to Switch Pri Mapping 
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 * \param   txPriMap      TX header priority map
 *
 * \return  None
 *
 **/
void CPSWPortTxHdrPriMapSet(unsigned int baseAddr, unsigned int txPriMap)
{
    HWREG(baseAddr + CPSW_PORT_TX_PRI_MAP) = txPriMap;
}

/**
 * \brief   Sets the maximum number of 1k memory blocks that may be allocated 
 *          to the FIFOs logical transmit priority queues. Port 0 should
 *          remain in flow control mode. 0x10 is the recommended value.
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 * \param   maxBlks       Number of blocks 
 *
 * \return  None
 *
 **/
void CPSWPortTxMaxBlksSet(unsigned int baseAddr, unsigned int maxBlks)
{
    HWREG(baseAddr + CPSW_PORT_MAX_BLKS) &= 
                       ~(CPSW_PORT_P0_MAX_BLKS_P0_TX_MAX_BLKS);
 
    HWREG(baseAddr + CPSW_PORT_MAX_BLKS) |= 
                       (maxBlks << CPSW_PORT_P0_MAX_BLKS_P0_TX_MAX_BLKS_SHIFT);
}

/**
 * \brief   Sets the maximum number of 1k memory blocks that may be allocated
 *          to the FIFOs logical receive queues. 0x4 is the recommended value. 
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 * \param   maxBlks       Number of blocks 
 *           0x3 <= 'maxBlks' <= 0x6
 *
 * \return  None
 *
 **/
void CPSWPortRxMaxBlksSet(unsigned int baseAddr, unsigned int maxBlks)
{
    HWREG(baseAddr + CPSW_PORT_MAX_BLKS) &= 
                       ~(CPSW_PORT_P0_MAX_BLKS_P0_RX_MAX_BLKS);
 
    HWREG(baseAddr + CPSW_PORT_MAX_BLKS) |= 
                       (maxBlks << CPSW_PORT_P0_MAX_BLKS_P0_RX_MAX_BLKS_SHIFT);
}

/**
 * \brief   Enables transmit FIFO input rate
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 *
 * \return  None
 *
 **/
void CPSWPortTxFIFOInputRateEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) |= CPSW_PORT_P0_TX_IN_CTL_TX_RATE_EN;
}

/**
 * \brief   Disables transmit FIFO input rate
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 *
 * \return  None
 *
 **/
void CPSWPortTxFIFOInputRateDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) &= ~CPSW_PORT_P0_TX_IN_CTL_TX_RATE_EN;
}

/**
 * \brief   Sets transmit FIFO input queue type to normal priority mode.
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 *
 * \return  None
 *
 **/
void CPSWPortTxFIFONormalPrioModeSet(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) &= ~CPSW_PORT_P0_TX_IN_CTL_TX_IN_SEL;
}

/**
 * \brief   Sets transmit FIFO input queue type to Dual MAC mode.
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 * \param   blksRem       Transmit FIFO Input Blocks to subtract
 *
 * \return  None
 *
 **/
void CPSWPortTxFIFODualMACModeSet(unsigned int baseAddr, unsigned int blksRem)
{
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) &= ~(CPSW_PORT_P0_TX_IN_CTL_TX_IN_SEL
                                         | CPSW_PORT_P0_TX_IN_CTL_TX_BLKS_REM);
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) |= 
                     (CPSW_PORT_DUAL_MAC_MODE
                      | (blksRem << CPSW_PORT_P0_TX_IN_CTL_TX_BLKS_REM_SHIFT));
}

/**
 * \brief   Sets transmit FIFO input queue type to Rate Limit mode
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 *
 * \return  None
 *
 **/
void CPSWPortRateLimitModeSet(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) &= ~CPSW_PORT_P0_TX_IN_CTL_TX_IN_SEL;
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) |= CPSW_PORT_RATE_LIM_MODE;
}

/**
 * \brief   Sets the transmit FIFO words in queue.
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 * \param   txPriWds      Base address of the CPSW Port Module registers.
 *
 * \return  None
 *
 **/
void CPSWPortTxFIFOWordsSet(unsigned int baseAddr, unsigned int txPriWds)
{
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) &= ~CPSW_PORT_P0_TX_IN_CTL_TX_PRI_WDS;
    HWREG(baseAddr + CPSW_PORT_TX_IN_CTL) |= txPriWds;
}

/**
 * \brief   Sets the RX DSCP priority map. 
 *
 * \param   baseAddr      Base address of the CPSW Port Module registers.
 * \param   priReg        The priority Register to be set
 * \param   priMap        The priority map in hex.
 *
 * \return  None
 *
 **/
void CPSWPortRxDscpPriMapSet(unsigned int baseAddr, unsigned int priReg,
                             unsigned int priMap)
{
    HWREG(baseAddr + CPSW_PORT_RX_DSCP_PRI_MAP(priReg)) = priMap;
}

/**
 * \brief   Returns the requested CPSW Statistics
 *
 * \param   baseAddr      Base address of the CPSW Status Module registers.
 * \param   statReg       Statistics Register to be read
 *
 * \return  The requested statistics
 *
 **/
unsigned int CPSWStatisticsGet(unsigned int baseAddr, unsigned int statReg)
{
    return (HWREG(baseAddr + statReg));
}

/**
 * \brief   Sets the the threshold value for issuing receive threshold 
 *          pending interrupts
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 * \param   channel       Channel number 
 * \param   threshVal     Threshold value to be set
 *
 * \return  None
 *
 **/
void CPSWCPDMARxFlowThreshSet(unsigned int baseAddr, unsigned int channel, 
                              unsigned int threshVal)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_PENDTHRESH(channel)) = threshVal;
}

/**
 * \brief   Commands the TX teardown for the specified channel
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 * \param   channel       Channel number
 *
 * \return  None
 *
 **/
void CPSWCPDMATxChanTearDown(unsigned int baseAddr, unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_TX_TEARDOWN) = channel;
}

/**
 * \brief   Commands the RX teardown for the specified channel
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 * \param   channel       Channel number
 *
 * \return  None
 *
 **/
void CPSWCPDMARxChanTearDown(unsigned int baseAddr, unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_TEARDOWN) = channel;
}

/**
 * \brief   Resets the CPDMA
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 *
 * \return  None
 *
 **/
void CPSWCPDMAReset(unsigned int baseAddr)
{
    unsigned int cnt;

    /* Reset the CPDMA */
    HWREG(baseAddr + CPSW_CPDMA_CPDMA_SOFT_RESET) =  
                     CPSW_CPDMA_CPDMA_SOFT_RESET_SOFT_RESET;

    /* Wait till the reset completes */
    while(HWREG(baseAddr + CPSW_CPDMA_CPDMA_SOFT_RESET)
          & CPSW_CPDMA_CPDMA_SOFT_RESET_SOFT_RESET);
 
    /* Initialize all the header descriptor pointer registers */
    for(cnt =  0; cnt< CPSW_MAX_HEADER_DESC; cnt++)
    {
        HWREG(baseAddr + CPSW_CPDMA_TX_HDP(cnt)) = 0;
        HWREG(baseAddr + CPSW_CPDMA_RX_HDP(cnt)) = 0;
        HWREG(baseAddr + CPSW_CPDMA_TX_CP(cnt)) = 0;
        HWREG(baseAddr + CPSW_CPDMA_RX_CP(cnt)) = 0;
    }
}

/**
 * \brief   Enables the TXPULSE Interrupt Generation.
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 * \param   channel       Channel number for which interrupt to be enabled
 *
 * \return  None
 *
 **/
void CPSWCPDMATxIntEnable(unsigned int baseAddr, unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_TX_INTMASK_SET) |= (1 << channel);
}

/**
 * \brief   Enables the RXPULSE Interrupt Generation.
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 * \param   channel       Channel number for which interrupt to be enabled
 *
 * \return  None
 *
 **/
void CPSWCPDMARxIntEnable(unsigned int baseAddr, unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_INTMASK_SET) |= (1 << channel);
}

/**
 * \brief   Disables the TXPULSE Interrupt Generation.
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 * \param   channel       Channel number for which interrupt to be disabled
 *
 * \return  None
 *
 **/
void CPSWCPDMATxIntDisable(unsigned int baseAddr, unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_TX_INTMASK_CLEAR) |= (1 << channel);

}

/**
 * \brief   Disables the RXPULSE Interrupt Generation.
 *
 * \param   baseAddr      Base address of the CPDMA Module registers.
 * \param   channel       Channel number for which interrupt to be disabled
 *
 * \return  None
 *
 **/
void CPSWCPDMARxIntDisable(unsigned int baseAddr, unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_INTMASK_CLEAR) |= (1 << channel);

}

/**
 * \brief   API to enable the transmit in the TX Control Register.
 *          After the transmit is enabled, any write to TXHDP of
 *          a channel will start transmission
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 *
 * \return  None
 *
 **/
void CPSWCPDMATxEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_CPDMA_TX_CONTROL) = CPSW_CPDMA_TX_CONTROL_TX_EN;
}

/**
 * \brief   API to enable the receive in the RX Control Register.
 *          After the receive is enabled, and write to RXHDP of
 *          a channel, the data can be received in the destination
 *          specified by the corresponding RX buffer descriptor.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 *
 * \return  None
 *
 **/
void CPSWCPDMARxEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_CONTROL) = CPSW_CPDMA_RX_CONTROL_RX_EN;
}

/**
 * \brief   API to write the TX HDP register. If transmit is enabled,
 *          write to the TX HDP will immediately start transmission.
 *          The data will be taken from the buffer pointer of the TX buffer
 *          descriptor written to the TX HDP
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 * \param   descHdr       Address of the TX buffer descriptor
 * \param   channel       Channel Number
 *
 * \return  None
 *
 **/
void CPSWCPDMATxHdrDescPtrWrite(unsigned int baseAddr, unsigned int descHdr,
                                unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_TX_HDP(channel)) = descHdr;
}

/**
 * \brief   API to write the RX HDP register. If receive is enabled,
 *          write to the RX HDP will enable data reception to point to
 *          the corresponding RX buffer descriptor's buffer pointer.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 * \param   descHdr       Address of the RX buffer descriptor
 * \param   channel       Channel Number
 *
 * \return  None
 *
 **/
void CPSWCPDMARxHdrDescPtrWrite(unsigned int baseAddr, unsigned int descHdr,
                                unsigned int channel)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_HDP(channel)) = descHdr;
}

/**
 * \brief   Writes the DMA End Of Interrupt Vector.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 * \param   eoiFlag       Type of interrupt to acknowledge to the CPDMA
 *            'eoiFlag' can take the following values \n                            
 *                CPSW_EOI_TX_PULSE - TX Pulse Interrupt \n                                    
 *                CPSW_EOI_RX_PULSE - RX Pulse Interrupt \n                                    
 *                CPSW_EOI_RX_THRESH_PULSE - RX Pulse Threshold Interrupt \n                   
 *                CPSW_EOI_MISC_PULSE - Misc Interrupt \n                                      
 *
 * \return  None
 * 
 **/
void CPSWCPDMAEndOfIntVectorWrite(unsigned int baseAddr, unsigned int eoiFlag)
{
    /* Acknowledge the CPDMA */
    HWREG(baseAddr + CPSW_CPDMA_CPDMA_EOI_VECTOR) = eoiFlag;
}

/**
 * \brief   Writes the the TX Completion Pointer for a specific channel
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 * \param   channel       Channel Number.
 * \param   comPtr        Completion Pointer Value to be written
 *
 * \return  None
 *
 **/
void CPSWCPDMATxCPWrite(unsigned int baseAddr, unsigned int channel, 
                        unsigned int comPtr)
{
    HWREG(baseAddr + CPSW_CPDMA_TX_CP(channel)) = comPtr;
}

/**
 * \brief   Writes the the RX Completion Pointer for a specific channel
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 * \param   channel       Channel Number.
 * \param   comPtr        Completion Pointer Value to be written
 *
 * \return  None
 *
 **/
void CPSWCPDMARxCPWrite(unsigned int baseAddr, unsigned int channel, 
                        unsigned int comPtr)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_CP(channel)) = comPtr;
}

/**
 * \brief   Set the free buffers for a specific channel
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 * \param   channel       Channel Number.
 * \param   nBuf          Number of free buffers
 *
 * \return  None
 *
 **/
void CPSWCPDMANumFreeBufSet(unsigned int baseAddr, unsigned int channel,
                            unsigned int nBuf)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_FREEBUFFER(channel)) = nBuf;
}

/**
 * \brief   Returns the CPDMA Status.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers.
 * \param   statFlag      The status flags to be read
 *     'statFlag' can take one of the following values \n
 *            CPDMA_STAT_IDLE - to check if CPDMA is idle. \n
 *            CPDMA_STAT_TX_HOST_ERR_CODE - TX host error code. \n
 *            CPDMA_STAT_TX_HOST_ERR_CHAN - TX host error channel. \n
 *            CPDMA_STAT_RX_HOST_ERR_CODE - RX host error code. \n
 *            CPDMA_STAT_RX_HOST_ERR_CHAN - RX host error channel. \n
 * 
 * \return  the DMA status for the status flag passed.
 *          The return values for CPDMA_STAT_IDLE are, \n
 *            CPDMA_STAT_IDLE - CPDMA is in idle state \n
 *            CPDMA_STAT_NOT_IDLE - CPDMA is not in idle state \n
 *
 *          The return values for CPDMA_STAT_TX_HOST_ERR_CODE are, \n
 *            CPDMA_STAT_TX_NO_ERR - No error \n 
 *            CPDMA_STAT_TX_SOP_ERR - SOP error \n 
 *            CPDMA_STAT_TX_OWN_ERR - Ownership bit not
 *                                                  set in SOP buffer \n 
 *            CPDMA_STAT_TX_ZERO_DESC - Zero Next Buffer 
 *                                       Descriptor Pointer Without EOP \n 
 *            CPDMA_STAT_TX_ZERO_BUF_PTR - Zero Buffer Pointer \n 
 *            CPDMA_STAT_TX_ZERO_BUF_LEN - Zero Buffer Length \n 
 *            CPDMA_STAT_TX_PKT_LEN_ERR - Packet Length Error \n 
 *
 *          The return values for CPDMA_STAT_RX_HOST_ERR_CODE are, \n
 *            CPDMA_STAT_RXi_NO_ERR - No error \n 
 *            CPDMA_STAT_RX_OWN_NOT_SET - Ownership bit not set in 
                                          input buffer \n
 *            CPDMA_STAT_RX_ZERO_BUF_PTR - Zero Buffer Pointer\n 
 *            CPDMA_STAT_RX_ZERO_BUF_LEN - Zero Buffer Length on 
 *                                       non-SOP descriptor \n 
 *            CPDMA_STAT_RX_SOP_BUF_LEN_ERR - SOP buffer length not
 *                                       greater than offset\n 
 *
 **/
unsigned int CPSWCPDMAStatusGet(unsigned int baseAddr, unsigned int statFlag)
{
    return (((HWREG(baseAddr + CPSW_CPDMA_DMASTATUS)) & statFlag) 
            >> (statFlag & CPDMA_ERR_CHANNEL_POS));   
}

/**
 * \brief   Configures the CPDMA module by writing the configuration value
 *          to the DMA control register. 
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 * \param   cfg           CPDMA configuration written to control register
 *     'cfg' shall be CPDMA_CFG(tx_rlim, rx_cef, cmd_idle, 
 *                              rx_offlen_blk, rx_own, tx_ptype). \n
 *        The parameter 'tx_rlim' to CPDMA_CFG can take one of the below
 *        values, showing which all channels are rate-limited. \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7 \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7_TO_6 \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7_TO_5 \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7_TO_4 \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7_TO_3 \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7_TO_2 \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7_TO_1 \n
 *            CPDMA_CFG_TX_RATE_LIM_CH_7_TO_0 \n
 *        The parameter 'rx_cef' to CPDMA_CFG can take one of the below 
 *        values \n
 *            CPDMA_CFG_COPY_ERR_FRAMES - To copy error frames to memory \n
 *            CPDMA_CFG_NO_COPY_ERR_FRAMES - Not to copy error frames \n
 *        The parameter 'cmd_idle' to CPDMA_CFG can take one of the below
 *        values \n
 *            CPDMA_CFG_IDLE_COMMAND - Idle commanded \n
 *            CPDMA_CFG_IDLE_COMMAND_NONE - Idle not commanded \n
 *        The parameter 'rx_offlen_blk' to CPDMA_CFG can take one of the below 
 *        values \n
 *            CPDMA_CFG_BLOCK_RX_OFF_LEN_WRITE - Block the DMA writes to the
 *                                               offset/length field during 
 *                                               packet processing. \n
 *            CPDMA_CFG_NOT_BLOCK_RX_OFF_LEN_WRITE - Do not Block the DMA writes
 *                                              to the offset/length field during 
 *                                              packet processing. \n
 *        The parameter 'rx_own' to CPDMA_CFG can take one of the below 
 *        values \n
 *            CPDMA_CFG_RX_OWN_1 - The CPDMA writes 1 to the ownership bit at 
 *                                 the end of packet processing. \n
 *            CPDMA_CFG_RX_OWN_0 - The CPDMA writes 0 to the ownership bit at 
 *                                 the end of packet processing. \n
 *        The parameter 'tx_ptype' to CPDMA_CFG can take one of the below 
 *        values \n
 *            CPDMA_CFG_TX_PRI_ROUND_ROBIN - The next channel for transmit is
 *                                           chosen round-robin. \n 
 *            CPDMA_CFG_TX_PRI_FIXED - The next channel for transmit is
 *                                     chosen priority based, channel 7 with the
 *                                     highest priority \n 
 *
 * \return  None
 *
 **/
void CPSWCPDMAConfig(unsigned int baseAddr, unsigned int cfg)
{
    HWREG(baseAddr + CPSW_CPDMA_DMACONTROL) = cfg;
}

/**
 * \brief   Enable the command idle mode for CPDMA. When this API is called, the
 *          CPSW stops all the reception and transmission. However, if receiving
 *          the current frame will be received completely before going to the idle
 *          state. Also, while transmitting, the contents in the fifo will be sent
 *          fully.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 *
 * \return  None
 *
 **/
void CPSWCPDMACmdIdleEnable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_CPDMA_DMACONTROL) |= CPSW_CPDMA_DMACONTROL_CMD_IDLE;
    
    /* Wait till the state changes to idle */
    while((HWREG(baseAddr + CPSW_CPDMA_DMASTATUS) & CPSW_CPDMA_DMASTATUS_IDLE)
          != CPSW_CPDMA_DMASTATUS_IDLE); 
}

/**
 * \brief   Disable the command idle mode for CPDMA. 
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 *
 * \return  None
 *
 **/
void CPSWCPDMACmdIdleDisable(unsigned int baseAddr)
{
    HWREG(baseAddr + CPSW_CPDMA_DMACONTROL) &= ~CPSW_CPDMA_DMACONTROL_CMD_IDLE;
}

/**
 * \brief   Sets the RX buffer offset value. The RX buffer offset will be
 *          written by the port into each frame SOP buffer descriptor 
 *          buffer_offset field. The frame data will begin after the 
 *          rx_buffer_offset value of bytes. This value will be used for  
 *          all the channels .
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 * \param   bufOff        Buffer offset value
 *
 * \return  None
 *
 **/
void CPSWCPDMARxBufOffsetSet(unsigned int baseAddr, unsigned int bufOff)
{
    HWREG(baseAddr + CPSW_CPDMA_RX_BUFFER_OFFSET) = bufOff;
}

/**
 * \brief   Returns the raw transmit interrupt pending status.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 * \param   chanMask      Channel Mask
 *    'chanMask' can be given for one or more channels. \n
 *         0x01- for 0th channel, 0x80 for 7th channel, 0x81 for both 0th
 *         and 7th channel etc. \n
 *
 * \return  Raw receive interrupt status \n
 *          bits for the 'chanMask' will be set if interrupt is pending \n
 *          bits for the 'chanMask' will be clear if interrupt is not
 *                                                              pending \n
 *
 **/
unsigned int CPSWCPDMATxIntStatRawGet(unsigned int baseAddr, 
                                      unsigned int chanMask)
{
    return (HWREG(baseAddr + CPSW_CPDMA_TX_INTSTAT_RAW) & chanMask);
}

/**
 * \brief   Returns the masked transmit interrupt pending status.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 * \param   chanMask      Channel Mask
 *    'chanMask' can be given for one or more channels. \n
 *         0x01- for 0th channel, 0x80 for 7th channel, 0x81 for both 0th
 *         and 7th channel etc. \n
 *
 * \return  Masked transmit interrupt status \n
 *          bits for the 'chanMask' will be set if interrupt is pending \n
 *          bits for the 'chanMask' will be cleared if interrupt is not
 *                                                              pending \n
 *
 **/
unsigned int CPSWCPDMATxIntStatMaskedGet(unsigned int baseAddr, 
                                         unsigned int chanMask)
{
    return (HWREG(baseAddr + CPSW_CPDMA_TX_INTSTAT_MASKED) & chanMask);
}

/**
 * \brief   Returns the raw receive interrupt pending status.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 * \param   chanMask      Channel Mask
 * \param   intType       Interrupt type
 *    'chanMask' can be given for one or more channels. \n
 *         0x01- for 0th channel, 0x80 for 7th channel, 0x81 for both 0th
 *         and 7th channel etc. \n
 *    'intType' can take one of the following values. \n
 *         CPDMA_RX_INT_THRESH_PEND - RX threshold interrupt pending \n               
 *         CPDMA_RX_INT_PULSE_PEND - RX pulse interrupt pending \n                    
 *
 * \return  Raw receive interrupt status \n
 *          bits for the 'chanMask' will be set if interrupt is pending \n
 *          bits for the 'chanMask' will be cleared if interrupt is not
 *                                                              pending \n
 *
 **/
unsigned int CPSWCPDMARxIntStatRawGet(unsigned int baseAddr, 
                                      unsigned int chanMask, 
                                      unsigned int intType)
{
    return ((HWREG(baseAddr + CPSW_CPDMA_RX_INTSTAT_RAW) >> intType)
             & chanMask);    
}

/**
 * \brief   Returns the masked receive interrupt pending status.
 *
 * \param   baseAddr      Base Address of the CPDMA module registers
 * \param   chanMask      Channel Mask
 * \param   intType       Interrupt type
 *    'chanMask' can be given for one or more channels. \n
 *         0x01- for 0th channel, 0x80 for 7th channel, 0x81 for both 0th
 *         and 7th channel etc. \n
 *    'intType' can take one of the following values. \n
 *         CPDMA_RX_INT_THRESH_PEND - RX threshold interrupt pending \n               
 *         CPDMA_RX_INT_PULSE_PEND - RX pulse interrupt pending \n                    
 *
 * \return  Masked receive interrupt status \n
 *          bits for the 'chanMask' will be set if interrupt is pending \n
 *          bits for the 'chanMask' will be cleared if interrupt is not 
 *                                                              pending \n
 *
 **/
unsigned int CPSWCPDMARxIntStatMaskedGet(unsigned int baseAddr, 
                                         unsigned int chanMask,
                                         unsigned int intType)
{
    return ((HWREG(baseAddr + CPSW_CPDMA_RX_INTMASK_SET) >> intType) 
             & chanMask);    
}

/**
 * \brief   Saves the CPSW register context. This can be used while going
 *          to power down mode where CPSW power will be cut down.
 *
 * \param   contextPtr   Pointer to the structure where CPSW register context
 *                       need to be saved.
 *
 * \return  None
 *
 **/
void CPSWContextSave(CPSWCONTEXT *contextPtr)
{
    unsigned int idx;
    unsigned int *cppiDest = (unsigned int*)contextPtr->cppiRamBase;

    CPSWCPDMACmdIdleEnable(contextPtr->cpdmaBase);

    /* Restore the CPPI RAM contents */
    for(idx = 0; idx < (CPSW_SIZE_CPPI_RAM / 4); idx++, cppiDest++)
    {
        contextPtr->cppiRam[idx] = *cppiDest;
    }

    contextPtr->aleCtrl = HWREG(contextPtr->aleBase + CPSW_ALE_CONTROL);
    contextPtr->alePortCtl[0] = HWREG(contextPtr->aleBase + CPSW_ALE_PORTCTL(0));
    contextPtr->alePortCtl[1] = HWREG(contextPtr->aleBase + CPSW_ALE_PORTCTL(1));
    contextPtr->alePortCtl[2] = HWREG(contextPtr->aleBase + CPSW_ALE_PORTCTL(2));

    for(idx = 0; idx < CPSW_MAX_NUM_ALE_ENTRY; idx++)
    {
        CPSWALETableEntryGet(contextPtr->aleBase, idx, 
                             &(contextPtr->aleEntry[idx * 3]));
    } 

    contextPtr->ssStatPortEn = HWREG(contextPtr->ssBase + CPSW_SS_STAT_PORT_EN);
    contextPtr->port1SaHi = HWREG(contextPtr->port1Base + CPSW_PORT_SA_HI);
    contextPtr->port1SaLo = HWREG(contextPtr->port1Base + CPSW_PORT_SA_LO);
    contextPtr->port2SaHi = HWREG(contextPtr->port2Base + CPSW_PORT_SA_HI);
    contextPtr->port2SaLo = HWREG(contextPtr->port2Base + CPSW_PORT_SA_LO);
    contextPtr->cpdmaRxFB = HWREG(contextPtr->cpdmaBase 
                                  + CPSW_CPDMA_RX_FREEBUFFER(0));
    contextPtr->cpdmaTxCtl = HWREG(contextPtr->cpdmaBase 
                                   + CPSW_CPDMA_TX_CONTROL);
    contextPtr->cpdmaRxCtl = HWREG(contextPtr->cpdmaBase
                                   + CPSW_CPDMA_RX_CONTROL);
    contextPtr->cpdmaRxHdp = HWREG(contextPtr->cpdmaBase 
                                   + CPSW_CPDMA_RX_HDP(0));
    contextPtr->txIntMaskSet = HWREG(contextPtr->cpdmaBase
                                     + CPSW_CPDMA_TX_INTMASK_SET);
    contextPtr->wrCoreIntTxPulse = HWREG(contextPtr->wrBase
                                         + CPSW_WR_C_RX_THRESH_EN(0) + 0x04);
    contextPtr->rxIntMaskSet = HWREG(contextPtr->cpdmaBase
                                     + CPSW_CPDMA_RX_INTMASK_SET);
    contextPtr->wrCoreIntRxPulse = HWREG(contextPtr->wrBase
                                         + CPSW_WR_C_RX_THRESH_EN(0) + 0x08);
    contextPtr->sl1MacCtl = HWREG(contextPtr->sl1Base + CPSW_SL_MACCONTROL);
    contextPtr->sl2MacCtl = HWREG(contextPtr->sl2Base + CPSW_SL_MACCONTROL);
}

/**
 * \brief   Restores the CPSW register context. This can be used while coming
 *          back from power down mode where CPSW power will be cut down.
 *
 * \param   contextPtr   Pointer to the structure where CPSW register context
 *                       need to be restored from.
 *
 * \return  None
 *
 **/
void CPSWContextRestore(CPSWCONTEXT *contextPtr)
{
    unsigned int idx;
    unsigned int *cppiDest = (unsigned int*)contextPtr->cppiRamBase; 

    /* Restore the CPPI RAM contents */
    for(idx = 0; idx < (CPSW_SIZE_CPPI_RAM / 4); idx++, cppiDest++)
    {
        *cppiDest = contextPtr->cppiRam[idx] ;
    }

    HWREG(contextPtr->aleBase + CPSW_ALE_CONTROL) = contextPtr->aleCtrl;
    HWREG(contextPtr->aleBase + CPSW_ALE_PORTCTL(0)) = contextPtr->alePortCtl[0];
    HWREG(contextPtr->aleBase + CPSW_ALE_PORTCTL(1)) = contextPtr->alePortCtl[1];
    HWREG(contextPtr->aleBase + CPSW_ALE_PORTCTL(2)) = contextPtr->alePortCtl[2];

    for(idx = 0; idx < CPSW_MAX_NUM_ALE_ENTRY; idx++)
    {
        CPSWALETableEntrySet(contextPtr->aleBase, idx,
                             &(contextPtr->aleEntry[idx * 3]));
    }

    HWREG(contextPtr->ssBase + CPSW_SS_STAT_PORT_EN) = contextPtr->ssStatPortEn;
    HWREG(contextPtr->port1Base + CPSW_PORT_SA_HI) = contextPtr->port1SaHi;
    HWREG(contextPtr->port1Base + CPSW_PORT_SA_LO) = contextPtr->port1SaLo;
    HWREG(contextPtr->port2Base + CPSW_PORT_SA_HI) = contextPtr->port2SaHi;
    HWREG(contextPtr->port2Base + CPSW_PORT_SA_LO) = contextPtr->port2SaLo;
    HWREG(contextPtr->cpdmaBase + CPSW_CPDMA_RX_FREEBUFFER(0)) =
                                contextPtr->cpdmaRxFB;
    HWREG(contextPtr->cpdmaBase + CPSW_CPDMA_TX_CONTROL) 
                                 = contextPtr->cpdmaTxCtl;
    HWREG(contextPtr->cpdmaBase + CPSW_CPDMA_RX_CONTROL)
                                 = contextPtr->cpdmaRxCtl;
    HWREG(contextPtr->cpdmaBase + CPSW_CPDMA_RX_HDP(0))
                                 = contextPtr->cpdmaRxHdp;
    HWREG(contextPtr->cpdmaBase + CPSW_CPDMA_TX_INTMASK_SET) 
                                 = contextPtr->txIntMaskSet;
    HWREG(contextPtr->wrBase + CPSW_WR_C_RX_THRESH_EN(0) + 0x04) 
                                 = contextPtr->wrCoreIntTxPulse;
    HWREG(contextPtr->cpdmaBase + CPSW_CPDMA_RX_INTMASK_SET)
                                 = contextPtr->rxIntMaskSet;
    HWREG(contextPtr->wrBase + CPSW_WR_C_RX_THRESH_EN(0) + 0x08)
                                 =  contextPtr->wrCoreIntRxPulse;
    HWREG(contextPtr->sl1Base + CPSW_SL_MACCONTROL) = contextPtr->sl1MacCtl;
    HWREG(contextPtr->sl2Base + CPSW_SL_MACCONTROL) = contextPtr->sl2MacCtl;
}

/***************************** End Of File ***********************************/
