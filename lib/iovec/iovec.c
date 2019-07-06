/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include <iovec.h>

#define LOCAL_TRACE 0

/*
 *  Calc total size of iovec buffers
 */
ssize_t iovec_size (const iovec_t *iov, uint iov_cnt) {
    if (!iov)
        return (ssize_t) ERR_INVALID_ARGS;

    size_t c = 0;
    for (uint i = 0; i < iov_cnt; i++, iov++) {
        c += iov->iov_len;
    }
    return (ssize_t) c;
}

/*
 *  Copy out portion of iovec started from given position
 *  into single buffer
 */
ssize_t iovec_to_membuf (uint8_t *buf, uint buf_len, const iovec_t *iov, uint iov_cnt, uint iov_pos) {
    uint buf_pos = 0;

    if (!buf || !iov)
        return (ssize_t) ERR_INVALID_ARGS;

    /* for all iovec */
    for (uint i = 0; i < iov_cnt; i++, iov++) {

        if  (iov_pos >= iov->iov_len) {
            iov_pos -= iov->iov_len; /* skip whole chunks */
            continue;
        }

        /* calc number of bytes left in current iov */
        size_t to_copy = (size_t) (iov->iov_len - iov_pos);

        /* limit it to number of bytes left in buffer */
        if (to_copy > buf_len)
            to_copy = buf_len;

        /* copy data out */
        memcpy (buf + buf_pos, (uint8_t *)iov->iov_base + iov_pos, to_copy);

        /* advance in buffer position */
        buf_pos += to_copy;
        buf_len -= to_copy;

        /* check if we need to copy more data */
        if (buf_len == 0)
            break;

        iov_pos  = 0; /* it is only possible to have fully copied iovec here */
    }

    return (ssize_t) buf_pos;
}
