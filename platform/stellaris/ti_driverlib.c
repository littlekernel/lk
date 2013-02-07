// Functions taken from the Texas Instruments Stellarisware driverlib

//*****************************************************************************
//
// Copyright (c) 2005-2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
// 
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the  
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This is part of revision 9453 of the Stellaris Peripheral Driver Library.
//
//*****************************************************************************


#include "ti_driverlib.h"

#include "inc/hw_uart.h"
#include "inc/hw_nvic.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"

static tBoolean UARTBaseValid(unsigned long ulBase) { return true; }

#define ASSERT(X)


//*****************************************************************************
//
// The system clock divider defining the maximum baud rate supported by the
// UART.
//
//*****************************************************************************
#define UART_CLK_DIVIDER        ((CLASS_IS_SANDSTORM ||                       \
                                  (CLASS_IS_FURY && REVISION_IS_A2) ||        \
                                  (CLASS_IS_DUSTDEVIL && REVISION_IS_A0)) ?   \
                                 16 : 8)


//*****************************************************************************
//
//! Disables transmitting and receiving.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function clears the UARTEN, TXE, and RXE bits, waits for the end of
//! transmission of the current character, and flushes the transmit FIFO.
//!
//! \return None.
//
//*****************************************************************************
void
UARTDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Wait for end of TX.
    //
    while(HWREG(ulBase + UART_O_FR) & UART_FR_BUSY)
    {
    }

    //
    // Disable the FIFO.
    //
    HWREG(ulBase + UART_O_LCRH) &= ~(UART_LCRH_FEN);

    //
    // Disable the UART.
    //
    HWREG(ulBase + UART_O_CTL) &= ~(UART_CTL_UARTEN | UART_CTL_TXE |
                                    UART_CTL_RXE);
}


//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \param ulBase is the base address of the UART port.
//! \param bMasked is \b false if the raw interrupt status is required and
//! \b true if the masked interrupt status is required.
//!
//! This function returns the interrupt status for the specified UART.  Either
//! the raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \return Returns the current interrupt status, enumerated as a bit field of
//! values described in UARTIntEnable().
//
//*****************************************************************************
unsigned long
UARTIntStatus(unsigned long ulBase, tBoolean bMasked)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(HWREG(ulBase + UART_O_MIS));
    }
    else
    {
        return(HWREG(ulBase + UART_O_RIS));
    }
}


//*****************************************************************************
//
//! Clears UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is a bit mask of the interrupt sources to be cleared.
//!
//! The specified UART interrupt sources are cleared, so that they no longer
//! assert.  This function must be called in the interrupt handler to keep the
//! interrupt from being triggered again immediately upon exit.
//!
//! The \e ulIntFlags parameter has the same definition as the \e ulIntFlags
//! parameter to UARTIntEnable().
//!
//! \note Because there is a write buffer in the Cortex-M processor, it may
//! take several clock cycles before the interrupt source is actually cleared.
//! Therefore, it is recommended that the interrupt source be cleared early in
//! the interrupt handler (as opposed to the very last action) to avoid
//! returning from the interrupt handler before the interrupt source is
//! actually cleared.  Failure to do so may result in the interrupt handler
//! being immediately reentered (because the interrupt controller still sees
//! the interrupt source asserted).
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntClear(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Clear the requested interrupt sources.
    //
    HWREG(ulBase + UART_O_ICR) = ulIntFlags;
}


//*****************************************************************************
//
//! Enables individual UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is the bit mask of the interrupt sources to be enabled.
//!
//! This function enables the indicated UART interrupt sources.  Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! The \e ulIntFlags parameter is the logical OR of any of the following:
//!
//! - \b UART_INT_9BIT - 9-bit Address Match interrupt
//! - \b UART_INT_OE - Overrun Error interrupt
//! - \b UART_INT_BE - Break Error interrupt
//! - \b UART_INT_PE - Parity Error interrupt
//! - \b UART_INT_FE - Framing Error interrupt
//! - \b UART_INT_RT - Receive Timeout interrupt
//! - \b UART_INT_TX - Transmit interrupt
//! - \b UART_INT_RX - Receive interrupt
//! - \b UART_INT_DSR - DSR interrupt
//! - \b UART_INT_DCD - DCD interrupt
//! - \b UART_INT_CTS - CTS interrupt
//! - \b UART_INT_RI - RI interrupt
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Enable the specified interrupts.
    //
    HWREG(ulBase + UART_O_IM) |= ulIntFlags;
}


//*****************************************************************************
//
//! Receives a character from the specified port.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function gets a character from the receive FIFO for the specified
//! port.
//!
//! This function replaces the original UARTCharNonBlockingGet() API and
//! performs the same actions.  A macro is provided in <tt>uart.h</tt> to map
//! the original API to this API.
//!
//! \return Returns the character read from the specified port, cast as a
//! \e long.  A \b -1 is returned if there are no characters present in the
//! receive FIFO.  The UARTCharsAvail() function should be called before
//! attempting to call this function.
//
//*****************************************************************************
long
UARTCharGetNonBlocking(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // See if there are any characters in the receive FIFO.
    //
    if(!(HWREG(ulBase + UART_O_FR) & UART_FR_RXFE))
    {
        //
        // Read and return the next character.
        //
        return(HWREG(ulBase + UART_O_DR));
    }
    else
    {
        //
        // There are no characters, so return a failure.
        //
        return(-1);
    }
}


//*****************************************************************************
//
//! Waits to send a character from the specified port.
//!
//! \param ulBase is the base address of the UART port.
//! \param ucData is the character to be transmitted.
//!
//! This function sends the character \e ucData to the transmit FIFO for the
//! specified port.  If there is no space available in the transmit FIFO, this
//! function waits until there is space available before returning.
//!
//! \return None.
//
//*****************************************************************************
void
UARTCharPut(unsigned long ulBase, unsigned char ucData)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Wait until space is available.
    //
    while(HWREG(ulBase + UART_O_FR) & UART_FR_TXFF)
    {
    }

    //
    // Send the char.
    //
    HWREG(ulBase + UART_O_DR) = ucData;
}


