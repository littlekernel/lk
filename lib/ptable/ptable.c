/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 * Copyright (c) 2014, Travis Geiselbrecht
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
#include <lib/ptable.h>
#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <err.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <list.h>
#include <lib/bio.h>
#include <lib/cksum.h>
#include <lk/init.h>

#define LOCAL_TRACE 0

#define PTABLE_MAGIC '1BTP'
#define PTABLE_MIN_ENTRIES 16
#define PTABLE_PART_NAME "ptable"

struct ptable_header {
    uint32_t magic;
    uint32_t crc32;         /* (total ptable according to total_length, 0 where crc field is) */
    uint32_t generation;    /* incremented by one every time its saved */
    uint32_t total_length;  /* valid length of table, only covers entries that are used */
};

struct ptable_mem_entry {
    struct list_node node;
    struct ptable_entry entry;
};

static struct ptable_state {
    bdev_t *bdev;
    uint32_t gen;
    struct list_node list;
} ptable;

#define PTABLE_HEADER_NUM_ENTRIES(header) (((header).total_length - sizeof(struct ptable_header)) / sizeof(struct ptable_entry))
#define BAIL(__err) do { err = __err; goto bailout; } while (0)

static inline size_t ptable_length(size_t entry_cnt)
{
    return sizeof(struct ptable_header) + (sizeof(struct ptable_entry) * entry_cnt);
}

static status_t validate_entry(const struct ptable_entry *entry)
{
    if (entry->offset > entry->offset + entry->length)
        return ERR_GENERIC;
    if (entry->offset + entry->length > (uint64_t)ptable.bdev->total_size)
        return ERR_GENERIC;

    uint i;
    for (i = 0; i < sizeof(entry->name); i++)
        if (entry->name[i] == 0)
            break;

    if (!i || (i >= sizeof(entry->name)))
        return ERR_GENERIC;

    return NO_ERROR;
}

static status_t ptable_write(void)
{
    uint8_t *buf = NULL;
    bdev_t *bdev = NULL;
    ssize_t err = ERR_GENERIC;

    if (!ptable_found_valid())
        return ERR_NOT_MOUNTED;

    bdev = bio_open(PTABLE_PART_NAME);
    if (!bdev)
        return ERR_BAD_STATE;

    /* count the number of entries in the list and calculate the total size */
    size_t count = 0;
    struct list_node *node;
    list_for_every(&ptable.list, node) {
        count++;
    }
    LTRACEF("%u entries\n", count);
    size_t total_length = sizeof(struct ptable_header) + sizeof(struct ptable_entry) * count;

    /* can we fit our partition table in our ptable subdevice? */
    if (total_length > bdev->total_size)
        BAIL(ERR_TOO_BIG);

    /* allocate a buffer to hold it */
    buf = malloc(total_length);
    if (!buf)
        BAIL(ERR_NO_MEMORY);

    /* fill in a default header */
    struct ptable_header *header = (struct ptable_header *)buf;
    header->magic = PTABLE_MAGIC;
    header->crc32 = 0;
    header->generation = ptable.gen++;
    header->total_length = total_length;

    /* start the crc calculation */
    header->crc32 = crc32(0, (void *)header, sizeof(*header));

    /* start by writing the entries */
    size_t off = sizeof(struct ptable_header);
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;

        memcpy(buf + off, entry, sizeof(struct ptable_entry));

        /* update the header */
        header->crc32 = crc32(header->crc32, (void *)entry, sizeof(struct ptable_entry));

        off += sizeof(struct ptable_entry);
    }

    /* write it to the block device.  If the device has an erase geometry, start
     * by erasing the partition.
     */
    if (bdev->geometry_count && bdev->geometry) {
        /* This is a subdevice, it should have a homogeneous erase geometry */
        DEBUG_ASSERT(1 == bdev->geometry_count);

        ssize_t err = bio_erase(bdev, 0, bdev->total_size);
        if (err != (ssize_t)bdev->total_size) {
            LTRACEF("error %d erasing device\n", (int)err);
            BAIL(ERR_IO);
        }
    }

    err = bio_write(bdev, buf, 0, total_length);
    if (err < (ssize_t)total_length) {
        LTRACEF("error %d writing data to device\n", (int)err);
        BAIL(ERR_IO);
    }

    LTRACEF("wrote ptable:\n");
    if (LOCAL_TRACE)
        hexdump(buf, total_length);

    err = NO_ERROR;

