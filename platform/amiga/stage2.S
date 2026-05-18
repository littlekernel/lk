/*
 * Copyright (c) 2025-2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

.text

kaddr_ptr:
   .dc.l 0
ioreq_ptr:
   .dc.l 0
hdr_ptr:
   .dc.l 0
ksize:
   .dc.l 0

/* AllocMem stuff */
.equ    MEMF_ANY,       0x00000000
.equ    MEMF_PUBLIC,    0x00000001
.equ    MEMF_CHIP,      0x00000002
.equ    MEMF_FAST,      0x00000004

.equ    MEMF_TOTAL,     0x00080000

.equ    MEMF_LOCAL,     0x00000100
.equ    MEMF_24BITDMA,  0x00000200
.equ    MEMF_KICK,      0x00000400

.equ    MEMF_CLEAR,     0x00010000
.equ    MEMF_LARGEST,   0x00020000
.equ    MEMF_REVERSE,   0x00040000

/* Kickstart Exec library routine offsets */
.equ _LVOAllocAbs,      -204
.equ _LVOAllocMem,      -198
.equ _LVOAvailMem,      -216
.equ _LVOCacheClearU,   -636
.equ _LVOWaitIO,        -318
.equ _LVOCheckIO,       -468
.equ _LVODoIO,          -456

/* IORequest stuff */
.equ    CMD_READ,   2
.equ    io_Unit,    24
.equ    io_Command, 28
.equ    io_Error,   31
.equ    io_Length,  36
.equ    io_Data,    40
.equ    io_Offset,  44

/* Stack stuff */
.equ    STACK_TOP,   0x00070000
.equ    STACK_SIZE,  0x00004000
.equ    STACK_BASE,  (STACK_TOP-STACK_SIZE)

lea STACK_TOP,%sp

lea     loader(%pc),%a5
move.l  %a5,0x00000080        /* Trap handler */

/* Enter supervisor mode */
trap    #0 

loader:
   lea STACK_TOP,%sp
   movea.l 0x00000004.w,%a6            /* Exec base */

   /* Stash IORequest address, passed in from Kickstart */
   lea ioreq_ptr,%a5
   move.l %a1,(%a5)
   
   move.l #STACK_SIZE,%d0
   move.l #STACK_BASE,%a1
   jsr _LVOAllocAbs(%a6)
   tst.l %d0
   beq failure

   /* Reserve memory and load header */
   move.l #1024,%d0
   move.l #(MEMF_CLEAR + MEMF_PUBLIC + MEMF_CHIP),%d1
   jsr _LVOAllocMem(%a6)
   tst.l %d0
   beq failure
   lea hdr_ptr,%a5
   move.l %d0,(%a5)
   move.l %d0,%a3

   /* Restore IOReq ptr */
   lea ioreq_ptr,%a1
   move.l (%a1),%a1

   bsr wait_ioreq_idle

   /* Retrieve kernel image size from header in ADF */
   move.l  #(2*512),io_Length(%a1)
   move.l  %a3,io_Data(%a1) /* Read header from floppy in to memory provided by AllocMem */
   move.l  #0x400,io_Offset(%a1)
   move.w  #CMD_READ,io_Command(%a1)
   jsr     _LVODoIO(%a6)
   tst.b io_Error(%a1)
   bne failure

   /* Determine how much trackdisk should read, based on kernel size. Round to next 512 */
   lea ksize(%pc),%a4
   move.l (%a3),%d2
   add.l #511,%d2
   and.l #0xFFFFFE00,%d2
   move.l %d2,(%a4)

   /* Allocate memory for kernel image */
   move.l (%a4),%d0
   move.l #(MEMF_REVERSE + MEMF_CLEAR + MEMF_CHIP),%d1
   jsr _LVOAllocMem(%a6)
   tst.l %d0
   beq failure
   lea kaddr_ptr,%a0
   move.l %d0,(%a0)
   move.l %d0,%a2

   /* Restore IOReq pointer before next DoIO call */
   lea ioreq_ptr,%a1
   move.l (%a1),%a1

   bsr wait_ioreq_idle

   /* Load kernel from floppy in to RAM */
   lea ksize(%pc),%a4
   move.l  (%a4),io_Length(%a1)
   move.l  %a2,io_Data(%a1) /* Read kernel from floppy in to memory provided by AllocMem */
   move.l  #0x800,io_Offset(%a1)
   move.w  #CMD_READ,io_Command(%a1)
   jsr     _LVODoIO(%a6)
   tst.b io_Error(%a1)
   bne failure

   bsr wait_ioreq_idle

   move.w  #9,io_Command(%a1)    /* td_motor */
   clr.l   io_Length(%a1)        /* Turn off FDD motor */
   jsr     _LVODoIO(%a6)
   tst.b io_Error(%a1)
   bne failure

   lea kaddr_ptr,%a0
   move.l (%a0),%a2
   jsr   _LVOCacheClearU(%a6)

   /* Detect available chip RAM, pass to kernel via stack */
   move.l #(MEMF_CHIP + MEMF_TOTAL),%d1
   jsr   _LVOAvailMem(%a6)

   clr.l -(%sp)
   clr.l -(%sp)
   clr.l -(%sp)
   move.l %d0,-(%sp)

   /* Jump to kernel. Kickstart will perform cleanup, etc. */
   move.l #0,%d0
   move.l %a2,%a0
   jsr (%a0)

failure:
   | TODO: Throw a useful error, more graceful failure
   move.w  #0xFF00,0x00DFF180    /* Flash a colour on screen for debugging */
   moveq #0xfffffff0,%d0
   rts

wait_ioreq_idle:
   move.l %a1,-(%sp)
   jsr   _LVOCheckIO(%a6)
   move.l (%sp),%a1
   tst.l %d0
   bne.s .done

   jsr   _LVOWaitIO(%a6)
.done:
   move.l (%sp)+,%a1
   rts
