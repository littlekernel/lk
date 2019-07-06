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
#include <ctype.h>
#include <sys/types.h>

int
strnicmp(char const *s1, char const *s2, size_t len) {
    unsigned char c1 = '\0';
    unsigned char c2 = '\0';

    if (len > 0) {
        do {
            c1 = *s1;
            c2 = *s2;
            s1++;
            s2++;
            if (!c1)
                break;
            if (!c2)
                break;
            if (c1 == c2)
                continue;
            c1 = tolower(c1);
            c2 = tolower(c2);
            if (c1 != c2)
                break;
        } while (--len);
    }
    return (int)c1 - (int)c2;
}
#pragma weak strncasecmp=strnicmp
