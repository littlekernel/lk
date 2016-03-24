/* Copyright (c) 2014, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdlib.h>

void *bsearch(const void *key, const void *base, size_t num_elems, size_t size,
              int (*compare)(const void *, const void *))
{
    size_t low = 0, high = num_elems - 1;

    if (num_elems == 0) {
        return NULL;
    }

    for (;;) {
        size_t mid = low + ((high - low) / 2);
        const void *mid_elem = ((unsigned char *) base) + mid*size;
        int r = compare(key, mid_elem);

        if (r < 0) {
            if (mid == 0) {
                return NULL;
            }
            high = mid - 1;
        } else if (r > 0) {
            low = mid + 1;
            if (low < mid || low > high) {
                return NULL;
            }
        } else {
            return (void *) mid_elem;
        }
    }
}
