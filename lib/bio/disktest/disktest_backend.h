//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#ifndef DISKTEST_BACKEND_H
#define DISKTEST_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

int dt_backend_open(const char *path, bool need_write, void **out_context);
int dt_backend_close(void *context);
int64_t dt_backend_read_block(void *context, uint64_t block_idx, uint64_t block_size, uint8_t *buf);
int64_t dt_backend_write_block(void *context, uint64_t block_idx, uint64_t block_size, const uint8_t *buf);
int dt_backend_get_target_size(void *context, uint64_t block_size, uint64_t *total_bytes, uint64_t *blocks);
int dt_backend_flush(void *context);
unsigned int dt_backend_seed(void);
void dt_backend_set_error(int err);
int dt_backend_get_error(void);
void dt_backend_perror(const char *msg);

#endif
