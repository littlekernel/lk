/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <err.h>
#include <debug.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include <iovec.h>

#define LOCAL_TRACE 0

/*
 *  Calc total size of iovec buffers
 */
ssize_t iovec_size (const iovec_t *iov, uint iov_cnt)
{
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
ssize_t iovec_to_membuf (uint8_t *buf, uint buf_len, const iovec_t *iov, uint iov_cnt, uint iov_pos)
{
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
