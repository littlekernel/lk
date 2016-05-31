/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
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

#include <debug.h>
#include <err.h>
#include <pow2.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <trace.h>

#include <kernel/mutex.h>
#include <lib/bio.h>
#include <lib/cksum.h>
#include <lib/console.h>
#include <lib/fs.h>
#include <lib/fs/spifs.h>
#include <list.h>
#include <lk/init.h>

#define LOCAL_TRACE 0

#define FS_VERSION 1
#define FS_MAGIC 0x53504653  // SPFS

#define SPIFS_ENTRY_LENGTH 32

#define TOC_HEADER_RESERVED_BYTES 16
#define TOC_FOOTER_RESERVED_BYTES 28
#define MAX_FILENAME_LENGTH 20

#define CORRUPT_TOC 0
#define NO_OPEN_RUNS 0

#define FRONT_TOC (1)
#define BACK_TOC  (-1)

#define FRONT_TOC_LABEL "front-toc"
#define BACK_TOC_LABEL "back-toc"

typedef int32_t toc_position_t;

typedef struct {
    uint8_t *page;
    uint32_t page_size;
    uint32_t page_count;
    uint32_t blocks_per_page;

    uint32_t generation;
    uint32_t num_entries;
    toc_position_t toc_position;

    struct list_node files;
    struct list_node dcookies;

    bdev_t *dev;

    mutex_t lock;
} spifs_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t num_entries;
    uint32_t generation;

    uint8_t _reserved[TOC_HEADER_RESERVED_BYTES];
} toc_header_t;

typedef struct {
    uint32_t page_idx;
    uint32_t length;
    uint32_t capacity;
    char filename[MAX_FILENAME_LENGTH];
} toc_file_t;

typedef struct {
    uint8_t _reserved[TOC_FOOTER_RESERVED_BYTES];
    uint32_t checksum;
} toc_footer_t;

typedef struct {
    struct list_node node;
    spifs_t *fs_handle;
    toc_file_t metadata;
} spifs_file_t;

struct dircookie {
    struct list_node node;
    spifs_t *fs;

    spifs_file_t *next_file;
};

typedef struct {
    uint32_t page_id;
    int32_t direction;
    uint32_t entry_length;
    uint8_t *data;
    uint8_t *page;
    spifs_t *spifs;
} cursor_t;

static status_t spifs_read_page(spifs_t *spifs, uint32_t page_addr);
static status_t spifs_write_page(spifs_t *spifs, uint32_t page_addr);

static status_t get_device_page_info(bdev_t *dev, uint32_t *page_size,
                                     uint32_t *page_count);


static status_t cursor_init(
    cursor_t *cursor, spifs_t *spifs, int32_t direction, uint32_t page_id,
    uint32_t entry_length
)
{
    // Make sure the cursor can only be advanced an integer number of times
    // per page.
    DEBUG_ASSERT(ispow2(entry_length));
    DEBUG_ASSERT(spifs->page_size % entry_length == 0);
    DEBUG_ASSERT(spifs->page);

    cursor->page_id = page_id;
    cursor->direction = direction;
    cursor->entry_length = entry_length;
    cursor->data = spifs->page;
    cursor->spifs = spifs;


    return spifs_read_page(spifs, page_id);
}

static uint8_t *cursor_get(cursor_t *cursor)
{
#if LK_DEBUGLEVEL > 1
    spifs_t *spifs = cursor->spifs;

    uint8_t *page_end = spifs->page + spifs->page_size;
    DEBUG_ASSERT(cursor->data < page_end);
#endif

    return cursor->data;
}

static status_t cursor_advance(cursor_t *cursor)
{
    spifs_t *spifs = cursor->spifs;

    uint8_t *page_end = spifs->page + spifs->page_size;

    cursor->data += cursor->entry_length;

    // We have walked past the end of our buffer.
    DEBUG_ASSERT(page_end >= cursor->data);

    // If we're at the end of this page, read the next page and move the cursor
    // to the beginning of it.
    if (cursor->data == page_end) {
        cursor->page_id += cursor->direction;
        cursor->data = spifs->page;

        return spifs_read_page(spifs, cursor->page_id);
    }

    return NO_ERROR;
}

static spifs_file_t *find_file(spifs_t *spifs, const char *name)
{
    spifs_file_t *file;

    list_for_every_entry(&spifs->files, file, spifs_file_t, node) {
        // Skip the ToC Entries
        if (file == list_peek_head_type(&spifs->files, spifs_file_t, node) ||
                file == list_peek_tail_type(&spifs->files, spifs_file_t, node)) {
            continue;
        }

        if (!strncmp(name, file->metadata.filename, MAX_FILENAME_LENGTH)) {
            return file;
        }
    }

    return NULL;
}

