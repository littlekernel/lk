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
#include <ctype.h>
#include <string.h>

void *memscan(void *addr, int c, size_t size) {
    unsigned char *p = (unsigned char *)addr;

    while (size) {
        if (*p == c)
            return (void *)p;
        p++;
        size--;
    }
    return (void *)p;
}
