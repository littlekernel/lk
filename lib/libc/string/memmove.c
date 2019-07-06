/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <string.h>
#include <sys/types.h>

#if !_ASM_MEMMOVE

typedef long word;

#define lsize sizeof(word)
#define lmask (lsize - 1)

void *
memmove(void *dest, void const *src, size_t count) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    int len;

    if (count == 0 || dest == src)
        return dest;

    if ((long)d < (long)s) {
        if (((long)d | (long)s) & lmask) {
            // src and/or dest do not align on word boundary
            if ((((long)d ^ (long)s) & lmask) || (count < lsize))
                len = count; // copy the rest of the buffer with the byte mover
            else
                len = lsize - ((long)d & lmask); // move the ptrs up to a word boundary

            count -= len;
            for (; len > 0; len--)
                *d++ = *s++;
        }
        for (len = count / lsize; len > 0; len--) {
            *(word *)d = *(word *)s;
            d += lsize;
            s += lsize;
        }
        for (len = count & lmask; len > 0; len--)
            *d++ = *s++;
    } else {
        d += count;
        s += count;
        if (((long)d | (long)s) & lmask) {
            // src and/or dest do not align on word boundary
            if ((((long)d ^ (long)s) & lmask) || (count <= lsize))
                len = count;
            else
                len = ((long)d & lmask);

            count -= len;
            for (; len > 0; len--)
                *--d = *--s;
        }
        for (len = count / lsize; len > 0; len--) {
            d -= lsize;
            s -= lsize;
            *(word *)d = *(word *)s;
        }
        for (len = count & lmask; len > 0; len--)
            *--d = *--s;
    }

    return dest;
}

#endif