static uint32_t find_open_run(spifs_t *spifs, uint32_t requested_length)
{
    spifs_file_t *file;
    list_for_every_entry(&spifs->files, file, spifs_file_t, node) {
        // Number of pages that this file occupies
        uint32_t page_size_shift = log2_uint(file->fs_handle->page_size);

        uint32_t file_page_length =
            divpow2(file->metadata.capacity, page_size_shift);

        // Index of the page immediately following the last page of this file.
        uint32_t file_end_page = file->metadata.page_idx + file_page_length;

        // Determine the page that the next file starts at.
        spifs_file_t *next =
            list_next_type(&spifs->files, &file->node, spifs_file_t, node);

        // End of list?
        if (next == NULL) {
            return NO_OPEN_RUNS;
        }

        uint32_t available_pages = next->metadata.page_idx - file_end_page;
        uint32_t available_bytes = available_pages * file->fs_handle->page_size;
        if (available_bytes >= requested_length) {
            return file_end_page;
        }
    }
    return NO_OPEN_RUNS;
}

static uint64_t used_space(spifs_t *spifs)
{
    uint64_t result = 0;

    spifs_file_t *file;
    list_for_every_entry(&spifs->files, file, spifs_file_t, node) {
        result += file->metadata.capacity;
    }

    return result;
}

static bool consistency_check(spifs_t *spifs)
{
    /* Return true iff the ToC is in a consistent state. */
    spifs_file_t *file;
    list_for_every_entry(&spifs->files, file, spifs_file_t, node) {
        // Number of pages that this file occupies
        uint32_t file_page_length =
            file->metadata.capacity / file->fs_handle->page_size;

        // Index of the last page of this file.
        uint32_t file_end_page = file->metadata.page_idx + file_page_length - 1;

        // Determine the page that the next file starts at.
        spifs_file_t *next =
            list_next_type(&spifs->files, &file->node, spifs_file_t, node);

        // End of list?
        if (next == NULL) {
            continue;
        }

        if (next->metadata.page_idx <= file_end_page) {
            return false;
        }
    }

    return true;
}

static status_t spifs_commit_toc(spifs_t *spifs)
{
    status_t err;

    // Get the next logical ToC.
    toc_position_t target_toc =
        spifs->toc_position == FRONT_TOC ? BACK_TOC : FRONT_TOC;

    // Bump the generation counter.
    uint32_t target_generation = spifs->generation + 1;

    uint32_t crc = 0;
    uint8_t *cursor = spifs->page;
    uint32_t toc_page_addr = target_toc == FRONT_TOC ?
                             0 : spifs->page_count - 1;

    // Setup the ToC Header.
    toc_header_t header = {
        .magic       = FS_MAGIC,
        .version     = FS_VERSION,
        .num_entries = spifs->num_entries,
        .generation  = target_generation,
    };
    memset(header._reserved, 0, TOC_HEADER_RESERVED_BYTES);

    crc = crc32(crc, (uint8_t *)&header, SPIFS_ENTRY_LENGTH);

    memcpy(cursor, (uint8_t *)&header, SPIFS_ENTRY_LENGTH);
    cursor += SPIFS_ENTRY_LENGTH;

    // Create an empty file to copy into the empty spots in the ToC
    static const toc_file_t empty = { 0 };

    spifs_file_t *file = list_peek_head_type(&spifs->files, spifs_file_t, node);
    for (uint32_t i = 0; i < spifs->num_entries; i++) {
        uint8_t *page_end = spifs->page + spifs->page_size;
        DEBUG_ASSERT(cursor <= page_end);

        if (cursor == page_end) {
            err = spifs_write_page(spifs, toc_page_addr);
            if (err != NO_ERROR) {
                return err;
            }

            toc_page_addr += target_toc;
            cursor = spifs->page;
        }

        if (file) {
            crc = crc32(crc, (uint8_t *)&file->metadata, SPIFS_ENTRY_LENGTH);
            memcpy(cursor, (uint8_t *)&file->metadata, SPIFS_ENTRY_LENGTH);
            file = list_next_type(&spifs->files, &file->node, spifs_file_t, node);
        } else {
            crc = crc32(crc, (uint8_t *)&empty, SPIFS_ENTRY_LENGTH);
            memcpy(cursor, (uint8_t *)&empty, SPIFS_ENTRY_LENGTH);
        }

        cursor += SPIFS_ENTRY_LENGTH;
    }

    // Sanity check. The cursor should be at the last position in this page
    // at this point.
#if LK_DEBUGLEVEL > 1
    uint8_t *expected_cursor_location =
        (spifs->page + spifs->page_size) - SPIFS_ENTRY_LENGTH;
    DEBUG_ASSERT(cursor == expected_cursor_location);
#endif

    toc_footer_t *footer = (toc_footer_t *)cursor;
    memset(footer, 0, SPIFS_ENTRY_LENGTH);
    footer->checksum = crc;

    err = spifs_write_page(spifs, toc_page_addr);
    if (err != NO_ERROR)
        return err;

    // Only update this once we're sure that the write went through.
    // This way, if the write failed, we'll try writing over the bad ToC again
    // rather than potentially corrupting both ToCs.
    spifs->generation = target_generation;
    spifs->toc_position = target_toc;

    return NO_ERROR;
}