//*****************************************************************************
//
//! Sets the configuration of a UART.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulUARTClk is the rate of the clock supplied to the UART module.
//! \param ulBaud is the desired baud rate.
//! \param ulConfig is the data format for the port (number of data bits,
//! number of stop bits, and parity).
//!
//! This function configures the UART for operation in the specified data
//! format.  The baud rate is provided in the \e ulBaud parameter and the data
//! format in the \e ulConfig parameter.
//!
//! The \e ulConfig parameter is the logical OR of three values: the number of
//! data bits, the number of stop bits, and the parity.  \b UART_CONFIG_WLEN_8,
//! \b UART_CONFIG_WLEN_7, \b UART_CONFIG_WLEN_6, and \b UART_CONFIG_WLEN_5
//! select from eight to five data bits per byte (respectively).
//! \b UART_CONFIG_STOP_ONE and \b UART_CONFIG_STOP_TWO select one or two stop
//! bits (respectively).  \b UART_CONFIG_PAR_NONE, \b UART_CONFIG_PAR_EVEN,
//! \b UART_CONFIG_PAR_ODD, \b UART_CONFIG_PAR_ONE, and \b UART_CONFIG_PAR_ZERO
//! select the parity mode (no parity bit, even parity bit, odd parity bit,
//! parity bit always one, and parity bit always zero, respectively).
//!
//! The peripheral clock is the same as the processor clock.  The frequency of
//! the system clock is the value returned by SysCtlClockGet(), or it can be
//! explicitly hard coded if it is constant and known (to save the
//! code/execution overhead of a call to SysCtlClockGet()).
//!
//! For Stellaris parts that have the ability to specify the UART baud clock
//! source (via UARTClockSourceSet()), the peripheral clock can be changed to
//! PIOSC.  In this case, the peripheral clock should be specified as
//! 16,000,000 (the nominal rate of PIOSC).
//!
//! This function replaces the original UARTConfigSet() API and performs the
//! same actions.  A macro is provided in <tt>uart.h</tt> to map the original
//! API to this API.
//!
//! \return None.
//
//*****************************************************************************
void
UARTConfigSetExpClk(unsigned long ulBase, unsigned long ulUARTClk,
                    unsigned long ulBaud, unsigned long ulConfig)
{
    unsigned long ulDiv;

    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    ASSERT(ulBaud != 0);
    ASSERT(ulUARTClk >= (ulBaud * UART_CLK_DIVIDER));

    //
    // Stop the UART.
    //
    UARTDisable(ulBase);

    //
    // Is the required baud rate greater than the maximum rate supported
    // without the use of high speed mode?
    //
    if((ulBaud * 16) > ulUARTClk)
    {
        //
        // Enable high speed mode.
        //
        HWREG(ulBase + UART_O_CTL) |= UART_CTL_HSE;

        //
        // Half the supplied baud rate to compensate for enabling high speed
        // mode.  This allows the following code to be common to both cases.
        //
        ulBaud /= 2;
    }
    else
    {
        //
        // Disable high speed mode.
        //
        HWREG(ulBase + UART_O_CTL) &= ~(UART_CTL_HSE);
    }

    //
    // Compute the fractional baud rate divider.
    //
    ulDiv = (((ulUARTClk * 8) / ulBaud) + 1) / 2;

    //
    // Set the baud rate.
    //
    HWREG(ulBase + UART_O_IBRD) = ulDiv / 64;
    HWREG(ulBase + UART_O_FBRD) = ulDiv % 64;

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(ulBase + UART_O_LCRH) = ulConfig;

    //
    // Clear the flags register.
    //
    HWREG(ulBase + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    UARTEnable(ulBase);
}


//*****************************************************************************
//
//! Enables transmitting and receiving.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function sets the UARTEN, TXE, and RXE bits and enables the transmit
//! and receive FIFOs.
//!
//! \return None.
//
//*****************************************************************************
void
UARTEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Enable the FIFO.
    //
    HWREG(ulBase + UART_O_LCRH) |= UART_LCRH_FEN;

    //
    // Enable RX, TX, and the UART.
    //
    HWREG(ulBase + UART_O_CTL) |= (UART_CTL_UARTEN | UART_CTL_TXE |
                                   UART_CTL_RXE);
}

tBoolean
UARTCharsAvail(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return the availability of characters.
    //
    return((HWREG(ulBase + UART_O_FR) & UART_FR_RXFE) ? false : true);
}

//*****************************************************************************
//
// An array that maps the crystal number in RCC to a frequency.
//
//*****************************************************************************
static const unsigned long g_pulXtals[] =
{
    1000000,
    1843200,
    2000000,
    2457600,
    3579545,
    3686400,
    4000000,
    4096000,
    4915200,
    5000000,
    5120000,
    6000000,
    6144000,
    7372800,
    8000000,
    8192000,
    10000000,
    12000000,
    12288000,
    13560000,
    14318180,
    16000000,
    16384000,
    18000000,
    20000000,
    24000000,
    25000000
};


