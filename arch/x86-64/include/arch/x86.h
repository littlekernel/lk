/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Intel Corporation
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

__BEGIN_CDECLS

#define PFEX_P			0x01
#define PFEX_W			0x02
#define PFEX_U			0x04
#define PFEX_RSV		0x08
#define PFEX_I			0x10
#define X86_8BYTE_MASK		0xFFFFFFFF
#define X86_CPUID_ADDR_WIDTH	0x80000008

void arch_mmu_init(void);

struct x86_iframe {
	uint64_t pivot;                                     // stack switch pivot
	uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;    	    // pushed by common handler
	uint64_t r8, r9, r10, r11, r12, r13, r14, r15;      // pushed by common handler
	uint64_t vector;                                    // pushed by stub
	uint64_t err_code;                                  // pushed by interrupt or stub
	uint64_t rip, cs, rflags;                           // pushed by interrupt
	uint64_t user_rsp, user_ss;                         // pushed by interrupt if priv change occurs
};
/*
 * x86 TSS structure
 */
typedef struct {
	uint16_t    backlink, __blh;
	uint32_t    esp0;
	uint16_t    ss0, __ss0h;
	uint32_t    esp1;
	uint16_t    ss1, __ss1h;
	uint32_t    esp2;
	uint16_t    ss2, __ss2h;
	uint32_t    cr3;
	uint32_t    eip;
	uint32_t    eflags;
	uint32_t    eax, ecx, edx, ebx;
	uint32_t    esp, ebp, esi, edi;
	uint16_t    es, __esh;
	uint16_t    cs, __csh;
	uint16_t    ss, __ssh;
	uint16_t    ds, __dsh;
	uint16_t    fs, __fsh;
	uint16_t    gs, __gsh;
	uint16_t    ldt, __ldth;
	uint16_t    trace, bitmap;

	uint8_t tss_bitmap[8192];
} __PACKED tss_t;

#define X86_CR0_PE      0x00000001 /* protected mode enable */
#define X86_CR0_MP      0x00000002 /* monitor coprocessor */
#define X86_CR0_EM      0x00000004 /* emulation */
#define X86_CR0_TS      0x00000008 /* task switched */
#define X86_CR0_WP      0x00010000 /* supervisor write protect */
#define X86_CR0_NW      0x20000000 /* not write-through */
#define X86_CR0_CD      0x40000000 /* cache disable */
#define X86_CR0_PG	0x80000000 /* enable paging */
#define x86_EFER_NXE	0x00000800 /* to enable execute disable bit */
#define x86_MSR_EFER	0xc0000080 /* EFER Model Specific Register id */

static inline void set_in_cr0(uint32_t mask)
{
	__asm__ __volatile__ (
		"movl %%cr0,%%eax	\n\t"
		"orl %0,%%eax		\n\t"
		"movl %%eax,%%cr0	\n\t"
		: : "irg" (mask)
		:"ax");
}

static inline void clear_in_cr0(uint32_t mask)
{
	__asm__ __volatile__ (
		"movq %%cr0, %%rax	\n\t"
		"andq %0, %%rax		\n\t"
		"movq %%rax, %%cr0	\n\t"
		: : "irg" (~mask)
		: "ax");
}

static inline void x86_clts(void) {__asm__ __volatile__ ("clts"); }
static inline void x86_hlt(void) {__asm__ __volatile__ ("hlt"); }
static inline void x86_sti(void) {__asm__ __volatile__ ("sti"); }
static inline void x86_cli(void) {__asm__ __volatile__ ("cli"); }
static inline void x86_ltr(uint16_t sel)
{
	__asm__ __volatile__ ("ltr %%ax" :: "a" (sel));
}

