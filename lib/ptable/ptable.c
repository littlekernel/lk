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

struct ptable_header {
    uint32_t magic;
    uint32_t crc32;         /* (total ptable according to total_length, 0 where crc field is) */
    uint32_t generation;    /* incremented by one every time its saved */
    uint32_t total_length;  /* valid length of table, only covers entries that are used */
};

static struct ptable_state {
    bool valid;
    bdev_t *bdev;

    uint64_t offset;
    uint32_t gen;

    struct list_node list;
} ptable;

struct ptable_mem_entry {
    struct list_node node;
    struct ptable_entry entry;
};

#define PTABLE_HEADER_NUM_ENTRIES(header) (((header).total_length - sizeof(struct ptable_header)) / sizeof(struct ptable_entry))
#define ENTRY_NUM_TO_OFFSET(num) (sizeof(struct ptable_header) + sizeof(struct ptable_entry) * (num))
#define ENTRY_NUM_TO_ENTRY(header, num) ((struct ptable_entry *)(((uint8_t *)(header)) + ENTRY_NUM_TO_OFFSET(num)))

#define FOR_ALL_PTABLE_ENTRIES \
    struct ptable_entry *entry = (void *)(PTABLE_HEADER + 1); \
    for (uint i = 0; i < PTABLE_HEADER_NUM_ENTRIES(*PTABLE_HEADER); i++, entry++)

static status_t validate_entry(const struct ptable_entry *entry)
{
    if (entry->offset > entry->offset + entry->length)
        return ERR_GENERIC;
    if (entry->offset + entry->length > (uint64_t)ptable.bdev->size)
        return ERR_GENERIC;

    bool nullterm = false;
    for (uint i = 0; i < sizeof(entry->name); i++) {
        if (entry->name[i] == 0)
            nullterm = true;
    }
    if (!nullterm)
        return ERR_GENERIC;

    return NO_ERROR;
}

static status_t write_ptable(void)
{
    ptable.valid = false;

    /* count the number of entries in the list and calculate the total size */
    size_t count = 0;
    struct list_node *node;
    list_for_every(&ptable.list, node) {
        count++;
    }
    LTRACEF("%u entries\n", count);
    size_t total_length = sizeof(struct ptable_header) + sizeof(struct ptable_entry) * count;

    /* allocate a buffer to hold it */
    uint8_t *buf = malloc(total_length);
    if (!buf) {
        return ERR_NO_MEMORY;
    }

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

    /* write it to the block device */
    ssize_t err = bio_erase(ptable.bdev, ptable.offset, 4096); // XXX get erase size from bdev
    if (err < 4096) {
        TRACEF("error %d erasing device\n", (int)err);
        free(buf);
        return ERR_IO;
    }

    err = bio_write(ptable.bdev, buf, ptable.offset, total_length);
    if (err < (ssize_t)total_length) {
        TRACEF("error %d writing data to device\n", (int)err);
        free(buf);
        return ERR_IO;
    }

    LTRACEF("wrote ptable:\n");
    if (LOCAL_TRACE)
        hexdump(buf, total_length);

    free(buf);

    ptable.valid = true;

    return NO_ERROR;
}

static void clear_ptable_list(void)
{
    /* walk through the partition list, clearing all entries */
    struct ptable_mem_entry *mentry;
    struct ptable_mem_entry *temp;
    list_for_every_entry_safe(&ptable.list, mentry, temp, struct ptable_mem_entry, node) {
        list_delete(&mentry->node);
        free(mentry);
    }
}

static void ptable_init(uint level)
{
    ptable.valid = false;
    list_initialize(&ptable.list);
}

LK_INIT_HOOK(ptable, &ptable_init, LK_INIT_LEVEL_THREADING);

