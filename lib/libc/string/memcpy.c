/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
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
#include <string.h>
#include <sys/types.h>


#if !_ASM_MEMCPY

typedef long word;

#define lsize sizeof(word)
#define lmask (lsize - 1)

void *memcpy(void *dest, const void *src, size_t count)
{
	char *d = (char *)dest;
	const char *s = (const char *)src;
	int len;

	if(count == 0 || dest == src)
		return dest;

	if(((long)d | (long)s) & lmask) {
		// src and/or dest do not align on word boundary
		if((((long)d ^ (long)s) & lmask) || (count < lsize))
			len = count; // copy the rest of the buffer with the byte mover
		else
			len = lsize - ((long)d & lmask); // move the ptrs up to a word boundary

		count -= len;
		for(; len > 0; len--)
			*d++ = *s++;
	}
	for(len = count / lsize; len > 0; len--) {
		*(word *)d = *(word *)s;
		d += lsize;
		s += lsize;
	}
	for(len = count & lmask; len > 0; len--)
		*d++ = *s++;

	return dest;
}

#endif
