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

size_t
strnlen(char const *s, size_t count) {
    const char *sc;

    for (sc = s; count-- && *sc != '\0'; ++sc)
        ;
    return sc - s;
}
