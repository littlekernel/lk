/* fw-m0sub.S
 *
 * Copyright 2015 Brian Swetland <swetland@frotz.net>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

.syntax unified

m0_vectors:
	.word 0x18003FF0
	.word m0_reset + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
	.word m0_fault + 1
// external IRQs
	.word m0_fault + 1
	.word m0_irq + 1

m0_fault:
	ldr r0, =0x18000000
	ldr r1, =0xeeee0000
	mrs r2, xpsr
	movs r3, #0xFF
	ands r2, r2, r3
	orrs r1, r1, r2
	str r1, [r0]
	b .

.ltorg

#define REPORT_DELAY	0

#define COMM_BASE	0x18004000

#define	COMM_CMD	0
#define COMM_ARG0	4
#define COMM_ARG1	8
#define COMM_RESP	12
#define COMM_RETRY	16


#define M4_TXEV		0x40043130 // write 0 to clear

#define SGPIO_BASE	(0x40101210)
#define OFF_IN		0
#define OFF_OUT		4
#define OFF_OEN		8
#define SGPIO_IN	(0x40101210)
#define SGPIO_OUT	(0x40101214)
#define SGPIO_OEN	(0x40101218)

#define CLK_BIT		11
#define DIO_BIT		14
#define TEN_BIT		15
#define CLK_MSK		(1 << CLK_BIT)
#define DIO_MSK		(1 << DIO_BIT)
#define TEN_MSK		(1 << TEN_BIT)

#define CLK1_OUT	(CLK_MSK | TEN_MSK)
#define CLK0_OUT	(TEN_MSK)
#define CLK1_IN		(CLK_MSK)
#define CLK0_IN		(0)

#define OEN_IN		((1 << CLK_BIT) | (1 << TEN_BIT))
#define OEN_OUT		((1 << CLK_BIT) | (1 << DIO_BIT) | (1 << TEN_BIT))

#define NOP4	nop ; nop ; nop ; nop
#define NOP8	NOP4 ; NOP4
#define NOP16	NOP8 ; NOP8

//#define DELAY	nop ; nop
//#define DELAY NOP8

// r11 CLK1_OUT     const
// r10 CLK0_OUT     const
// r9  delay        subroutine
// r8  comm_base    addr
// r7  SGPIO_BASE   addr
// r6  DIO_MSK      const
// r5  CLK1_IN      const
// r4  CLK0_IN      const
// r3  outbits      data

snooze_2m:
	nop ; nop ; nop ; nop
	nop ; nop ; nop ; nop
	nop ; nop ; nop ; nop
	nop ; nop ; nop ; nop
snooze_3m:
	nop ; nop ; nop ; nop
	nop ; nop ; nop ; nop
snooze_4m:
	nop ; nop ; nop ; nop
	nop ; nop ; nop ; nop
snooze_6m:
	nop ; nop ; nop ; nop
snooze_8m:
	bx lr

// delay    0 nops  16MHz
// delay    2 nops  12MHz
// delay    4 nops   9.6MHz
#define DELAY blx r9

// 12 cycles + DELAY x 2
.macro ONE_BIT_OUT
	lsls r2, r3, #DIO_BIT	// shift bit 1 to posn
	ands r2, r2, r6		// isolate bit 1
	movs r1, r2		// save bit 1
	add r2, r2, r10		// combine with CLK1
	DELAY
	str r2, [r7, #OFF_OUT]	// commit negative edge
	lsrs r3, r3, #1		// advance to next bit
	add r1, r1, r11		// combine with CLK1
	nop
	nop
	DELAY
	str r1, [r7, #OFF_OUT]	// commit positive edge
.endm

.macro ONE_BIT_IN
	ands r0, r0, r6		// isolate input bit
	lsls r0, r0, #(31-DIO_BIT) // move to posn 31
	lsrs r3, r3, #1		// make room
	orrs r3, r3, r0		// add bit
	DELAY
	str r4, [r7, #OFF_OUT]	// commit negative edge
	ldr r0, [r7, #OFF_IN]	// sample input
	nop
	nop
	DELAY
	str r5, [r7, #OFF_OUT]	// commit positive edge
.endm

// used for the final parity and turn bits on input so this
// actually only reads one bit
read_2:
	push {lr}
	nop
	nop
	nop
	nop
	DELAY
	str r4, [r7, #OFF_OUT]
	ldr r0, [r7, #OFF_IN]
	nop
	nop
	DELAY
	str r5, [r7, #OFF_OUT]
	ands r0, r0, r6		// isolate bit
	lsrs r0, r0, #DIO_BIT	// shift to bit0
	nop
	nop
	DELAY
	str r4, [r7, #OFF_OUT]
	nop
	nop
	nop
	nop
	DELAY
	str r5, [r7, #OFF_OUT]
	pop {pc}

// w0: <15> <parity:1> <cmd:16>
// w1: <data:32>


write_16:
	push {lr}
	b _write_16
write_32:
	push {lr}
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
_write_16:
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	ONE_BIT_OUT
	pop {pc}
write_1:
	push {lr}
	ONE_BIT_OUT
	pop {pc}

read_4:
	push {lr}
	b _read_4
read_32:
	push {lr}
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
_read_4:
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ONE_BIT_IN
	ands r0, r0, r6		// isolate input bit
	lsls r0, r0, #(31-DIO_BIT) // move to posn 31
	lsrs r3, r3, #1		// make room
	orrs r3, r3, r0		// add bit
	pop {pc}

init:
	ldr r0, =CLK1_OUT
	mov r11, r0
	ldr r0, =CLK0_OUT
	mov r10, r0
	ldr r0, =(snooze_4m + 1)
	mov r9, r0
	ldr r0, =COMM_BASE
	mov r8, r0
	ldr r7, =SGPIO_BASE
	ldr r6, =DIO_MSK
	ldr r5, =CLK1_IN
	ldr r4, =CLK0_IN
	bx lr

#define MAX_RETRY	8192

err_fail:
	movs r0, #3
	mov r3, r8
	str r0, [r3, #COMM_RESP];
	pop {pc}

err_timeout:
	movs r0, #2
	mov r3, r8
	str r0, [r3, #COMM_RESP];
	pop {pc}

cmd_read_txn:
	push {lr}

	ldr r0, =MAX_RETRY
	//movs r0, #MAX_RETRY
	mov r12, r0

rd_retry:
	ldr r3, [r3, #COMM_ARG0]
	bl write_16

	ldr r3, =OEN_IN
	str r3, [r7, #OFF_OEN]
	bl read_4

	lsrs r3, r3, #29
	cmp r3, #1		// OK
	beq rd_okay

	ldr r1, =OEN_OUT
	str r1, [r7, #OFF_OEN]

	cmp r3, #2		// WAIT
	bne err_fail

	mov r0, r12
	subs r0, r0, #1
	mov r12, r0
	beq err_timeout
	mov r3, r8
	b rd_retry

rd_okay:
	bl read_32
	bl read_2
	ldr r1, =OEN_OUT
	str r1, [r7, #OFF_OEN]
	mov r1, r11
	orrs r1, r1, r6
	str r1, [r7, #OFF_OUT]

	mov r1, r8		// get COMM_BASE
	str r3, [r1, #COMM_ARG0]
	str r0, [r1, #COMM_ARG1]
	movs r0, #0
	str r0, [r1, #COMM_RESP]
#if REPORT_DELAY
	mov r0, r12
	str r0, [r1, #COMM_RETRY]
#endif
	pop {pc}
	

cmd_write_txn:
	push {lr}

	ldr r0, =MAX_RETRY
	mov r12, r0

wr_retry:
	ldr r3, [r3, #COMM_ARG0]
	bl write_16
	push {r3}		// stash parity bit

	ldr r3, =OEN_IN
	str r3, [r7, #OFF_OEN]
	bl read_4

	lsrs r3, r3, #29
	cmp r3, #1		// OK
	beq wr_okay

	pop {r0}		// discard saved parity bit

	ldr r1, =OEN_OUT
	str r1, [r7, #OFF_OEN]

	cmp r3, #2		// WAIT
	bne err_fail

	mov r0, r12
	subs r0, r0, #1
	mov r12, r0
	beq err_timeout

	mov r3, r8
	b wr_retry

wr_okay:
	ldr r3, =OEN_OUT
	str r3, [r7, #OFF_OEN]
	bl write_1

	mov r3, r8
	ldr r3, [r3, #COMM_ARG1]
	bl write_32

	pop {r3}		// recover parity bit
	bl write_1

	mov r3, r8		// get COMM_BASE
	movs r0, #0
	str r0, [r3, #COMM_RESP]
#if REPORT_DELAY
	mov r0, r12
	str r0, [r3, #COMM_RETRY]
#endif
	pop {pc}

cmd_reset:
	push {lr}
	ldr r3, =0xffffffff
	mov r12, r3
	bl write_32
	mov r3, r12
	bl write_32

	ldr r3, =0b1110011110011110
	bl write_16

	mov r3, r12
	bl write_32
	mov r3, r12
	bl write_32

	mov r3, r8
	movs r0, #0
	str r0, [r3, #COMM_RESP]
	pop {pc}


m0_irq:
	push {lr}

	// clear event from m4
	ldr r0, =M4_TXEV
	movs r1, #0
	str r1, [r0]

	mov r3, r8		// get COMM_BASE
	ldr r0, [r3, #COMM_CMD]
	cmp r0, #5
	bls good_cmd
	movs r0, #0
good_cmd:
	lsls r0, r0, #2
	adr r1, cmd_table
	ldr r2, [r1, r0]
	blx r2

	pop {pc}

.align 2
cmd_table:
	.word cmd_invalid + 1
	.word cmd_nop + 1
	.word cmd_read_txn + 1
	.word cmd_write_txn + 1
	.word cmd_reset + 1
	.word cmd_setclock + 1

cmd_invalid:
	movs r0, #9
	str r0, [r3, #COMM_RESP]
	bx lr

cmd_nop:
	movs r0, #0
	str r0, [r3, #COMM_RESP]
	bx lr

cmd_setclock:
	ldr r0, [r3, #COMM_ARG0]
	cmp r0, #8
	bls good_clock
	movs r0, #0
good_clock:
	lsls r0, r0, #2
	adr r1, snooze_table
	ldr r1, [r1, r0]
	mov r9, r1

	movs r0, #0
	str r0, [r3, #COMM_RESP]
	bx lr

.align 2
snooze_table:
	.word snooze_2m + 1
	.word snooze_2m + 1
	.word snooze_2m + 1
	.word snooze_3m + 1
	.word snooze_4m + 1
	.word snooze_4m + 1
	.word snooze_6m + 1
	.word snooze_6m + 1
	.word snooze_8m + 1
	
m0_reset:
	ldr r0, =0x18000000
	ldr r1, =0xaaaa0000
	str r1, [r0]

	bl init

	// enable IRQ1 (Event From M4)
	ldr r0, =0xE000E100
	movs r1, #2
	str r1, [r0]

m0_idle:
	wfi
	b m0_idle