bailout:
    if (bdev)
        bio_close(bdev);

    free(buf);

    return err;
}

static void ptable_init(uint level)
{
    memset(&ptable, 0, sizeof(ptable));
    list_initialize(&ptable.list);
}

LK_INIT_HOOK(ptable, &ptable_init, LK_INIT_LEVEL_THREADING);

static void ptable_unpublish(struct ptable_mem_entry *mentry)
{
    if (mentry) {
        bdev_t *bdev;

        bdev = bio_open((char *)mentry->entry.name);
        if (bdev) {
            bio_unregister_device(bdev);
            bio_close(bdev);
        }

        if (list_in_list(&mentry->node))
            list_delete(&mentry->node);

        free(mentry);
    }
}

static void ptable_reset(void)
{
    /* walk through the partition list, clearing any entries */
    struct ptable_mem_entry *mentry;
    struct ptable_mem_entry *temp;
    list_for_every_entry_safe(&ptable.list, mentry, temp, struct ptable_mem_entry, node) {
        ptable_unpublish(mentry);
    }

    /* release our reference to our primary device */
    if (NULL != ptable.bdev)
        bio_close(ptable.bdev);

    /* Reset initialize our bookkeeping */
    ptable_init(LK_INIT_LEVEL_THREADING);
}

static void ptable_push_entry (struct ptable_mem_entry *mentry)
{
    DEBUG_ASSERT (mentry);

    // iterator for the list
    struct ptable_mem_entry *it_mentry;

    // The ptable list must be ordered by offset, so let's find the correct
    // spot for this entry
    list_for_every_entry(&ptable.list, it_mentry, struct ptable_mem_entry, node) {
        if (it_mentry->entry.offset > mentry->entry.offset) {
            // push the entry and we are done !
            list_add_before(&it_mentry->node, &mentry->node);
            // All done
            return;
        }
    }

    // if we exist the loop, that means that the
    // entry has not been added, let add it at the tail
    list_add_tail(&ptable.list, &mentry->node);
}

static status_t ptable_publish(const struct ptable_entry *entry)
{
    status_t err;
    struct ptable_mem_entry *mentry = NULL;

    DEBUG_ASSERT(entry && ptable.bdev);
    size_t block_mask = ((size_t)0x01 << ptable.bdev->block_shift) - 1;

    err = validate_entry(entry);
    if (err < 0) {
        LTRACEF("entry failed valid check\n");
        BAIL(ERR_NOT_FOUND);
    }

    // Make sure the partition does not already exist.
    const char *part_name = (const char *)entry->name;
    err = ptable_find(part_name, 0);
    if (err >= 0) {
        LTRACEF("entry \"%s\" already exists\n", part_name);
        BAIL(ERR_ALREADY_EXISTS);
    }

    // make sure that the partition is aligned properly
    if ((entry->offset & block_mask) || (entry->length & block_mask)) {
        LTRACEF("Entry in parition (\"%s\") is misaligned "
                "(off 0x%llx len 0x%llx blockmask 0x%zx\n",
                part_name, entry->offset, entry->length, block_mask);
        BAIL(ERR_BAD_STATE);
    }

    // make sure that length is non-zero and does not wrap
    if ((entry->offset + entry->length) <= entry->offset) {
        LTRACEF("Bad offset/length 0x%llx/0x%llx\n", entry->offset, entry->length);
        BAIL(ERR_INVALID_ARGS);
    }

    // make sure entry can fit in the device
    if ((entry->offset + entry->length) > (uint64_t)ptable.bdev->total_size) {
        LTRACEF("outside of device\n");
        BAIL(ERR_INVALID_ARGS);
    }

    /* create an in-memory copy and attempt to publish a subdevice for the
     * partition
     */
    mentry = calloc(1, sizeof(struct ptable_mem_entry));
    if (!mentry) {
        LTRACEF("Out of memory\n");
        BAIL(ERR_NO_MEMORY);
    }

    memcpy(&mentry->entry, entry, sizeof(struct ptable_entry));
    err = bio_publish_subdevice(ptable.bdev->name, part_name,
                                entry->offset >> ptable.bdev->block_shift,
                                entry->length >> ptable.bdev->block_shift);
    if (err < 0) {
        LTRACEF("Failed to publish subdevice for \"%s\"\n", part_name);
        goto bailout;
    }

    err = NO_ERROR;

bailout:
    /* If we failed to publish, clean up whatever we may have allocated.
     * Otherwise, put our new entry on the in-memory list.
     */
    if (err < 0) {
        ptable_unpublish(mentry);
    } else {
        ptable_push_entry (mentry);
    }

    return err;
}

