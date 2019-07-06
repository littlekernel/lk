/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stddef.h>
#include <sys/types.h>

__BEGIN_CDECLS

typedef struct iovec {
    void *iov_base;
    size_t iov_len;
} iovec_t;

ssize_t iovec_size(const iovec_t *iov, uint iov_cnt);

ssize_t iovec_to_membuf(uint8_t *buf, uint buf_len,
                        const iovec_t *iov, uint iov_cnt, uint iov_pos);

__END_CDECLS

