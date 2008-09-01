/* init_clock.S -- AT91SAM7 clock coldstart code
**
** Copyright 2006, Brian Swetland.  All rights reserved.     
** See provided LICENSE file or http://frotz.net/LICENSE for details.
*/

.globl init_clock

init_clock:
/* init flash controller timing for 18.432MHz */
	mov r1, #0xffffff00
	ldr r0, =0x00340100
	str r0, [r1, #0x60]

#define PMC_MOR    0x20
#define PMC_MCFR   0x24
#define PMC_PLLR   0x2c
#define PMC_MCKR   0x30
#define PMC_SR     0x68
		
/* PMC_MOR */
#define PMC_MOSCEN     0x01
#define PMC_OSCBYPASS  0x02

/* PMC_MCFR */
#define PMC_MAINRDY	   0x00010000

/* PMC_SR */
#define PMC_MOSCS      0x01
#define PMC_LOCK       0x04
#define PMC_MCKRDY     0x08

/* PMC_MCKR */
#define PMC_CSS_SLOW   0x00
#define PMC_CSS_MAIN   0x01
#define PMC_CSS_PLL    0x03
#define PMC_PRES_NONE  0x00
#define PMC_PRES_DIV2  0x04
#define PMC_PRES_DIV4  0x08

/* Oscillator Init Sequence based on the Atmel sample code
** in cstartup_boot_SAM7S32_64.s  
**
** I cleaned it up a bit -- why they use a temporary register,
** AND and then CMP instead of just TSTing against an immediate
** boggles my mind.  I think this could be a bit simpler yet, 
** but debugging it is a pain, so Good Enough wins for now.
*/
	ldr r1, =0xfffffc00

/* bypass main oscillator */
	mov r0, #PMC_OSCBYPASS
	str r0, [r1, #PMC_MOR]

/* compensate MAINRDY rising flag (45 SCLK) */
	mov r0, #45
1:	subs r0, r0, #1
	bhi 1b

/* if MAINRDY is set, we have an external oscillator */
	ldr r0, [r1, #PMC_MCFR]
	tst r0, #PMC_MAINRDY
	bne ext_osc_found

/* reset MOSCS flag */
	mov r0, #0
	str r0, [r1, #PMC_MOR]

/* enable main oscillator */
	ldr r0, =((0x40 << 8) | PMC_MOSCEN)
	str r0, [r1, #PMC_MOR]

/* wait for main oscillator to come online */
1:	ldr r0, [r1, #PMC_SR]
	tst r0, #PMC_MOSCS
	beq 1b

ext_osc_found:
/* select main oscillator, no prescaler for MCK */
	mov r0, #(PMC_CSS_MAIN | PMC_PRES_NONE)
	str r0, [r1, #PMC_MCKR]

/* wait until MCK settles to continue */
1:	ldr r0, [r1, #PMC_SR]
	tst r0, #PMC_MCKRDY
	beq 1b

/* this is a bit of voodoo for selecting a 96.109MHz PLL
** freq (MUL=72, DIV=14, OUT=0, USBDIV=/1) from the 18.432MHz
** main clock.
*/
	ldr r0, =0x10483f0e
	str r0, [r1, #PMC_PLLR]

/* let the PLL lock before we continue */
1:	ldr r0, [r1, #PMC_SR]
	tst r0, #PMC_LOCK
	beq 1b

	mov pc, lr

