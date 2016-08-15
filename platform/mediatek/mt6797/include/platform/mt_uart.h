/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef ___MTK_UART_H__
#define ___MTK_UART_H__

#include <platform/mt_reg_base.h>

typedef enum {
    UART1 = AP_UART0_BASE,
    UART2 = AP_UART1_BASE,
    UART3 = AP_UART2_BASE,
    UART4 = AP_UART3_BASE
} MTK_UART;

#define UART_SRC_CLK            26000000

#define CONFIG_BAUDRATE         921600

#define UART_BASE(uart)                   (uart)
#define UART_RBR(uart)                    (UART_BASE(uart)+0x0)  /* Read only */
#define UART_THR(uart)                    (UART_BASE(uart)+0x0)  /* Write only */
#define UART_IER(uart)                    (UART_BASE(uart)+0x4)
#define UART_IIR(uart)                    (UART_BASE(uart)+0x8)  /* Read only */
#define UART_FCR(uart)                    (UART_BASE(uart)+0x8)  /* Write only */
#define UART_LCR(uart)                    (UART_BASE(uart)+0xc)
#define UART_MCR(uart)                    (UART_BASE(uart)+0x10)
#define UART_LSR(uart)                    (UART_BASE(uart)+0x14)
#define UART_MSR(uart)                    (UART_BASE(uart)+0x18)
#define UART_SCR(uart)                    (UART_BASE(uart)+0x1c)
#define UART_DLL(uart)                    (UART_BASE(uart)+0x0)  /* Only when LCR.DLAB = 1 */
#define UART_DLH(uart)                    (UART_BASE(uart)+0x4)  /* Only when LCR.DLAB = 1 */
#define UART_EFR(uart)                    (UART_BASE(uart)+0x8)  /* Only when LCR = 0xbf */
#define UART_XON1(uart)                   (UART_BASE(uart)+0x10) /* Only when LCR = 0xbf */
#define UART_XON2(uart)                   (UART_BASE(uart)+0x14) /* Only when LCR = 0xbf */
#define UART_XOFF1(uart)                  (UART_BASE(uart)+0x18) /* Only when LCR = 0xbf */
#define UART_XOFF2(uart)                  (UART_BASE(uart)+0x1c) /* Only when LCR = 0xbf */
#define UART_AUTOBAUD_EN(uart)            (UART_BASE(uart)+0x20)
#define UART_HIGHSPEED(uart)              (UART_BASE(uart)+0x24)
#define UART_SAMPLE_COUNT(uart)           (UART_BASE(uart)+0x28)
#define UART_SAMPLE_POINT(uart)           (UART_BASE(uart)+0x2c)
#define UART_AUTOBAUD_REG(uart)           (UART_BASE(uart)+0x30)
#define UART_RATE_FIX_AD(uart)            (UART_BASE(uart)+0x34)
#define UART_AUTOBAUD_SAMPLE(uart)        (UART_BASE(uart)+0x38)
#define UART_GUARD(uart)                  (UART_BASE(uart)+0x3c)
#define UART_ESCAPE_DAT(uart)             (UART_BASE(uart)+0x40)
#define UART_ESCAPE_EN(uart)              (UART_BASE(uart)+0x44)
#define UART_SLEEP_EN(uart)               (UART_BASE(uart)+0x48)
#define UART_VFIFO_EN(uart)               (UART_BASE(uart)+0x4c)
#define UART_RXTRI_AD(uart)               (UART_BASE(uart)+0x50)

#define UART_FIFO_SIZE              (16)
#define IO_OFFSET                   (0)


/* IER */
#define UART_IER_ERBFI              (1 << 0) /* RX buffer contains data int. */
#define UART_IER_ETBEI              (1 << 1) /* TX FIFO threshold trigger int. */
#define UART_IER_ELSI               (1 << 2) /* BE, FE, PE, or OE int. */
#define UART_IER_EDSSI              (1 << 3) /* CTS change (DCTS) int. */
#define UART_IER_XOFFI              (1 << 5)
#define UART_IER_RTSI               (1 << 6)
#define UART_IER_CTSI               (1 << 7)