status_t ptable_scan(bdev_t *bdev, uint64_t offset)
{
    DEBUG_ASSERT(bdev);

    ptable.valid = false;
    ptable.bdev = bdev;
    ptable.offset = offset;

    /* validate the header */
    struct ptable_header header;

    ssize_t err = bio_read(bdev, &header, ptable.offset, sizeof(header));
    if (err < (ssize_t)sizeof(header)) {
        return err;
    }

    if (LOCAL_TRACE)
        hexdump(&header, sizeof(struct ptable_header));

    if (header.magic != PTABLE_MAGIC) {
        LTRACEF("failed magic test\n");
        return ERR_NOT_FOUND;
    }
    if (header.total_length < sizeof(struct ptable_header)) {
        LTRACEF("total length too short\n");
        return ERR_NOT_FOUND;
    }
    if (header.total_length > ptable.bdev->block_size) {
        LTRACEF("total length too long\n");
        return ERR_NOT_FOUND;
    }
    if (((header.total_length - sizeof(struct ptable_header)) % sizeof(struct ptable_entry)) != 0) {
        LTRACEF("total length not multiple of header + multiple of entry size\n");
        return ERR_NOT_FOUND;
    }

    /* start a crc check by calculating the header */
    uint32_t crc;
    uint32_t saved_crc = header.crc32;
    header.crc32 = 0;
    crc = crc32(0, (void *)&header, sizeof(header));
    header.crc32 = saved_crc;

    /* read the entries into memory */
    bool seen_ptable_entry = false;
    off_t off = ptable.offset + sizeof(struct ptable_header);
    for (uint i = 0; i < PTABLE_HEADER_NUM_ENTRIES(header); i++) {
        struct ptable_entry entry;

        /* read the next entry off the device */
        err = bio_read(bdev, &entry, off, sizeof(entry));
        if (err < 0) {
            LTRACEF("failed to read entry\n");
            return err;
        }

        LTRACEF("looking at entry:\n");
        if (LOCAL_TRACE)
            hexdump(&entry, sizeof(entry));

        status_t err = validate_entry(&entry);
        if (err < 0) {
            LTRACEF("entry failed valid check\n");
            return ERR_NOT_FOUND;
        }

        /* append the crc */
        crc = crc32(crc, (void *)&entry, sizeof(entry));

        /* if one of them is named "ptable", make sure it is in the right spot */
        if (!strcmp((char *)entry.name, "ptable")) {
            if (entry.offset == ptable.offset && entry.length == ptable.bdev->block_size) {
                seen_ptable_entry = true;
            }
        }

        /* create an in-memory copy */
        struct ptable_mem_entry *mentry;
        mentry = malloc(sizeof(struct ptable_mem_entry));
        if (!mentry) {
            clear_ptable_list();
            return ERR_NO_MEMORY;
        }

        memcpy(&mentry->entry, &entry, sizeof(struct ptable_entry));

        list_add_tail(&ptable.list, &mentry->node);

        off += sizeof(struct ptable_entry);
    }

    if (header.crc32 != crc) {
        TRACEF("failed crc check at the end\n");
        clear_ptable_list();
        return ERR_NOT_FOUND;
    }

    if (!seen_ptable_entry) {
        clear_ptable_list();
        return ERR_NOT_FOUND;
    }

    ptable.valid = true;

    return NO_ERROR;
}

bool ptable_found_valid(void)
{
    return ptable.valid;
}

bdev_t *ptable_get_device(void)
{
    return ptable.bdev;
}

status_t ptable_find(const char *name, struct ptable_entry *_entry)
{
    if (!ptable.valid)
        return ERR_NOT_FOUND;

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

status_t ptable_create_default(bdev_t *bdev, uint64_t offset)
{
    ptable.bdev = bdev;
    ptable.gen = 0;
    ptable.offset = offset;

    /* clear the old entries */
    clear_ptable_list();

    /* mark the ptable valid so ptable_add will continue */
    ptable.valid = true;

    /* create a new entry with a pointer to ourselves, and flush the table */
    status_t err = ptable_add("ptable", ptable.offset, ptable.bdev->block_size, 0);

    /* if we failed, make sure we're properly marked invalid */
    if (err < 0)
        ptable.valid = false;

    return err;
}

status_t ptable_remove(const char *name)
{
    DEBUG_ASSERT(ptable.bdev);

    LTRACEF("name %s\n", name);

    if (!ptable.valid)
        return ERR_INVALID_ARGS;
    if (!name)
        return ERR_INVALID_ARGS;

    if (!strcmp(name, "ptable"))
        return ERR_NOT_ALLOWED;

    bool found = false;
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;
        if (strcmp(name, (void *)entry->name) == 0) {
            list_delete(&mentry->node);
            found = true;
            break;
        }
    }

    if (!found)
        return ERR_NOT_FOUND;

    /* delete the entry */
    DEBUG_ASSERT(mentry);
    free(mentry);

    /* rewrite the page table */
    status_t err = write_ptable();
    return err;
}