static void spifs_add_ascending(spifs_t *spifs, spifs_file_t *target)
{
    spifs_file_t *file;
    list_for_every_entry(&spifs->files, file, spifs_file_t, node) {
        if (file->metadata.page_idx > target->metadata.page_idx) {
            list_add_before(&file->node, &target->node);
            return;
        }
    }

    list_add_tail(&spifs->files, &target->node);
}


static status_t spifs_read_page(spifs_t *spifs, uint32_t page_addr)
{
    off_t block_addr = page_addr * spifs->blocks_per_page;

    ssize_t bytes = bio_read_block(spifs->dev, spifs->page, block_addr,
                                   spifs->blocks_per_page);

    if ((uint32_t)bytes != spifs->page_size) {
        return ERR_IO;
    }

    return NO_ERROR;
}

static status_t spifs_write_page(spifs_t *spifs, uint32_t page_addr)
{
    off_t block_addr = page_addr * spifs->blocks_per_page;
    off_t device_addr = block_addr * spifs->dev->block_size;

    // Device requires erase before write?
    if (spifs->dev->geometry_count != 0) {
        ssize_t bytes = bio_erase(spifs->dev, device_addr, spifs->page_size);
        if ((uint32_t)bytes != spifs->page_size) {
            return ERR_IO;
        }
    }

    ssize_t bytes = bio_write_block(spifs->dev, spifs->page, block_addr,
                                    spifs->blocks_per_page);

    if ((uint32_t)bytes != spifs->page_size) {
        return ERR_IO;
    }

    return NO_ERROR;
}

static uint32_t get_toc_generation(spifs_t *spifs, toc_position_t toc_pos)
{
    LTRACEF("spifs %p\n", spifs);

    uint32_t candidate_generation;

    DEBUG_ASSERT(spifs);

    DEBUG_ASSERT(toc_pos == FRONT_TOC || toc_pos == BACK_TOC);
    uint32_t toc_page = toc_pos == FRONT_TOC ?
                        0 : (spifs->page_count - 1);


    cursor_t cursor;
    if (cursor_init(&cursor, spifs, toc_pos, toc_page, SPIFS_ENTRY_LENGTH) !=
            NO_ERROR) {
        return CORRUPT_TOC;
    }

    toc_header_t *header = (toc_header_t *)cursor_get(&cursor);

    if (header->magic != FS_MAGIC) {
        return CORRUPT_TOC;
    }

    if (header->version != FS_VERSION) {
        return CORRUPT_TOC;
    }

    candidate_generation = header->generation;
    uint32_t num_toc_entries = header->num_entries;

    uint32_t crc = 0;
    crc = crc32(crc, (uint8_t *)header, SPIFS_ENTRY_LENGTH);

    header = NULL;

    for (size_t i = 0; i < num_toc_entries; i++) {
        if (cursor_advance(&cursor) != NO_ERROR)
            return CORRUPT_TOC;

        crc = crc32(crc, cursor_get(&cursor), SPIFS_ENTRY_LENGTH);
    }

    if (cursor_advance(&cursor) != NO_ERROR)
        return CORRUPT_TOC;

    toc_footer_t *footer = (toc_footer_t *)cursor_get(&cursor);
    if (footer->checksum != crc) {
        return CORRUPT_TOC;
    }

    return candidate_generation;
}

