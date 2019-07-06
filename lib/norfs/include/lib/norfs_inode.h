/*
 * Copyright (c) 2013 Heather Lee Wilson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __NORFS_INODE_H
#define __NORFS_INODE_H

#include <lk/list.h>
#include <stdint.h>

struct norfs_inode {
    struct list_node lnode;
    uint32_t location;
    uint32_t reference_count;
};

#endif