static off_t ptable_adjust_request_for_erase_geometry(uint64_t  region_start,
        uint64_t  region_len,
        uint64_t *plength,
        bool      alloc_end)
{
    DEBUG_ASSERT(plength && ptable.bdev);

    LTRACEF("[0x%llx, 0x%llx) len 0x%llx%s\n",
            region_start, region_start + region_len, *plength, alloc_end ? " (alloc end)" : "");

    uint64_t block_mask = ((uint64_t)0x1 << ptable.bdev->block_shift) - 1;
    DEBUG_ASSERT(!(*plength & block_mask));
    DEBUG_ASSERT(!(region_start & block_mask));
    DEBUG_ASSERT(!(region_len & block_mask));

    uint64_t region_end = region_start + region_len;
    DEBUG_ASSERT(region_end >= region_start);

    // Can we fit in the region at all?
    if (*plength > region_len) {
        LTRACEF("Request too large for region (0x%llx > 0x%llx)\n", *plength, region_len);
        return ERR_TOO_BIG;
    }

    // If our block device does not have an erase geometry to obey, then great!
    // No special modifications to the request are needed.  Just determine the
    // offset based on if we are allocating from the start or the end.
    if (!ptable.bdev->geometry_count || !ptable.bdev->geometry) {
        off_t ret = alloc_end ? (region_start + region_len - *plength) : region_start;
        LTRACEF("No geometry; allocating at [0x%llx, 0x%llx)\n", ret, ret + *plength);
        return ret;
    }

    // Intersect each of the erase regions with the region being proposed and
    // see if we can fit the allocation request in the intersection, after
    // adjusting the intersection and requested length to multiples of and
    // aligned to the erase block size.  Test the geometries back-to-front
    // instead of front-to-back if alloc_end has been reqeusted.
    for (size_t i = 0; i < ptable.bdev->geometry_count; ++i) {
        size_t geo_index = alloc_end ?  (ptable.bdev->geometry_count - i - 1) : i;
        const bio_erase_geometry_info_t *geo = ptable.bdev->geometry + geo_index;
        uint64_t erase_mask = ((uint64_t)0x1 << geo->erase_shift) - 1;

        LTRACEF("Considering erase region [0x%llx, 0x%llx) (erase size 0x%zx)\n",
                geo->start, geo->start + geo->size, geo->erase_size);

        // If the erase region and the allocation region do not intersect at
        // all, just move on to the next region.
        if (!bio_does_overlap(region_start, region_len, geo->start, geo->size)) {
            LTRACEF("No overlap...\n");
            continue;
        }

        // Compute the intersection of the request region with the erase region.
        uint64_t erase_end = geo->start + geo->size;
        uint64_t rstart    = MAX(region_start, (uint64_t)geo->start);
        uint64_t rend      = MIN(region_end, erase_end);

        // Align to erase unit boundaries.  Move the start of the intersected
        // region up and the end of the intersected region down.
        rstart = (rstart + erase_mask) & ~erase_mask;
        rend = rend & ~erase_mask;

        // Round the requested length up to a multiple of the erase unit.
        uint64_t length = (*plength + erase_mask) & ~erase_mask;

        LTRACEF("Trimmed and aligned request [0x%llx, 0x%llx) len 0x%llx%s\n",
                rstart, rend, length, alloc_end ? " (alloc end)" : "");

        // Is there enough space in the aligned intersection to hold the
        // request?
        uint64_t tmp = rstart + length;
        if ((tmp < rstart) || (rend < tmp)) {
            LTRACEF("Not enough space\n");
            continue;
        }

        // Yay!  We found space for this allocation!  Adjust the requested
        // length and return the approprate offset based on whether we want to
        // allocate from the start or the end.
        off_t ret;
        *plength = length;
        ret      = alloc_end ? (rend - length) : rstart;
        LTRACEF("Allocating at [0x%llx, 0x%llx) (erase_size 0x%zx)\n",
                ret, ret + *plength, geo->erase_size);
        return ret;
    }

    // Looks like we didn't find a place to put this allocation.
    LTRACEF("No location found!\n");
    return ERR_INVALID_ARGS;
}

