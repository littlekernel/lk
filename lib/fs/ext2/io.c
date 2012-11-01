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

#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <lib/fs/ext2.h>
#include "ext2_priv.h"

#define LOCAL_TRACE 0

int ext2_read_block(ext2_t *ext2, void *buf, blocknum_t bnum)
{
	return bcache_read_block(ext2->cache, buf, bnum);
}

int ext2_get_block(ext2_t *ext2, void **ptr, blocknum_t bnum)
{
	return bcache_get_block(ext2->cache, ptr, bnum);
}

int ext2_put_block(ext2_t *ext2, blocknum_t bnum)
{
	return bcache_put_block(ext2->cache, bnum);
}

static int ext2_calculate_block_pointer_pos(ext2_t *ext2, blocknum_t block_to_find, uint32_t *level, uint32_t pos[])
{
	uint32_t block_ptr_per_block, block_ptr_per_2nd_block;

	// XXX optimize this

	// See if it's in the direct blocks
	if (block_to_find < EXT2_NDIR_BLOCKS) {
		*level = 0;
		pos[0] = block_to_find;
		return 0;
	}

	block_ptr_per_block = EXT2_ADDR_PER_BLOCK(ext2->sb);
	block_to_find -= EXT2_NDIR_BLOCKS;
	// See if it's in the first indirect block
	if (block_to_find < block_ptr_per_block) {
		*level = 1;
		pos[0] = EXT2_IND_BLOCK;
		pos[1] = block_to_find;
		return 0;
	}

	block_to_find -= block_ptr_per_block;
	block_ptr_per_2nd_block = block_ptr_per_block * block_ptr_per_block;
	// See if it's in the second indirect block
	if (block_to_find < (block_ptr_per_2nd_block)) {
		*level = 2;
		pos[0] = EXT2_DIND_BLOCK;
		pos[1] = block_to_find / block_ptr_per_block;
		pos[2] = block_to_find % block_ptr_per_block;
		return 0;
	}

	block_to_find -= block_ptr_per_2nd_block;
	// See if it's in the third indirect block
	if (block_to_find < (block_ptr_per_2nd_block * block_ptr_per_block)) {
		*level = 3;
		pos[0] = EXT2_TIND_BLOCK;
		pos[1] = block_to_find / block_ptr_per_2nd_block;
		pos[2] = (block_to_find % block_ptr_per_2nd_block) / block_ptr_per_block;
		pos[3] = (block_to_find % block_ptr_per_2nd_block) % block_ptr_per_block;
		return 0;
	}

	// The block requested must be too big.
	return -1;
}

// This function returns a pointer to the cache block that corresponds to the indirect block pointer.
int ext2_get_indirect_block_pointer_cache_block(ext2_t *ext2, struct ext2_inode *inode, blocknum_t **cache_block, uint32_t level, uint32_t pos[], uint *block_loaded)
{
	uint32_t current_level = 0;
	uint current_block = 0, last_block;
	blocknum_t *block = NULL;
	int err;

	if ((level > 3) || (level == 0)) {
		err = -1;
		goto error;
	}

	// Dig down into the indirect blocks. When done, current_block should point to the target.
	while (current_level < level) {
		if (current_level == 0) {
			// read the direct block, simulates a prior loop
			current_block = LE32(inode->i_block[pos[0]]);
		}

		if (current_block == 0) {
			err = -1;
			goto error;
		}

		last_block = current_block;
		current_level++;
		*block_loaded = current_block;

		err = ext2_get_block(ext2, (void **)(void *)&block, current_block);
		if (err < 0) {
			goto error;
		}

		if (current_level < level) {
			current_block = LE32(block[pos[current_level]]);
			ext2_put_block(ext2, last_block);
		}
	}

	*cache_block = block;
	return 0;

error:
	*cache_block = NULL;
	*block_loaded = 0;
	return err;
}

