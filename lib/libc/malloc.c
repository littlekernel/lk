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
#include <debug.h>
#include <malloc.h>
#include <string.h>
#include <lib/heap.h>

void *malloc(size_t size)
{
	return heap_alloc(size, 0);
}

void *memalign(size_t boundary, size_t size)
{
	return heap_alloc(size, boundary);
}

void *calloc(size_t count, size_t size)
{
	void *ptr;
	size_t realsize = count * size;

	ptr = heap_alloc(realsize, 0);
	if (!ptr)
		return NULL;

	memset(ptr, 0, realsize);
	return ptr;
}

void *realloc(void *ptr, size_t size)
{
	if (!ptr)
		return malloc(size);

	// XXX better implementation
	void *p = malloc(size);
	if (!p)
		return NULL;

	memcpy(p, ptr, size); // XXX wrong
	free(ptr);

	return p;
}

void free(void *ptr)
{
	return heap_free(ptr);
}

