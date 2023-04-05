/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
 * Copyright 2022 Google LLC
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

int strcasecmp(char const *cs, char const *ct) {
    signed char __res;

    while (1) {
        if ((__res = tolower(*cs) - tolower(*ct)) != 0 || !*cs)
            break;
        cs++;
        ct++;
    }

    return __res;
}
