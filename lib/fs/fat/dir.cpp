/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "dir.h"

#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <ctype.h>
#include <endian.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fat_fs.h"
#include "fat_priv.h"
#include "file_iterator.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

fat_dir::fat_dir(fat_fs *f) : fat_file(f) {}

fat_dir::~fat_dir() = default;

// structure that represents an open dir handle. holds the offset into the directory
// that is being walked.
struct fat_dir_cookie {
    fat_dir *dir;

    struct list_node node;

    // next directory index offset in bytes, 0xffffffff for EOD
    uint32_t index;
    static const uint32_t index_eod = 0xffffffff;
};

// walk one entry into the dir, starting at byte offset into the directory block iterator.
// both dbi and offset will be modified during the call.
// filles out the entry and returns a pointer into the passed in buffer in out_filename.
// NOTE: *must* pass at least a MAX_FILE_NAME_LEN byte char pointer in the filename_buffer slot.
static status_t fat_find_next_entry(fat_fs *fat, file_block_iterator &dbi, uint32_t &offset, dir_entry *entry,
        char filename_buffer[MAX_FILE_NAME_LEN], char **out_filename) {

    DEBUG_ASSERT(entry && filename_buffer && out_filename);
    DEBUG_ASSERT(offset <= fat->info().bytes_per_sector); // passing offset == bytes_per_sector is okay

    // lfn parsing state
    struct lfn_parse_state {
        size_t  pos = 0;
        uint8_t last_sequence = 0xff; // 0xff means we haven't seen anything
                                      // since last reset
        uint8_t checksum = 0;

        void reset() {
            last_sequence = 0xff;
        }
    } lfn_state;

    for (;;) {
        if (LOCAL_TRACE >= 2) {
            LTRACEF("dir sector:\n");
            hexdump8_ex(dbi.get_bcache_ptr(0), fat->info().bytes_per_sector, 0);
        }

        // walk within a sector
        while (offset < fat->info().bytes_per_sector) {
            LTRACEF_LEVEL(2, "looking at offset %#x\n", offset);
            const uint8_t *ent = dbi.get_bcache_ptr(offset);
            if (ent[0] == 0) { // no more entries
                // we're completely done
                LTRACEF("completely done\n");
                return ERR_NOT_FOUND;
            } else if (ent[0] == 0xE5) { // deleted entry
                LTRACEF("deleted entry\n");
                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if (ent[0x0B] == (uint8_t)fat_attribute::volume_id) {
                // skip volume ids
                LTRACEF("skipping volume id\n");
                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if (ent[0x0B] == (uint8_t)fat_attribute::lfn) {
                // part of a LFN sequence
                uint8_t sequence = ent[0] & ~0x40;
                if (ent[0] & 0x40) {
                    // end sequence, start a new backwards fill of the lfn name
                    lfn_state.pos = MAX_FILE_NAME_LEN;
                    filename_buffer[--lfn_state.pos] = 0;
                    lfn_state.last_sequence = sequence;
                    lfn_state.checksum = ent[0x0d];
                    LTRACEF_LEVEL(2, "start of new LFN entry, sequence %u\n", sequence);
                } else {
                    if (lfn_state.last_sequence != sequence + 1) {
                        // our entry is out of sequence? drop it and start over
                        LTRACEF("ent out of sequence %u (last sequence %u)\n", sequence, lfn_state.last_sequence);
                        lfn_state.reset();
                        offset += DIR_ENTRY_LENGTH;
                        continue;
                    }
                    if (lfn_state.checksum != ent[0x0d]) {
                        // all of the long sequences need to match the checksum
                        LTRACEF("ent mismatches previous checksum\n");
                        lfn_state.reset();
                        offset += DIR_ENTRY_LENGTH;
                        continue;
                    }
                    lfn_state.last_sequence = sequence;
                }

                // walk backwards through the entry, picking out unicode characters
                // table of unicode character offsets:
                const size_t table[] = { 30, 28, 24, 22, 20, 18, 16, 14, 9, 7, 5, 3, 1 };
                for (auto off : table) {
                    uint16_t c = fat_read16(ent, off);
                    if (c != 0xffff && c != 0x0) {
                        // TODO: properly deal with unicode -> utf8
                        filename_buffer[--lfn_state.pos] = c & 0xff;
                    }
                }

                LTRACEF_LEVEL(2, "lfn filename thus far: '%s' sequence %hhu\n", filename_buffer + lfn_state.pos, sequence);

                // iterate one more entry, since we need to at least need to find the corresponding SFN
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else {
                // regular entry, extract the short file name
                char short_filename[8 + 1 + 3 + 1]; // max short name (8 . 3 NULL)
                size_t fname_pos = 0;

                // Ignore trailing spaces in filename and/or extension
                int fn_len = 8, ext_len = 3;
                for (int i = 7; i >= 0; i--) {
                    if (ent[i] == 0x20) {
                        fn_len--;
                    } else {
                        break;
                    }
                }
                for (size_t i = 10; i >= 8; i--) {
                    if (ent[i] == 0x20) {
                        ext_len--;
                    } else {
                        break;
                    }
                }

                for (int i = 0; i < fn_len; i++) {
                    short_filename[fname_pos++] = ent[i];
                }
                if (ext_len > 0) {
                    short_filename[fname_pos++] = '.';
                    for (int i=0; i < ext_len; i++) {
                        short_filename[fname_pos++] = ent[8 + i];
                    }
                }
                short_filename[fname_pos++] = 0;
                DEBUG_ASSERT(fname_pos <= sizeof(short_filename));

                // now we have the SFN, see if we just got finished parsing a corresponding LFN
                // in the previous entries
                if (lfn_state.last_sequence == 1) {
                    // TODO: compute checksum and make sure it matches
                    *out_filename = filename_buffer + lfn_state.pos;
                } else {
                    // copy the parsed short file name into the out buffer
                    strlcpy(filename_buffer, short_filename, sizeof(short_filename));
                    *out_filename = filename_buffer;
                }

                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;

                // fall through, we've found a file entry
            }

            LTRACEF("found filename '%s'\n", *out_filename);

            // fill out the passed in dir entry and exit
            uint16_t target_cluster = fat_read16(ent, 0x1a);
            entry->length = fat_read32(ent, 0x1c);
            entry->attributes = (fat_attribute)ent[0x0B];
            entry->start_cluster = target_cluster;
            return NO_ERROR;
        }

        DEBUG_ASSERT(offset <= fat->info().bytes_per_sector);

        // move to the next sector
        status_t err = dbi.next_sector();
        if (err < 0) {
            break;
        }
        // starting over at offset 0 in the new sector
        offset = 0;
    }

    // we're out of entries
    return ERR_NOT_FOUND;
}

static status_t fat_find_file_in_dir(fat_fs *fat, uint32_t starting_cluster, const char *name, dir_entry *entry, uint32_t *found_offset) {
    LTRACEF("start_cluster %u, name '%s', out entry %p\n", starting_cluster, name, entry);

    DEBUG_ASSERT(fat->lock.is_held());
    DEBUG_ASSERT(entry);

    // cache the length of the string we're matching against
    const size_t namelen = strlen(name);

    // kick start our directory sector iterator
    file_block_iterator dbi(fat, starting_cluster);
    status_t err = dbi.next_sectors(0);
    if (err < 0) {
        return err;
    }

    uint32_t offset = 0;
    for (;;) {
        char filename_buffer[MAX_FILE_NAME_LEN]; // max fat file name length
        char *filename;

        // step forward one entry and see if we got something
        err = fat_find_next_entry(fat, dbi, offset, entry, filename_buffer, &filename);
        if (err < 0) {
            return err;
        }

        const size_t filenamelen = strlen(filename);

        // see if we've matched an entry
        if (filenamelen == namelen && !strnicmp(name, filename, filenamelen)) {
            // we have, return with a good status
            if (found_offset) {
                *found_offset = offset;
            }
            return NO_ERROR;
        }
    }
}

status_t fat_dir_walk(fat_fs *fat, const char *path, dir_entry *out_entry, dir_entry_location *loc) {
    LTRACEF("path %s\n", path);

    DEBUG_ASSERT(fat->lock.is_held());

    // routine to push the path element ahead one bump
    // will leave path pointing at the next element, and path_element_size
    // in characters for the next element (or 0 if finished).
    size_t path_element_size = 0;
    auto path_increment = [&path, &path_element_size]() {
        path += path_element_size;
        path_element_size = 0;

        // we're at the end of the string
        if (*path == 0) {
            return;
        }

        // push path past the next /
        while (*path == '/' && path != 0) {
            path++;
        }

        // count the size of the next element
        const char *ptr = path;
        while (*ptr != '/' && *ptr != 0) {
            ptr++;
            path_element_size++;
        }
    };

    // increment it once past leading / and establish an initial path_element_size
    path_increment();
    LTRACEF("first path '%s' path_element_size %zu\n", path, path_element_size);

    // set up the starting cluster to search
    uint32_t dir_start_cluster;
    if (fat->info().root_cluster) {
        dir_start_cluster = fat->info().root_cluster;
    } else {
        // fat 12/16 has a linear root dir, cluster 0 is a special case to fat_find_file_in_dir below
        dir_start_cluster = 0;
    }

    // output entry
    dir_entry entry {};

    // walk the directory structure
    for (;;) {
        char name_element[MAX_FILE_NAME_LEN];
        strlcpy(name_element, path, MIN(sizeof(name_element), path_element_size + 1));

        LTRACEF("searching for element %s\n", name_element);

        uint32_t found_offset = 0;
        auto status = fat_find_file_in_dir(fat, dir_start_cluster, name_element, &entry, &found_offset);
        if (status < 0) {
            return ERR_NOT_FOUND;
        }

        // we found something
        LTRACEF("found dir entry attributes %#hhx length %u start_cluster %u\n",
                (uint8_t)entry.attributes, entry.length, entry.start_cluster);

        // push the name element forward one notch so we can see if we're at the end (or iterate once again)
        path_increment();
        if (path_element_size > 0) {
            // did we find a directory on an inner node of the path? we can continue iterating
            if (entry.attributes == fat_attribute::directory) {
                dir_start_cluster = entry.start_cluster;
            } else {
                LTRACEF("found a non dir at a non terminal path entry, exit\n");
                return ERR_NOT_FOUND;
            }
        } else {
            // we got a hit at the terminal entry of the path, pass it out to the caller as a success
            if (out_entry) {
                *out_entry = entry;
            }
            if (loc) {
                loc->starting_dir_cluster = dir_start_cluster;
                loc->dir_offset = found_offset;
            }
            return NO_ERROR;
        }
    }
}

// splits a path into the part of it leading up to the last element and the last element
// if the leading part is zero length, return a single "/" element
// will modify string passed in
// TODO: write unit test
static void split_path(char *path, const char **leading_path, const char **last_element) {
    char *last_slash = strrchr(path, '/');
    if (last_slash) {
        *last_slash = 0;
        if (path[0] != 0) {
            *leading_path = path;
        } else {
            *leading_path = "/";
        }
        *last_element = last_slash + 1;
    } else {
        *leading_path = "/";
        *last_element = path;
    }
}

// construct a short file name from the incoming name
// the sfn is padded out with spaces the same way a real FAT entry is
// TODO: write unit test
static status_t name_to_short_file_name(char sfn[8 + 3 + 1], const char *name) {
    // zero length inputs don't fly
    if (name[0] == 0) {
        return ERR_INVALID_ARGS;
    }

    // start off with a spaced out sfn
    memset(sfn, ' ', 8 + 3);
    sfn[8 + 3] = 0;

    size_t input_pos = 0;
    size_t output_pos = 0;

    // pick out the 8 entry part
    for (auto i = 0; i < 8; i++) {
        char c = name[input_pos];
        if (c == 0) {
            break;
        } else if (c == '.') {
            output_pos = 8;
            break;
        } else {
            sfn[output_pos++] = toupper(c);
            input_pos++;
        }
    }

    // at this point input pos had better be looking at a . or a null
    if (name[input_pos] == 0) {
        return NO_ERROR;
    }
    if (name[input_pos] != '.') {
        return ERR_INVALID_ARGS;
    }
    input_pos++;

    for (auto i = 0; i < 3; i++) {
        char c = name[input_pos];
        if (c == 0) {
            break;
        } else if (c == '.') {
            // can only see '.' once
            return ERR_INVALID_ARGS;
        } else {
            sfn[output_pos++] = toupper(c);
            input_pos++;
        }
    }

    // at this point we should be looking at the end of the input string
    if (name[input_pos] != 0) {
        return ERR_INVALID_ARGS;
    }

    return NO_ERROR;
}

status_t fat_dir_allocate(fat_fs *fat, const char *path, const fat_attribute attr, const uint32_t starting_cluster, const uint32_t size, dir_entry_location *loc) {
    LTRACEF("path %s\n", path);

    DEBUG_ASSERT(fat->lock.is_held());

    // trim the last segment off the path, splitting into stuff leading up to the last segment and the last segment
    char local_path[FS_MAX_FILE_LEN + 1];
    strlcpy(local_path, path, FS_MAX_FILE_LEN);

    const char *leading_path;
    const char *last_element;
    split_path(local_path, &leading_path, &last_element);

    DEBUG_ASSERT(leading_path && last_element);

    LTRACEF("path is now split into %s and %s\n", leading_path, last_element);

    // find the starting directory cluster of the container directory
    // 0 may mean root dir on fat12/16
    uint32_t starting_dir_cluster;
    if (strcmp(leading_path, "/") == 0) {
        // root dir is a special case since we know where to start
        if (fat->info().root_cluster) {
            starting_dir_cluster = fat->info().root_cluster;
        } else {
            // fat 12/16 has a linear root dir, cluster 0 is a special case to fat_find_file_in_dir below
            starting_dir_cluster = 0;
        }
    } else {
        // walk to find the containing directory
        dir_entry entry;
        dir_entry_location dir_loc;
        status_t err = fat_dir_walk(fat, local_path, &entry, &dir_loc);
        if (err < 0) {
            return err;
        }

        // verify it's a directory
        if (entry.attributes != fat_attribute::directory) {
            return ERR_BAD_PATH;
        }

        LTRACEF("found containing dir at %u:%u: starting cluster %u\n", dir_loc.starting_dir_cluster, dir_loc.dir_offset, entry.start_cluster);

        starting_dir_cluster = entry.start_cluster;
        if (starting_dir_cluster < 2 || starting_dir_cluster >= fat->info().total_clusters) {
            TRACEF("directory entry contains out of bounds cluster %u\n", starting_dir_cluster);
            return ERR_BAD_STATE;
        }
    }

    LTRACEF("starting dir cluster of parent dir %u\n", starting_dir_cluster);

    // verify the file doesn't already exist
    dir_entry entry;
    status_t err = fat_find_file_in_dir(fat, starting_dir_cluster, last_element, &entry, nullptr);
    if (err >= 0) {
        // we found it, cant create a new file in its place
        return ERR_ALREADY_EXISTS;
    }

    // TODO: handle long file names
    char sfn[8 + 3 + 1];
    err = name_to_short_file_name(sfn, last_element);
    if (err < 0) {
        // if we couldn't convert to a SFN trivially, abort
        return err;
    }

    LTRACEF("short file name '%s'\n", sfn);

    // now we have a starting cluster for the containing directory and proof that it doesn't already exist.
    // start walking to find a free slot
    file_block_iterator dbi(fat, starting_dir_cluster);
    err = dbi.next_sectors(0);
    if (err < 0) {
        return err;
    }

    uint32_t dir_offset = 0;
    uint32_t sector_offset = 0;
    for (;;) {
        if (LOCAL_TRACE >= 2) {
            LTRACEF("dir sector:\n");
            hexdump8_ex(dbi.get_bcache_ptr(0), fat->info().bytes_per_sector, 0);
        }

        // walk within a sector
        while (sector_offset < fat->info().bytes_per_sector) {
            LTRACEF_LEVEL(2, "looking at offset %#x\n", sector_offset);
            uint8_t *ent = dbi.get_bcache_ptr(sector_offset);
            if (ent[0] == 0xe5 || ent[0] == 0) {
                // deleted or last entry in the list
                LTRACEF("found usable at offset %#x\n", sector_offset);
                if (LOCAL_TRACE > 1) hexdump8_ex(ent, DIR_ENTRY_LENGTH, 0);

                // fill in an entry here
                memcpy(&ent[0], sfn, 11); // name
                ent[11] = (uint8_t)attr; // attribute
                ent[12] = 0; // reserved
                ent[13] = 0; // creation time tenth of second
                fat_write16(ent, 14, 0); // creation time seconds / 2
                fat_write16(ent, 16, 0); // creation date
                fat_write16(ent, 18, 0); // last accessed date
                fat_write16(ent, 20, starting_cluster >> 16); // fat cluster high
                fat_write16(ent, 22, 0); // modification time
                fat_write16(ent, 24, 0); // modification date
                fat_write16(ent, 26, starting_cluster); // fat cluster low
                fat_write32(ent, 28, size); // file size

                LTRACEF_LEVEL(2, "filled in entry\n");
                if (LOCAL_TRACE > 1) hexdump8_ex(ent, DIR_ENTRY_LENGTH, 0);

                // flush the data and exit
                dbi.mark_bcache_dirty();

                // flush it
                bcache_flush(fat->bcache());

                // fill in our location data and exit
                if (loc) {
                    loc->starting_dir_cluster = starting_dir_cluster;
                    loc->dir_offset = dir_offset;
                }

                return NO_ERROR;
            }

            dir_offset += DIR_ENTRY_LENGTH;
            sector_offset += DIR_ENTRY_LENGTH;
        }

        // move to the next sector
        err = dbi.next_sector();
        if (err < 0) {
            return err;
        }
        // starting over at offset 0 in the new sector
        sector_offset = 0;
    }

    // TODO: we probably ran out of space, add another cluster to the dir and start over

    return ERR_NOT_IMPLEMENTED;
}

status_t fat_dir_update_entry(fat_fs *fat, const dir_entry_location &loc, uint32_t starting_cluster, uint32_t size) {
    LTRACEF("fat %p, loc %u:%u, cluster %u, size %u\n", fat, loc.starting_dir_cluster, loc.dir_offset, starting_cluster, size);

    // find the dir entry and open the block
    uint32_t sector;
    if (loc.starting_dir_cluster == 0) {
        // special case on fat12/16 to represent the root dir.
        // load 0 into cluster and use sector_offset as relative to the
        // start of the volume.
        sector = fat->info().root_start_sector;
    } else {
        sector = fat_sector_for_cluster(fat, loc.starting_dir_cluster);
    }
    sector += loc.dir_offset / fat->info().bytes_per_sector;

    uint8_t *ent;
    bcache_get_block(fat->bcache(), (void **)&ent, sector);
    ent += loc.dir_offset % fat->info().bytes_per_sector;

    //hexdump8_ex(ent, 0x20, 0);

    fat_write32(ent, 28, size); // file size
    fat_write16(ent, 20, starting_cluster >> 16); // fat cluster high
    fat_write16(ent, 26, starting_cluster); // fat cluster low

    //hexdump8_ex(ent, 0x20, 0);

    bcache_mark_block_dirty(fat->bcache(), sector);
    bcache_put_block(fat->bcache(), sector);

    return NO_ERROR;
}

status_t fat_dir::opendir_priv(const dir_entry &entry, const dir_entry_location &loc, fat_dir_cookie **out_cookie) {
    // fill in our file info based on the entry
    start_cluster_ = entry.start_cluster;
    length_ = 0; // dirs all have 0 length entry
    dir_loc_ = loc;
    inc_ref();

    // create a dir cookie
    auto dir_cookie = new fat_dir_cookie;
    dir_cookie->dir = this;
    dir_cookie->index = 0;

    // add it to the dir object
    list_add_tail(&cookies_, &dir_cookie->node);

    *out_cookie = dir_cookie;

    return NO_ERROR;
}

status_t fat_dir::opendir(fscookie *cookie, const char *name, dircookie **dcookie) {
    auto fat = (fat_fs *)cookie;

    LTRACEF("cookie %p name '%s' dircookie %p\n", cookie, name, dcookie);

    AutoLock guard(fat->lock);

    dir_entry entry;
    dir_entry_location loc;

    // special case for /
    if (name[0] == 0 || !strcmp(name, "/")) {
        entry.attributes = fat_attribute::directory;
        entry.length = 0;
        if (fat->info().fat_bits == 32) {
            entry.start_cluster = fat->info().root_cluster;
        } else {
            entry.start_cluster = 0;
        }

        // special case for the root dir
        // 0:0 is not sufficient, since we could actually find a file in the root dir
        // on a fat 12/16 volume (magic cluster 0) at offset 0. cluster 1 is never used
        // so mark root dir as 1:0
        loc.starting_dir_cluster = 1;
        loc.dir_offset = 0;
    } else {
        status_t err = fat_dir_walk(fat, name, &entry, &loc);
        if (err != NO_ERROR) {
            return err;
        }
    }

    // if we walked and found a proper directory, it's a hit
    if (entry.attributes == fat_attribute::directory) {
        fat_dir *dir;

        // see if this dir is already present in the fs list
        fat_file *file = fat->lookup_file(loc);
        if (file) {
            // XXX replace with hand rolled RTTI
            dir = reinterpret_cast<fat_dir *>(file);
        } else {
            dir = new fat_dir(fat);
        }
        DEBUG_ASSERT(dir);

        fat_dir_cookie *dir_cookie;

        status_t err = dir->opendir_priv(entry, loc, &dir_cookie);
        if (err < 0) {
            // weird state, should we dec the ref?
            PANIC_UNIMPLEMENTED;
            return err;
        }
        DEBUG_ASSERT(dir_cookie);

        *dcookie = (dircookie *)dir_cookie;
        return NO_ERROR;
    } else {
        return ERR_NOT_FILE;
    }

    return ERR_NOT_IMPLEMENTED;
};

status_t fat_dir::readdir_priv(fat_dir_cookie *cookie, struct dirent *ent) {
    LTRACEF("dircookie %p ent %p, current index %u\n", cookie, ent, cookie->index);

    if (!ent)
        return ERR_INVALID_ARGS;

    // make sure the cookie makes sense
    DEBUG_ASSERT((cookie->index % DIR_ENTRY_LENGTH) == 0);

    char filename_buffer[MAX_FILE_NAME_LEN];
    char *filename;
    dir_entry entry;

    {
        AutoLock guard(fs_->lock);

        // kick start our directory sector iterator
        LTRACEF("start cluster %u\n", start_cluster_);
        file_block_iterator dbi(fs_, start_cluster_);

        // move it forward to our index point
        // also loads the buffer
        status_t err = dbi.next_sectors(cookie->index / fs_->info().bytes_per_sector);
        if (err < 0) {
            return err;
        }

        // reset how many sectors the dbi has pushed forward so we can account properly for index shifts later
        dbi.reset_sector_inc_count();

        // pass the index in units of sector offset
        uint32_t offset = cookie->index % fs_->info().bytes_per_sector;
        err = fat_find_next_entry(fs_, dbi, offset, &entry, filename_buffer, &filename);
        if (err < 0) {
            return err;
        }

        // bump the index forward by extracting how much the sector iterator pushed things forward
        uint32_t index_inc = offset - (cookie->index % fs_->info().bytes_per_sector);
        index_inc += dbi.get_sector_inc_count() * fs_->info().bytes_per_sector;
        LTRACEF("calculated index increment %u (old index %u, offset %u, sector_inc_count %u)\n",
                index_inc, cookie->index, offset, dbi.get_sector_inc_count());
        cookie->index += index_inc;
    }

    // copy the info into the fs layer's entry
    strlcpy(ent->name, filename, MIN(sizeof(ent->name), MAX_FILE_NAME_LEN));

    return NO_ERROR;
}

status_t fat_dir::readdir(dircookie *dcookie, struct dirent *ent) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto dir = cookie->dir;

    return dir->readdir_priv(cookie, ent);
}

status_t fat_dir::closedir_priv(fat_dir_cookie *cookie, bool *last_ref) {
    LTRACEF("dircookie %p\n", cookie);

    AutoLock guard(fs_->lock);

    // remove the dircookie from the list
    DEBUG_ASSERT(list_in_list(&cookie->node));
    list_delete(&cookie->node);

    // delete it
    delete cookie;

    // drop a ref to the dir
    *last_ref = dec_ref();
    if (*last_ref) {
        DEBUG_ASSERT(list_is_empty(&cookies_));
    }

    return NO_ERROR;
}

status_t fat_dir::closedir(dircookie *dcookie) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto dir = cookie->dir;

    bool last_ref;
    status_t err = dir->closedir_priv(cookie, &last_ref);
    if (err < 0) {
        return err;
    }

    if (last_ref) {
        LTRACEF("last ref, deleting %p (%u:%u)\n", dir, dir->dir_loc().starting_dir_cluster, dir->dir_loc().dir_offset);
        delete dir;
    }

    return NO_ERROR;
}

