#include <debug.h>
#include <assert.h>
#include <err.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <list.h>
#include <dev/flash_nor.h>
#include <lib/cksum.h>
#include <lib/flash_ptable.h>

#define LOCAL_TRACE 0

#define PTABLE_MAGIC '0BTP'

struct ptable_header {
    uint32_t magic;
    uint32_t crc32;         /* (total ptable according to total_length, 0 where crc field is) */
    uint32_t generation;    /* incremented by one every time its saved */
    uint32_t total_length;  /* valid length of table, only covers entries that are used */
};

static struct ptable_state {
    bool valid;
    const struct flash_nor_bank *flash;
    size_t flash_offset;

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
    if (entry->offset + entry->length > ptable.flash->len)
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

    flash_nor_begin(ptable.flash->num);

    /* fill in a default header */
    struct ptable_header header;
    header.magic = PTABLE_MAGIC;
    header.crc32 = 0;
    header.generation = ptable.gen++;

    /* count the number of entries in the list and calculate the total size */
    size_t count = 0;
    struct list_node *node;
    list_for_every(&ptable.list, node) {
        count++;
    }
    LTRACEF("%u entries\n", count);
    header.total_length = sizeof(header) + sizeof(struct ptable_entry) * count;

    /* start the crc calculation */
    header.crc32 = crc32(header.crc32, (void *)((&header.crc32) + 1), sizeof(header) - 8);

    /* start by writing the entries */
    size_t off = sizeof(struct ptable_header);
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;
        flash_nor_write_pending(ptable.flash->num, ptable.flash_offset + off, sizeof(struct ptable_entry), entry);

        /* update the header */
        header.crc32 = crc32(header.crc32, (void *)entry, sizeof(struct ptable_entry));

        off += sizeof(struct ptable_entry);
    }

    /* write the header */
    flash_nor_write_pending(ptable.flash->num, ptable.flash_offset, sizeof(header), &header);

    /* flush it */
    flash_nor_flush(ptable.flash->num);

    flash_nor_end(ptable.flash->num);

    LTRACEF("wrote ptable:\n");
    LHEXDUMP(FLASH_PTR(ptable.flash, ptable.flash_offset), off);

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

status_t flash_ptable_init(void)
{
    ptable.valid = false;
    list_initialize(&ptable.list);

    return NO_ERROR;
}

static status_t validate_ptable(const struct ptable_header *header)
{
    /* validate the header */
    LTRACEF("looking at header at %p\n", header);

    LHEXDUMP(header, 256);

    if (header->magic != PTABLE_MAGIC)
        return ERR_NOT_FOUND;
    if (header->total_length < sizeof(struct ptable_header))
        return ERR_NOT_FOUND;
    if (header->total_length > ptable.flash->page_size)
        return ERR_NOT_FOUND;
    if (((header->total_length - sizeof(struct ptable_header)) % sizeof(struct ptable_entry)) != 0)
        return ERR_NOT_FOUND;

    /* crc check */
    uint32_t crc;

    crc = crc32(0, (void *)(&(header->crc32) + 1), header->total_length - 8);
    if (header->crc32 != crc)
        return ERR_NOT_FOUND;

    /* validate each entry */
    const struct ptable_entry *entry = (void *)(header + 1);
    for (uint i = 0; i < PTABLE_HEADER_NUM_ENTRIES(*header); i++) {
        LTRACEF("validating entry %p\n", &entry[i]);

        status_t err = validate_entry(&entry[i]);
        if (err < 0)
            return ERR_NOT_FOUND;
    }

    return NO_ERROR;
}

