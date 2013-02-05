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

#ifndef __EXT2_PRIV_H
#define __EXT2_PRIV_H

#include <lib/bio.h>
#include <lib/bcache.h>
#include "ext2_fs.h"

typedef uint32_t blocknum_t;
typedef uint32_t inodenum_t;
typedef uint32_t groupnum_t;

typedef struct {
	bdev_t *dev;
	bcache_t cache;

	struct ext2_super_block sb;
	int s_group_count;
	struct ext2_group_desc *gd;
	struct ext2_inode root_inode;
} ext2_t;

struct cache_block {
	blocknum_t num;
	void *ptr;
};

/* open file handle */
typedef struct {
	ext2_t *ext2;

	struct cache_block ind_cache[3]; // cache of indirect blocks as they're scanned
	struct ext2_inode inode;
} ext2_file_t;

/* internal routines */
int ext2_load_inode(ext2_t *ext2, inodenum_t num, struct ext2_inode *inode);
int ext2_lookup(ext2_t *ext2, const char *path, inodenum_t *inum); // path to inode

/* io */
int ext2_read_block(ext2_t *ext2, void *buf, blocknum_t bnum);
int ext2_get_block(ext2_t *ext2, void **ptr, blocknum_t bnum);
int ext2_put_block(ext2_t *ext2, blocknum_t bnum);

off_t ext2_file_len(ext2_t *ext2, struct ext2_inode *inode);
int ext2_read_inode(ext2_t *ext2, struct ext2_inode *inode, void *buf, off_t offset, size_t len);
int ext2_read_link(ext2_t *ext2, struct ext2_inode *inode, char *str, size_t len);

/* mode stuff */
#define S_IFMT      0170000
#define S_IFIFO     0010000
#define S_IFCHR     0020000
#define S_IFDIR     0040000
#define S_IFBLK     0060000
#define S_IFREG     0100000
#define S_IFLNK     0120000
#define S_IFSOCK    0140000

#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)

#endif

