/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <endian.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fat_fs.h"
#include "fat_priv.h"

#define LOCAL_TRACE 0

namespace {

// Local object used to track the state of a directory iterator, as it walks through a directory file
// one sector at a time, using the block cache. Holds a reference to the open block cache block and
// intelligently closes and gets the next one when necessary.
class dir_block_iterator {
public:
    dir_block_iterator(fat_fs_t *_fat, uint32_t starting_cluster) : fat(_fat) {
        if (starting_cluster == 0) {
            // special case on fat12/16 to represent the root dir.
            // load 0 into cluster and use sector_offset as relative to the
            // start of the volume.
            DEBUG_ASSERT(fat->fat_bits == 12 || fat->fat_bits == 16);
            DEBUG_ASSERT(fat->root_start_sector != 0);
            cluster = 0;
            sector_offset = fat->root_start_sector;
        } else {
            cluster = starting_cluster;
            sector_offset = 0;
        }
    }

    ~dir_block_iterator() {
        put_bcache_block();
    }

    status_t load_current_bcache_block() {
        // close the current bcache
        put_bcache_block();

        if (cluster == 0) {
            // we're in a linear root dir, the sector_offset variable represents the raw sector number
            return load_bcache_block(sector_offset);
        } else { // cluster != 0
            DEBUG_ASSERT(sector_offset < fat->bytes_per_sector);

            // compute the sector we should be on given the cluster and sector_offset
            auto sector = fat_sector_for_cluster(fat, cluster) + sector_offset;

            return load_bcache_block(sector);
        }
    }

    const uint8_t *get_dir_entry(size_t offset) {
        DEBUG_ASSERT(offset < fat->bytes_per_sector);
        DEBUG_ASSERT(bcache_buf);
        return (const uint8_t *)bcache_buf + offset;
    }

    // move to the next sector
    status_t next_sector() {
        if (cluster == 0) {
            // on linear dirs it's just the next sector
            sector_offset++;
            // are we at the end of the root dir?
            if (sector_offset >= fat->root_start_sector + fat->root_dir_sectors) {
                return ERR_OUT_OF_RANGE;
            }
        } else {
            // for non linear dirs we step sector by sector into clusters
            // and then wrap to the next cluster.
            sector_offset++;
            if (sector_offset == fat->sectors_per_cluster) {
                sector_offset = 0;

                cluster = fat_next_cluster_in_chain(fat, cluster);
                if (is_eof_cluster(cluster)) {
                    return ERR_OUT_OF_RANGE;
                }
            }
        }

        load_current_bcache_block();

        return NO_ERROR;
    }

private:
    void put_bcache_block() {
        if (bcache_buf) {
            bcache_put_block(fat->cache, bcache_bnum);
            bcache_buf = nullptr;
            bcache_bnum = 0;
        }
    }

    status_t load_bcache_block(bnum_t bnum) {
        DEBUG_ASSERT(!bcache_buf);

        auto err = bcache_get_block(fat->cache, &bcache_buf, bnum);
        if (err >= 0) {
            DEBUG_ASSERT(bcache_buf);
            bcache_bnum = bnum;
        }

        return err;
    }

    fat_fs_t *fat;
    uint32_t cluster;           // current cluster we're on
    uint32_t sector_offset;     // sector number within cluster

    void *bcache_buf = nullptr; // current pointer to the bcache, if held
    bnum_t bcache_bnum;         // current block number of the bcache_buf, if valid
};

status_t fat_find_file_in_dir(fat_fs_t *fat, uint32_t starting_cluster, const char *name, dir_entry *entry) {
    LTRACEF("start_cluster %u, name '%s', out entry %p\n", starting_cluster, name, entry);

    DEBUG_ASSERT(fat->lock.is_held());

    const size_t namelen = strlen(name);

    // kick start our directory sector iterator
    dir_block_iterator dbi(fat, starting_cluster);
    status_t err = dbi.load_current_bcache_block();
    if (err < 0) {
        return err;
    }

    char long_filename[256]; // max long file name incl NULL
    char short_filename[8 + 1 + 3 + 1]; // max short name (8 . 3 NULL)

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
        if (LOCAL_TRACE) {
            LTRACEF("dir sector:\n");
            hexdump8(dbi.get_dir_entry(0), fat->bytes_per_sector);
        }

        // walk within a cluster
        uint32_t offset = 0;
        while (offset < fat->bytes_per_sector) {
            LTRACEF_LEVEL(2, "looking at offset %u\n", offset);
            char *filename;

            const uint8_t *ent = dbi.get_dir_entry(offset);
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
                    lfn_state.pos = sizeof(long_filename);
                    long_filename[--lfn_state.pos] = 0;
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
                        long_filename[--lfn_state.pos] = c & 0xff;
                    }
                }