status_t flash_ptable_scan(const struct flash_nor_bank *flash)
{
    DEBUG_ASSERT(flash);

    ptable.valid = false;
    ptable.flash = flash;

    /* find the spot at the end of flash we'll need */
    ptable.flash_offset = ptable.flash->len - ptable.flash->page_size;

    LTRACEF("spot at 0x%x\n", ptable.flash_offset);

    /* validate the header */
    struct ptable_header *header = FLASH_PTR(ptable.flash, ptable.flash_offset);
    status_t err = validate_ptable(header);
    if (err < 0) {
        return err;
    }

    /* read the entries into memory */
    bool seen_ptable_entry = false;
    const struct ptable_entry *entry = (void *)(header + 1);
    for (uint i = 0; i < PTABLE_HEADER_NUM_ENTRIES(*header); i++) {
        status_t err = validate_entry(entry);
        if (err < 0)
            return ERR_NOT_FOUND;

        /* if one of them is named "ptable", make sure it is in the right spot */
        if (!strcmp((char *)entry->name, "ptable")) {
            if (entry->offset == ptable.flash_offset && entry->length == ptable.flash->page_size) {
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

        memcpy(&mentry->entry, entry, sizeof(struct ptable_entry));

        list_add_tail(&ptable.list, &mentry->node);

        entry++;
    }

    if (!seen_ptable_entry) {
        clear_ptable_list();
        return ERR_NOT_FOUND;
    }

    ptable.valid = true;

    return NO_ERROR;
}

bool flash_ptable_found_valid(void)
{
    return ptable.valid;
}

const struct flash_nor_bank *flash_ptable_get_flash_bank(void)
{
    return ptable.flash;
}

status_t flash_ptable_find(struct ptable_entry *_entry, const char *name)
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

status_t flash_ptable_create_default(const struct flash_nor_bank *bank)
{
    ptable.flash = bank;

    /* find the spot at the end of flash we'll need */
    ptable.flash_offset = ptable.flash->len - ptable.flash->page_size;
    LTRACEF("spot at 0x%x\n", ptable.flash_offset);

    ptable.gen = 0;

    /* clear the old entries */
    clear_ptable_list();

    /* mark the ptable valid so flash_ptable_add will continue */
    ptable.valid = true;

    /* create a new entry with a pointer to ourselves, and flush the table */
    status_t err = flash_ptable_add("ptable", ptable.flash_offset, ptable.flash->page_size, 0);

    /* if we failed, make sure we're properly marked invalid */
    if (err < 0)
        ptable.valid = false;

    return err;
}

status_t flash_ptable_remove(const char *name)
{
    DEBUG_ASSERT(ptable.flash);

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

static bool does_overlap(size_t offset1, size_t len1, size_t offset2, size_t len2)
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

status_t flash_ptable_add(const char *name, uint32_t offset, uint32_t len, uint32_t flags)
{
    status_t err;
    DEBUG_ASSERT(ptable.flash);

    LTRACEF("name %s offset 0x%x len 0x%x flags 0x%x\n", name, offset, len, flags);

    if (!ptable.valid)
        return ERR_INVALID_ARGS;

    /* see if the name is valid */
    if (strlen(name) > MAX_FLASH_PTABLE_NAME_LEN - 1)
        return ERR_INVALID_ARGS;

    /* see if it already exists */
    if (flash_ptable_find(NULL, name) == NO_ERROR)
        return ERR_ALREADY_EXISTS;

    /* see if the offset and length are aligned */
    if (!FLASH_NOR_IS_PAGE_ALIGNED(ptable.flash, offset)) {
        LTRACEF("unaligned offset\n");
        return ERR_INVALID_ARGS;
    }

    /* check to see if it has a bogus size (0 len or wraparound) */
    if (offset >= offset + len) {
        LTRACEF("bogus size\n");
        return ERR_INVALID_ARGS;
    }

    /* make sure its within the bounds of flash */
    if (offset + len > ptable.flash->len) {
        LTRACEF("outside of flash\n");
        return ERR_INVALID_ARGS;
    }

    len = ROUNDUP(len, ptable.flash->page_size);

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

ssize_t flash_ptable_allocate(size_t length, uint flags)
{
    DEBUG_ASSERT(ptable.flash);

    LTRACEF("length 0x%x, flags 0x%x\n", length, flags);

    if (!ptable.valid)
        return ERR_INVALID_ARGS;

    length = ROUNDUP(length, ptable.flash->page_size);

    /* walk through the existing table making sure it's not a duplicate and where we'll put it */
    ssize_t offset = ERR_NOT_FOUND;

#define ALLOC_END (flags & FLASH_PTABLE_ALLOC_END)

    if (list_is_empty(&ptable.list)) {
        /* if the ptable is empty, see if we can simply fit in flash and alloc at the start or end */
        LTRACEF("empty list\n");
        if (length <= ptable.flash->len) {
            offset = ALLOC_END ? ptable.flash->len - length : 0;
            LTRACEF("spot at 0x%x\n", offset);
        }
    } else {
        const struct ptable_entry *lastentry = NULL;
        struct ptable_mem_entry *mentry;
        list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
            const struct ptable_entry *entry = &mentry->entry;
            LTRACEF("looking at entry %p: offset 0x%x, length 0x%x\n", entry, entry->offset, entry->length);
            if (!lastentry) {
                /* can it fit before the first one? */
                LTRACEF("first entry\n");
                if (entry->offset >= length) {
                    offset = ALLOC_END ? entry->offset - length : 0;
                    if (!ALLOC_END) goto done;
                }
            } else {
                LTRACEF("not first entry, lastentry %p: offset 0x%x, length 0x%x\n",
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

        if (lastentry->offset + lastentry->length + length <= ptable.flash->len)
            offset = ALLOC_END ? ptable.flash->len - length : lastentry->offset + lastentry->length;
    }

#undef ALLOC_END

done:
    LTRACEF("returning 0x%x\n", offset);
    return offset;
}

ssize_t flash_ptable_allocate_at(size_t offset, size_t length)
{
    DEBUG_ASSERT(ptable.flash);

    if (!ptable.valid)
        return ERR_INVALID_ARGS;

    length = ROUNDUP(length, ptable.flash->page_size);

    if (offset + length < offset || offset + length > ptable.flash->len)
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

void flash_ptable_dump(void)
{
    int i = 0;
    struct ptable_mem_entry *mentry;
    list_for_every_entry(&ptable.list, mentry, struct ptable_mem_entry, node) {
        const struct ptable_entry *entry = &mentry->entry;

        printf("%d: %16s off 0x%08x len 0x%08x flags 0x%08x\n",
               i, entry->name, entry->offset, entry->length, entry->flags);
        i++;
    }
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_flash_ptable(int argc, const cmd_args *argv)
{
    if (argc < 2) {
notenoughargs:
        printf("not enough arguments\n");
usage:
        printf("usage: %s init\n", argv[0].str);
        printf("usage: %s scan <bank>\n", argv[0].str);
        printf("usage: %s default <bank>\n", argv[0].str);
        printf("usage: %s list\n", argv[0].str);
        printf("usage: %s add <name> <offset> <length> <flags>\n", argv[0].str);
        printf("usage: %s remove <name>\n", argv[0].str);
        printf("usage: %s alloc <len>\n", argv[0].str);
        printf("usage: %s allocend <len>\n", argv[0].str);
        return -1;
    }

    status_t err;
    if (!strcmp(argv[1].str, "init")) {
        flash_ptable_init();
    } else if (!strcmp(argv[1].str, "scan")) {
        if (argc < 2) goto notenoughargs;
        const struct flash_nor_bank *bank = flash_nor_get_bank(argv[1].u);
        status_t err = flash_ptable_scan(bank);
        printf("flash_ptable_scan returns %d\n", err);
    } else if (!strcmp(argv[1].str, "default")) {
        if (argc < 2) goto notenoughargs;
        const struct flash_nor_bank *bank = flash_nor_get_bank(argv[1].u);
        status_t err = flash_ptable_create_default(bank);
        printf("flash_ptable_create_default returns %d\n", err);
    } else if (!strcmp(argv[1].str, "list")) {
        flash_ptable_dump();
    } else if (!strcmp(argv[1].str, "nuke")) {
        flash_nor_erase_pages(ptable.flash->num, ptable.flash_offset, ptable.flash->page_size);
    } else if (!strcmp(argv[1].str, "default")) {
        err = flash_ptable_create_default(ptable.flash);
        printf("flash_ptable_create_default returns %d\n", err);
    } else if (!strcmp(argv[1].str, "add")) {
        if (argc < 6) goto notenoughargs;
        err = flash_ptable_add(argv[2].str, argv[3].u, argv[4].u, argv[5].u);
        if (err < NO_ERROR)
            printf("flash_ptable_add returns err %d\n", err);
    } else if (!strcmp(argv[1].str, "remove")) {
        if (argc < 3) goto notenoughargs;
        flash_ptable_remove(argv[2].str);
    } else if (!strcmp(argv[1].str, "alloc")) {
        if (argc < 3) goto notenoughargs;
        ssize_t off = flash_ptable_allocate(argv[2].u, 0);
        printf("off 0x%x\n", off);
    } else if (!strcmp(argv[1].str, "allocend")) {
        if (argc < 3) goto notenoughargs;
        ssize_t off = flash_ptable_allocate(argv[2].u, FLASH_PTABLE_ALLOC_END);
        printf("off 0x%x\n", off);
    } else {
        goto usage;
    }

    return 0;
}

STATIC_COMMAND_START
{ "flash_ptable", "commands for manipulating the flash partition table", &cmd_flash_ptable },
STATIC_COMMAND_END(flash_ptable);

#endif // WITH_LIB_CONSOLE