//*****************************************************************************
//
//! Gets the processor clock rate.
//!
//! This function determines the clock rate of the processor clock, which is
//! also the clock rate of the peripheral modules (with the exception of
//! PWM, which has its own clock divider; other peripherals may have different
//! clocking, see the device data sheet for details).
//!
//! \note This cannot return accurate results if SysCtlClockSet() has not
//! been called to configure the clocking of the device, or if the device is
//! directly clocked from a crystal (or a clock source) that is not one of the
//! supported crystal frequencies.  In the latter case, this function should be
//! modified to directly return the correct system clock rate.
//!
//! \return The processor clock rate.
//
//*****************************************************************************
unsigned long
SysCtlClockGet(void)
{
    unsigned long ulRCC, ulRCC2, ulPLL, ulClk;
    unsigned long ulPLL1;

    //
    // Read RCC and RCC2.  For Sandstorm-class devices (which do not have
    // RCC2), the RCC2 read returns 0, indicating that RCC2 is
    // disabled (because the SYSCTL_RCC2_USERCC2 bit is clear).
    //
    ulRCC = HWREG(SYSCTL_RCC);
    ulRCC2 = HWREG(SYSCTL_RCC2);

    //
    // Get the base clock rate.
    //
    switch((ulRCC2 & SYSCTL_RCC2_USERCC2) ?
           (ulRCC2 & SYSCTL_RCC2_OSCSRC2_M) :
           (ulRCC & SYSCTL_RCC_OSCSRC_M))
    {
        //
        // The main oscillator is the clock source.  Determine its rate from
        // the crystal setting field.
        //
        case SYSCTL_RCC_OSCSRC_MAIN:
        {
            ulClk = g_pulXtals[(ulRCC & SYSCTL_RCC_XTAL_M) >>
                               SYSCTL_RCC_XTAL_S];
            break;
        }

        //
        // The internal oscillator is the source clock.
        //
        case SYSCTL_RCC_OSCSRC_INT:
        {
            //
            // See if this is a Sandstorm-class or Fury-class device.
            //
            if(CLASS_IS_SANDSTORM)
            {
                //
                // The internal oscillator on a Sandstorm-class device is
                // 15 MHz +/- 50%.
                //
                ulClk = 15000000;
            }
            else if((CLASS_IS_FURY && REVISION_IS_A2) ||
                    (CLASS_IS_DUSTDEVIL && REVISION_IS_A0))
            {
                //
                // The internal oscillator on a rev A2 Fury-class device and a
                // Dustdevil-class device is 12 MHz +/- 30%.
                //
                ulClk = 12000000;
            }
            else
            {
                //
                // The internal oscillator on all other devices is 16 MHz.
                //
                ulClk = 16000000;
            }
            break;
        }

        //
        // The internal oscillator divided by four is the source clock.
        //
        case SYSCTL_RCC_OSCSRC_INT4:
        {
            //
            // See if this is a Sandstorm-class or Fury-class device.
            //
            if(CLASS_IS_SANDSTORM)
            {
                //
                // The internal oscillator on a Sandstorm-class device is
                // 15 MHz +/- 50%.
                //
                ulClk = 15000000 / 4;
            }
            else if((CLASS_IS_FURY && REVISION_IS_A2) ||
                    (CLASS_IS_DUSTDEVIL && REVISION_IS_A0))
            {
                //
                // The internal oscillator on a rev A2 Fury-class device and a
                // Dustdevil-class device is 12 MHz +/- 30%.
                //
                ulClk = 12000000 / 4;
            }
            else
            {
                //
                // The internal oscillator on a Tempest-class device is 16 MHz.
                //
                ulClk = 16000000 / 4;
            }
            break;
        }

        //
        // The internal 30-KHz oscillator is the source clock.
        //
        case SYSCTL_RCC_OSCSRC_30:
        {
            //
            // The internal 30-KHz oscillator has an accuracy of +/- 30%.
            //
            ulClk = 30000;
            break;
        }

        //
        // The 4.194304-MHz clock from the hibernate module is the clock
        // source.
        //
        case SYSCTL_RCC2_OSCSRC2_419:
        {
            ulClk = 4194304;
            break;
        }

        //
        // The 32.768-KHz clock from the hibernate module is the source clock.
        //
        case SYSCTL_RCC2_OSCSRC2_32:
        {
            ulClk = 32768;
            break;
        }

        //
        // An unknown setting, so return a zero clock (that is, an unknown
        // clock rate).
        //
        default:
        {
            return(0);
        }
    }

    //
    // See if the PLL is being used.
    //
    if(((ulRCC2 & SYSCTL_RCC2_USERCC2) && !(ulRCC2 & SYSCTL_RCC2_BYPASS2)) ||
       (!(ulRCC2 & SYSCTL_RCC2_USERCC2) && !(ulRCC & SYSCTL_RCC_BYPASS)))
    {
        //
        // See if this is a Blizzard-class device.
        //
        if(CLASS_IS_BLIZZARD)
        {
            //
            // Read the two PLL frequency registers.  The formula for a
            // Blizzard-class device is "(xtal * m) / ((q + 1) * (n + 1))".
            //
            ulPLL = HWREG(SYSCTL_PLLFREQ0);
            ulPLL1 = HWREG(SYSCTL_PLLFREQ1);

            //
            // Divide the input clock by the dividers.
            //
            ulClk /= ((((ulPLL1 & SYSCTL_PLLFREQ1_Q_M) >>
                        SYSCTL_PLLFREQ1_Q_S) + 1) *
                      (((ulPLL1 & SYSCTL_PLLFREQ1_N_M) >>
                        SYSCTL_PLLFREQ1_N_S) + 1) * 2);

            //
            // Multiply the clock by the multiplier, which is split into an
            // integer part and a fractional part.
            //
            ulClk = ((ulClk * ((ulPLL & SYSCTL_PLLFREQ0_MINT_M) >>
                               SYSCTL_PLLFREQ0_MINT_S)) +
                     ((ulClk * ((ulPLL & SYSCTL_PLLFREQ0_MFRAC_M) >>
                                SYSCTL_PLLFREQ0_MFRAC_S)) >> 10));
        }

        //
        // Older device classes used a different PLL.
        //
        else
        {
            //
            // Get the PLL configuration.
            //
            ulPLL = HWREG(SYSCTL_PLLCFG);

            //
            // See if this is a Sandstorm-class or Fury-class device.
            //
            if(CLASS_IS_SANDSTORM)
            {
                //
                // Compute the PLL output frequency based on its input
                // frequency.  The formula for a Sandstorm-class devices is
                // "(xtal * (f + 2)) / (r + 2)".
                //
                ulClk = ((ulClk * (((ulPLL & SYSCTL_PLLCFG_F_M) >>
                                    SYSCTL_PLLCFG_F_S) + 2)) /
                         (((ulPLL & SYSCTL_PLLCFG_R_M) >>
                           SYSCTL_PLLCFG_R_S) + 2));
            }
            else
            {
                //
                // Compute the PLL output frequency based on its input
                // frequency.  The formula for a Fury-class device is
                // "(xtal * f) / ((r + 1) * 2)".
                //
                ulClk = ((ulClk * ((ulPLL & SYSCTL_PLLCFG_F_M) >>
                                   SYSCTL_PLLCFG_F_S)) /
                         ((((ulPLL & SYSCTL_PLLCFG_R_M) >>
                            SYSCTL_PLLCFG_R_S) + 1) * 2));
            }

            //
            // See if the optional output divide by 2 is being used.
            //
            if(ulPLL & SYSCTL_PLLCFG_OD_2)
            {
                ulClk /= 2;
            }

            //
            // See if the optional output divide by 4 is being used.
            //
            if(ulPLL & SYSCTL_PLLCFG_OD_4)
            {
                ulClk /= 4;
            }
        }

        //
        // Force the system divider to be enabled.  It is always used when
        // using the PLL, but in some cases it does not read as being enabled.
        //
        ulRCC |= SYSCTL_RCC_USESYSDIV;
    }

    //
    // See if the system divider is being used.
    //
    if(ulRCC & SYSCTL_RCC_USESYSDIV)
    {
        //
        // Adjust the clock rate by the system clock divider.
        //
        if(ulRCC2 & SYSCTL_RCC2_USERCC2)
        {
            if((ulRCC2 & SYSCTL_RCC2_DIV400) &&
               (((ulRCC2 & SYSCTL_RCC2_USERCC2) &&
                 !(ulRCC2 & SYSCTL_RCC2_BYPASS2)) ||
                (!(ulRCC2 & SYSCTL_RCC2_USERCC2) &&
                 !(ulRCC & SYSCTL_RCC_BYPASS))))

            {
                ulClk = ((ulClk * 2) / (((ulRCC2 & (SYSCTL_RCC2_SYSDIV2_M |
                                                    SYSCTL_RCC2_SYSDIV2LSB)) >>
                                         (SYSCTL_RCC2_SYSDIV2_S - 1)) + 1));
            }
            else
            {
                ulClk /= (((ulRCC2 & SYSCTL_RCC2_SYSDIV2_M) >>
                           SYSCTL_RCC2_SYSDIV2_S) + 1);
            }
        }
        else
        {
            ulClk /= (((ulRCC & SYSCTL_RCC_SYSDIV_M) >> SYSCTL_RCC_SYSDIV_S) +
                      1);
        }
    }

    //
    // Return the computed clock rate.
    //
    return(ulClk);
}


//*****************************************************************************
//
// This macro extracts the array index out of the peripheral number.
//
//*****************************************************************************
#define SYSCTL_PERIPH_INDEX(a)  (((a) >> 28) & 0xf)

//*****************************************************************************
//
// This macro constructs the peripheral bit mask from the peripheral number.
//
//*****************************************************************************
#define SYSCTL_PERIPH_MASK(a)   (((a) & 0xffff) << (((a) & 0x001f0000) >> 16))


static void __attribute__((naked))
SysCtlDelay(unsigned long ulCount)
{
    __asm("    subs    r0, #1\n"
          "    bne     SysCtlDelay\n"
          "    bx      lr");
}


