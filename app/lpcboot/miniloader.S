
.syntax unified

miniloader_vectors:
	.word 0x20003FF0
	.word miniloader_reset + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1
	.word miniloader_fault + 1

// miniloader_boot(unsigned src, unsigned count)
miniloader_boot:
	mov r2, #0x10000000
miniloader_boot_loop:
	ldr r3, [r0], #4
	str r3, [r2], #4
	subs r1, #1
	bne miniloader_boot_loop
	mov r0, #0x10000000
	ldr sp, [r0]
	ldr r0, [r0, #4]
	bx r0

miniloader_reset:
	ldr r0, =0x20003FF8
	ldr r1, =0xAA113377
	ldr r2, =0xAA773311
	ldr r3, [r0]
	ldr r4, [r0, #4]
	mov r5, #0
	str r5, [r0]
	str r5, [r0, #4]
	cmp r1, r3
	bne start_bootloader
	cmp r2, r4
	bne start_bootloader
start_app:
	ldr r0, =0x8000
	ldr r1, =(131072/4)
	b miniloader_boot
start_bootloader:
	ldr r0, =0x00001000
	ldr r1, =(32768/4)
	b miniloader_boot

miniloader_fault:
	b .