static inline uint64_t x86_get_cr2(void)
{
	uint64_t rv;

	__asm__ __volatile__ (
	    "movq %%cr2, %0"
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
	__asm__ __volatile__ ("pushfq \n\t"	
		"cli \n\t"	
		"cld \n\t"	
		"rep insb \n\t"	
		"popfq \n\t"	
		:
		: "d" (_port),
		  "D" (_buffer),
		  "c" (_reads));
}

static inline void outprep(uint16_t _port, uint8_t *_buffer, uint32_t _writes) {
	__asm__ __volatile__ ("pushfq \n\t"
		"cli \n\t"
		"cld \n\t"
		"rep outsb \n\t"
		"popfq \n\t"
		:
		: "d" (_port),
		  "S" (_buffer),
		  "c" (_writes));
}

static inline void inpwrep(uint16_t _port, uint16_t *_buffer, uint32_t _reads) {
	__asm__ __volatile__ ("pushfq \n\t"	
		"cli \n\t"	
		"cld \n\t"	
		"rep insw \n\t"	
		"popfq \n\t"	
		:
		: "d" (_port),
		  "D" (_buffer),
		  "c" (_reads));
}

static inline void outpwrep(uint16_t _port, uint16_t *_buffer,
	uint32_t _writes) {
	__asm__ __volatile__ ("pushfq \n\t"
		"cli \n\t"
		"cld \n\t"
		"rep outsw \n\t"
		"popfq \n\t"
		:
		: "d" (_port),
		  "S" (_buffer),
		  "c" (_writes));
}

static inline void inpdrep(uint16_t _port, uint32_t *_buffer,
	uint32_t _reads) {
	__asm__ __volatile__ ("pushfq \n\t"	
		"cli \n\t"	
		"cld \n\t"	
		"rep insl \n\t"	
		"popfq \n\t"	
		:
		: "d" (_port),
		  "D" (_buffer),
		  "c" (_reads));
}

static inline void outpdrep(uint16_t _port, uint32_t *_buffer,
	uint32_t _writes) {
	__asm__ __volatile__ ("pushfq \n\t"
		"cli \n\t"
		"cld \n\t"
		"rep outsl \n\t"
		"popfq \n\t"
		:
		: "d" (_port),
		  "S" (_buffer),
		  "c" (_writes));
}

static inline uint64_t read_msr (uint32_t msr_id)
{
	uint64_t msr_read_val = 0;
	uint32_t low_val = 0;
	uint32_t high_val = 0;

	__asm__ __volatile__ (
		"rdmsr \n\t"
		: "=a" (low_val), "=d"(high_val)
		: "c" (msr_id));

        msr_read_val = high_val;
        msr_read_val = (msr_read_val << 32) | low_val;

        return msr_read_val;
}

static inline void write_msr (uint32_t msr_id, uint64_t msr_write_val)
{
	uint32_t low_val = (uint32_t)msr_write_val;
	uint32_t high_val = (uint32_t)(msr_write_val >> 32);

	__asm__ __volatile__ (
		"wrmsr \n\t"
		: : "c" (msr_id), "a" (low_val), "d"(high_val));
}

static inline uint64_t x86_get_cr3(void)
{
	uint64_t rv;

	__asm__ __volatile__ (
		"movq %%cr3, %0"
		: "=r" (rv));
	return rv;
}

static inline void x86_set_cr3(uint64_t in_val)
{
	__asm__ __volatile__ (
		"movq %0,%%cr3 \n\t"
		:
		:"r" (in_val));
}

static inline uint64_t x86_get_cr0(void)
{
	uint64_t rv;

	__asm__ __volatile__ (
		"movq %%cr0, %0 \n\t"
		: "=r" (rv));
	return rv;
}

static inline void x86_set_cr0(uint64_t in_val)
{
	__asm__ __volatile__ (
		"movq %0,%%cr0 \n\t"
		:
		:"r" (in_val));
}

static inline uint32_t x86_get_address_width(void)
{
	uint32_t rv;

	__asm__ __volatile__ (
		"cpuid \n\t"
		:"=a" (rv)
		:"a" (X86_CPUID_ADDR_WIDTH));

	/* Extracting bit 15:8 from eax register */
	return ((rv >> 8) & 0x0ff);
}

__END_CDECLS

#endif