//*****************************************************************************
//
//! Sets the clocking of the device.
//!
//! \param ulConfig is the required configuration of the device clocking.
//!
//! This function configures the clocking of the device.  The input crystal
//! frequency, oscillator to be used, use of the PLL, and the system clock
//! divider are all configured with this function.
//!
//! The \e ulConfig parameter is the logical OR of several different values,
//! many of which are grouped into sets where only one can be chosen.
//!
//! The system clock divider is chosen with one of the following values:
//! \b SYSCTL_SYSDIV_1, \b SYSCTL_SYSDIV_2, \b SYSCTL_SYSDIV_3, ...
//! \b SYSCTL_SYSDIV_64.  Only \b SYSCTL_SYSDIV_1 through \b SYSCTL_SYSDIV_16
//! are valid on Sandstorm-class devices. Half-dividers, such as
//! \b SYSCTL_SYSDIV_2_5 and \b SYSCTL_SYSDIV_3_5. are available on Tempest-,
//! Firestorm-, and Blizzard-class devices.
//!
//! The use of the PLL is chosen with either \b SYSCTL_USE_PLL or
//! \b SYSCTL_USE_OSC.
//!
//! The external crystal frequency is chosen with one of the following values:
//! \b SYSCTL_XTAL_1MHZ, \b SYSCTL_XTAL_1_84MHZ, \b SYSCTL_XTAL_2MHZ,
//! \b SYSCTL_XTAL_2_45MHZ, \b SYSCTL_XTAL_3_57MHZ, \b SYSCTL_XTAL_3_68MHZ,
//! \b SYSCTL_XTAL_4MHZ, \b SYSCTL_XTAL_4_09MHZ, \b SYSCTL_XTAL_4_91MHZ,
//! \b SYSCTL_XTAL_5MHZ, \b SYSCTL_XTAL_5_12MHZ, \b SYSCTL_XTAL_6MHZ,
//! \b SYSCTL_XTAL_6_14MHZ, \b SYSCTL_XTAL_7_37MHZ, \b SYSCTL_XTAL_8MHZ,
//! \b SYSCTL_XTAL_8_19MHZ, \b SYSCTL_XTAL_10MHZ, \b SYSCTL_XTAL_12MHZ,
//! \b SYSCTL_XTAL_12_2MHZ, \b SYSCTL_XTAL_13_5MHZ, \b SYSCTL_XTAL_14_3MHZ,
//! \b SYSCTL_XTAL_16MHZ, \b SYSCTL_XTAL_16_3MHZ, \b SYSCTL_XTAL_18MHZ,
//! \b SYSCTL_XTAL_20MHZ, \b SYSCTL_XTAL_24MHZ, or \b SYSCTL_XTAL_25MHz.
//! Values below \b SYSCTL_XTAL_3_57MHZ are not valid when the PLL is in
//! operation on Sandstorm-, Fury-, Dustdevil-, Tempest-, and Firestorm-class.
//! devices. Values below \b SYSCTL_XTAL_5MHZ are not valid when the PLL is in
//! operation on Blizzard-class devices.  Values below \b SYSCTL_XTAL_4MHZ
//! are never valid on Blizzard-class devices. On  Sandstorm- and Fury-class
//! devices, values above \b SYSCTL_XTAL_8_19MHZ are not valid. On Dustdevil-,
//! Tempest-, and Firestorm-class devices, values above \b SYSCTL_XTAL_16_3MHZ
//! are not valid.
//!
//! The oscillator source is chosen with one of the following values:
//! \b SYSCTL_OSC_MAIN, \b SYSCTL_OSC_INT, \b SYSCTL_OSC_INT4,
//! \b SYSCTL_OSC_INT30, or \b SYSCTL_OSC_EXT32.  On Sandstorm-class devices,
//! \b SYSCTL_OSC_INT30 and \b SYSCTL_OSC_EXT32 are not valid.
//! \b SYSCTL_OSC_EXT32 is only available on devices with the hibernate module,
//! and then only when the hibernate module has been enabled.
//!
//! The internal and main oscillators are disabled with the
//! \b SYSCTL_INT_OSC_DIS and \b SYSCTL_MAIN_OSC_DIS flags, respectively.
//! The external oscillator must be enabled in order to use an external clock
//! source.  Note that attempts to disable the oscillator used to clock the
//! device is prevented by the hardware.
//!
//! To clock the system from an external source (such as an external crystal
//! oscillator), use \b SYSCTL_USE_OSC \b | \b SYSCTL_OSC_MAIN.  To clock the
//! system from the main oscillator, use \b SYSCTL_USE_OSC \b |
//! \b SYSCTL_OSC_MAIN.  To clock the system from the PLL, use
//! \b SYSCTL_USE_PLL \b | \b SYSCTL_OSC_MAIN, and select the appropriate
//! crystal with one of the \b SYSCTL_XTAL_xxx values.
//!
//! \note If selecting the PLL as the system clock source (that is, via
//! \b SYSCTL_USE_PLL), this function polls the PLL lock interrupt to
//! determine when the PLL has locked.  If an interrupt handler for the
//! system control interrupt is in place, and it responds to and clears the
//! PLL lock interrupt, this function delays until its timeout has occurred
//! instead of completing as soon as PLL lock is achieved.
//!
//! \return None.
//
//*****************************************************************************
void
SysCtlClockSet(unsigned long ulConfig)
{
    unsigned long ulDelay, ulRCC, ulRCC2;

    //
    // See if this is a Sandstorm-class device and clocking features from newer
    // devices were requested.
    //
    if(CLASS_IS_SANDSTORM && (ulConfig & SYSCTL_RCC2_USERCC2))
    {
        //
        // Return without changing the clocking because the requested
        // configuration can not be achieved.
        //
        return;
    }

    //
    // Get the current value of the RCC and RCC2 registers.  If using a
    // Sandstorm-class device, the RCC2 register reads back as zero and the
    // writes to it from within this function are ignored.
    //
    ulRCC = HWREG(SYSCTL_RCC);
    ulRCC2 = HWREG(SYSCTL_RCC2);

    //
    // Bypass the PLL and system clock dividers for now.
    //
    ulRCC |= SYSCTL_RCC_BYPASS;
    ulRCC &= ~(SYSCTL_RCC_USESYSDIV);
    ulRCC2 |= SYSCTL_RCC2_BYPASS2;

    //
    // Write the new RCC value.
    //
    HWREG(SYSCTL_RCC) = ulRCC;
    HWREG(SYSCTL_RCC2) = ulRCC2;

    //
    // See if either oscillator needs to be enabled.
    //
    if(((ulRCC & SYSCTL_RCC_IOSCDIS) && !(ulConfig & SYSCTL_RCC_IOSCDIS)) ||
       ((ulRCC & SYSCTL_RCC_MOSCDIS) && !(ulConfig & SYSCTL_RCC_MOSCDIS)))
    {
        //
        // Make sure that the required oscillators are enabled.  For now, the
        // previously enabled oscillators must be enabled along with the newly
        // requested oscillators.
        //
        ulRCC &= (~(SYSCTL_RCC_IOSCDIS | SYSCTL_RCC_MOSCDIS) |
                  (ulConfig & (SYSCTL_RCC_IOSCDIS | SYSCTL_RCC_MOSCDIS)));

        //
        // Write the new RCC value.
        //
        HWREG(SYSCTL_RCC) = ulRCC;

        //
        // Wait for a bit, giving the oscillator time to stabilize.  The number
        // of iterations is adjusted based on the current clock source; a
        // smaller number of iterations is required for slower clock rates.
        //
        if(((ulRCC2 & SYSCTL_RCC2_USERCC2) &&
            (((ulRCC2 & SYSCTL_RCC2_OSCSRC2_M) == SYSCTL_RCC2_OSCSRC2_30) ||
             ((ulRCC2 & SYSCTL_RCC2_OSCSRC2_M) == SYSCTL_RCC2_OSCSRC2_32))) ||
           (!(ulRCC2 & SYSCTL_RCC2_USERCC2) &&
            ((ulRCC & SYSCTL_RCC_OSCSRC_M) == SYSCTL_RCC_OSCSRC_30)))
        {
            //
            // Delay for 4096 iterations.
            //
            SysCtlDelay(4096);
        }
        else
        {
            //
            // Delay for 524,288 iterations.
            //
            SysCtlDelay(524288);
        }
    }

    //
    // Set the new crystal value and oscillator source.  Because the OSCSRC2
    // field in RCC2 overlaps the XTAL field in RCC, the OSCSRC field has a
    // special encoding within ulConfig to avoid the overlap.
    //
    ulRCC &= ~(SYSCTL_RCC_XTAL_M | SYSCTL_RCC_OSCSRC_M);
    ulRCC |= ulConfig & (SYSCTL_RCC_XTAL_M | SYSCTL_RCC_OSCSRC_M);
    ulRCC2 &= ~(SYSCTL_RCC2_USERCC2 | SYSCTL_RCC2_OSCSRC2_M);
    ulRCC2 |= ulConfig & (SYSCTL_RCC2_USERCC2 | SYSCTL_RCC_OSCSRC_M);
    ulRCC2 |= (ulConfig & 0x00000008) << 3;

    //
    // Write the new RCC value.
    //
    HWREG(SYSCTL_RCC) = ulRCC;
    HWREG(SYSCTL_RCC2) = ulRCC2;

    //
    // Wait for a bit so that new crystal value and oscillator source can take
    // effect.
    //
    SysCtlDelay(16);

    //
    // Set the PLL configuration.
    //
    ulRCC &= ~(SYSCTL_RCC_PWRDN | SYSCTL_RCC_OEN);
    ulRCC |= ulConfig & (SYSCTL_RCC_PWRDN | SYSCTL_RCC_OEN);
    ulRCC2 &= ~(SYSCTL_RCC2_PWRDN2);
    ulRCC2 |= ulConfig & SYSCTL_RCC2_PWRDN2;

    //
    // Clear the PLL lock interrupt.
    //
    HWREG(SYSCTL_MISC) = SYSCTL_INT_PLL_LOCK;

    //
    // Write the new RCC value.
    //
    if(ulRCC2 & SYSCTL_RCC2_USERCC2)
    {
        HWREG(SYSCTL_RCC2) = ulRCC2;
        HWREG(SYSCTL_RCC) = ulRCC;
    }
    else
    {
        HWREG(SYSCTL_RCC) = ulRCC;
        HWREG(SYSCTL_RCC2) = ulRCC2;
    }

    //
    // Set the requested system divider and disable the appropriate
    // oscillators.  This value is not written immediately.
    //
    ulRCC &= ~(SYSCTL_RCC_SYSDIV_M | SYSCTL_RCC_USESYSDIV |
               SYSCTL_RCC_IOSCDIS | SYSCTL_RCC_MOSCDIS);
    ulRCC |= ulConfig & (SYSCTL_RCC_SYSDIV_M | SYSCTL_RCC_USESYSDIV |
                         SYSCTL_RCC_IOSCDIS | SYSCTL_RCC_MOSCDIS);
    ulRCC2 &= ~(SYSCTL_RCC2_SYSDIV2_M);
    ulRCC2 |= ulConfig & SYSCTL_RCC2_SYSDIV2_M;
    if(ulConfig & SYSCTL_RCC2_DIV400)
    {
        ulRCC |= SYSCTL_RCC_USESYSDIV;
        ulRCC2 &= ~(SYSCTL_RCC_USESYSDIV);
        ulRCC2 |= ulConfig & (SYSCTL_RCC2_DIV400 | SYSCTL_RCC2_SYSDIV2LSB);
    }
    else
    {
        ulRCC2 &= ~(SYSCTL_RCC2_DIV400);
    }

    //
    // See if the PLL output is being used to clock the system.
    //
    if(!(ulConfig & SYSCTL_RCC_BYPASS))
    {
        //
        // Wait until the PLL has locked.
        //
        for(ulDelay = 32768; ulDelay > 0; ulDelay--)
        {
            if(HWREG(SYSCTL_RIS) & SYSCTL_INT_PLL_LOCK)
            {
                break;
            }
        }

        //
        // Enable use of the PLL.
        //
        ulRCC &= ~(SYSCTL_RCC_BYPASS);
        ulRCC2 &= ~(SYSCTL_RCC2_BYPASS2);
    }

    //
    // Write the final RCC value.
    //
    HWREG(SYSCTL_RCC) = ulRCC;
    HWREG(SYSCTL_RCC2) = ulRCC2;

    //
    // Delay for a little bit so that the system divider takes effect.
    //
    SysCtlDelay(16);
}