// page_size will be populated with the device's page size if this function
// returns NO_ERROR, otherwise the contents of page_size are undefined.
static status_t get_device_page_info(bdev_t *dev, uint32_t *page_size, uint32_t *page_count)
{
    LTRACEF("dev %p, page_size %p\n", dev, page_size);

    switch (dev->geometry_count) {
        case 0: {
            // Device has no erase geometry; overwriting is supported.
            *page_size = dev->block_size;
            *page_count = dev->total_size / (*page_size);
            return NO_ERROR;
        }
        case 1: {
            // Device has erase geometry.
            size_t erase_size = valpow2(dev->geometry->erase_size);
            size_t block_size = dev->block_size;

            if (erase_size % block_size != 0) {
                // erase_size must be a multiple of the block size.
                return ERR_NOT_SUPPORTED;
            }

            *page_size = erase_size;
            *page_count = dev->total_size / (*page_size);
            return NO_ERROR;
        }
        default: {
            // We don't support non-uniform erase geometry.
            return ERR_NOT_SUPPORTED;
        }
    }
}

static status_t spifs_format(bdev_t *dev, const void *args)
{
    status_t err = NO_ERROR;

    LTRACEF("dev %p, args %p\n", dev, args);

    if (!dev) {
        return ERR_INVALID_ARGS;
    }

    spifs_format_args_t *spifs_args;
    spifs_format_args_t default_args = {
        .toc_pages = 1,
    };

    if (!args) {
        spifs_args = &default_args;
    } else {
        spifs_args = (spifs_format_args_t *)args;
    }

    // Make sure that each of the three data structures are the same size.
    STATIC_ASSERT(sizeof(toc_header_t) == SPIFS_ENTRY_LENGTH);
    STATIC_ASSERT(sizeof(toc_file_t) == SPIFS_ENTRY_LENGTH);
    STATIC_ASSERT(sizeof(toc_footer_t) == SPIFS_ENTRY_LENGTH);

    uint32_t page_size;
    uint32_t page_count;
    err = get_device_page_info(dev, &page_size, &page_count);
    if (err != NO_ERROR)
        return err;

    // Make sure entries can be exactly packed into pages.
    if (page_size % SPIFS_ENTRY_LENGTH != 0) {
        return ERR_NOT_SUPPORTED;
    }

    // Make sure the device size is some multiple of the page size;
    // we don't want a partial page at the end of the device.
    if (dev->total_size % page_size != 0) {
        return ERR_NOT_SUPPORTED;
    }

    uint32_t entires_per_page = page_size / SPIFS_ENTRY_LENGTH;

    // Number of ToC entrries is the total number of entries less 2 for the
    // header/footer
    uint32_t num_entries = spifs_args->toc_pages * entires_per_page;
    uint32_t num_toc_entries = num_entries - 2;

    // Four entries will be consumed by metadata: Header, Front ToC entry,
    // Back ToC entry, footer. If there are only four entries, there will be
    // no room for files.
    if (num_entries <= 4) {
        return ERR_TOO_BIG;
    }

    // Create a mock spifs_t for the purposes of formatting the fs.
    spifs_t spifs = {
        .page_size = page_size,
        .page_count = page_count,
        .blocks_per_page = divpow2(page_size, dev->block_shift),
        .generation = 1,
        .num_entries = num_toc_entries,
        .toc_position = FRONT_TOC,
        .dev = dev,
    };
    spifs.page = memalign(CACHE_LINE, page_size);
    list_initialize(&spifs.files);
    list_initialize(&spifs.dcookies);
    mutex_init(&spifs.lock);

    spifs_file_t f_toc;
    f_toc.metadata.page_idx = 0;
    f_toc.metadata.length = spifs_args->toc_pages * page_size;
    f_toc.metadata.capacity = spifs_args->toc_pages * page_size;
    f_toc.fs_handle = &spifs;
    memset(f_toc.metadata.filename, 0, MAX_FILENAME_LENGTH);
    strlcpy(f_toc.metadata.filename, FRONT_TOC_LABEL, MAX_FILENAME_LENGTH);

    spifs_file_t b_toc;
    b_toc.metadata.page_idx = page_count - spifs_args->toc_pages;
    b_toc.metadata.length = spifs_args->toc_pages * page_size;
    b_toc.metadata.capacity = spifs_args->toc_pages * page_size;
    b_toc.fs_handle = &spifs;
    memset(b_toc.metadata.filename, 0, MAX_FILENAME_LENGTH);
    strlcpy(b_toc.metadata.filename, BACK_TOC_LABEL, MAX_FILENAME_LENGTH);

    spifs_add_ascending(&spifs, &f_toc);
    spifs_add_ascending(&spifs, &b_toc);

    // Commit the first toc.
    err = spifs_commit_toc(&spifs);
    if (err != NO_ERROR)
        goto err;

    // Commit the other toc.
    err = spifs_commit_toc(&spifs);
    if (err != NO_ERROR)
        goto err;

err:
    free(spifs.page);

    return err;
}