#define UART_IER_ALL_INTS          (UART_IER_ERBFI|UART_IER_ETBEI|UART_IER_ELSI|\
                                    UART_IER_EDSSI|UART_IER_XOFFI|UART_IER_RTSI|\
                                    UART_IER_CTSI)
#define UART_IER_HW_NORMALINTS     (UART_IER_ERBFI|UART_IER_ELSI|UART_IER_EDSSI)
#define UART_IER_HW_ALLINTS        (UART_IER_ERBFI|UART_IER_ETBEI| \
                                    UART_IER_ELSI|UART_IER_EDSSI)

/* FCR */
#define UART_FCR_FIFOE              (1 << 0)
#define UART_FCR_CLRR               (1 << 1)
#define UART_FCR_CLRT               (1 << 2)
#define UART_FCR_DMA1               (1 << 3)
#define UART_FCR_RXFIFO_1B_TRI      (0 << 6)
#define UART_FCR_RXFIFO_6B_TRI      (1 << 6)
#define UART_FCR_RXFIFO_12B_TRI     (2 << 6)
#define UART_FCR_RXFIFO_RX_TRI      (3 << 6)
#define UART_FCR_TXFIFO_1B_TRI      (0 << 4)
#define UART_FCR_TXFIFO_4B_TRI      (1 << 4)
#define UART_FCR_TXFIFO_8B_TRI      (2 << 4)
#define UART_FCR_TXFIFO_14B_TRI     (3 << 4)

#define UART_FCR_FIFO_INIT          (UART_FCR_FIFOE|UART_FCR_CLRR|UART_FCR_CLRT)
#define UART_FCR_NORMAL             (UART_FCR_FIFO_INIT | \
                                     UART_FCR_TXFIFO_4B_TRI| \
                                     UART_FCR_RXFIFO_12B_TRI)

/* LCR */
#define UART_LCR_BREAK              (1 << 6)
#define UART_LCR_DLAB               (1 << 7)

#define UART_WLS_5                  (0 << 0)
#define UART_WLS_6                  (1 << 0)
#define UART_WLS_7                  (2 << 0)
#define UART_WLS_8                  (3 << 0)
#define UART_WLS_MASK               (3 << 0)

#define UART_1_STOP                 (0 << 2)
#define UART_2_STOP                 (1 << 2)
#define UART_1_5_STOP               (1 << 2)    /* Only when WLS=5 */
#define UART_STOP_MASK              (1 << 2)

#define UART_NONE_PARITY            (0 << 3)
#define UART_ODD_PARITY             (0x1 << 3)
#define UART_EVEN_PARITY            (0x3 << 3)
#define UART_MARK_PARITY            (0x5 << 3)
#define UART_SPACE_PARITY           (0x7 << 3)
#define UART_PARITY_MASK            (0x7 << 3)

/* MCR */
#define UART_MCR_DTR                (1 << 0)
#define UART_MCR_RTS                (1 << 1)
#define UART_MCR_OUT1               (1 << 2)
#define UART_MCR_OUT2               (1 << 3)
#define UART_MCR_LOOP               (1 << 4)
#define UART_MCR_XOFF               (1 << 7)    /* read only */
#define UART_MCR_NORMAL             (UART_MCR_DTR|UART_MCR_RTS)

/* LSR */
#define UART_LSR_DR                 (1 << 0)
#define UART_LSR_OE                 (1 << 1)
#define UART_LSR_PE                 (1 << 2)
#define UART_LSR_FE                 (1 << 3)
#define UART_LSR_BI                 (1 << 4)
#define UART_LSR_THRE               (1 << 5)
#define UART_LSR_TEMT               (1 << 6)
#define UART_LSR_FIFOERR            (1 << 7)

