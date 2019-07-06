/*
 * Copyright (c) 2012 Ian McKellar
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdarg.h>
#include <lk/reg.h>
#include <lk/debug.h>
#include <stdio.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <target/debugconfig.h>
#include <arch/arm/cm.h>

#include "ti_driverlib.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#define DEBUG_UART UART0_BASE

static cbuf_t debug_rx_buf;

void stellaris_uart0_irq(void) {
    arm_cm_irq_entry();

    //
    // Get the interrrupt status.
    //
    unsigned long ulStatus = UARTIntStatus(DEBUG_UART, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(DEBUG_UART, ulStatus);

    //
    // Loop while there are characters in the receive FIFO.
    //
    bool resched = false;
    while (UARTCharsAvail(DEBUG_UART)) {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        unsigned char c = UARTCharGetNonBlocking(DEBUG_UART);
        cbuf_write_char(&debug_rx_buf, c, false);

        resched = true;
    }

    arm_cm_irq_exit(resched);
}

void stellaris_debug_early_init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    /* we only support UART0 right now */
    STATIC_ASSERT(DEBUG_UART == UART0_BASE);

    if (DEBUG_UART == UART0_BASE) {
#if defined(PART_LM4F120H5QR)
        /* Set GPIO A0 and A1 as UART pins. */
        GPIOPinConfigure(GPIO_PA0_U0RX);
        GPIOPinConfigure(GPIO_PA1_U0TX);
        GPIOPinTypeUART(GPIO_PORTA_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
#endif
    }

    UARTConfigSetExpClk(DEBUG_UART, SysCtlClockGet(), 115200, UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE|UART_CONFIG_PAR_NONE);

    UARTEnable(DEBUG_UART);
}

void stellaris_debug_init(void) {
    cbuf_initialize(&debug_rx_buf, 16);

    /* Enable the UART interrupt. */
    UARTIntEnable(DEBUG_UART, UART_INT_RX | UART_INT_RT);

    NVIC_EnableIRQ(INT_UART0 - 16);

}

void platform_dputc(char c) {
    if (c == '\n') {
        platform_dputc('\r');
    }

    UARTCharPut(DEBUG_UART, c);
}

int platform_dgetc(char *c, bool wait) {
    return cbuf_read_char(&debug_rx_buf, c, wait);
}

