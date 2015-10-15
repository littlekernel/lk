/*
 * Copyright (c) 2015 Google, Inc. All rights reserved
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
#ifndef __LIB_PAGE_ALLOC_H
#define __LIB_PAGE_ALLOC_H

#include <stddef.h>
#include <sys/types.h>
#include <compiler.h>

__BEGIN_CDECLS;

#if WITH_KERNEL_VM

typedef struct list_node page_alloc_handle;

#else

typedef struct {
    void* address;
    size_t pages;
} page_alloc_handle;

#endif

void *page_alloc(size_t pages, page_alloc_handle *list);
void page_free(page_alloc_handle *list);

// You can call this once at the start, and it will either return a page or it
// will return some non-page-aligned memory that would otherwise go to waste.
void *page_first_alloc(size_t *size_return);

__END_CDECLS;

#endif
