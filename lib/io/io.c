/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/io.h>

#include <lk/err.h>
#include <ctype.h>
#include <lk/debug.h>
#include <assert.h>

ssize_t io_write(io_handle_t *io, const char *buf, size_t len) {
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (!io->hooks->write)
        return ERR_NOT_SUPPORTED;

    return io->hooks->write(io, buf, len);
}

ssize_t io_read(io_handle_t *io, char *buf, size_t len) {
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (!io->hooks->read)
        return ERR_NOT_SUPPORTED;

    return io->hooks->read(io, buf, len);
}

