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

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <debug.h>
#include <lib/fs/ext2.h>
#include "ext2_priv.h"

#define LOCAL_TRACE 0

int ext2_open_file(fscookie cookie, const char *path, fsfilecookie *fcookie)
{
	ext2_t *ext2 = (ext2_t *)cookie;
	int err;

	/* do a path lookup */
	inodenum_t inum;
	err = ext2_lookup(ext2, path, &inum);
	if (err < 0)
		return err;

	/* create the file object */
	ext2_file_t *file = malloc(sizeof(ext2_file_t));
	memset(file, 0, sizeof(ext2_file_t));

	/* read in the inode */
	err = ext2_load_inode(ext2, inum, &file->inode);
	if (err < 0) {
		free(file);
		return err;
	}

	file->ext2 = ext2;
	*fcookie = file;

	return 0;
}

int ext2_read_file(fsfilecookie fcookie, void *buf, off_t offset, size_t len)
{
	ext2_file_t *file = (ext2_file_t *)fcookie;
	int err;

	// test that it's a file
	if (!S_ISREG(file->inode.i_mode)) {
		dprintf(INFO, "ext2_read_file: not a file\n");
		return -1;
	}

	// read from the inode
	err = ext2_read_inode(file->ext2, &file->inode, buf, offset, len);

	return err;
}

int ext2_close_file(fsfilecookie fcookie)
{
	ext2_file_t *file = (ext2_file_t *)fcookie;

	// see if we need to free any of the cache blocks
	int i;
	for (i=0; i < 3; i++) {
		if (file->ind_cache[i].num != 0) {
			free(file->ind_cache[i].ptr);
		}
	}

	free(file);

	return 0;
}

off_t ext2_file_len(ext2_t *ext2, struct ext2_inode *inode)
{
	/* calculate the file size */
	off_t len = inode->i_size;
	if ((ext2->sb.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE) && (S_ISREG(inode->i_mode))) {
		/* can potentially be a large file */
		len |= (off_t)inode->i_size_high << 32;
	}

	return len;
}

int ext2_stat_file(fsfilecookie fcookie, struct file_stat *stat)
{
	ext2_file_t *file = (ext2_file_t *)fcookie;

	stat->size = ext2_file_len(file->ext2, &file->inode);

	/* is it a dir? */
	stat->is_dir = false;
	if (S_ISDIR(file->inode.i_mode))
		stat->is_dir = true;

	return 0;
}

int ext2_read_link(ext2_t *ext2, struct ext2_inode *inode, char *str, size_t len)
{
	LTRACEF("inode %p, str %p, len %zu\n", inode, str, len);

	off_t linklen = ext2_file_len(ext2, inode);

	if ((linklen < 0) || (linklen + 1 > len))
		return ERR_NO_MEMORY;

	if (linklen > 60) {
		int err = ext2_read_inode(ext2, inode, str, 0, linklen);
		if (err < 0)
			return err;
		str[linklen] = 0;
	} else {
		memcpy(str, &inode->i_block[0], linklen);
		str[linklen] = 0;
	}

	LTRACEF("read link '%s'\n", str);

	return linklen;
}

