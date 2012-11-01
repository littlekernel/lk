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
#ifndef __ENDIAN_H
#define __ENDIAN_H

#include <sys/types.h>

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif

#if __POWERPC__
#include <ppc_intrinsics.h>
#endif

#if defined(ARCH_ARM)
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if defined(__i386__) || defined(_X86_)
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifndef BYTE_ORDER
#error "need to get the BYTE_ORDER define from somewhere"
#endif

// define a macro that unconditionally swaps
#define SWAP_32(x) \
    (((uint32_t)(x) << 24) | (((uint32_t)(x) & 0xff00) << 8) |(((uint32_t)(x) & 0x00ff0000) >> 8) | ((uint32_t)(x) >> 24))
#define SWAP_16(x) \
    ((((uint16_t)(x) & 0xff) << 8) | ((uint16_t)(x) >> 8))

// standard swap macros
#if BYTE_ORDER == BIG_ENDIAN
#define LE32(val) SWAP_32(val)
#define LE16(val) SWAP_16(val)
#define BE32(val) (val)
#define BE16(val) (val)
#else
#define LE32(val) (val)
#define LE16(val) (val)
#define BE32(val) SWAP_32(val)
#define BE16(val) SWAP_16(val)
#endif

#define LE32SWAP(var) (var) = LE32(var);
#define LE16SWAP(var) (var) = LE16(var);
#define BE32SWAP(var) (var) = BE32(var);
#define BE16SWAP(var) (var) = BE16(var);

/* classic network byte swap stuff */
#define ntohs(n) BE16(n)
#define htons(h) BE16(h)
#define ntohl(n) BE32(n)
#define htonl(h) BE32(h)

// some memory access macros
#if __POWERPC__
#define READ_MEM_WORD(ptr)      __lwbrx((word *)(ptr), 0)
#define READ_MEM_HALFWORD(ptr)  __lhbrx((halfword *)(ptr), 0)
#define READ_MEM_BYTE(ptr)      (*(byte *)(ptr))
#define WRITE_MEM_WORD(ptr, data)   __stwbrx(data, (word *)(ptr), 0)
#define WRITE_MEM_HALFWORD(ptr, data)   __sthbrx(data, (halfword *)(ptr), 0)
#define WRITE_MEM_BYTE(ptr, data)   (*(byte *)(ptr) = (data))
#else
#define READ_MEM_WORD(ptr)      SWAPIT_32(*(word *)(ptr))
#define READ_MEM_HALFWORD(ptr)  SWAPIT_16(*(halfword *)(ptr))
#define READ_MEM_BYTE(ptr)      (*(byte *)(ptr))
#define WRITE_MEM_WORD(ptr, data)   (*(word *)(ptr) = SWAPIT_32(data))
#define WRITE_MEM_HALFWORD(ptr, data)   (*(halfword *)(ptr) = SWAPIT_16(data))
#define WRITE_MEM_BYTE(ptr, data)   (*(byte *)(ptr) = (data))
#endif


#endif