//*****************************************************************************
//
//! Enables a peripheral.
//!
//! \param ulPeripheral is the peripheral to enable.
//!
//! This function enables peripherals.  At power-up, all peripherals
//! are disabled; they must be enabled in order to operate or respond to
//! register reads/writes.
//!
//! The \e ulPeripheral parameter must be only one of the following values:
//! \b SYSCTL_PERIPH_ADC0, \b SYSCTL_PERIPH_ADC1, \b SYSCTL_PERIPH_CAN0,
//! \b SYSCTL_PERIPH_CAN1, \b SYSCTL_PERIPH_CAN2, \b SYSCTL_PERIPH_COMP0,
//! \b SYSCTL_PERIPH_COMP1, \b SYSCTL_PERIPH_COMP2, \b SYSCTL_PERIPH_EEPROM0,
//! \b SYSCTL_PERIPH_EPI0, \b SYSCTL_PERIPH_ETH, \b SYSCTL_PERIPH_FAN0,
//! \b SYSCTL_PERIPH_GPIOA, \b SYSCTL_PERIPH_GPIOB, \b SYSCTL_PERIPH_GPIOC,
//! \b SYSCTL_PERIPH_GPIOD, \b SYSCTL_PERIPH_GPIOE, \b SYSCTL_PERIPH_GPIOF,
//! \b SYSCTL_PERIPH_GPIOG, \b SYSCTL_PERIPH_GPIOH, \b SYSCTL_PERIPH_GPIOJ,
//! \b SYSCTL_PERIPH_GPIOK, \b SYSCTL_PERIPH_GPIOL, \b SYSCTL_PERIPH_GPIOM,
//! \b SYSCTL_PERIPH_GPION, \b SYSCTL_PERIPH_GPIOP, \b SYSCTL_PERIPH_GPIOQ,
//! \b SYSCTL_PERIPH_HIBERNATE, \b SYSCTL_PERIPH_I2C0, \b SYSCTL_PERIPH_I2C1,
//! \b SYSCTL_PERIPH_I2C2, \b SYSCTL_PERIPH_I2C3, \b SYSCTL_PERIPH_I2C4,
//! \b SYSCTL_PERIPH_I2C5, \b SYSCTL_PERIPH_I2S0, \b SYSCTL_PERIPH_LPC0,
//! \b SYSCTL_PERIPH_PECI0, \b SYSCTL_PERIPH_PWM0, \b SYSCTL_PERIPH_PWM1,
//! \b SYSCTL_PERIPH_QEI0, \b SYSCTL_PERIPH_QEI1, \b SYSCTL_PERIPH_SSI0,
//! \b SYSCTL_PERIPH_SSI1, \b SYSCTL_PERIPH_SSI2, \b SYSCTL_PERIPH_SSI3,
//! \b SYSCTL_PERIPH_TIMER0, \b SYSCTL_PERIPH_TIMER1, \b SYSCTL_PERIPH_TIMER2,
//! \b SYSCTL_PERIPH_TIMER3, \b SYSCTL_PERIPH_TIMER4, \b SYSCTL_PERIPH_TIMER5,
//! \b SYSCTL_PERIPH_UART0, \b SYSCTL_PERIPH_UART1, \b SYSCTL_PERIPH_UART2,
//! \b SYSCTL_PERIPH_UART3, \b SYSCTL_PERIPH_UART4, \b SYSCTL_PERIPH_UART5,
//! \b SYSCTL_PERIPH_UART6, \b SYSCTL_PERIPH_UART7, \b SYSCTL_PERIPH_UDMA,
//! \b SYSCTL_PERIPH_USB0, \b SYSCTL_PERIPH_WDOG0, \b SYSCTL_PERIPH_WDOG1,
//! \b SYSCTL_PERIPH_WTIMER0, \b SYSCTL_PERIPH_WTIMER1,
//! \b SYSCTL_PERIPH_WTIMER2, \b SYSCTL_PERIPH_WTIMER3,
//! \b SYSCTL_PERIPH_WTIMER4, or \b SYSCTL_PERIPH_WTIMER5.
//!
//! \note It takes five clock cycles after the write to enable a peripheral
//! before the the peripheral is actually enabled.  During this time, attempts
//! to access the peripheral result in a bus fault.  Care should be taken
//! to ensure that the peripheral is not accessed during this brief time
//! period.
//!
//! \return None.
//
//*****************************************************************************
void
SysCtlPeripheralEnable(unsigned long ulPeripheral)
{
    //
    // Check the arguments.
    //
    ASSERT(SysCtlPeripheralValid(ulPeripheral));

    //
    // See if the peripheral index is 15, indicating a peripheral that is
    // accessed via the SYSCTL_RCGCperiph registers.
    //
    if((ulPeripheral & 0xf0000000) == 0xf0000000)
    {
        //
        // Enable this peripheral.
        //
        HWREGBITW(SYSCTL_RCGCBASE + ((ulPeripheral & 0xff00) >> 8),
                  ulPeripheral & 0xff) = 1;
    }
    else
    {
        //
        // Enable this peripheral.
        //
        HWREG(g_pulRCGCRegs[SYSCTL_PERIPH_INDEX(ulPeripheral)]) |=
            SYSCTL_PERIPH_MASK(ulPeripheral);
    }
}