/* MSR */
#define UART_MSR_DCTS               (1 << 0)
#define UART_MSR_DDSR               (1 << 1)
#define UART_MSR_TERI               (1 << 2)
#define UART_MSR_DDCD               (1 << 3)
#define UART_MSR_CTS                (1 << 4)
#define UART_MSR_DSR                (1 << 5)
#define UART_MSR_RI                 (1 << 6)
#define UART_MSR_DCD                (1 << 7)

/* EFR */
#define UART_EFR_EN                 (1 << 4)
#define UART_EFR_AUTO_RTS           (1 << 6)
#define UART_EFR_AUTO_CTS           (1 << 7)
#define UART_EFR_SW_CTRL_MASK       (0xf << 0)

#define UART_EFR_NO_SW_CTRL         (0)
#define UART_EFR_NO_FLOW_CTRL       (0)
#define UART_EFR_AUTO_RTSCTS        (UART_EFR_AUTO_RTS|UART_EFR_AUTO_CTS)
#define UART_EFR_XON1_XOFF1         (0xa) /* TX/RX XON1/XOFF1 flow control */
#define UART_EFR_XON2_XOFF2         (0x5) /* TX/RX XON2/XOFF2 flow control */
#define UART_EFR_XON12_XOFF12       (0xf) /* TX/RX XON1,2/XOFF1,2 flow
control */

#define UART_EFR_XON1_XOFF1_MASK    (0xa)
#define UART_EFR_XON2_XOFF2_MASK    (0x5)

/* IIR (Read Only) */
#define UART_IIR_NO_INT_PENDING     (0x01)
#define UART_IIR_RLS                (0x06) /* Receiver Line Status */
#define UART_IIR_RDA                (0x04) /* Receive Data Available */
#define UART_IIR_CTI                (0x0C) /* Character Timeout Indicator */
#define UART_IIR_THRE               (0x02) /* Transmit Holding Register Empty
*/
#define UART_IIR_MS                 (0x00) /* Check Modem Status Register */
#define UART_IIR_SW_FLOW_CTRL       (0x10) /* Receive XOFF characters */
#define UART_IIR_HW_FLOW_CTRL       (0x20) /* CTS or RTS Rising Edge */
#define UART_IIR_FIFO_EN            (0xc0)
#define UART_IIR_INT_MASK           (0x1f)

/* RateFix */
#define UART_RATE_FIX               (1 << 0)
//#define UART_AUTORATE_FIX           (1 << 1)
//#define UART_FREQ_SEL               (1 << 2)
#define UART_FREQ_SEL               (1 << 1)

#define UART_RATE_FIX_13M           (1 << 0) /* means UARTclk = APBclk / 4 */
#define UART_AUTORATE_FIX_13M       (1 << 1)
#define UART_FREQ_SEL_13M           (1 << 2)
#define UART_RATE_FIX_ALL_13M       (UART_RATE_FIX_13M|UART_AUTORATE_FIX_13M| \
                                     UART_FREQ_SEL_13M)

#define UART_RATE_FIX_26M           (0 << 0) /* means UARTclk = APBclk / 2 */
#define UART_AUTORATE_FIX_26M       (0 << 1)
#define UART_FREQ_SEL_26M           (0 << 2)
#define UART_RATE_FIX_ALL_26M       (UART_RATE_FIX_26M|UART_AUTORATE_FIX_26M| \
                                     UART_FREQ_SEL_26M)

#define UART_RATE_FIX_32M5          (0 << 0)    /* means UARTclk = APBclk / 2 */
#define UART_FREQ_SEL_32M5          (0 << 1)
#define UART_RATE_FIX_ALL_32M5      (UART_RATE_FIX_32M5|UART_FREQ_SEL_32M5)

#define UART_RATE_FIX_16M25         (0 << 0)    /* means UARTclk = APBclk / 4 */
#define UART_FREQ_SEL_16M25         (0 << 1)
#define UART_RATE_FIX_ALL_16M25     (UART_RATE_FIX_16M25|UART_FREQ_SEL_16M25)

extern void mtk_set_current_uart(MTK_UART uart_base);
extern int mtk_get_current_uart(void);

#endif /* !___MTK_UART_H__ */