/* translate a file block to a physical block */
static blocknum_t file_block_to_fs_block(ext2_t *ext2, struct ext2_inode *inode, uint fileblock)
{
	int err;
	blocknum_t block;

	LTRACEF("inode %p, fileblock %u\n", inode, fileblock);

	uint32_t pos[4];
	uint32_t level = 0;
	ext2_calculate_block_pointer_pos(ext2, fileblock, &level, pos);

	LTRACEF("level %d, pos 0x%x 0x%x 0x%x 0x%x\n", level, pos[0], pos[1], pos[2], pos[3]);

	if (level == 0) {
		/* direct block, just return it directly */
		block = LE32(inode->i_block[fileblock]);
	} else {
		/* at least one level of indirection, get a pointer to the final indirect block table and dereference it */
		blocknum_t *ind_table;
		blocknum_t phys_block;
		err = ext2_get_indirect_block_pointer_cache_block(ext2, inode, &ind_table, level, pos, &phys_block);
		if (err < 0)
			return 0;

		/* dereference the final entry in the final table */
		block = LE32(ind_table[pos[level]]);
		LTRACEF("block %u, indirect_block %u\n", block, phys_block);

		/* release the ref on the cache block */
		ext2_put_block(ext2, phys_block);
	}

	LTRACEF("returning %u\n", block);

	return block;
}

int ext2_read_inode(ext2_t *ext2, struct ext2_inode *inode, void *_buf, off_t offset, size_t len)
{
	int err = 0;
	int bytes_read = 0;
	uint8_t *buf = _buf;

	/* calculate the file size */
	off_t file_size = ext2_file_len(ext2, inode);

	LTRACEF("inode %p, offset %lld, len %zd, file_size %lld\n", inode, offset, len, file_size);

	/* trim the read */
	if (offset > file_size)
		return 0;
	if (offset + len >= file_size)
		len = file_size - offset;
	if (len == 0)
		return 0;

	/* calculate the starting file block */
	uint file_block = offset / EXT2_BLOCK_SIZE(ext2->sb);

	/* handle partial first block */
	if ((offset % EXT2_BLOCK_SIZE(ext2->sb)) != 0) {
		uint8_t temp[EXT2_BLOCK_SIZE(ext2->sb)];

		/* calculate the block and read it */
		blocknum_t phys_block = file_block_to_fs_block(ext2, inode, file_block);
		if (phys_block == 0) {
			memset(temp, 0, EXT2_BLOCK_SIZE(ext2->sb));
		} else {
			ext2_read_block(ext2, temp, phys_block);
		}

		/* copy out what we need */
		size_t block_offset = offset % EXT2_BLOCK_SIZE(ext2->sb);
		size_t tocopy = MIN(len, EXT2_BLOCK_SIZE(ext2->sb) - block_offset);
		memcpy(buf, temp + block_offset, tocopy);

		/* increment our stuff */
		file_block++;
		len -= tocopy;
		bytes_read += tocopy;
		buf += tocopy;
	}

	/* handle middle blocks */
	while (len >= EXT2_BLOCK_SIZE(ext2->sb)) {
		/* calculate the block and read it */
		blocknum_t phys_block = file_block_to_fs_block(ext2, inode, file_block);
		if (phys_block == 0) {
			memset(buf, 0, EXT2_BLOCK_SIZE(ext2->sb));
		} else {
			ext2_read_block(ext2, buf, phys_block);
		}

		/* increment our stuff */
		file_block++;
		len -= EXT2_BLOCK_SIZE(ext2->sb);
		bytes_read += EXT2_BLOCK_SIZE(ext2->sb);
		buf += EXT2_BLOCK_SIZE(ext2->sb);
	}

	/* handle partial last block */
	if (len > 0) {
		uint8_t temp[EXT2_BLOCK_SIZE(ext2->sb)];

		/* calculate the block and read it */
		blocknum_t phys_block = file_block_to_fs_block(ext2, inode, file_block);
		if (phys_block == 0) {
			memset(temp, 0, EXT2_BLOCK_SIZE(ext2->sb));
		} else {
			ext2_read_block(ext2, temp, phys_block);
		}

		/* copy out what we need */
		memcpy(buf, temp, len);

		/* increment our stuff */
		bytes_read += len;
	}

	LTRACEF("err %d, bytes_read %d\n", err, bytes_read);

	return (err < 0) ? err : bytes_read;
}

