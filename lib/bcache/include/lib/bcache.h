/*
 * Copyright (c) 2007 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/bio.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

typedef void *bcache_t;

bcache_t bcache_create(bdev_t *dev, size_t block_size, int block_count);
void bcache_destroy(bcache_t);

int bcache_read_block(bcache_t, void *, uint block);

// get and put a pointer directly to the block
int bcache_get_block(bcache_t, void **, uint block);
int bcache_put_block(bcache_t, uint block);
int bcache_mark_block_dirty(bcache_t priv, uint blocknum);
int bcache_zero_block(bcache_t priv, uint blocknum);
int bcache_flush(bcache_t priv);
void bcache_dump(bcache_t priv, const char *name);

__END_CDECLS