static status_t spifs_mount(bdev_t *dev, fscookie **cookie)
{
    status_t status;

    LTRACEF("dev %p, cookie %p\n", dev, cookie);

    spifs_t *spifs = malloc(sizeof(*spifs));
    if (!spifs) {
        return ERR_NO_MEMORY;
    }

    status = get_device_page_info(dev, &spifs->page_size, &spifs->page_count);
    if (status != NO_ERROR) {
        free(spifs);
        return status;
    }

    spifs->blocks_per_page = divpow2(spifs->page_size, dev->block_shift);

    spifs->page = memalign(CACHE_LINE, spifs->page_size);
    if (!spifs->page) {
        free(spifs);
        return ERR_NO_MEMORY;
    }

    spifs->dev = dev;

    list_initialize(&spifs->files);
    list_initialize(&spifs->dcookies);
    mutex_init(&spifs->lock);

    // Determine which of the two Table of Contents we should use.
    uint32_t f_toc_generation = get_toc_generation(spifs, FRONT_TOC);
    uint32_t b_toc_generation = get_toc_generation(spifs, BACK_TOC);

    if (f_toc_generation == CORRUPT_TOC && b_toc_generation == CORRUPT_TOC) {
        // Both ToCs are corrupt.
        status = ERR_CRC_FAIL;
        goto err;
    }

    spifs->toc_position =
        f_toc_generation > b_toc_generation ? FRONT_TOC : BACK_TOC;
    spifs->generation = MAX(f_toc_generation, b_toc_generation);

    uint32_t toc_page_addr = spifs->toc_position == FRONT_TOC ?
                             0 : spifs->page_count - 1;

    cursor_t cursor;
    status = cursor_init(&cursor, spifs, spifs->toc_position, toc_page_addr,
                         SPIFS_ENTRY_LENGTH);
    if (status != NO_ERROR)
        goto err;

    toc_header_t *header = (toc_header_t *)cursor_get(&cursor);
    spifs->num_entries = header->num_entries;
    header = NULL;

    // Create in-memory versions of metadata for files.
    spifs_file_t *file;
    for (size_t i = 0; i < spifs->num_entries; i++) {
        status = cursor_advance(&cursor);
        if (status != NO_ERROR)
            goto err;

        toc_file_t *file_entry = (toc_file_t *)cursor_get(&cursor);
        if (file_entry->capacity == 0) {
            continue;
        }

        file = malloc(sizeof(*file));
        if (!file) {
            status = ERR_NO_MEMORY;
            goto err;
        }

        memcpy(&file->metadata, file_entry, SPIFS_ENTRY_LENGTH);

        file->fs_handle = spifs;

        list_add_tail(&spifs->files, &file->node);
    }

    if (!consistency_check(spifs)) {
        status = ERR_BAD_STATE;
        goto err;
    }

    *cookie = (fscookie *)spifs;

    return NO_ERROR;

err:
    while ((file = list_remove_head_type(&spifs->files, spifs_file_t, node))) {
        free(file);
    }

    free(spifs->page);
    free(spifs);
    return status;
}

static status_t spifs_unmount(fscookie *cookie)
{
    LTRACEF("cookie %p\n", cookie);

    spifs_t *spifs = (spifs_t *)cookie;

    mutex_acquire(&spifs->lock);

    spifs_file_t *file;
    while ((file = list_remove_head_type(&spifs->files, spifs_file_t, node))) {
        free(file);
    }

    free(spifs->page);

    mutex_release(&spifs->lock);

    free(spifs);

    return NO_ERROR;
}