static bool does_overlap(uint64_t offset1, uint64_t len1, uint64_t offset2, uint64_t len2)
{
    size_t end1 = offset1 + len1;
    size_t end2 = offset2 + len2;

    DEBUG_ASSERT(end1 >= offset1);
    DEBUG_ASSERT(end2 >= offset2);

    if (offset1 >= offset2 && offset1 < end2) {
        return true;
    }
    if (end1 > offset2 && end1 <= end2) {
        return true;
    }
    if (offset1 < offset2 && end1 > end2) {
        return true;
    }
    return false;
}

status_t ptable_add(const char *name, uint64_t offset, uint64_t len, uint32_t flags)
{
    status_t err;
    DEBUG_ASSERT(ptable.bdev);

    LTRACEF("name %s offset 0x%llx len 0x%llx flags 0x%x\n", name, offset, len, flags);

    if (!ptable.valid)
        return ERR_INVALID_ARGS;

    /* see if the name is valid */
    if (strlen(name) > MAX_FLASH_PTABLE_NAME_LEN - 1)
        return ERR_INVALID_ARGS;

    /* see if it already exists */
    if (ptable_find(name, NULL) == NO_ERROR)
        return ERR_ALREADY_EXISTS;

    /* see if the offset and length are aligned */
    if (!IS_ALIGNED(offset, ptable.bdev->block_size)) {
        LTRACEF("unaligned offset\n");
        return ERR_INVALID_ARGS;
    }

    /* check to see if it has a bogus size (0 len or wraparound) */
    if (offset >= offset + len) {
        LTRACEF("bogus size\n");
        return ERR_INVALID_ARGS;
    }

    /* make sure its within the bounds of the device */
    if (offset + len > (uint64_t)ptable.bdev->size) {
        LTRACEF("outside of device\n");
        return ERR_INVALID_ARGS;
    }

    len = ROUNDUP(len, ptable.bdev->block_size);

    /* try to find its slot */
    struct list_node *insert_before = NULL;
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;

        /* check to see if we overlap */
        if (does_overlap(offset, len, entry->offset, entry->length)) {
            LTRACEF("overlaps with existing partition\n");
            return ERR_INVALID_ARGS;
        }

        /* see if we fit before this entry */
        if (offset + len <= entry->offset) {
            insert_before = &mentry->node;
            break;
        }
    }

    /* create a struct */
    struct ptable_mem_entry *newentry = malloc(sizeof(struct ptable_mem_entry));
    if (!newentry)
        return ERR_NO_MEMORY;

    newentry->entry.offset = offset;
    newentry->entry.length = len;
    newentry->entry.flags = flags;
    memset(newentry->entry.name, 0, sizeof(newentry->entry.name));
    strlcpy((char *)newentry->entry.name, name, sizeof(newentry->entry.name));

    LTRACEF("new entry at %p\n", newentry);

    /* validate it */
    if (validate_entry(&newentry->entry) < 0) {
        free(newentry);
        return ERR_INVALID_ARGS;
    }

    /* add it to the list */
    if (insert_before)
        list_add_before(insert_before, &newentry->node);
    else
        list_add_tail(&ptable.list, &newentry->node);

    /* recalc crc and write */
    err = write_ptable();

    return err;
}

off_t ptable_allocate(uint64_t length, uint flags)
{
    DEBUG_ASSERT(ptable.bdev);

    LTRACEF("length 0x%llx, flags 0x%x\n", length, flags);

    if (!ptable.valid)
        return ERR_INVALID_ARGS;

    length = ROUNDUP(length, ptable.bdev->block_size);

    /* walk through the existing table making sure it's not a duplicate and where we'll put it */
    off_t offset = ERR_NOT_FOUND;

#define ALLOC_END (flags & FLASH_PTABLE_ALLOC_END)

    if (list_is_empty(&ptable.list)) {
        /* if the ptable is empty, see if we can simply fit in flash and alloc at the start or end */
        LTRACEF("empty list\n");
        if (length <= (uint64_t)ptable.bdev->size) {
            offset = ALLOC_END ? ptable.bdev->size - length : 0;
            LTRACEF("spot at 0x%llx\n", offset);
        }
    } else {
        const struct ptable_entry *lastentry = NULL;
        struct ptable_mem_entry *mentry;
        list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
            const struct ptable_entry *entry = &mentry->entry;
            LTRACEF("looking at entry %p: offset 0x%llx, length 0x%llx\n", entry, entry->offset, entry->length);
            if (!lastentry) {
                /* can it fit before the first one? */
                LTRACEF("first entry\n");
                if (entry->offset >= length) {
                    offset = ALLOC_END ? entry->offset - length : 0;
                    if (!ALLOC_END) goto done;
                }
            } else {
                LTRACEF("not first entry, lastentry %p: offset 0x%llx, length 0x%llx\n",
                        lastentry, lastentry->offset, lastentry->length);

                if (entry->offset - (lastentry->offset + lastentry->length) >= length) {
                    /* space between the last entry and this one */
                    offset = ALLOC_END ? entry->offset - length : lastentry->offset + lastentry->length;
                    if (!ALLOC_END) goto done;
                }
            }
            lastentry = entry;
        }

        /* didn't find a slot */

        /* see if we can fit off the end */
        DEBUG_ASSERT(lastentry); /* should always have a valid tail */

        if (lastentry->offset + lastentry->length + length <= (uint64_t)ptable.bdev->size)
            offset = ALLOC_END ? (uint64_t)ptable.bdev->size - length : lastentry->offset + lastentry->length;
    }

