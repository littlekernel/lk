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

char *
strncat(char *dest, char const *src, size_t count) {
    char *tmp = dest;

    if (count > 0) {
        while (*dest)
            dest++;
        while ((*dest++ = *src++)) {
            if (--count == 0) {
                *dest = '\0';
                break;
            }
        }
    }

    return tmp;
}

