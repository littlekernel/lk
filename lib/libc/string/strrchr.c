/*
** Copyright 2001, Manuel J. Petit. All rights reserved.
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
strrchr(char const *s, int c) {
    char const *last= c?0:s;


    while (*s) {
        if (*s== c) {
            last= s;
        }

        s+= 1;
    }

    return (char *)last;
}
