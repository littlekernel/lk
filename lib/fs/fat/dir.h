/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <inttypes.h>
#include <lk/list.h>

#include "file.h"

class fat_fs;
struct dir_entry;
struct dir_entry_location;
struct fat_dir_cookie;

// Convert UTF-8 to UCS-2 for FAT LFN handling.
// Returns NO_ERROR on success, ERR_INVALID_ARGS for malformed/unrepresentable UTF-8,
// and ERR_TOO_BIG if max_ucs2_len is insufficient.
status_t fat_utf8_to_ucs2(const char *utf8, uint16_t *ucs2, size_t max_ucs2_len,
                          size_t *out_ucs2_len);

// structure that represents an open dir, may have multiple cookies in its list
// at any point in time,
class fat_dir : public fat_file {
  public:
    explicit fat_dir(fat_fs *f);
    virtual ~fat_dir();

    static status_t remove(fscookie *cookie, const char *path);
    static status_t rmdir(fscookie *cookie, const char *path);
    static status_t mkdir(fscookie *cookie, const char *path);
    static status_t opendir(fscookie *cookie, const char *name, dircookie **dcookie);
    static status_t readdir(dircookie *dcookie, struct dirent *ent);
    static status_t closedir(dircookie *dcookie);

  private:
    status_t opendir_priv(const dir_entry &entry, const dir_entry_location &loc, fat_dir_cookie **out_cookie);
    status_t readdir_priv(fat_dir_cookie *cookie, struct dirent *ent);
    status_t closedir_priv(fat_dir_cookie *cookie, bool *last_ref);

    // list of all open dir handles and their offsets within us
    list_node cookies_ = LIST_INITIAL_VALUE(cookies_);
};