                LTRACEF_LEVEL(2, "lfn filename thus far: '%s' sequence %hhu\n", long_filename + lfn_state.pos, sequence);

                // iterate one more entry, since we need to at least need to find the corresponding SFN
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else {
                // regular entry, extract the short file name
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
                    filename = long_filename + lfn_state.pos;
                } else {
                    filename = short_filename;
                }

                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;

                // fall through, we've found a file entry
            }

            // at this point we should have found a long or short file name and set the filename pointer
            DEBUG_ASSERT(filename);

            LTRACEF("found filename '%s'\n", filename);
            const size_t filenamelen = strlen(filename);

            // see if we've matched an entry
            if (filenamelen == namelen && !strnicmp(name, filename, filenamelen)) {
                // we have, fill out the passed in dir entry and exit
                uint16_t target_cluster = fat_read16(ent, 0x1a);
                entry->length = fat_read32(ent, 0x1c);
                entry->attributes = (fat_attribute)ent[0x0B];
                entry->start_cluster = target_cluster;
                return NO_ERROR;
            }
        }

        // move to the next sector
        err = dbi.next_sector();
        if (err < 0) {
            break;
        }
    }

    return ERR_NOT_FOUND;
}

} // namespace

status_t fat_walk(fat_fs_t *fat, const char *path, dir_entry *out_entry) {
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
    if (fat->root_cluster) {
        dir_start_cluster = fat->root_cluster;
    } else {
        // fat 12/16 has a linear root dir, cluster 0 is a special case to fat_find_file_in_dir below
        dir_start_cluster = 0;
    }

    // output entry
    dir_entry entry {};

    // walk the directory structure
    for (;;) {
        char name_element[256];
        strlcpy(name_element, path, MIN(sizeof(name_element), path_element_size + 1));

        LTRACEF("searching for element %s\n", name_element);

        auto status = fat_find_file_in_dir(fat, dir_start_cluster, name_element, &entry);
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
            *out_entry = entry;
            return NO_ERROR;
        }
    }
}

struct fat_dir_cookie {
    struct list_node node;
    struct fat_dir *dir;

    // next directory index offset (in units of 0x20), 0xffffffff for EOD
    uint32_t index;
    static const uint32_t index_eod = 0xffffffff;
};

struct fat_dir {
    struct list_node node;
    struct list_node cookies = LIST_INITIAL_VALUE(cookies);

    fat_fs_t *fat;

    // id of the directory is keyed off the starting cluster (or 0 for the root dir)
    uint32_t start_cluster;
};

status_t fat_opendir(fscookie *cookie, const char *name, dircookie **dcookie) {
    auto fat = (fat_fs_t *)cookie;

    LTRACEF("cookie %p name '%s' dircookie %p\n", cookie, name, dcookie);

    AutoLock guard(fat->lock);

    dir_entry entry;

    // special case for /
    if (name[0] == 0 || !strcmp(name, "/")) {
        entry.attributes = fat_attribute::directory;
        entry.length = 0;
        entry.start_cluster = 0;
    } else {
        status_t err = fat_walk(fat, name, &entry);
        if (err != NO_ERROR) {
            return err;
        }
    }

    // if we walked and found a proper directory, it's a hit
    if (entry.attributes == fat_attribute::directory) {
        // TODO: see if the dir is already in the list
        auto dir = new fat_dir;
        dir->fat = fat;
        dir->start_cluster = entry.start_cluster;
        list_add_head(&fat->dir_list, &dir->node);

        // create a dir cookie
        auto dir_cookie = new fat_dir_cookie;
        dir_cookie->dir = dir;
        dir_cookie->index = 0;

        // add it to the dir object
        list_add_tail(&dir->cookies, &dir_cookie->node);

        *dcookie = (dircookie *)dir_cookie;
        return NO_ERROR;
    } else {
        return ERR_NOT_FILE;
    }

    return ERR_NOT_IMPLEMENTED;
};

status_t fat_readdir(dircookie *dcookie, struct dirent *ent) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto fat = cookie->dir->fat;

    LTRACEF("dircookie %p ent %p, current index %u\n", dcookie, ent, cookie->index);

    if (!ent)
        return ERR_INVALID_ARGS;

    AutoLock guard(fat->lock);

    // TODO: actually walk the dir
    cookie->index = fat_dir_cookie::index_eod;
    return ERR_NOT_FOUND;
}


status_t fat_closedir(dircookie *dcookie) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto fat = cookie->dir->fat;

    LTRACEF("dircookie %p\n", dcookie);

    AutoLock guard(fat->lock);

    // free the dircookie
    //mutex_acquire(&dcookie->fs->lock);
    list_delete(&cookie->node);
    //mutex_release(&dcookie->fs->lock);

    free(dcookie);

    return NO_ERROR;
}