static status_t spifs_create(fscookie *cookie, const char *name, filecookie **fcookie, uint64_t len)
{
    status_t status = NO_ERROR;

    LTRACEF("cookie %p name '%s' filecookie %p len %llu\n", cookie, name, fcookie, len);

    spifs_t *spifs = (spifs_t *)cookie;

    // Strip leading fwd-slashes
    name = trim_name(name);

    // File system is flat, directories not supported.
    if (strchr(name, '/'))
        return ERR_NOT_SUPPORTED;

    // Check that filename is not too long.
    if (strnlen(name, MAX_FILENAME_LENGTH) == MAX_FILENAME_LENGTH)
        return ERR_BAD_PATH;

    // Length is bigger than 4GB?
    if (len > 0xFFFFFFFF)
        return ERR_TOO_BIG;

    mutex_acquire(&spifs->lock);

    if (find_file(spifs, name)) {
        status = ERR_ALREADY_EXISTS;
        goto err;
    }

    // Is the ToC full? Have we reached the limit on the number of files?
    size_t num_files_in_toc = list_length(&spifs->files);
    DEBUG_ASSERT(num_files_in_toc <= spifs->num_entries);
    if (num_files_in_toc >= spifs->num_entries) {
        status = ERR_TOO_BIG;
        goto err;
    }

    uint32_t capacity;
    if (len == 0) {
        capacity = spifs->page_size;
    } else {
        capacity = ROUNDUP(len, spifs->page_size);
    }

    uint32_t open_run = find_open_run(spifs, capacity);
    if (open_run == NO_OPEN_RUNS) {
        status = ERR_TOO_BIG;
        goto err;
    }

    spifs_file_t *file = malloc(sizeof(*file));
    if (!file) {
        status = ERR_NO_MEMORY;
        goto err;
    }

    file->fs_handle = spifs;
    file->metadata.page_idx = open_run;
    file->metadata.length = len;
    file->metadata.capacity = capacity;
    memset(file->metadata.filename, 0, MAX_FILENAME_LENGTH);
    strlcpy(file->metadata.filename, name, MAX_FILENAME_LENGTH);

    // Erase the memory allocated to the file.
    if (bio_erase(spifs->dev, open_run * spifs->page_size, capacity) !=
            (ssize_t)capacity) {

        free(file);

        status = ERR_IO;
        goto err;
    }

    spifs_add_ascending(spifs, file);

    if (spifs_commit_toc(spifs) != NO_ERROR) {
        // If the commit fails, make sure we don't leave any residue of the file
        // lying around.
        list_delete(&file->node);
        free(file);
        *fcookie = NULL;

        status = ERR_IO;
        goto err;
    }

    *fcookie = (filecookie *) file;

err:
    mutex_release(&spifs->lock);

    return status;
}

static status_t spifs_open(fscookie *cookie, const char *name, filecookie **fcookie)
{
    LTRACEF("cookie %p name '%s' filecookie %p\n", cookie, name, fcookie);

    spifs_t *spifs = (spifs_t *)cookie;

    name = trim_name(name);

    mutex_acquire(&spifs->lock);

    spifs_file_t *file = find_file(spifs, name);

    mutex_release(&spifs->lock);

    if (!file)
        return ERR_NOT_FOUND;

    *fcookie = (filecookie *)file;

    return NO_ERROR;
}

static status_t spifs_close(filecookie *fcookie)
{
    spifs_file_t *file = (spifs_file_t *)fcookie;

    LTRACEF("cookie %p name '%s'\n", fcookie, file->metadata.filename);

    return NO_ERROR;
}

static status_t spifs_remove(fscookie *cookie, const char *name)
{
    status_t status;

    LTRACEF("cookie %p name '%s'\n", cookie, name);

    spifs_t *spifs = (spifs_t *)cookie;

    // make sure we strip out any leading /
    name = trim_name(name);

    mutex_acquire(&spifs->lock);

    spifs_file_t *file = find_file(spifs, name);

    if (!file) {
        status = ERR_NOT_FOUND;
        goto err;
    }

    // Make sure there are no dirents open that point to the file that we're
    // deleting.
    dircookie *dcookie;
    list_for_every_entry(&spifs->dcookies, dcookie, dircookie, node) {
        if (dcookie->next_file == file) {
            dcookie->next_file = list_next_type(&dcookie->fs->files,
                                                &dcookie->next_file->node,
                                                spifs_file_t, node);
        }
    }

    list_delete(&file->node);
    free(file);

    status = spifs_commit_toc(spifs);

err:
    mutex_release(&spifs->lock);

    return status;
}

static ssize_t spifs_read(filecookie *fcookie, void *buf, off_t off, size_t len)
{
    LTRACEF("filecookie %p buf %p offset %lld len %zu\n", fcookie, buf, off, len);

    spifs_file_t *file = (spifs_file_t *)fcookie;
    spifs_t *spifs = file->fs_handle;

    if (off < 0)
        return ERR_INVALID_ARGS;

    mutex_acquire(&spifs->lock);

    uint32_t file_start = file->fs_handle->page_size * file->metadata.page_idx;
    uint32_t file_end = file_start + file->metadata.length;

    uint32_t read_start = file_start + off;
    uint32_t read_end = read_start + len;

    if (read_start >= file_end) {
        len = 0;
    } else if (read_end > file_end) {
        len = file_end - read_start;
    }

    DEBUG_ASSERT(file->fs_handle->dev);

    ssize_t result = bio_read(file->fs_handle->dev, buf, read_start, len);

    mutex_release(&spifs->lock);

    return result;
}

