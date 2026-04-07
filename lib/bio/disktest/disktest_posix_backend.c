//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#define _XOPEN_SOURCE 700

#include "disktest_backend.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct DtPosixBackendContext {
    int fd;
} DtPosixBackendContext;

void dt_backend_set_error(int err) {
    errno = err;
}

int dt_backend_get_error(void) {
    return errno;
}

void dt_backend_perror(const char *msg) {
    perror(msg);
}

int dt_backend_open(const char *path, bool need_write, void **out_context) {
    int fd = open(path, need_write ? O_RDWR : O_RDONLY);

    if (out_context == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    if (fd < 0) {
        return -1;
    }

    DtPosixBackendContext *ctx = malloc(sizeof(*ctx));
    if (ctx == NULL) {
        close(fd);
        errno = ENOMEM;
        return -1;
    }

    ctx->fd = fd;
    *out_context = ctx;
    return 0;
}

int dt_backend_close(void *context) {
    DtPosixBackendContext *ctx = (DtPosixBackendContext *)context;

    if (ctx == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    int rc = close(ctx->fd);
    free(ctx);
    return rc;
}

int64_t dt_backend_read_block(void *context, uint64_t block_idx, uint64_t block_size, uint8_t *buf) {
    DtPosixBackendContext *ctx = (DtPosixBackendContext *)context;
    off_t byte_offset = (off_t)(block_idx * block_size);

    if (ctx == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    return (int64_t)pread(ctx->fd, buf, (size_t)block_size, byte_offset);
}

int64_t dt_backend_write_block(void *context, uint64_t block_idx, uint64_t block_size, const uint8_t *buf) {
    DtPosixBackendContext *ctx = (DtPosixBackendContext *)context;
    off_t byte_offset = (off_t)(block_idx * block_size);

    if (ctx == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    return (int64_t)pwrite(ctx->fd, buf, (size_t)block_size, byte_offset);
}

int dt_backend_get_target_size(void *context, uint64_t block_size, uint64_t *total_bytes, uint64_t *blocks) {
    DtPosixBackendContext *ctx = (DtPosixBackendContext *)context;
    off_t end;

    if (ctx == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    end = lseek(ctx->fd, 0, SEEK_END);
    if (end < 0) {
        dt_backend_perror("lseek(SEEK_END)");
        return -1;
    }

    if (lseek(ctx->fd, 0, SEEK_SET) < 0) {
        dt_backend_perror("lseek(SEEK_SET)");
        return -1;
    }

    if ((uint64_t)end == 0) {
        fprintf(stderr, "Target size is zero bytes\n");
        return -1;
    }

    if (((uint64_t)end % block_size) != 0) {
        fprintf(stderr,
                "Target size (%" PRIu64 ") is not a multiple of block size (%" PRIu64 ")\n",
                (uint64_t)end,
                block_size);
        return -1;
    }

    *total_bytes = (uint64_t)end;
    *blocks = (uint64_t)end / block_size;
    return 0;
}

int dt_backend_flush(void *context) {
    DtPosixBackendContext *ctx = (DtPosixBackendContext *)context;

    if (ctx == NULL) {
        dt_backend_set_error(EINVAL);
        return -1;
    }

    return fsync(ctx->fd);
}

unsigned int dt_backend_seed(void) {
    return (unsigned int)(time(NULL) ^ (unsigned long)getpid());
}
