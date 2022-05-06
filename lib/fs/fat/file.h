/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <inttypes.h>
#include "fat_fs.h"
#include "fat_priv.h"

class fat_fs;

class fat_file {
public:
    explicit fat_file(fat_fs *f);
    virtual ~fat_file();

    const dir_entry_location &dir_loc() const { return dir_loc_; }
    bool is_dir() const { return attributes_ == fat_attribute::directory; }

    // top level fs hooks
    static status_t open_file(fscookie *cookie, const char *path, filecookie **fcookie);
    static ssize_t read_file(filecookie *fcookie, void *_buf, const off_t offset, size_t len);
    static status_t stat_file(filecookie *fcookie, struct file_stat *stat);
    static status_t close_file(filecookie *fcookie);

    // used by fs node list maintenance
    // node in the fs's list of open files and dirs
    list_node node_ = LIST_INITIAL_CLEARED_VALUE;

private:
    // private versions of the above
    status_t open_file_priv(const dir_entry &entry, const dir_entry_location &loc);
    ssize_t read_file_priv(void *_buf, const off_t offset, size_t len);
    status_t stat_file_priv(struct file_stat *stat);
    status_t close_file_priv(bool *last_ref);

protected:
    // increment the ref and add/remove the file from the fs list
    void inc_ref();
    bool dec_ref(); // returns true when ref reaches zero

    // members
    int ref_ = 0;

    fat_fs *fs_ = nullptr; // pointer back to the fs instance we're in

    // pointer to our dir entry, acts as our unique key
    dir_entry_location dir_loc_ {};

    // our start cluster and length
    uint32_t start_cluster_ = 0;
    uint32_t length_ = 0;

    // saved attributes from our dir entry
    fat_attribute attributes_ = fat_attribute(0);
};

