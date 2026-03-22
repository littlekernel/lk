.syntax unified
.thumb

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 4
#endif

// QMI register constants for RP2350.
#define XIP_QMI_BASE       0x400d0000
#define QMI_M0_TIMING_OFF  0x0c
#define QMI_M0_RFMT_OFF    0x10
#define QMI_M0_RCMD_OFF    0x14

// Match SDK boot2_generic_03h defaults for serial read (03h) XIP setup.
#define INIT_M0_TIMING ((1u << 30) | (1u << 8) | (PICO_FLASH_SPI_CLKDIV << 0))
#define INIT_M0_RCMD   0x00000003u
#define INIT_M0_RFMT   0x00000020u

.section .text.boot, "ax"
.p2align 2

.global _stage2_boot
.type _stage2_boot, %function
.thumb_func
_stage2_boot:
	push {lr}

	ldr r3, =XIP_QMI_BASE
	ldr r0, =INIT_M0_TIMING
	str r0, [r3, #QMI_M0_TIMING_OFF]

	ldr r0, =INIT_M0_RCMD
	str r0, [r3, #QMI_M0_RCMD_OFF]

	ldr r0, =INIT_M0_RFMT
	str r0, [r3, #QMI_M0_RFMT_OFF]

	pop {pc}

.ltorg