#undef ALLOC_END

done:
    LTRACEF("returning 0x%llx\n", offset);
    return offset;
}

off_t ptable_allocate_at(off_t _offset, uint64_t length)
{
    DEBUG_ASSERT(ptable.bdev);

    if (!ptable.valid)
        return ERR_INVALID_ARGS;

    if (_offset < 0)
        return ERR_INVALID_ARGS;

    /* to make life easier, get our offset into unsigned */
    uint64_t offset = (uint64_t)_offset;

    length = ROUNDUP(length, ptable.bdev->block_size);

    if (offset + length < offset || offset + length > (uint64_t)ptable.bdev->size)
        return ERR_INVALID_ARGS;

    /* check all ptable entries for overlap with the requested spot */
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;
        if (entry->offset < offset && entry->offset + entry->length > offset)
            return ERR_NOT_FOUND;

        if (entry->offset >= offset && entry->offset < offset + length)
            return ERR_NOT_FOUND;
    }

    return offset;
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
        printf("usage: %s add <name> <offset> <length> <flags>\n", argv[0].str);
        printf("usage: %s remove <name>\n", argv[0].str);
        printf("usage: %s alloc <len>\n", argv[0].str);
        printf("usage: %s allocend <len>\n", argv[0].str);
        return -1;
    }

    status_t err;
    if (!strcmp(argv[1].str, "scan")) {
        if (argc < 4) goto notenoughargs;
        bdev_t *bdev = bio_open(argv[2].str);
        if (!bdev) {
            printf("couldn't open bdev\n");
            return ERR_NOT_FOUND;
        }
        status_t err = ptable_scan(bdev, argv[3].u);
        printf("ptable_scan returns %d\n", err);
    } else if (!strcmp(argv[1].str, "default")) {
        if (argc < 4) goto notenoughargs;
        bdev_t *bdev = bio_open(argv[2].str);
        if (!bdev) {
            printf("couldn't open bdev\n");
            return ERR_NOT_FOUND;
        }
        status_t err = ptable_create_default(bdev, argv[3].u);
        printf("ptable_create_default returns %d\n", err);
    } else if (!strcmp(argv[1].str, "list")) {
        ptable_dump();
    } else if (!strcmp(argv[1].str, "nuke")) {
        bio_erase(ptable.bdev, ptable.offset, ptable.bdev->block_size);
    } else if (!strcmp(argv[1].str, "add")) {
        if (argc < 6) goto notenoughargs;
        err = ptable_add(argv[2].str, argv[3].u, argv[4].u, argv[5].u);
        if (err < NO_ERROR)
            printf("ptable_add returns err %d\n", err);
    } else if (!strcmp(argv[1].str, "remove")) {
        if (argc < 3) goto notenoughargs;
        ptable_remove(argv[2].str);
    } else if (!strcmp(argv[1].str, "alloc")) {
        if (argc < 3) goto notenoughargs;
        off_t off = ptable_allocate(argv[2].u, 0);
        printf("off 0x%llx\n", off);
    } else if (!strcmp(argv[1].str, "allocend")) {
        if (argc < 3) goto notenoughargs;
        off_t off = ptable_allocate(argv[2].u, FLASH_PTABLE_ALLOC_END);
        printf("off 0x%llx\n", off);
    } else {
        goto usage;
    }

    return 0;
}

STATIC_COMMAND_START
{ "ptable", "commands for manipulating the flash partition table", &cmd_ptable },
STATIC_COMMAND_END(ptable);

#endif // WITH_LIB_CONSOLE

