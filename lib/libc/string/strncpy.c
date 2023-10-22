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
strncpy(char *dest, char const *src, size_t count) {
    char *tmp = dest;

    size_t i;
    for (i = 0; i++ < count && (*dest++ = *src++) != '\0'; )
        ;
    for (; i < count; i++)
        *dest++ = '\0';

    return tmp;
}

