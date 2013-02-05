/*
 * Copyright (c) 2005-2010 Travis Geiselbrecht
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
#ifndef __MEMMAP_H
#define __MEMMAP_H

#define MEMBANK_SIZE (4*1024*1024)

/* some helpful macros */
#define REG(x) ((volatile unsigned int *)(x))
#define REG_H(x) ((volatile unsigned short *)(x))
#define REG_B(x) ((volatile unsigned char *)(x))

/* memory map of our generic arm system */
// XXX make more dynamic
#define MAINMEM_BASE 0x0
#define MAINMEM_SIZE (MEMBANK_SIZE)

/* peripherals are all mapped here */
#define PERIPHERAL_BASE   (0xf0000000)

/* system info */
#define SYSINFO_REGS_BASE (PERIPHERAL_BASE)
#define SYSINFO_REGS_SIZE MEMBANK_SIZE
#define SYSINFO_FEATURES  (SYSINFO_REGS_BASE + 0)
#define SYSINFO_FEATURE_DISPLAY 0x00000001
#define SYSINFO_FEATURE_CONSOLE 0x00000002
#define SYSINFO_FEATURE_NETWORK 0x00000004
#define SYSINFO_FEATURE_BLOCKDEV 0x00000008

/* a write to this register latches the current emulator system time, so the next two regs can be read atomically */
#define SYSINFO_TIME_LATCH (SYSINFO_REGS_BASE + 4)
/* gettimeofday() style time values */
#define SYSINFO_TIME_SECS  (SYSINFO_REGS_BASE + 8)
#define SYSINFO_TIME_USECS (SYSINFO_REGS_BASE + 12)

/* display */
#define DISPLAY_BASE      (SYSINFO_REGS_BASE + SYSINFO_REGS_SIZE)
#define DISPLAY_SIZE      MEMBANK_SIZE
#define DISPLAY_FRAMEBUFFER DISPLAY_BASE
#define DISPLAY_REGS_BASE (DISPLAY_BASE + DISPLAY_SIZE)
#define DISPLAY_REGS_SIZE MEMBANK_SIZE

#define DISPLAY_WIDTH     (DISPLAY_REGS_BASE + 0) // pixels width/height read/only
#define DISPLAY_HEIGHT    (DISPLAY_REGS_BASE + 4)
#define DISPLAY_BPP       (DISPLAY_REGS_BASE + 8) // bits per pixel (16/32)

/* console (keyboard controller */
#define CONSOLE_REGS_BASE (DISPLAY_REGS_BASE + DISPLAY_REGS_SIZE)
#define CONSOLE_REGS_SIZE MEMBANK_SIZE
#define KYBD_STAT         (CONSOLE_REGS_BASE + 0)
#define KYBD_DATA         (CONSOLE_REGS_BASE + 4)

/* programmable timer */
#define PIT_REGS_BASE     (CONSOLE_REGS_BASE + CONSOLE_REGS_SIZE)
#define PIT_REGS_SIZE     MEMBANK_SIZE
#define PIT_STATUS        (PIT_REGS_BASE + 0) // status bit
#define PIT_CLEAR         (PIT_REGS_BASE + 4) // a nonzero write clears any pending timer
#define PIT_CLEAR_INT     (PIT_REGS_BASE + 8) // a nonzero write clears the pending interrupt
#define PIT_INTERVAL      (PIT_REGS_BASE + 12) // set the countdown interval, and what the interval is reset to if periodic
#define PIT_START_ONESHOT (PIT_REGS_BASE + 16) // a nonzero write starts a oneshot countdown
#define PIT_START_PERIODIC (PIT_REGS_BASE + 20) // a nonzero write starts a periodic countdown

#define PIT_STATUS_ACTIVE    0x1
#define PIT_STATUS_INT_PEND  0x2

/* interrupt controller */
#define PIC_REGS_BASE     (PIT_REGS_BASE + PIT_REGS_SIZE)
#define PIC_REGS_SIZE     MEMBANK_SIZE

/* Current vector mask, read-only */
#define PIC_MASK          (PIC_REGS_BASE + 0)
/* Mask any of the 32 interrupt vectors by writing a 1 in the appropriate bit */
#define PIC_MASK_LATCH    (PIC_REGS_BASE + 4)
/* Unmask any of the 32 interrupt vectors by writing a 1 in the appropriate bit */
#define PIC_UNMASK_LATCH  (PIC_REGS_BASE + 8)
/* each bit corresponds to the current status of the interrupt line */
#define PIC_STAT          (PIC_REGS_BASE + 12)
/* one bit set for the highest priority non-masked active interrupt */
#define PIC_CURRENT_BIT   (PIC_REGS_BASE + 16)
/* holds the current interrupt number of the highest priority non-masked active interrupt,
 * or 0xffffffff if no interrupt is active
 */
