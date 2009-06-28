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
#ifndef __POW2_H
#define __POW2_H

#include <sys/types.h>
#include <compiler.h>

/* routines for dealing with power of 2 values for efficiency */
static inline __ALWAYS_INLINE bool ispow2(uint val)
{
	return ((val - 1) & val) == 0;
}

static inline __ALWAYS_INLINE uint log2(uint val)
{
	if (!ispow2(val))
		return 0; // undefined

	return __builtin_ctz(val);
}

static inline __ALWAYS_INLINE uint valpow2(uint valp2)
{
	return 1 << valp2;
}

static inline __ALWAYS_INLINE uint divpow2(uint val, uint divp2)
{
	return val >> divp2;
}

static inline __ALWAYS_INLINE uint modpow2(uint val, uint modp2)
{
	return val & ((1UL << modp2) - 1);
}


#endif