static off_t ptable_allocate(uint64_t *plength, uint flags)
{
    DEBUG_ASSERT(plength);

    if (!ptable.bdev)
        return ERR_BAD_STATE;

    LTRACEF("length 0x%llx, flags 0x%x\n", *plength, flags);

    uint64_t block_mask = ((uint64_t)0x1 << ptable.bdev->block_shift) - 1;
    uint64_t length = (*plength + block_mask) & ~block_mask;
    off_t offset = ERR_NOT_FOUND;
    bool alloc_end = 0 != (flags & FLASH_PTABLE_ALLOC_END);

    if (list_is_empty(&ptable.list)) {
        /* If the ptable is empty, then we have the entire device to use for
         * allocation.  Apply the erase geometry and return the result.
         */
        offset = ptable_adjust_request_for_erase_geometry(0,
                 ptable.bdev->total_size,
                 &length,
                 alloc_end);
        goto done;
    }

    const struct ptable_entry *lastentry = NULL;
    struct ptable_mem_entry *mentry;
    uint64_t region_start;
    uint64_t region_len;
    uint64_t test_len;
    off_t    test_offset;

    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;

        // Figure out the region we are testing, then adjust the request
        // based on the device erase geometry.
        region_start = lastentry ? (lastentry->offset + lastentry->length): 0;
        region_len   = entry->offset - region_start;
        DEBUG_ASSERT((int64_t)region_len >= 0);

        LTRACEF("Considering region [0x%llx, 0x%llx) between \"%s\" and \"%s\"\n",
                region_start,
                region_start + region_len,
                lastentry ? (char *)lastentry->name : "<device start>",
                entry->name);
        lastentry = entry;

        // Don't bother with the region if it is of zero length
        if (!region_len)
            continue;

        test_len = length;
        test_offset = ptable_adjust_request_for_erase_geometry(region_start,
                      region_len,
                      &test_len,
                      alloc_end);

        // If this region was no good, move onto the next one.
        if (test_offset < 0)
            continue;

        // We found a possible answer, go ahead and record it.  If we are
        // allocating from the front, then we are finished.  If we are
        // attempting to allocate from the back, keep looking to see if
        // there are other (better) answers.
        offset = test_offset;
        length = test_len;
        if (!alloc_end)
            goto done;
    }

    /* still looking... the final region to test goes from the end of the previous
     * region to the end of the device.
     */
    DEBUG_ASSERT(lastentry); /* should always have a valid tail */

    region_start = lastentry->offset + lastentry->length;
    region_len   = ptable.bdev->total_size - region_start;
    DEBUG_ASSERT((int64_t)region_len >= 0);

    if (region_len) {
        LTRACEF("Considering region [0x%llx, 0x%llx) between \"%s\" and \"%s\"\n",
                region_start,
                region_start + region_len,
                lastentry->name,
                "<device end>");
        test_len = length;
        test_offset = ptable_adjust_request_for_erase_geometry(region_start,
                      region_len,
                      &test_len,
                      alloc_end);
        if (test_offset >= 0) {
            offset = test_offset;
            length = test_len;
        }
    }

done:
    if (offset < 0) {
        LTRACEF("Failed to find a suitable region of at least length %llu (err %lld)\n",
                *plength, offset);
    } else {
        LTRACEF("Found region for %lld byte request @[%lld, %lld)\n",
                *plength, offset, offset + length);
        *plength = length;
    }

    return offset;
}