//*****************************************************************************
//
//! Enables the SysTick counter.
//!
//! This function starts the SysTick counter.  If an interrupt handler has been
//! registered, it is called when the SysTick counter rolls over.
//!
//! \note Calling this function causes the SysTick counter to (re)commence
//! counting from its current value.  The counter is not automatically reloaded
//! with the period as specified in a previous call to SysTickPeriodSet().  If
//! an immediate reload is required, the \b NVIC_ST_CURRENT register must be
//! written to force the reload.  Any write to this register clears the SysTick
//! counter to 0 and causes a reload with the supplied period on the next
//! clock.
//!
//! \return None.
//
//*****************************************************************************
void
SysTickEnable(void)
{
    //
    // Enable SysTick.
    //
    HWREG(NVIC_ST_CTRL) |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_ENABLE;
}


//*****************************************************************************
//
//! Enables the SysTick interrupt.
//!
//! This function enables the SysTick interrupt, allowing it to be
//! reflected to the processor.
//!
//! \note The SysTick interrupt handler is not required to clear the SysTick
//! interrupt source because it is cleared automatically by the NVIC when the
//! interrupt handler is called.
//!
//! \return None.
//
//*****************************************************************************
void
SysTickIntEnable(void)
{
    //
    // Enable the SysTick interrupt.
    //
    HWREG(NVIC_ST_CTRL) |= NVIC_ST_CTRL_INTEN;
}


//*****************************************************************************
//
//! Sets the period of the SysTick counter.
//!
//! \param ulPeriod is the number of clock ticks in each period of the SysTick
//! counter and must be between 1 and 16,777,216, inclusive.
//!
//! This function sets the rate at which the SysTick counter wraps, which
//! equates to the number of processor clocks between interrupts.
//!
//! \note Calling this function does not cause the SysTick counter to reload
//! immediately.  If an immediate reload is required, the \b NVIC_ST_CURRENT
//! register must be written.  Any write to this register clears the SysTick
//! counter to 0 and causes a reload with the \e ulPeriod supplied here on
//! the next clock after SysTick is enabled.
//!
//! \return None.
//
//*****************************************************************************
void
SysTickPeriodSet(unsigned long ulPeriod)
{
    //
    // Check the arguments.
    //
    ASSERT((ulPeriod > 0) && (ulPeriod <= 16777216));

    //
    // Set the period of the SysTick counter.
    //
    HWREG(NVIC_ST_RELOAD) = ulPeriod - 1;
}


//*****************************************************************************
//
//! Gets the current value of the SysTick counter.
//!
//! This function returns the current value of the SysTick counter, which is
//! a value between the period - 1 and zero, inclusive.
//!
//! \return Returns the current value of the SysTick counter.
//
//*****************************************************************************
unsigned long
SysTickValueGet(void)
{
    //
    // Return the current value of the SysTick counter.
    //
    return(HWREG(NVIC_ST_CURRENT));
}


//*****************************************************************************
//
//! Enables the lazy stacking of floating-point registers.
//!
//! This function enables the lazy stacking of floating-point registers s0-s15
//! when an interrupt is handled.  When lazy stacking is enabled, space is
//! reserved on the stack for the floating-point context, but the
//! floating-point state is not saved.  If a floating-point instruction is
//! executed from within the interrupt context, the floating-point context is
//! first saved into the space reserved on the stack.  On completion of the
//! interrupt handler, the floating-point context is only restored if it was
//! saved (as the result of executing a floating-point instruction).
//!
//! This method provides a compromise between fast interrupt response (because
//! the floating-point state is not saved on interrupt entry) and the ability
//! to use floating-point in interrupt handlers (because the floating-point
//! state is saved if floating-point instructions are used).
//!
//! \return None.
//
//*****************************************************************************
void
FPULazyStackingEnable(void)
{
    //
    // Enable automatic and lazy state preservation for the floating-point
    // unit.
    //
    HWREG(NVIC_FPCC) |= NVIC_FPCC_ASPEN | NVIC_FPCC_LSPEN;
}


//*****************************************************************************
//
// Values that can be passed to GPIODirModeSet as the ulPinIO parameter, and
// returned from GPIODirModeGet.
//
//*****************************************************************************
//#define GPIO_DIR_MODE_IN        0x00000000  // Pin is a GPIO input
//#define GPIO_DIR_MODE_OUT       0x00000001  // Pin is a GPIO output
#define GPIO_DIR_MODE_HW        0x00000002  // Pin is a peripheral function


//*****************************************************************************
//
// Values that can be passed to GPIOPadConfigSet as the ulStrength parameter,
// and returned by GPIOPadConfigGet in the *pulStrength parameter.
//
//*****************************************************************************
#define GPIO_STRENGTH_2MA       0x00000001  // 2mA drive strength
//#define GPIO_STRENGTH_4MA       0x00000002  // 4mA drive strength
//#define GPIO_STRENGTH_8MA       0x00000004  // 8mA drive strength
//#define GPIO_STRENGTH_8MA_SC    0x0000000C  // 8mA drive with slew rate control