static ssize_t spifs_write(filecookie *fcookie, const void *buf, off_t off, size_t size)
{
    status_t err = NO_ERROR;
    size_t len = size;

    LTRACEF("filecookie %p buf %p offset %lld len %zu\n", fcookie, buf, off, len);

    spifs_file_t *file = (spifs_file_t *)fcookie;
    spifs_t *spifs = (spifs_t *)(file->fs_handle);

    if (off < 0)
        return ERR_INVALID_ARGS;

    mutex_acquire(&spifs->lock);

    if (off + len > file->metadata.capacity) {
        err = ERR_OUT_OF_RANGE;
        goto err;
    }

    bool dirty_toc = false;

    uint32_t start_addr =
        off + (file->metadata.page_idx * spifs->page_size);

    uint32_t page_shift = log2_uint(spifs->page_size);
    uint32_t target_page_id = divpow2(start_addr, page_shift);

    // Are we growing the file?
    if (off + len > file->metadata.length) {
        file->metadata.length = off + len;
        dirty_toc = true;
    }

    // Leading Partial Page.
    uint32_t page_offset = start_addr % spifs->page_size;
    if (page_offset) {
        uint32_t page_end = ROUNDUP(start_addr, spifs->page_size);

        uint32_t n_bytes = MIN(len, page_end - start_addr);

        // read..
        err = spifs_read_page(spifs, target_page_id);
        if (err != NO_ERROR) {
            goto err;
        }

        // modify..
        memcpy(spifs->page + page_offset, buf, n_bytes);

        // write..
        err = spifs_write_page(spifs, target_page_id);
        if (err != NO_ERROR) {
            goto err;
        }

        len -= n_bytes;
        buf += n_bytes;
        target_page_id++;
    }

    // Internal Full Pages.
    while (len >= spifs->page_size) {
        memcpy(spifs->page, buf, spifs->page_size);
        err = spifs_write_page(spifs, target_page_id);
        if (err != NO_ERROR) {
            goto err;
        }

        len -= spifs->page_size;
        buf += spifs->page_size;
        target_page_id++;
    }

    // Trailing Partial Page.
    if (len) { // Bytes remaining?
        // read..
        err = spifs_read_page(spifs, target_page_id);
        if (err != NO_ERROR) {
            goto err;
        }

        // modify..
        memcpy(spifs->page, buf, len);

        // write..
        err = spifs_write_page(spifs, target_page_id);
        if (err != NO_ERROR) {
            goto err;
        } else {
            len = 0;
        }
    }

    if (dirty_toc) {
        err = spifs_commit_toc(spifs);
    }

err:
    mutex_release(&spifs->lock);
    return len == 0 ? (ssize_t)size : err;
}

static status_t spifs_truncate(filecookie *fcookie, uint64_t len)
{
    LTRACEF("filecookie %p, len %llu\n", fcookie, len);

    status_t rc = NO_ERROR;

    spifs_file_t *file = (spifs_file_t *)fcookie;

    mutex_acquire(&file->fs_handle->lock);

    spifs_t *spifs = (spifs_t *)(file->fs_handle);

    // Can't use truncate to grow a file.
    if (len > file->metadata.length) {
        rc = ERR_INVALID_ARGS;
        goto finish;
    }

    file->metadata.length = len;

    rc = spifs_commit_toc(spifs);

finish:
    mutex_release(&file->fs_handle->lock);

    return rc;
}

static status_t spifs_stat(filecookie *fcookie, struct file_stat *stat)
{
    LTRACEF("filecookie %p stat %p\n", fcookie, stat);

    spifs_file_t *file = (spifs_file_t *)fcookie;

    mutex_acquire(&file->fs_handle->lock);

    if (stat) {
        stat->is_dir = false;
        stat->size = file->metadata.length;
        stat->capacity = file->metadata.capacity;
    }

    mutex_release(&file->fs_handle->lock);

    return NO_ERROR;
}

static status_t spifs_opendir(fscookie *cookie, const char *name, dircookie **dcookie)
{
    LTRACEF("cookie %p name '%s' dircookie %p\n", cookie, name, dcookie);

    spifs_t *spifs = (spifs_t *)cookie;

    name = trim_name(name);

    if (strcmp("", name))
        return ERR_NOT_FOUND;

    dircookie *dir = malloc(sizeof(*dir));
    if (!dir)
        return ERR_NO_MEMORY;

    dir->fs = spifs;

    mutex_acquire(&spifs->lock);

    spifs_file_t *front_toc_file =
        list_peek_head_type(&spifs->files, spifs_file_t, node);
    dir->next_file = list_next_type(&spifs->files, &front_toc_file->node,
                                    spifs_file_t, node);
    list_add_head(&spifs->dcookies, &dir->node);

    mutex_release(&spifs->lock);

    *dcookie = dir;

    return NO_ERROR;
}

