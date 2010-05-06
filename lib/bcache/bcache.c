/*
 * Copyright (c) 2007 Travis Geiselbrecht
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
#include <list.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <debug.h>
#include <lib/bcache.h>
#include <lib/bio.h>

#define LOCAL_TRACE 0

struct bcache_block {
	struct list_node node;
	bnum_t blocknum;
	int ref_count;
	bool is_dirty;
	void *ptr;
};

struct bcache_stats {
	uint32_t hits;
	uint32_t depth;
	uint32_t misses;
	uint32_t reads;
	uint32_t writes;
};

struct bcache {
	bdev_t *dev;
	size_t block_size;
	int count;
	struct bcache_stats stats;

	struct list_node free_list;
	struct list_node lru_list;

	struct bcache_block *blocks;
};

bcache_t bcache_create(bdev_t *dev, size_t block_size, int block_count)
{
	struct bcache *cache;

	cache = malloc(sizeof(struct bcache));
	
	cache->dev = dev;
	cache->block_size = block_size;
	cache->count = block_count;
	memset(&cache->stats, 0, sizeof(cache->stats));

	list_initialize(&cache->free_list);
	list_initialize(&cache->lru_list);

	cache->blocks = malloc(sizeof(struct bcache_block) * block_count);
	int i;
	for (i=0; i < block_count; i++) {
		cache->blocks[i].ref_count = 0;
		cache->blocks[i].is_dirty = false;
		cache->blocks[i].ptr = malloc(block_size);
		// add to the free list
		list_add_head(&cache->free_list, &cache->blocks[i].node);	
	}

	return (bcache_t)cache;
}

static int flush_block(struct bcache *cache, struct bcache_block *block)
{
	int rc;

	rc = bio_write(cache->dev, block->ptr,
			(off_t)block->blocknum * cache->block_size,
			cache->block_size);
	if (rc < 0)
		goto exit;

	block->is_dirty = false;
	cache->stats.writes++;
	rc = 0;
exit:
	return (rc);
}

void bcache_destroy(bcache_t _cache)
{
	struct bcache *cache = _cache;
	int i;

	for (i=0; i < cache->count; i++) {
		DEBUG_ASSERT(cache->blocks[i].ref_count == 0);

		if (cache->blocks[i].is_dirty)
			printf("warning: freeing dirty block %u\n",
				cache->blocks[i].blocknum);

		free(cache->blocks[i].ptr);
	}

	free(cache);
}

/* find a block if it's already present */
static struct bcache_block *find_block(struct bcache *cache, uint blocknum)
{
	uint32_t depth = 0;
	struct bcache_block *block;

	LTRACEF("num %u\n", blocknum);

	block = NULL;
	list_for_every_entry(&cache->lru_list, block, struct bcache_block, node) {
		LTRACEF("looking at entry %p, num %u\n", block, block->blocknum);
		depth++;

		if (block->blocknum == blocknum) {
			list_delete(&block->node);
			list_add_tail(&cache->lru_list, &block->node);
			cache->stats.hits++;
			cache->stats.depth += depth;
			return block;
		}
	}

	cache->stats.misses++;
	return NULL;
}

/* allocate a new block */
static struct bcache_block *alloc_block(struct bcache *cache)
{
	int err;
	struct bcache_block *block;

	/* pop one off the free list if it's present */
	block = list_remove_head_type(&cache->free_list, struct bcache_block, node);
	if (block) {
		block->ref_count = 0;
		list_add_tail(&cache->lru_list, &block->node);
		LTRACEF("found block %p on free list\n", block);
		return block;
	}

	/* walk the lru, looking for a free block */
	list_for_every_entry(&cache->lru_list, block, struct bcache_block, node) {
		LTRACEF("looking at %p, num %u\n", block, block->blocknum);
		if (block->ref_count == 0) {
			if (block->is_dirty) {
				err = flush_block(cache, block);
				if (err)
					return NULL;
			}

			// add it to the tail of the lru
			list_delete(&block->node);
			list_add_tail(&cache->lru_list, &block->node);
			return block;
		}
	}

	return NULL;
}

