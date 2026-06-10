/*
 * Copyright (c) 2014 Chris Anderson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "minip-internal.h"

uint16_t ones_sum16(uint32_t sum, const void *_buf, int len) {
    const uint16_t *buf = _buf;

    while (len >= 2) {
        sum += *buf++;
        if (sum & 0x80000000)
            sum = (sum & 0xffff) + (sum >> 16);
        len -= 2;
    }

    if (len) {
        uint16_t temp = htons((*(uint8_t *)buf) << 8);
        sum += temp;
    }

    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return sum;
}