#define PIC_CURRENT_NUM   (PIC_REGS_BASE + 20)

/* interrupt map */
#define INT_PIT      0
#define INT_KEYBOARD 1
#define INT_NET      2
#define PIC_MAX_INT 32

/* debug interface */
#define DEBUG_REGS_BASE (PIC_REGS_BASE + PIC_REGS_SIZE)
#define DEBUG_REGS_SIZE MEMBANK_SIZE
#define DEBUG_STDOUT (DEBUG_REGS_BASE + 0) /* writes to this register are sent through to stdout */
#define DEBUG_STDIN  (DEBUG_REGS_BASE + 0) /* reads from this register return the contents of stdin
                                            * or -1 if no data is pending */
#define DEBUG_REGDUMP (DEBUG_REGS_BASE + 4) /* writes to this register cause the emulator to dump registers */
#define DEBUG_HALT    (DEBUG_REGS_BASE + 8) /* writes to this register will halt the emulator */

#define DEBUG_MEMDUMPADDR (DEBUG_REGS_BASE + 12)      /* set the base address of memory to dump */
#define DEBUG_MEMDUMPLEN  (DEBUG_REGS_BASE + 16)      /* set the length of memory to dump */
#define DEBUG_MEMDUMP_BYTE  (DEBUG_REGS_BASE + 20)    /* trigger a memory dump in byte format */
#define DEBUG_MEMDUMP_HALFWORD (DEBUG_REGS_BASE + 24) /* trigger a memory dump in halfword format */
#define DEBUG_MEMDUMP_WORD (DEBUG_REGS_BASE + 28)     /* trigger a memory dump in word format */

/* lets you set the trace level of the various subsystems from within the emulator */
/* only works on emulator builds that support dynamic trace levels */
#define DEBUG_SET_TRACELEVEL_CPU (DEBUG_REGS_BASE + 32)
#define DEBUG_SET_TRACELEVEL_UOP (DEBUG_REGS_BASE + 36)
#define DEBUG_SET_TRACELEVEL_SYS (DEBUG_REGS_BASE + 40)
#define DEBUG_SET_TRACELEVEL_MMU (DEBUG_REGS_BASE + 44)

#define DEBUG_CYCLE_COUNT (DEBUG_REGS_BASE + 48)
#define DEBUG_INS_COUNT (DEBUG_REGS_BASE + 52)

/* network interface */
#define NET_REGS_BASE (DEBUG_REGS_BASE + DEBUG_REGS_SIZE)
#define NET_REGS_SIZE MEMBANK_SIZE

#define NET_BUF_LEN 2048
#define NET_IN_BUF_COUNT 32

#define NET_HEAD    (NET_REGS_BASE + 0)     /* current next buffer the hardware will write to */
#define NET_TAIL    (NET_REGS_BASE + 4)     /* currently selected input buffer */
#define NET_SEND    (NET_REGS_BASE + 8)     /* writes to this register sends whatever is in the out buf */
#define NET_SEND_LEN (NET_REGS_BASE + 12)   /* length of packet to send */
#define NET_OUT_BUF (NET_REGS_BASE + NET_BUF_LEN)

#define NET_IN_BUF_LEN (NET_REGS_BASE + 16) /* length of the currently selected in buffer, via tail register */
#define NET_IN_BUF  (NET_REGS_BASE + NET_BUF_LEN*2)

/* block device interface */
#define BDEV_REGS_BASE (NET_REGS_BASE + NET_REGS_SIZE)
#define BDEV_REGS_SIZE MEMBANK_SIZE

#define BDEV_CMD    (BDEV_REGS_BASE + 0)    /* command */
#define BDEV_CMD_ADDR   (BDEV_REGS_BASE + 4)    /* address of next transfer, 32bit */
#define BDEV_CMD_OFF    (BDEV_REGS_BASE + 8)    /* offset of next transfer, 64bit */
#define BDEV_CMD_LEN    (BDEV_REGS_BASE + 16)   /* length of next transfer, 32bit */

#define BDEV_LEN    (BDEV_REGS_BASE + 20)   /* length of block device, 64bit */

/* BDEV_CMD bits */
#define BDEV_CMD_MASK   (0x3)
#define BDEV_CMD_NOP    (0)
#define BDEV_CMD_READ   (1)
#define BDEV_CMD_WRITE  (2)
#define BDEV_CMD_ERASE  (3)
#define BDEV_CMD_ERRSHIFT   16
#define BDEV_CMD_ERRMASK    (0xffff << BDEV_CMD_ERRSHIFT)
#define BDEV_CMD_ERR_NONE (0 << BDEV_CMD_ERRSHIFT)
#define BDEV_CMD_ERR_GENERAL (1 << BDEV_CMD_ERRSHIFT)
#define BDEV_CMD_ERR_BAD_OFFSET (2 << BDEV_CMD_ERRSHIFT)

#endif