static status_t ptable_allocate_at(off_t _offset, uint64_t *plength)
{
    if (!ptable.bdev)
        return ERR_BAD_STATE;

    if ((_offset < 0) || !plength)
        return ERR_INVALID_ARGS;

    /* to make life easier, get our offset into unsigned */
    uint64_t offset = (uint64_t)_offset;

    /* Make certain the request was aligned to a program block boundary, and
     * adjust the length to be a multiple of program blocks in size.
     */
    uint64_t block_mask = ((uint64_t)0x1 << ptable.bdev->block_shift) - 1;
    if (offset & block_mask)
        return ERR_INVALID_ARGS;

    *plength = (*plength + block_mask) & ~block_mask;

    /* Make sure the request is contained within the extent of the device
     * itself.
     */
    if (!bio_contains_range(0, ptable.bdev->total_size, offset, *plength))
        return ERR_INVALID_ARGS;

    /* Adjust the request base on the erase geometry.  If the offset needs to
     * move to accomadate the erase geometry, we cannot satisfy this request.
     */
    uint64_t new_offset = ptable_adjust_request_for_erase_geometry(offset,
                          ptable.bdev->total_size - offset,
                          plength,
                          false);
    if (new_offset != offset)
        return ERR_INVALID_ARGS;

    /* Finally, check the adjusted request against all of the existing
     * partitions.  The final region may not overlap an of the existing
     * partitions.
     */
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;

        if (bio_does_overlap(offset, *plength, entry->offset, entry->length))
            return ERR_NOT_FOUND;
    }

    // Success.
    return NO_ERROR;
}

status_t ptable_scan(const char *bdev_name, uint64_t offset)
{
    ssize_t err;
    DEBUG_ASSERT(bdev_name);

    ptable_reset();

    /* Open a reference to the main block device */
    ptable.bdev = bio_open(bdev_name);
    if (NULL == ptable.bdev) {
        LTRACEF("Failed to find device \"%s\"", bdev_name);
        BAIL(ERR_NOT_FOUND);
    }

    /* validate the header */
    struct ptable_header header;

    err = bio_read(ptable.bdev, &header, offset, sizeof(header));
    if (err < (ssize_t)sizeof(header)) {
        LTRACEF("failed to read partition table header @%llu (%ld)\n", offset, err);
        goto bailout;
    }

    if (LOCAL_TRACE)
        hexdump(&header, sizeof(struct ptable_header));

    if (header.magic != PTABLE_MAGIC) {
        LTRACEF("failed magic test\n");
        BAIL(ERR_NOT_FOUND);
    }
    if (header.total_length < sizeof(struct ptable_header)) {
        LTRACEF("total length too short\n");
        BAIL(ERR_NOT_FOUND);
    }
    if (header.total_length > ptable.bdev->block_size) {
        LTRACEF("total length too long\n");
        BAIL(ERR_NOT_FOUND);
    }
    if (((header.total_length - sizeof(struct ptable_header)) % sizeof(struct ptable_entry)) != 0) {
        LTRACEF("total length not multiple of header + multiple of entry size\n");
        BAIL(ERR_NOT_FOUND);
    }

    /* start a crc check by calculating the header */
    uint32_t crc;
    uint32_t saved_crc = header.crc32;
    header.crc32 = 0;
    crc = crc32(0, (void *)&header, sizeof(header));
    header.crc32 = saved_crc;
    bool found_ptable = false;

    /* read the entries into memory */
    off_t off = offset + sizeof(struct ptable_header);
    for (uint i = 0; i < PTABLE_HEADER_NUM_ENTRIES(header); i++) {
        struct ptable_entry entry;

        /* read the next entry off the device */
        err = bio_read(ptable.bdev, &entry, off, sizeof(entry));
        if (err < 0) {
            LTRACEF("failed to read entry\n");
            goto bailout;
        }

        LTRACEF("looking at entry:\n");
        if (LOCAL_TRACE)
            hexdump(&entry, sizeof(entry));

        /* Attempt to publish the entry */
        err = ptable_publish(&entry);
        if (err < 0) {
            goto bailout;
        }

        /* If this was the "ptable" entry, was it in the right place? */
        if (!strncmp((char *)entry.name, PTABLE_PART_NAME, sizeof(entry.name))) {
            found_ptable = true;

            if (entry.offset != offset) {
                LTRACEF("\"ptable\" in the wrong location! (expected %lld got %lld)\n",
                        offset, entry.offset);
                BAIL(ERR_BAD_STATE);
            }
        }

        /* append the crc */
        crc = crc32(crc, (void *)&entry, sizeof(entry));

        /* Move on to the next entry */
        off += sizeof(struct ptable_entry);
    }

    if (header.crc32 != crc) {
        LTRACEF("failed crc check at the end (0x%08x != 0x%08x)\n", header.crc32, crc);
        BAIL(ERR_CRC_FAIL);
    }

    if (!found_ptable) {
        LTRACEF("\"ptable\" partition not found\n");
        BAIL(ERR_NOT_FOUND);
    }

    err = NO_ERROR;

bailout:
    if (err < 0)
        ptable_reset();

    return (status_t)err;
}

