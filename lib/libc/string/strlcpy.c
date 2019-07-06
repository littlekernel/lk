/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
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

size_t
strlcpy(char *dst, char const *src, size_t s) {
    size_t i= 0;

    if (!s) {
        return strlen(src);
    }

    for (i= 0; ((i< s-1) && src[i]); i++) {
        dst[i]= src[i];
    }

    dst[i]= 0;

    return i + strlen(src+i);
}
