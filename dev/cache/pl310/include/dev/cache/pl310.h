/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <stdbool.h>

status_t pl310_set_enable(bool enable);

/* operations */
void pl310_invalidate(void);
void pl310_flush_invalidate(void);
void pl310_sync_range(void);
void pl310_clean_range(addr_t start, size_t len);
void pl310_clean_invalidate_range(addr_t start, size_t len);
void pl310_invalidate_range(addr_t start, size_t len);

void pl310_pin_cache_range(addr_t start, size_t len);