bool ptable_found_valid(void)
{
    return (NULL != ptable.bdev);
}

bdev_t *ptable_get_device(void)
{
    return ptable.bdev;
}

status_t ptable_find(const char *name, struct ptable_entry *_entry)
{
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;
        if (strcmp(name, (void *)entry->name) == 0) {
            /* copy the entry to the passed in pointer */
            if (_entry) {
                memcpy(_entry, entry, sizeof(struct ptable_entry));
            }

            return NO_ERROR;
        }
    }

    return ERR_NOT_FOUND;
}

status_t ptable_create_default(const char *bdev_name, uint64_t offset)
{
    DEBUG_ASSERT(bdev_name);

    /* Reset the system */
    ptable_reset();
    ptable.bdev = bio_open(bdev_name);
    if (!ptable.bdev) {
        LTRACEF("Failed to open \"%s\"\n", bdev_name);
        return ERR_NOT_FOUND;
    }

    /* See if we can put the partition table partition at the requested
     * location, and determine the size needed based on program block size and
     * erase block geometry.
     */
    uint64_t len = ptable_length(PTABLE_MIN_ENTRIES);
    status_t err = ptable_allocate_at(offset, &len);
    if (err < 0) {
        LTRACEF("Failed to allocate partition of len 0x%llx @ 0x%llx (err %d)\n",
                len, offset, err);
        goto bailout;
    }

    /* Publish the ptable partition */
    struct ptable_entry ptable_entry;
    ptable_entry.offset = offset;
    ptable_entry.length = len;
    ptable_entry.flags  = 0;
    strncpy((char *)ptable_entry.name, PTABLE_PART_NAME, sizeof(ptable_entry.name));
    err = ptable_publish(&ptable_entry);
    if (err < 0) {
        LTRACEF("Failed to publish ptable partition\n");
        goto bailout;
    }

    /* Commit the partition table to storage */
    err = ptable_write();
    if (err < 0) {
        LTRACEF("Failed to commit ptable\n");
        goto bailout;
    }

bailout:
    /* if we failed, reset the system. */
    if (err < 0)
        ptable_reset();

    return err;
}

status_t ptable_remove(const char *name)
{
    DEBUG_ASSERT(ptable.bdev);

    LTRACEF("name %s\n", name);

    if (!ptable_found_valid())
        return ERR_NOT_MOUNTED;

    if (!name)
        return ERR_INVALID_ARGS;

    if (!strcmp(name, "ptable"))
        return ERR_NOT_ALLOWED;

    bool found = false;
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;
        if (strcmp(name, (void *)entry->name) == 0) {
            ptable_unpublish(mentry);
            found = true;
            break;
        }
    }

    if (!found)
        return ERR_NOT_FOUND;

    /* rewrite the page table */
    status_t err = ptable_write();
    return err;
}