static status_t spifs_readdir(dircookie *dcookie, struct dirent *ent)
{
    status_t err;

    LTRACEF("dircookie %p ent %p\n", dcookie, ent);

    mutex_acquire(&dcookie->fs->lock);

    spifs_file_t *back_toc_file =
        list_peek_tail_type(&dcookie->fs->files, spifs_file_t, node);

    if (dcookie->next_file != back_toc_file) {
        strlcpy(ent->name, dcookie->next_file->metadata.filename, sizeof(ent->name));
        dcookie->next_file =
            list_next_type(&dcookie->fs->files, &dcookie->next_file->node,
                           spifs_file_t, node);
        err = NO_ERROR;
    } else {
        err = ERR_NOT_FOUND;
    }

    mutex_release(&dcookie->fs->lock);

    return err;
}

static status_t spifs_closedir(dircookie *dcookie)
{
    LTRACEF("dircookie %p\n", dcookie);

    mutex_acquire(&dcookie->fs->lock);
    list_delete(&dcookie->node);
    mutex_release(&dcookie->fs->lock);

    free(dcookie);

    return NO_ERROR;
}

static status_t spifs_fs_stat(fscookie *cookie, struct fs_stat *stat)
{
    LTRACEF("cookie %p, stat %p\n", cookie, stat);

    spifs_t *spifs = (spifs_t *)cookie;

    stat->total_space = (uint64_t)spifs->dev->total_size;
    stat->free_space  = stat->total_space - used_space(spifs);

    stat->total_inodes = spifs->num_entries;
    stat->free_inodes  = stat->total_inodes - list_length(&spifs->files);

    return NO_ERROR;
}

static status_t spifs_ioctl_get_file_addr(filecookie *cookie, void **argp)
{
    LTRACEF("cookie %p, argp %p\n", cookie, argp);

    if (unlikely(!argp)) {
        return ERR_INVALID_ARGS;
    }

    status_t result;

    spifs_file_t *file = (spifs_file_t *)cookie;
    spifs_t *spifs = file->fs_handle;
    bdev_t *dev = spifs->dev;

    // Get the base address of the underlying BIO device.
    void *result_addr;
    result = bio_ioctl(dev, BIO_IOCTL_GET_MAP_ADDR, &result_addr);
    if (result != NO_ERROR) {
        return result;
    }

    // Get the offset of the file.
    result_addr += file->metadata.page_idx * spifs->page_size;
    *argp = result_addr;

    return NO_ERROR;
}

static status_t spifs_ioctl_is_linear(filecookie *cookie, void **argp)
{
    LTRACEF("cookie %p, argp %p\n", cookie, argp);

    if (unlikely(!argp)) {
        return ERR_INVALID_ARGS;
    }

    spifs_file_t *file = (spifs_file_t *)cookie;
    spifs_t *spifs = file->fs_handle;
    bdev_t *dev = spifs->dev;

    // Get the base address of the underlying BIO device.
    status_t result = bio_ioctl(dev, BIO_IOCTL_IS_MAPPED, argp);
    if (result != NO_ERROR) {
        return result;
    }

    return NO_ERROR;
}

static status_t spifs_file_ioctl(filecookie *cookie, int request, void *argp)
{
    LTRACEF("request %d, argp %p\n", request, argp);

    switch (request) {
        case FS_IOCTL_GET_FILE_ADDR: {
            return spifs_ioctl_get_file_addr(cookie, (void **)argp);
        }
        case FS_IOCTL_IS_LINEAR: {
            return spifs_ioctl_is_linear(cookie, (void **)argp);
        }
        default: {
            return ERR_NOT_SUPPORTED;
        }
    }
    return ERR_NOT_SUPPORTED;
}

static const struct fs_api spifs_api = {
    .format = spifs_format,
    .fs_stat = spifs_fs_stat,

    .mount = spifs_mount,
    .unmount = spifs_unmount,

    .create = spifs_create,
    .open = spifs_open,
    .remove = spifs_remove,
    .close = spifs_close,

    .read = spifs_read,
    .write = spifs_write,
    .truncate = spifs_truncate,

    .stat = spifs_stat,

    .file_ioctl = spifs_file_ioctl,

    .opendir = spifs_opendir,
    .readdir = spifs_readdir,
    .closedir = spifs_closedir,
};

STATIC_FS_IMPL(spifs, &spifs_api);