//*****************************************************************************
//
// Values that can be passed to GPIOPadConfigSet as the ulPadType parameter,
// and returned by GPIOPadConfigGet in the *pulPadType parameter.
//
//*****************************************************************************
#define GPIO_PIN_TYPE_STD       0x00000008  // Push-pull
//#define GPIO_PIN_TYPE_STD_WPU   0x0000000A  // Push-pull with weak pull-up
//#define GPIO_PIN_TYPE_STD_WPD   0x0000000C  // Push-pull with weak pull-down
//#define GPIO_PIN_TYPE_OD        0x00000009  // Open-drain
//#define GPIO_PIN_TYPE_OD_WPU    0x0000000B  // Open-drain with weak pull-up
//#define GPIO_PIN_TYPE_OD_WPD    0x0000000D  // Open-drain with weak pull-down
#define GPIO_PIN_TYPE_ANALOG    0x00000000  // Analog comparator


//*****************************************************************************
//
// The base addresses of all the GPIO modules.  Both the APB and AHB apertures
// are provided.
//
//*****************************************************************************
static const unsigned long g_pulGPIOBaseAddrs[] =
{
    GPIO_PORTA_BASE, GPIO_PORTA_AHB_BASE,
    GPIO_PORTB_BASE, GPIO_PORTB_AHB_BASE,
    GPIO_PORTC_BASE, GPIO_PORTC_AHB_BASE,
    GPIO_PORTD_BASE, GPIO_PORTD_AHB_BASE,
    GPIO_PORTE_BASE, GPIO_PORTE_AHB_BASE,
    GPIO_PORTF_BASE, GPIO_PORTF_AHB_BASE,
    GPIO_PORTG_BASE, GPIO_PORTG_AHB_BASE,
    GPIO_PORTH_BASE, GPIO_PORTH_AHB_BASE,
    GPIO_PORTJ_BASE, GPIO_PORTJ_AHB_BASE,
    GPIO_PORTK_BASE, GPIO_PORTK_BASE,
    GPIO_PORTL_BASE, GPIO_PORTL_BASE,
    GPIO_PORTM_BASE, GPIO_PORTM_BASE,
    GPIO_PORTN_BASE, GPIO_PORTN_BASE,
    GPIO_PORTP_BASE, GPIO_PORTP_BASE,
    GPIO_PORTQ_BASE, GPIO_PORTQ_BASE,
};


//*****************************************************************************
//
//! Sets the pad configuration for the specified pin(s).
//!
//! \param ulPort is the base address of the GPIO port.
//! \param ucPins is the bit-packed representation of the pin(s).
//! \param ulStrength specifies the output drive strength.
//! \param ulPinType specifies the pin type.
//!
//! This function sets the drive strength and type for the specified pin(s)
//! on the selected GPIO port.  For pin(s) configured as input ports, the
//! pad is configured as requested, but the only real effect on the input
//! is the configuration of the pull-up or pull-down termination.
//!
//! The parameter \e ulStrength can be one of the following values:
//!
//! - \b GPIO_STRENGTH_2MA
//! - \b GPIO_STRENGTH_4MA
//! - \b GPIO_STRENGTH_8MA
//! - \b GPIO_STRENGTH_8MA_SC
//!
//! where \b GPIO_STRENGTH_xMA specifies either 2, 4, or 8 mA output drive
//! strength, and \b GPIO_OUT_STRENGTH_8MA_SC specifies 8 mA output drive with
//! slew control.
//!
//! Some Stellaris devices also support output drive strengths of 6, 10, and 12
//! mA.
//!
//! The parameter \e ulPinType can be one of the following values:
//!
//! - \b GPIO_PIN_TYPE_STD
//! - \b GPIO_PIN_TYPE_STD_WPU
//! - \b GPIO_PIN_TYPE_STD_WPD
//! - \b GPIO_PIN_TYPE_OD
//! - \b GPIO_PIN_TYPE_OD_WPU
//! - \b GPIO_PIN_TYPE_OD_WPD
//! - \b GPIO_PIN_TYPE_ANALOG
//!
//! where \b GPIO_PIN_TYPE_STD* specifies a push-pull pin, \b GPIO_PIN_TYPE_OD*
//! specifies an open-drain pin, \b *_WPU specifies a weak pull-up, \b *_WPD
//! specifies a weak pull-down, and \b GPIO_PIN_TYPE_ANALOG specifies an analog
//! input.
//!
//! The pin(s) are specified using a bit-packed byte, where each bit that is
//! set identifies the pin to be accessed, and where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \return None.
//
//*****************************************************************************
void
GPIOPadConfigSet(unsigned long ulPort, unsigned char ucPins,
                 unsigned long ulStrength, unsigned long ulPinType)
{
    //
    // Check the arguments.
    //
    ASSERT(GPIOBaseValid(ulPort));
    ASSERT((ulStrength == GPIO_STRENGTH_2MA) ||
           (ulStrength == GPIO_STRENGTH_4MA) ||
           (ulStrength == GPIO_STRENGTH_8MA) ||
           (ulStrength == GPIO_STRENGTH_8MA_SC));
    ASSERT((ulPinType == GPIO_PIN_TYPE_STD) ||
           (ulPinType == GPIO_PIN_TYPE_STD_WPU) ||
           (ulPinType == GPIO_PIN_TYPE_STD_WPD) ||
           (ulPinType == GPIO_PIN_TYPE_OD) ||
           (ulPinType == GPIO_PIN_TYPE_OD_WPU) ||
           (ulPinType == GPIO_PIN_TYPE_OD_WPD) ||
           (ulPinType == GPIO_PIN_TYPE_ANALOG));

    //
    // Set the output drive strength.
    //
    HWREG(ulPort + GPIO_O_DR2R) = ((ulStrength & 1) ?
                                   (HWREG(ulPort + GPIO_O_DR2R) | ucPins) :
                                   (HWREG(ulPort + GPIO_O_DR2R) & ~(ucPins)));
    HWREG(ulPort + GPIO_O_DR4R) = ((ulStrength & 2) ?
                                   (HWREG(ulPort + GPIO_O_DR4R) | ucPins) :
                                   (HWREG(ulPort + GPIO_O_DR4R) & ~(ucPins)));
    HWREG(ulPort + GPIO_O_DR8R) = ((ulStrength & 4) ?
                                   (HWREG(ulPort + GPIO_O_DR8R) | ucPins) :
                                   (HWREG(ulPort + GPIO_O_DR8R) & ~(ucPins)));
    HWREG(ulPort + GPIO_O_SLR) = ((ulStrength & 8) ?
                                  (HWREG(ulPort + GPIO_O_SLR) | ucPins) :
                                  (HWREG(ulPort + GPIO_O_SLR) & ~(ucPins)));

    //
    // Set the pin type.
    //
    HWREG(ulPort + GPIO_O_ODR) = ((ulPinType & 1) ?
                                  (HWREG(ulPort + GPIO_O_ODR) | ucPins) :
                                  (HWREG(ulPort + GPIO_O_ODR) & ~(ucPins)));
    HWREG(ulPort + GPIO_O_PUR) = ((ulPinType & 2) ?
                                  (HWREG(ulPort + GPIO_O_PUR) | ucPins) :
                                  (HWREG(ulPort + GPIO_O_PUR) & ~(ucPins)));
    HWREG(ulPort + GPIO_O_PDR) = ((ulPinType & 4) ?
                                  (HWREG(ulPort + GPIO_O_PDR) | ucPins) :
                                  (HWREG(ulPort + GPIO_O_PDR) & ~(ucPins)));
    HWREG(ulPort + GPIO_O_DEN) = ((ulPinType & 8) ?
                                  (HWREG(ulPort + GPIO_O_DEN) | ucPins) :
                                  (HWREG(ulPort + GPIO_O_DEN) & ~(ucPins)));

    //
    // Set the analog mode select register.  This register only appears in
    // DustDevil-class (and later) devices, but is a harmless write on
    // Sandstorm- and Fury-class devices.
    //
    HWREG(ulPort + GPIO_O_AMSEL) =
        ((ulPinType == GPIO_PIN_TYPE_ANALOG) ?
         (HWREG(ulPort + GPIO_O_AMSEL) | ucPins) :
         (HWREG(ulPort + GPIO_O_AMSEL) & ~(ucPins)));
}


