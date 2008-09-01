/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#ifndef __SMC91C96_P_H
#define __SMC91C96_P_H

// LAN91C96 stuffs

/* registers */

#define SMC_BSR   14

/* bank 0 */
#define SMC_TCR	  0
#define SMC_EPHSR 2
#define SMC_RCR   4
#define SMC_ECR   6
#define SMC_MIR   8
#define SMC_MCR   10

/* bank 1 */
#define SMC_CR    0
#define SMC_BAR   2
#define SMC_IAR0  4
#define SMC_IAR1  5
#define SMC_IAR2  6
#define SMC_IAR3  7
#define SMC_IAR4  8
#define SMC_IAR5  9
#define SMC_GPR   10
#define SMC_CTR   12

/* bank 2 */
#define SMC_MMUCR 0
#define SMC_AUTOTX 1
#define SMC_PNR   2
#define SMC_ARR   3
#define SMC_FIFO  4
#define SMC_PTR   6
#define SMC_DATA0 8
#define SMC_DATA1 10
#define SMC_IST   12
#define SMC_ACK   12
#define SMC_MSK   13

/* bank 3 */
#define SMC_MT0   0
#define SMC_MT1   1
#define SMC_MT2   2
#define SMC_MT3   3
#define SMC_MT4   4
#define SMC_MT5   5
#define SMC_MT6   6
#define SMC_MT7   7
#define SMC_MGMT  8
#define SMC_REV   10
#define SMC_ERCV  12


#endif

