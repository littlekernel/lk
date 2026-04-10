//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "disktest_backend.h"

#include <errno.h>
#include <inttypes.h>
#include <lib/bio.h>
#include <lk/err.h>
#include <platform/time.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct DtLkBackendContext {
    bdev_t *dev;
} DtLkBackendContext;

static int dt_last_error;

void dt_backend_set_error(int err) {
    dt_last_error = err;
}

int dt_backend_get_error(void) {
    return dt_last_error;
}

void dt_backend_perror(const char *msg) {
    fprintf(stderr, "%s: error %d\n", msg, dt_backend_get_error());
}

static int map_status_to_errno(int rc) {
    switch (rc) {
        case ERR_NOT_FOUND:
            return ENOENT;
        case ERR_INVALID_ARGS:
        case ERR_NOT_VALID:
            return EINVAL;
        case ERR_ALREADY_EXISTS:
            return EEXIST;
        case ERR_NOT_SUPPORTED:
            return ENOTSUP;
        case ERR_NO_MEMORY:
            return ENOMEM;
        default:
            return EIO;
    }
}

int dt_backend_open(const char *path, bool need_write, void **out_context) {
    if (out_context == NULL || path == NULL || path[0] == '\0') {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    bdev_t *const dev = bio_open(path);
    if (dev == NULL) {
        dt_backend_set_error(ENOENT);
        return -1;
    }

    if (need_write && dev->write == NULL && dev->write_block == NULL) {
        bio_close(dev);
        dt_backend_set_error(EROFS);
        return -1;
    }

    DtLkBackendContext *const ctx = malloc(sizeof(*ctx));
    if (ctx == NULL) {
        bio_close(dev);
        dt_backend_set_error(ENOMEM);
        return -1;
    }

    ctx->dev = dev;
    *out_context = ctx;
    return 0;
}

int dt_backend_close(void *context) {
    DtLkBackendContext *ctx = (DtLkBackendContext *)context;

    if (ctx == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    bio_close(ctx->dev);
    free(ctx);
    return 0;
}

int64_t dt_backend_read_blocks(void *context,
                               uint64_t block_idx,
                               uint64_t block_size,
                               uint64_t block_count,
                               uint8_t *buf) {
    DtLkBackendContext *ctx = (DtLkBackendContext *)context;

    if (ctx == NULL || buf == NULL || block_count == 0) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    if (block_size > (UINT64_MAX / block_count) || block_size * block_count > SIZE_MAX) {
        dt_backend_set_error(EOVERFLOW);
        return -1;
    }

    const size_t transfer_size = (size_t)(block_size * block_count);
    const off_t byte_offset = (off_t)(block_idx * block_size);
    const ssize_t rc = bio_read(ctx->dev, buf, byte_offset, transfer_size);
    if (rc < 0) {
        dt_backend_set_error(map_status_to_errno((int)rc));
        return -1;
    }

    return (int64_t)rc;
}

int64_t dt_backend_write_blocks(void *context,
                                uint64_t block_idx,
                                uint64_t block_size,
                                uint64_t block_count,
                                const uint8_t *buf) {
    DtLkBackendContext *ctx = (DtLkBackendContext *)context;

    if (ctx == NULL || buf == NULL || block_count == 0) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    if (block_size > (UINT64_MAX / block_count) || block_size * block_count > SIZE_MAX) {
        dt_backend_set_error(EOVERFLOW);
        return -1;
    }

    const size_t transfer_size = (size_t)(block_size * block_count);
    const off_t byte_offset = (off_t)(block_idx * block_size);
    const ssize_t rc = bio_write(ctx->dev, buf, byte_offset, transfer_size);
    if (rc < 0) {
        dt_backend_set_error(map_status_to_errno((int)rc));
        return -1;
    }

    return (int64_t)rc;
}

int dt_backend_get_target_size(void *context, uint64_t block_size, uint64_t *total_bytes, uint64_t *blocks) {
    DtLkBackendContext *ctx = (DtLkBackendContext *)context;

    if (ctx == NULL || total_bytes == NULL || blocks == NULL || block_size == 0) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    if ((uint64_t)ctx->dev->total_size == 0) {
        fprintf(stderr, "Target size is zero bytes\n");
        dt_backend_set_error(EINVAL);
        return -1;
    }

    if (((uint64_t)ctx->dev->total_size % block_size) != 0) {
        fprintf(stderr,
                "Target size (%" PRIu64 ") is not a multiple of block size (%" PRIu64 ")\n",
                (uint64_t)ctx->dev->total_size,
                block_size);
        dt_backend_set_error(EINVAL);
        return -1;
    }

    *total_bytes = (uint64_t)ctx->dev->total_size;
    *blocks = *total_bytes / block_size;
    return 0;
}

int dt_backend_flush(void *context) {
    DtLkBackendContext *ctx = (DtLkBackendContext *)context;

    if (ctx == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    (void)ctx;
    return 0;
}

unsigned int dt_backend_seed(void) {
    return (unsigned int)current_time_hires();
}