//*****************************************************************************
//
//! Sets the direction and mode of the specified pin(s).
//!
//! \param ulPort is the base address of the GPIO port
//! \param ucPins is the bit-packed representation of the pin(s).
//! \param ulPinIO is the pin direction and/or mode.
//!
//! This function configures the specified pin(s) on the selected GPIO port
//! as either input or output under software control, or it configures the
//! pin to be under hardware control.
//!
//! The parameter \e ulPinIO is an enumerated data type that can be one of
//! the following values:
//!
//! - \b GPIO_DIR_MODE_IN
//! - \b GPIO_DIR_MODE_OUT
//! - \b GPIO_DIR_MODE_HW
//!
//! where \b GPIO_DIR_MODE_IN specifies that the pin is programmed as a
//! software controlled input, \b GPIO_DIR_MODE_OUT specifies that the pin is
//! programmed as a software controlled output, and \b GPIO_DIR_MODE_HW
//! specifies that the pin is placed under hardware control.
//!
//! The pin(s) are specified using a bit-packed byte, where each bit that is
//! set identifies the pin to be accessed, and where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \note GPIOPadConfigSet() must also be used to configure the corresponding
//! pad(s) in order for them to propagate the signal to/from the GPIO.
//!
//! \return None.
//
//*****************************************************************************
void
GPIODirModeSet(unsigned long ulPort, unsigned char ucPins,
               unsigned long ulPinIO)
{
    //
    // Check the arguments.
    //
    ASSERT(GPIOBaseValid(ulPort));
    ASSERT((ulPinIO == GPIO_DIR_MODE_IN) || (ulPinIO == GPIO_DIR_MODE_OUT) ||
           (ulPinIO == GPIO_DIR_MODE_HW));

    //
    // Set the pin direction and mode.
    //
    HWREG(ulPort + GPIO_O_DIR) = ((ulPinIO & 1) ?
                                  (HWREG(ulPort + GPIO_O_DIR) | ucPins) :
                                  (HWREG(ulPort + GPIO_O_DIR) & ~(ucPins)));
    HWREG(ulPort + GPIO_O_AFSEL) = ((ulPinIO & 2) ?
                                    (HWREG(ulPort + GPIO_O_AFSEL) | ucPins) :
                                    (HWREG(ulPort + GPIO_O_AFSEL) &
                                     ~(ucPins)));
}


//*****************************************************************************
//
//! Configures the alternate function of a GPIO pin.
//!
//! \param ulPinConfig is the pin configuration value, specified as only one of
//! the \b GPIO_P??_??? values.
//!
//! This function configures the pin mux that selects the peripheral function
//! associated with a particular GPIO pin.  Only one peripheral function at a
//! time can be associated with a GPIO pin, and each peripheral function should
//! only be associated with a single GPIO pin at a time (despite the fact that
//! many of them can be associated with more than one GPIO pin). To fully
//! configure a pin, a GPIOPinType*() function should also be called.
//!
//! The available mappings are supplied on a per-device basis in
//! <tt>pin_map.h</tt>.  The \b PART_IS_<partno> define enables the
//! appropriate set of defines for the device that is being used.
//!
//! \note This function is not valid on Sandstorm, Fury, and Dustdevil-class
//! devices. Also, if the same signal is assigned to two different GPIO port
//! pins, the signal is assigned to the port with the lowest letter and the
//! assignment to the higher letter port is ignored.
//!
//! \return None.
//
//*****************************************************************************
void
GPIOPinConfigure(unsigned long ulPinConfig)
{
    unsigned long ulBase, ulShift;

    //
    // Check the argument.
    //
    ASSERT(((ulPinConfig >> 16) & 0xff) < 15);
    ASSERT(((ulPinConfig >> 8) & 0xe3) == 0);

    //
    // Extract the base address index from the input value.
    //
    ulBase = (ulPinConfig >> 16) & 0xff;

    //
    // Get the base address of the GPIO module, selecting either the APB or the
    // AHB aperture as appropriate.
    //
    if(HWREG(SYSCTL_GPIOHBCTL) & (1 << ulBase))
    {
        ulBase = g_pulGPIOBaseAddrs[(ulBase << 1) + 1];
    }
    else
    {
        ulBase = g_pulGPIOBaseAddrs[ulBase << 1];
    }

    //
    // Extract the shift from the input value.
    //
    ulShift = (ulPinConfig >> 8) & 0xff;

    //
    // Write the requested pin muxing value for this GPIO pin.
    //
    HWREG(ulBase + GPIO_O_PCTL) = ((HWREG(ulBase + GPIO_O_PCTL) &
                                    ~(0xf << ulShift)) |
                                   ((ulPinConfig & 0xf) << ulShift));
}


//*****************************************************************************
//
//! Configures pin(s) for use by the UART peripheral.
//!
//! \param ulPort is the base address of the GPIO port.
//! \param ucPins is the bit-packed representation of the pin(s).
//!
//! The UART pins must be properly configured for the UART peripheral to
//! function correctly.  This function provides a typical configuration for
//! those pin(s); other configurations may work as well depending upon the
//! board setup (for example, using the on-chip pull-ups).
//!
//! The pin(s) are specified using a bit-packed byte, where each bit that is
//! set identifies the pin to be accessed, and where bit 0 of the byte
//! represents GPIO port pin 0, bit 1 represents GPIO port pin 1, and so on.
//!
//! \note This function cannot be used to turn any pin into a UART pin; it
//! only configures a UART pin for proper operation.  Devices with flexible
//! pin muxing also require a GPIOPinConfigure() function call.
//!
//! \return None.
//
//*****************************************************************************
void
GPIOPinTypeUART(unsigned long ulPort, unsigned char ucPins)
{
    //
    // Check the arguments.
    //
    ASSERT(GPIOBaseValid(ulPort));

    //
    // Make the pin(s) be peripheral controlled.
    //
    GPIODirModeSet(ulPort, ucPins, GPIO_DIR_MODE_HW);

    //
    // Set the pad(s) for standard push-pull operation.
    //
    GPIOPadConfigSet(ulPort, ucPins, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
}


