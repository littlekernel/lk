/*
 * Copyright (c) 2007 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include "ext2_priv.h"

#define LOCAL_TRACE 0

int ext2_open_file(fscookie *cookie, const char *path, filecookie **fcookie) {
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
    *fcookie = (filecookie *)file;

    return 0;
}

ssize_t ext2_read_file(filecookie *fcookie, void *buf, off_t offset, size_t len) {
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

int ext2_close_file(filecookie *fcookie) {
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

off_t ext2_file_len(ext2_t *ext2, struct ext2_inode *inode) {
    /* calculate the file size */
    off_t len = inode->i_size;
    if ((ext2->sb.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE) && (S_ISREG(inode->i_mode))) {
        /* can potentially be a large file */
        len |= (off_t)inode->i_size_high << 32;
    }

    return len;
}

int ext2_stat_file(filecookie *fcookie, struct file_stat *stat) {
    ext2_file_t *file = (ext2_file_t *)fcookie;

    stat->size = ext2_file_len(file->ext2, &file->inode);

    /* is it a dir? */
    stat->is_dir = false;
    if (S_ISDIR(file->inode.i_mode))
        stat->is_dir = true;

    return 0;
}

int ext2_read_link(ext2_t *ext2, struct ext2_inode *inode, char *str, size_t len) {
    LTRACEF("inode %p, str %p, len %zu\n", inode, str, len);

    off_t linklen = ext2_file_len(ext2, inode);

    if ((linklen < 0) || (linklen + 1 > (off_t)len))
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