static struct bcache_block *find_or_fill_block(struct bcache *cache, uint blocknum)
{
	int err;

	LTRACEF("block %u\n", blocknum);

	/* see if it's already in the cache */
	struct bcache_block *block = find_block(cache, blocknum);
	if (block == NULL) {
		LTRACEF("wasn't allocated\n");

		/* allocate a new block and fill it */
		block = alloc_block(cache);
		DEBUG_ASSERT(block);

		LTRACEF("wasn't allocated, new block %p\n", block);

		block->blocknum = blocknum;
		err = bio_read(cache->dev, block->ptr, (off_t)blocknum * cache->block_size, cache->block_size);
		if (err < 0) {
			/* free the block, return an error */
			list_add_tail(&cache->free_list, &block->node);
			return NULL;
		}

		cache->stats.reads++;
	}

	DEBUG_ASSERT(block->blocknum == blocknum);

	return block;
}

int bcache_read_block(bcache_t _cache, void *buf, uint blocknum)
{
	struct bcache *cache = _cache;

	LTRACEF("buf %p, blocknum %u\n", buf, blocknum);

	struct bcache_block *block = find_or_fill_block(cache, blocknum);
	if (block == NULL) {
		/* error */
		return -1;
	}

	memcpy(buf, block->ptr, cache->block_size);
	return 0;
}

int bcache_get_block(bcache_t _cache, void **ptr, uint blocknum)
{
	struct bcache *cache = _cache;

	LTRACEF("ptr %p, blocknum %u\n", ptr, blocknum);

	DEBUG_ASSERT(ptr);

	struct bcache_block *block = find_or_fill_block(cache, blocknum);
	if (block == NULL) {
		/* error */
		return -1;
	}

	/* increment the ref count to keep it from being freed */
	block->ref_count++;
	*ptr = block->ptr;

	return 0;
}

int bcache_put_block(bcache_t _cache, uint blocknum)
{
	struct bcache *cache = _cache;

	LTRACEF("blocknum %u\n", blocknum);

	struct bcache_block *block = find_block(cache, blocknum);

	/* be pretty hard on the caller for now */
	DEBUG_ASSERT(block);
	DEBUG_ASSERT(block->ref_count > 0);

	block->ref_count--;

	return 0;
}

int bcache_mark_block_dirty(bcache_t priv, uint blocknum)
{
	int err;
	struct bcache *cache = priv;
	struct bcache_block *block;

	block = find_block(cache, blocknum);
	if (!block) {
		err = -1;
		goto exit;
	}

	block->is_dirty = true;
	err = 0;
exit:
	return (err);
}

int bcache_zero_block(bcache_t priv, uint blocknum)
{
	int err;
	struct bcache *cache = priv;
	struct bcache_block *block;

	block = find_block(cache, blocknum);
	if (!block) {
		block = alloc_block(cache);
		if (!block) {
			err = -1;
			goto exit;
		}

		block->blocknum = blocknum;
	}

	memset(block->ptr, 0, cache->block_size);
	block->is_dirty = true;
	err = 0;
exit:
	return (err);
}

int bcache_flush(bcache_t priv)
{
	int err;
	struct bcache *cache = priv;
	struct bcache_block *block;

	list_for_every_entry(&cache->lru_list, block, struct bcache_block, node) {
		if (block->is_dirty) {
			err = flush_block(cache, block);
			if (err)
				goto exit;
		}
	}

	err = 0;
exit:
	return (err);
}

void bcache_dump(bcache_t priv, const char *name)
{
	uint32_t finds;
	struct bcache *cache = priv;

	finds = cache->stats.hits + cache->stats.misses;

	printf("%s: hits=%u(%u%%) depth=%u misses=%u(%u%%) reads=%u writes=%u\n",
		name,
		cache->stats.hits,
		finds ? (cache->stats.hits * 100) / finds : 0,
		cache->stats.hits ? cache->stats.depth / cache->stats.hits : 0,
		cache->stats.misses,
		finds ? (cache->stats.misses * 100) / finds : 0,
		cache->stats.reads,
		cache->stats.writes);
}
