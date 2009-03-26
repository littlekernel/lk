/*
 * Copyright (c) 2009 Corey Tabaka
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
#ifndef __ARCH_X86_H
#define __ARCH_X86_H

#include <compiler.h>
#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

void x86_mmu_init(void);

struct x86_iframe {
	uint32_t pivot;										// stack switch pivot
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;	// pushed by common handler using pusha
	uint32_t ds, es, fs, gs;							// pushed by common handler
	uint32_t vector;									// pushed by stub
	uint32_t err_code;									// pushed by interrupt or stub
	uint32_t eip, cs, eflags;							// pushed by interrupt
	uint32_t user_esp, user_ss;							// pushed by interrupt if priv change occurs
};

/*
 * x86 TSS structure
 */
typedef struct {
	uint16_t	backlink, __blh;
	uint32_t	esp0;
	uint16_t	ss0, __ss0h;
	uint32_t	esp1;
	uint16_t	ss1, __ss1h;
	uint32_t	esp2;
	uint16_t	ss2, __ss2h;
	uint32_t	cr3;
	uint32_t	eip;
	uint32_t	eflags;
	uint32_t	eax, ecx, edx, ebx;
	uint32_t	esp, ebp, esi, edi;
	uint16_t	es, __esh;
	uint16_t	cs, __csh;
	uint16_t	ss, __ssh;
	uint16_t	ds, __dsh;
	uint16_t	fs, __fsh;
	uint16_t	gs, __gsh;
	uint16_t	ldt, __ldth;
	uint16_t	trace, bitmap;
	
	uint8_t tss_bitmap[8192];
} __PACKED tss_t;

#define X86_CR0_PE		0x00000001 /* protected mode enable */
#define X86_CR0_MP		0x00000002 /* monitor coprocessor */
#define X86_CR0_EM		0x00000004 /* emulation */
#define X86_CR0_TS		0x00000008 /* task switched */
#define X86_CR0_WP		0x00010000 /* supervisor write protect */
#define X86_CR0_NW		0x20000000 /* not write-through */
#define X86_CR0_CD		0x40000000 /* cache disable */
#define X86_CR0_PG		0x80000000 /* enable paging */

static inline void set_in_cr0(uint32_t mask) {
	__asm__ __volatile__ (
		"movl %%cr0,%%eax	\n\t"
		"orl %0,%%eax		\n\t"
		"movl %%eax,%%cr0	\n\t"
		: : "irg" (mask)
		:"ax");
}

static inline void clear_in_cr0(uint32_t mask) {
	__asm__ __volatile__ (
		"movl %%cr0, %%eax	\n\t"
		"andl %0, %%eax		\n\t"
		"movl %%eax, %%cr0	\n\t"
		: : "irg" (~mask)
		: "ax");
}

static inline void x86_clts(void) {__asm__ __volatile__ ("clts"); }
static inline void x86_hlt(void) {__asm__ __volatile__ ("hlt"); }
static inline void x86_sti(void) {__asm__ __volatile__ ("sti"); }
static inline void x86_cli(void) {__asm__ __volatile__ ("cli"); }
static inline void x86_ltr(uint16_t sel) {
	__asm__ __volatile__ ("ltr %%ax" :: "a" (sel));
}

static inline uint32_t x86_get_cr2(void) {
	uint32_t rv;
	
	__asm__ __volatile__ (
		"movl %%cr2, %0"
		: "=r" (rv)
	);
	
	return rv;
}

#define rdtsc(low,high) \
     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

#define rdtscl(low) \
     __asm__ __volatile__("rdtsc" : "=a" (low) : : "edx")

#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))

static inline uint8_t inp(uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0"
	  : "=a" (rv)
	  : "d" (_port));
    return(rv);
}

static inline uint16_t inpw (uint16_t _port) {
    uint16_t rv;
    __asm__ __volatile__ ("inw %1, %0"
	  : "=a" (rv)
	  : "d" (_port));
    return(rv);
}

static inline uint32_t inpd(uint16_t _port) {
    uint32_t rv;
    __asm__ __volatile__ ("inl %1, %0"
	  : "=a" (rv)
	  : "d" (_port));
    return(rv);
}

static inline void outp(uint16_t _port, uint8_t _data) {
    __asm__ __volatile__ ("outb %1, %0"
	  :
	  : "d" (_port),
	    "a" (_data));
}

static inline void outpw(uint16_t _port, uint16_t _data) {
    __asm__ __volatile__ ("outw %1, %0"
	  :
	  : "d" (_port),
	    "a" (_data));
}

static inline void outpd(uint16_t _port, uint32_t _data) {
    __asm__ __volatile__ ("outl %1, %0"
	  :
	  : "d" (_port),
	    "a" (_data));
}

static inline void inprep(uint16_t _port, uint8_t *_buffer, uint32_t _reads) {
	__asm__ __volatile__ ("pushal \n\t"	
		"pushfl \n\t"	
		"cli \n\t"	
		"cld \n\t"	
		"rep insb \n\t"	
		"popfl \n\t"	
		"popal"	
		:
		: "d" (_port),
		  "D" (_buffer),
		  "c" (_reads));
}

static inline void outprep(uint16_t _port, uint8_t *_buffer, uint32_t _writes) {
	__asm__ __volatile__ ("pushal \n\t"
		"pushfl \n\t"
		"cli \n\t"
		"cld \n\t"
		"rep outsb \n\t"
		"popfl \n\t"
		"popal"
		:
		: "d" (_port),
		  "S" (_buffer),
		  "c" (_writes));
}

static inline void inpwrep(uint16_t _port, uint16_t *_buffer, uint32_t _reads) {
	__asm__ __volatile__ ("pushal \n\t"	
		"pushfl \n\t"	
		"cli \n\t"	
		"cld \n\t"	
		"rep insw \n\t"	
		"popfl \n\t"	
		"popal"	
		:
		: "d" (_port),
		  "D" (_buffer),
		  "c" (_reads));
}

static inline void outpwrep(uint16_t _port, uint16_t *_buffer,
	uint32_t _writes) {
	__asm__ __volatile__ ("pushal \n\t"
		"pushfl \n\t"
		"cli \n\t"
		"cld \n\t"
		"rep outsw \n\t"
		"popfl \n\t"
		"popal"
		:
		: "d" (_port),
		  "S" (_buffer),
		  "c" (_writes));
}

static inline void inpdrep(uint16_t _port, uint32_t *_buffer,
	uint32_t _reads) {
	__asm__ __volatile__ ("pushal \n\t"	
		"pushfl \n\t"	
		"cli \n\t"	
		"cld \n\t"	
		"rep insl \n\t"	
		"popfl \n\t"	
		"popal"	
		:
		: "d" (_port),
		  "D" (_buffer),
		  "c" (_reads));
}

static inline void outpdrep(uint16_t _port, uint32_t *_buffer,
	uint32_t _writes) {
	__asm__ __volatile__ ("pushal \n\t"
		"pushfl \n\t"
		"cli \n\t"
		"cld \n\t"
		"rep outsl \n\t"
		"popfl \n\t"
		"popal"
		:
		: "d" (_port),
		  "S" (_buffer),
		  "c" (_writes));
}

#if defined(__cplusplus)
}
#endif

#endif