status_t ptable_add(const char *name, uint64_t min_len, uint32_t flags)
{
    LTRACEF("name %s min_len 0x%llx flags 0x%x\n", name, min_len, flags);

    if (!ptable_found_valid())
        return ERR_NOT_MOUNTED;

    /* see if the name is valid */
    if (strlen(name) > MAX_FLASH_PTABLE_NAME_LEN - 1) {
        LTRACEF("Name too long\n");
        return ERR_INVALID_ARGS;
    }

    // Find a place for the requested partition, adjust the length as needed
    off_t part_loc = ptable_allocate(&min_len, flags);
    if (part_loc < 0) {
        LTRACEF("Failed to usable find location.\n");
        return (status_t)part_loc;
    }

    /* Attempt to publish the partition */
    struct ptable_entry ptable_entry;
    ptable_entry.offset = part_loc;
    ptable_entry.length = min_len;
    ptable_entry.flags  = 0;
    strncpy((char *)ptable_entry.name, name, sizeof(ptable_entry.name));
    status_t err = ptable_publish(&ptable_entry);
    if (err < 0) {
        LTRACEF("Failed to publish\n");
        return err;
    }

    /* Commit the partition table */
    err = ptable_write();
    if (err < 0) {
        LTRACEF("Failed to commit ptable\n");
    }

    return err;
}

void ptable_dump(void)
{
    int i = 0;
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;

        printf("%d: %16s off 0x%016llx len 0x%016llx flags 0x%08x\n",
               i, entry->name, entry->offset, entry->length, entry->flags);
        i++;
    }
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_ptable(int argc, const cmd_args *argv)
{
    if (argc < 2) {
notenoughargs:
        printf("not enough arguments\n");
usage:
        printf("usage: %s scan <bio_device> <offset>\n", argv[0].str);
        printf("usage: %s default <bio_device> <offset>\n", argv[0].str);
        printf("usage: %s list\n", argv[0].str);
        printf("usage: %s add <name> <length> <flags>\n", argv[0].str);
        printf("usage: %s remove <name>\n", argv[0].str);
        printf("usage: %s alloc <len>\n", argv[0].str);
        printf("usage: %s allocend <len>\n", argv[0].str);
        printf("usage: %s write\n", argv[0].str);
        return -1;
    }

    status_t err;
    if (!strcmp(argv[1].str, "scan")) {
        if (argc < 4) goto notenoughargs;
        status_t err = ptable_scan(argv[2].str, argv[3].u);
        printf("ptable_scan returns %d\n", err);
    } else if (!strcmp(argv[1].str, "default")) {
        if (argc < 4) goto notenoughargs;
        status_t err = ptable_create_default(argv[2].str, argv[3].u);
        printf("ptable_create_default returns %d\n", err);
    } else if (!strcmp(argv[1].str, "list")) {
        ptable_dump();
    } else if (!strcmp(argv[1].str, "nuke")) {
        bdev_t *ptable_dev = bio_open(PTABLE_PART_NAME);

        if (ptable_dev) {
            status_t err;
            err = bio_erase(ptable_dev, 0, ptable_dev->total_size);
            if (err < 0) {
                printf("ptable nuke failed (err %d)\n", err);
            } else {
                printf("ptable nuke OK\n");
            }
            bio_close(ptable_dev);
        } else {
            printf("Failed to find ptable device\n");
        }
    } else if (!strcmp(argv[1].str, "add")) {
        if (argc < 5) goto notenoughargs;
        err = ptable_add(argv[2].str, argv[3].u, argv[4].u);
        if (err < NO_ERROR)
            printf("ptable_add returns err %d\n", err);
    } else if (!strcmp(argv[1].str, "remove")) {
        if (argc < 3) goto notenoughargs;
        ptable_remove(argv[2].str);
    } else if (!strcmp(argv[1].str, "alloc") ||
               !strcmp(argv[1].str, "allocend")) {
        if (argc < 3) goto notenoughargs;

        uint     flags = !strcmp(argv[1].str, "allocend") ? FLASH_PTABLE_ALLOC_END : 0;
        uint64_t len   = argv[2].u;
        off_t    off   = ptable_allocate(&len, flags);

        if (off < 0) {
            printf("%s of 0x%lx failed (err %lld)\n",
                   argv[1].str, argv[2].u, off);
        } else {
            printf("%s of 0x%lx gives [0x%llx, 0x%llx)\n",
                   argv[1].str, argv[2].u, off, off + len);
        }
    } else if (!strcmp(argv[1].str, "write")) {
        printf("ptable_write result %d\n", ptable_write());
    } else {
        goto usage;
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("ptable", "commands for manipulating the flash partition table", &cmd_ptable)
STATIC_COMMAND_END(ptable);

#endif // WITH_LIB_CONSOLE

