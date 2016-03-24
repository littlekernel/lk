/*
 * Copyright (c) 2013 Heather Lee Wilson
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
#include <dev/flash_nor.h>
#include <lib/norfs.h>
#include <lib/norfs_inode.h>
#include <lib/norfs_config.h>
#include <iovec.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <platform.h>
#include <lib/cksum.h>
#include <platform/flash_nor_config.h>
#include <list.h>
#include <debug.h>

/* FRIEND_TEST non-static if unit testing, in order to
 * allow functions to be exposed by a test header file.
 */
#ifdef WITH_LIB_UNITTEST
#define FRIEND_TEST
#else
#define FRIEND_TEST static
#endif

#define member_size(type, member) sizeof(((type *)0)->member)


/* Handle rolling over of version field beyond max(uint16_t) by assuming that
 * the difference between the smallest and largest versions are no more than
 * half the range of uint16_t.  By subtracting one unsigned uint16_t from
 * another, then casting the delta to int16_t, if the delta is greater than
 * 1/2 * max(uint16_t), then the most significant bit will be a 1, causing the
 * signed integer to be negative and VERSION_GREATER_THAN to return false.
 */
#define VERSION_GREATER_THAN(a, b) ((int16_t)((a) - (b)) > 0)

struct norfs_header {
    uint32_t key;
    uint16_t version;
    uint16_t len;
    uint8_t flags;
    uint16_t crc;
};

/* Block header written after successful erase. */
FRIEND_TEST const unsigned char NORFS_BLOCK_HEADER[4] = {'T', 'O', 'F', 'U'};
/* Block header to indicate garbage collection has started. */
FRIEND_TEST  const unsigned char NORFS_BLOCK_GC_STARTED_HEADER[2] = {'S', 'O'};
/* Block header to indicate block was not interrupted during garbage
 * collection.
 */
FRIEND_TEST const unsigned char NORFS_BLOCK_GC_FINISHED_HEADER[2] = {'U', 'P'};

FRIEND_TEST uint32_t write_pointer = 0;
FRIEND_TEST uint32_t total_remaining_space = NORFS_AVAILABLE_SPACE;
FRIEND_TEST uint8_t num_free_blocks = 0;
static bool fs_mounted = false;
FRIEND_TEST uint32_t norfs_nvram_offset;
static struct list_node inode_list;

static bool block_free[NORFS_NUM_BLOCKS];

static status_t collect_garbage(void);
static status_t load_and_verify_obj(uint32_t *ptr, struct norfs_header *header);

FRIEND_TEST uint8_t block_num(uint32_t flash_pointer)
{
    return flash_pointer/FLASH_PAGE_SIZE;
}

/* Update pointer to a free block.  If no free blocks, return error. */
FRIEND_TEST status_t find_free_block(uint32_t *ptr)
{
    uint8_t i = block_num(*ptr) + 1;
    uint8_t imod;
    for (uint8_t j = 0;  j < NORFS_NUM_BLOCKS; i++, j++) {
        imod  = i % NORFS_NUM_BLOCKS;
        /* If is a free block, return. */
        if (block_free[imod]) {
            *ptr = imod * FLASH_PAGE_SIZE + sizeof(NORFS_BLOCK_HEADER);
            return NO_ERROR;
        }
    }
    /* A free block could not be found. */
    return ERR_NO_MEMORY;
}

static uint32_t curr_block_free_space(uint32_t pointer)
{
    return (block_num(pointer) + 1) * FLASH_PAGE_SIZE - pointer;
}

static bool block_full(uint8_t block, uint32_t ptr)
{
    if (block != block_num(ptr)) {
        return true;
    }
    return curr_block_free_space(ptr) < NORFS_OBJ_OFFSET;
}

static uint8_t select_garbage_block(uint32_t ptr)
{
    return (block_num(ptr) + 1) % 8;
}

static ssize_t nvram_read(size_t offset, size_t length, void *ptr)
{
    return flash_nor_read(NORFS_BANK, offset + norfs_nvram_offset, length, ptr);
}

static ssize_t nvram_write(size_t offset, size_t length, const void *ptr)
{
    return flash_nor_write(NORFS_BANK, offset + norfs_nvram_offset, length,
                           ptr);
}

static ssize_t nvram_erase_pages(size_t offset, size_t length)
{
    return flash_nor_erase_pages(NORFS_BANK, offset + norfs_nvram_offset,
                                 length);
}

unsigned char *nvram_flash_pointer(uint32_t loc)
{
    return FLASH_PTR(flash_nor_get_bank(NORFS_BANK), loc + norfs_nvram_offset);
}

FRIEND_TEST bool get_inode(uint32_t key, struct norfs_inode **inode)
{
    struct list_node *curr_lnode;
    struct norfs_inode *curr_inode;
    uint32_t curr_key;

    if (!inode)
        return false;

    *inode = NULL;
    list_for_every(&inode_list, curr_lnode) {
        curr_inode = containerof(curr_lnode, struct norfs_inode, lnode);
        nvram_read(curr_inode->location + NORFS_KEY_OFFSET,
                   sizeof(curr_key), &curr_key);
        if (curr_key == key) {
            *inode = curr_inode;
            return true;
        }
    }
    return false;
}

static uint16_t calculate_header_crc(uint32_t key, uint16_t version,
                                     uint16_t len, uint8_t flags)
{
    uint16_t crc = crc16((unsigned char *) &key, sizeof(key));
    crc = update_crc16(crc, (unsigned char *) &version, sizeof(version));
    crc = update_crc16(crc, (unsigned char *) &len, sizeof(len));
    crc = update_crc16(crc, (unsigned char *) &flags, sizeof(flags));
    return crc;
}

/* Read header into parameter buffers. Return bytes written. */
static ssize_t read_header(uint32_t ptr, struct norfs_header *header)
{
    ssize_t total_bytes_read = 0;
    ssize_t bytes_read = 0;
    bytes_read = nvram_read(ptr + NORFS_KEY_OFFSET,
                            sizeof(header->key), &header->key);
    if (bytes_read < 0) {
        return bytes_read;
    }
    total_bytes_read += bytes_read;
    bytes_read = nvram_read(ptr + NORFS_VERSION_OFFSET,
                            sizeof(header->version), &header->version);
    if (bytes_read < 0) {
        return bytes_read;
    }
    total_bytes_read += bytes_read;
    bytes_read = nvram_read(ptr + NORFS_LENGTH_OFFSET,
                            sizeof(header->len), &header->len);
    if (bytes_read < 0) {
        return bytes_read;
    }
    total_bytes_read += bytes_read;
    bytes_read = nvram_read(ptr + NORFS_FLAGS_OFFSET,
                            sizeof(header->flags), &header->flags);
    if (bytes_read < 0) {
        return bytes_read;
    }
    total_bytes_read += bytes_read;

    /* Increment pointer by one byte to account for unused padding. */
    total_bytes_read += 1;
    bytes_read = nvram_read(ptr + NORFS_CHECKSUM_OFFSET,
                            sizeof(header->crc), &header->crc);
    if (bytes_read < 0) {
        return bytes_read;
    }
    total_bytes_read += bytes_read;

    return total_bytes_read;
}

status_t norfs_read_obj_iovec(uint32_t key, iovec_t *obj_iov,
                              uint32_t iov_count, size_t *bytes_read, uint8_t flags)
{
    if (!fs_mounted)
        return ERR_NOT_MOUNTED;

    uint32_t read_ptr;
    uint16_t to_read, total_to_read;
    struct norfs_inode *inode;
    struct norfs_header stored_header;
    int16_t bytes = 0;
    uint8_t i = 0;
    ssize_t iov_size = iovec_size(obj_iov, iov_count);

    if (bytes_read) {
        *bytes_read = 0;
    }

    if (!get_inode(key, &inode)) {
        return ERR_NOT_FOUND;
    }
    read_ptr = inode->location;
    bytes = read_header(read_ptr, &stored_header);

    total_to_read = MIN(stored_header.len, iov_size);
    if (bytes < 0) {
        TRACEF("Error reading header. Status: %d\n", bytes);
        return bytes;
    }
    read_ptr += bytes;

    if (stored_header.flags & NORFS_DELETED_MASK) {
        TRACEF("Object is already deleted.\n");
        return ERR_NOT_FOUND;
    }

    while (*bytes_read < total_to_read) {
        to_read = MIN(total_to_read - *bytes_read, obj_iov[i].iov_len);
        bytes = nvram_read(read_ptr + *bytes_read,
                           to_read, obj_iov[i].iov_base);
        if (bytes < 0) {
            TRACEF("Read failed with error: %d\n", bytes);
            return bytes;
        }
        if (bytes_read) {
            *bytes_read += bytes;
        }
        i++;
    }

    return NO_ERROR;
}

static status_t write_obj_header(uint32_t *ptr, uint32_t key, uint16_t version,
                                 uint16_t len, uint8_t flags, uint16_t crc)
{
    unsigned char buff[WORD_SIZE];
    int bytes_written;

    bytes_written = nvram_write(*ptr, sizeof(key), &key);
    if (bytes_written < 0) {
        return bytes_written;
    }
    *ptr += bytes_written;

    memcpy(buff, &version, sizeof(version));
    memcpy(buff + sizeof(version), &len, sizeof(len));
    bytes_written = nvram_write(*ptr, sizeof(buff), &buff);
    if (bytes_written < 0) {
        return bytes_written;
    }
    *ptr += bytes_written;

    memcpy(buff, &flags, sizeof(flags));
    memset(buff + 1, 1, sizeof(flags));
    memcpy(buff + 2, &crc, sizeof(crc));
    bytes_written = nvram_write(*ptr, sizeof(buff), &buff);
    if (bytes_written < 0) {
        return bytes_written;
    }
    *ptr += bytes_written;
    return NO_ERROR;
}

static status_t copy_iovec_to_disk(const struct iovec *iov, uint16_t iov_count,
                                   uint32_t *location, uint16_t *crc)
{
    unsigned char word[4] = {0};
    uint16_t iov_ptr = 0;
    uint16_t word_ptr = 0;
    uint8_t word_size = sizeof(word);
    int bytes_written;
    for (uint16_t i = 0; i < iov_count; i++) {
        while ((uint16_t)iov[i].iov_len - iov_ptr > word_size - word_ptr) {
            memcpy(word + word_ptr,
                   (unsigned char *)iov[i].iov_base + iov_ptr, word_size -
                   word_ptr);
            iov_ptr += word_size - word_ptr;

            bytes_written = nvram_write(*location, sizeof(word),
                                        &word);
            if (bytes_written < 0) {
                TRACEF("Error while writing: %d\n", bytes_written);
                return bytes_written;
            }

            *location += bytes_written;
            *crc = update_crc16(*crc, word, sizeof(word));
            memset(word, 0, sizeof(word));
            word_ptr = 0;
        }

        memcpy((unsigned char *)word + word_ptr,
               (unsigned char *)iov[i].iov_base + iov_ptr,
               iov[i].iov_len - iov_ptr);
        word_ptr += iov[i].iov_len - iov_ptr;

        iov_ptr = 0;
    }

    if (word_ptr > 0) {
        bytes_written = nvram_write(*location, sizeof(word),
                                    &word);

        if (bytes_written < 0) {
            TRACEF("Error writing: %d\n", bytes_written);
            return bytes_written;
        }

        *location += bytes_written;
        *crc = update_crc16(*crc, word, word_ptr);
    }
    return NO_ERROR;
}

static status_t initialize_next_block(uint32_t *ptr)
{
    uint32_t header_pointer;
    ssize_t bytes_written;
    status_t status;

    /* Update write pointer. */
    status = find_free_block(ptr);
    if (status) {
        TRACEF("Error finding free block.  Status: %d\n", status);
        return status;
    }

    num_free_blocks--;
    block_free[block_num(*ptr)] = false;
    bytes_written = nvram_write(*ptr,
                                sizeof(NORFS_BLOCK_GC_STARTED_HEADER), &NORFS_BLOCK_GC_STARTED_HEADER);

    if (bytes_written < 0) {
        TRACEF("Error writing header.\n");
        return bytes_written;
    }

    header_pointer = *ptr + bytes_written;
    *ptr += sizeof(NORFS_BLOCK_GC_STARTED_HEADER) +
            sizeof(NORFS_BLOCK_GC_FINISHED_HEADER);

    if (num_free_blocks < NORFS_MIN_FREE_BLOCKS) {
        status = collect_garbage();
        if (status) {
            TRACEF("Failed to collection garbage.  Error: %d\n.",
                   status);
            return status;
        }
    }

    status = nvram_write(header_pointer,
                         sizeof(NORFS_BLOCK_GC_FINISHED_HEADER),
                         &NORFS_BLOCK_GC_FINISHED_HEADER);
    if (status < 0) {
        TRACEF("Failed to write header.\n");
        return status;
    }

    return NO_ERROR;
}

status_t write_obj_iovec(const iovec_t *iov, uint iov_count, uint *location,
                         uint32_t key, uint16_t version, uint8_t flags)
{
    uint16_t crc = 0;
    uint16_t len = iovec_size(iov, iov_count);
    status_t status;

    uint32_t header_loc = *location;
    *location += NORFS_OBJ_OFFSET;
    crc = calculate_header_crc(key, version, len, flags);

    if (!(flags & NORFS_DELETED_MASK)) {
        /* If object is not a deletion header, write object bytes to disk. */
        status = copy_iovec_to_disk(iov, iov_count, location, &crc);
        if (status) {
            TRACEF("Error writing.  Status: %d\n", status);
            return status;
        }
    }

    status = write_obj_header(&header_loc, key, version, len, flags, crc);
    if (status) {
        TRACEF("Error writing.  Status: %d\n", status);
        return status;
    }

    return NO_ERROR;
}

status_t norfs_read_obj(uint32_t key, unsigned char *buffer,
                        uint16_t buffer_len, size_t *bytes_read,
                        uint8_t flags)
{
    if (!fs_mounted)
        return ERR_NOT_MOUNTED;

    struct iovec vec[1];
    vec->iov_base = buffer;
    vec->iov_len = buffer_len;
    status_t status = norfs_read_obj_iovec(key, vec, 1, bytes_read, flags);

    return status;
}

status_t norfs_put_obj(uint32_t key, unsigned char *obj, uint16_t len,
                       uint8_t flags)
{
    if (!fs_mounted)
        return ERR_NOT_MOUNTED;

    if (key == 0xFFFF) {
        return ERR_INVALID_ARGS;
    }
    status_t status;
    struct iovec const vec[1] = {{obj, len}};
    status = norfs_put_obj_iovec(key, vec, 1, flags);
    return status;
}

bool is_deleted(uint32_t loc)
{
    uint8_t flags;
    nvram_read(loc + NORFS_FLAGS_OFFSET, sizeof(flags), &flags);
    return NORFS_DELETED_MASK & flags;
}

status_t norfs_remove_obj(uint32_t key)
{
    if (!fs_mounted)
        return ERR_NOT_MOUNTED;

    struct norfs_inode *inode;
    uint16_t prior_len;
    struct iovec iov[1];
    status_t status;
    bool success = get_inode(key, &inode);
    if (!success || is_deleted(inode->location))
        return ERR_NOT_FOUND;

    status = nvram_read(inode->location + NORFS_LENGTH_OFFSET,
                        sizeof(uint16_t), &prior_len);
    if (status < 0) {
        TRACEF("Failed to read during norfs_remove_obj.  Status: %d\n", status);
        return status;
    }

    iov->iov_base = NULL;
    iov->iov_len = 0;

    /*
     * Write a deleted object by passing a null iovec pointer.  Only header
     * will be written.
     */
    status = norfs_put_obj_iovec(key, iov, 0, NORFS_DELETED_MASK);
    if (status)
        TRACEF("Error putting object. %d\n", status);

    return status;
}

static status_t find_space_for_object(uint16_t obj_len, uint32_t *ptr)
{
    status_t status;
    uint8_t initial_block_num = block_num(*ptr);
    while (curr_block_free_space(*ptr) < (uint16_t) NORFS_FLASH_SIZE(obj_len)) {
        status = initialize_next_block(ptr);
        if (status)
            return status;
        /* If we have already tried and failed to write to this block, exit. */
        if (block_num(*ptr) == initial_block_num)
            return ERR_NO_MEMORY;
    }
    return NO_ERROR;
}

/*
 * Store an object in flash.  Moves write_pointer to new block and garbage
 * collects if needed.  If write fails, will reattempt.
 * How to handle write failures is not fully defined - at the moment I stop once
 * find_free_block is attempting to rewrite to a block it has already failed to
 * write to - after a full loop in a round-robin style garbage selection of
 * blocks.  Which is a lot of write attempts.
 */
status_t norfs_put_obj_iovec(uint32_t key, const iovec_t *iov,
                             uint32_t iov_count, uint8_t flags)
{
    if (!fs_mounted)
        return ERR_NOT_MOUNTED;

    if (key == 0xFFFF) {
        return ERR_INVALID_ARGS;
    }

    uint8_t block_num_to_write;
    struct norfs_inode *inode;
    uint16_t len = iovec_size(iov, iov_count);
    status_t status;
    uint8_t stored_flags;
    uint16_t version = 0;
    uint32_t header_loc;
    bool deletion = flags & NORFS_DELETED_MASK;
    bool obj_preexists = get_inode(key, &inode);
    if (obj_preexists) {
        nvram_read(inode->location + NORFS_FLAGS_OFFSET,
                   sizeof(stored_flags), &stored_flags);
        if (stored_flags & flags & NORFS_DELETED_MASK) {
            /* Attempting to delete an object no longer in filesystem. */
            TRACEF("The object attempting to be removed has already been \
			deleted. stored_flags: 0x%x\n", stored_flags);
            return ERR_NOT_FOUND;
        }

        nvram_read(inode->location + NORFS_VERSION_OFFSET,
                   sizeof(version), &version);
        version++;
    } else if (deletion) {
        /* Attempting to delete a non-existent object. */
        TRACEF("Attempting to remove an object not in filesystem.\n");
        return ERR_NOT_FOUND;
    } else {
        inode = malloc(sizeof(struct norfs_inode));
        inode->reference_count = 1;
    }

    flash_nor_begin(NORFS_BANK);

    if (NORFS_FLASH_SIZE(len) > NORFS_MAX_OBJ_LEN) {
        TRACEF("Object too big.  Not adding.\n");
        flash_nor_end(NORFS_BANK);
        return ERR_TOO_BIG;
    }

    if (!deletion && (NORFS_FLASH_SIZE(len) > total_remaining_space)) {
        TRACEF("Not enough remaining space.  Remaining space:  %d\tObject len: \
			    %d\n", total_remaining_space, len);
        flash_nor_end(NORFS_BANK);
        return ERR_NO_MEMORY;
    }

    status = find_space_for_object(len, &write_pointer);
    if (status) {
        TRACEF("Error finding space for object.  Status: %d\n", status);
        flash_nor_end(NORFS_BANK);
        return ERR_IO;
    }

    block_num_to_write = block_num(write_pointer);
    header_loc = write_pointer;
    status = write_obj_iovec(iov, iov_count, &write_pointer, key,
                             version, flags);
    if (!status) {
        if (!obj_preexists) {
            list_add_tail(&inode_list, &inode->lnode);
        } else {
            /* If object preexists, remove outdated version from remaining space. */
            uint16_t prior_len;
            nvram_read(inode->location + NORFS_LENGTH_OFFSET,
                       sizeof(uint16_t), &prior_len);
            total_remaining_space += NORFS_FLASH_SIZE(prior_len);
            inode->reference_count++;
        }
        inode->location = header_loc;
        total_remaining_space -= NORFS_FLASH_SIZE(len);
    } else {
        TRACEF("Error writing object. Status: %d\n", status);
    }

    /* If write error, or if fell off block, find new block. */
    if (status < 0 || block_num(write_pointer) != block_num_to_write) {
        initialize_next_block(&write_pointer);
    }

    flash_nor_end(NORFS_BANK);
    return status;
}

static void remove_inode(struct norfs_inode *inode)
{
    if (!inode)
        return;
    list_delete(&inode->lnode);
    free(inode);
    inode = NULL;
}

/*  Verifies objects, and copies to new block if it is the latest version. */
static status_t collect_garbage_object(uint32_t *garbage_read_pointer,
                                       uint32_t *garbage_write_pointer)
{
    struct norfs_inode *inode;
    struct norfs_header header;
    bool inode_found;
    status_t status;
    struct iovec iov[1];
    uint32_t new_obj_loc;
    uint32_t garb_obj_loc = *garbage_read_pointer;
    status = load_and_verify_obj(garbage_read_pointer, &header);
    if (status) {
        TRACEF("Failed to load garbage_obj at %d\n", *garbage_read_pointer);
        return status;
    }
    inode_found = get_inode(header.key, &inode);
    if (inode_found) {
        if (garb_obj_loc == inode->location) {
            /* Object in garbage block is latest version. */
            if (header.flags & NORFS_DELETED_MASK && (inode->reference_count == 1)) {
                /* If last version of object, remove. */
                remove_inode(inode);
                total_remaining_space += NORFS_OBJ_OFFSET;
                return NO_ERROR;
            }
            iov->iov_base = nvram_flash_pointer(garb_obj_loc + NORFS_OBJ_OFFSET);
            iov->iov_len = header.len;
            new_obj_loc = *garbage_write_pointer;
            status = write_obj_iovec(iov, 1, garbage_write_pointer, header.key,
                                     header.version + 1, header.flags);
            if (status) {
                TRACEF("Failed to copy garbage object.  Status: %d\n", status);
                return status;
            }
            inode->location = new_obj_loc;
            return NO_ERROR;
        } else {
            inode->reference_count--;
            return NO_ERROR;
        }
    }

    if (is_deleted(*garbage_read_pointer)) {
        return NO_ERROR;
    }
    return ERR_NOT_FOUND;
}

static status_t erase_block(uint8_t block)
{
    ssize_t bytes_erased;
    ssize_t bytes_written;
    status_t status;
    uint32_t loc = block * FLASH_PAGE_SIZE;

    /* Block must fall within range of actual number of NVRAM blocks. */
    if (block > NORFS_NUM_BLOCKS) {
        TRACEF("Invalid block number: %d.\n", block);
        return ERR_INVALID_ARGS;
    }
    status = flash_nor_begin(NORFS_BANK);
    if (status) {
        flash_nor_end(NORFS_BANK);
        return status;
    }

    bytes_erased = nvram_erase_pages(loc, FLASH_PAGE_SIZE);
    if (bytes_erased != FLASH_PAGE_SIZE) {
        flash_nor_end(NORFS_BANK);
        TRACEF("Did not erase exactly one flash page.  Something went \
				wrong.\n");
        return ERR_IO;
    }

    bytes_written = nvram_write(loc, sizeof(NORFS_BLOCK_HEADER),
                                &NORFS_BLOCK_HEADER);

    flash_nor_end(NORFS_BANK);
    if (bytes_written < 0) {
        TRACEF("Error during nvram_write.  Status: %d\n", bytes_written);
        return bytes_written;
    }
    block_free[block] = true;
    num_free_blocks++;

    return NO_ERROR;
}

FRIEND_TEST status_t collect_block(uint32_t garbage_block,
                                   uint32_t *garbage_write_ptr)
{
    status_t status;
    uint32_t garbage_read_ptr = garbage_block * FLASH_PAGE_SIZE +
                                NORFS_BLOCK_HEADER_SIZE;

    while (!(block_full(garbage_block, garbage_read_ptr))) {
        status = collect_garbage_object(&garbage_read_ptr, garbage_write_ptr);
        if (status) {
            break;
        }
    }
    return erase_block(garbage_block);
}

static status_t collect_garbage(void)
{
    status_t status;
    uint8_t garbage_read_block = select_garbage_block(write_pointer);
    status = collect_block(garbage_read_block, &write_pointer);

    return status;
}

/*
 * Load object into buffer and verify object's integrity via crc.  ptr parameter
 * is updated upon successful verification.
 */
static status_t load_and_verify_obj(uint32_t *ptr, struct norfs_header *header)
{
    uint16_t calculated_crc;
    ssize_t bytes;
    ssize_t total_bytes_read = 0;
    bytes = read_header(*ptr, header);
    unsigned char *obj_ptr;

    if (bytes < 0) {
        TRACEF("Reading header failed at location: %d. Err: %d.\n", *ptr, bytes);
        return bytes;
    }

    total_bytes_read += bytes;

    /* If object is longer than the remaining space in current block, fail.
     * When attempting to verify unwritten file space, will fail here. */
    if (block_num(*ptr + header->len - 1) != block_num(*ptr)) {
        return ERR_IO;
    }

    calculated_crc = calculate_header_crc(header->key, header->version,
                                          header->len, header->flags);


    obj_ptr = nvram_flash_pointer(*ptr + total_bytes_read);
    calculated_crc = update_crc16(calculated_crc, obj_ptr, header->len);

    total_bytes_read += header->len;
    if (calculated_crc != header->crc) {
        TRACEF("CRC check failed.  Calculated: 0x%x\tActual: 0x%x\n",
               calculated_crc, header->crc);
        return ERR_CRC_FAIL;
    }
    *ptr += total_bytes_read;
    *ptr = ROUNDUP(*ptr, WORD_SIZE);
    return NO_ERROR;
}

status_t read_block_verification(uint32_t *ptr)
{
    unsigned char block_header[sizeof(NORFS_BLOCK_HEADER) +
                               sizeof(NORFS_BLOCK_GC_STARTED_HEADER) +
                               sizeof(NORFS_BLOCK_GC_FINISHED_HEADER)];
    int bytes_read = nvram_read(*ptr, sizeof(block_header),
                                block_header);

    if (bytes_read < 0) {
        TRACEF("Error reading while verifying block.  Location: %d\n", *ptr);
        return ERR_BAD_STATE;
    }

    for (uint8_t i = 0; i < sizeof(NORFS_BLOCK_HEADER); i++) {
        if (block_header[i] != *(NORFS_BLOCK_HEADER + i)) {
            return ERR_BAD_STATE;
        }
    }

    bool valid_free_block = true;
    for (uint8_t i = sizeof(NORFS_BLOCK_HEADER);
            i < sizeof(NORFS_BLOCK_HEADER) +
            sizeof(NORFS_BLOCK_GC_STARTED_HEADER) +
            sizeof(NORFS_BLOCK_GC_FINISHED_HEADER);
            i++) {
        valid_free_block = valid_free_block && (block_header[i] == 0xFF);
    }

    /* This block has been successfully erased but has not been written to. */
    if (valid_free_block) {
        return ERR_NOT_CONFIGURED;
    }

    if (block_header[4] != NORFS_BLOCK_GC_STARTED_HEADER[0] ||
            block_header[5] != NORFS_BLOCK_GC_STARTED_HEADER[1] ||
            block_header[6] != NORFS_BLOCK_GC_FINISHED_HEADER[0] ||
            block_header[7] != NORFS_BLOCK_GC_FINISHED_HEADER[1]) {
        TRACEF("Garbage collection header invalid.\n");
        return ERR_BAD_STATE;
    }

    *ptr += bytes_read;
    return NO_ERROR;
}

static status_t mount_next_obj(void)
{
    uint16_t curr_obj_loc;
    uint16_t inode_version, inode_len;
    curr_obj_loc = write_pointer;
    struct norfs_inode *inode;
    struct norfs_header header;
    status_t status;

    status = load_and_verify_obj(&write_pointer, &header);
    if (status) {
        return status;
    }
    if (get_inode(header.key, &inode)) {
        nvram_read(inode->location + NORFS_VERSION_OFFSET,
                   sizeof(inode_version), &inode_version);
        if (VERSION_GREATER_THAN(header.version, inode_version)) {
            /* This is a newer version of object than the version
               currently being linked to in the inode. */
            nvram_read(inode->location + NORFS_LENGTH_OFFSET,
                       sizeof(inode_len), &inode_len);
            total_remaining_space += inode_len;
            total_remaining_space -= header.len;
            inode->location = curr_obj_loc;
        }
        inode->reference_count += 1;
    } else {
        /* Object not yet held in memory.  Create new inode. */
        inode = malloc(sizeof(struct norfs_inode));
        inode->location = curr_obj_loc;

        inode->reference_count = 1;

        list_add_tail(&inode_list, &inode->lnode);
        total_remaining_space -= NORFS_FLASH_SIZE(header.len);
    }

    return NO_ERROR;
}

/*
 * Inodes for deleted objects need to be maintained during mounting, in case
 * references show up in later blocks.  However, these references need to be
 * pruned prior to usage.
 */
static void purge_unreferenced_inodes(void)
{
    struct list_node *curr_lnode, *temp_node;
    struct norfs_inode *curr_inode;
    list_for_every_safe(&inode_list, curr_lnode, temp_node) {
        curr_inode = containerof(curr_lnode, struct norfs_inode, lnode);
        if (curr_inode->reference_count == 0) {
            remove_inode(curr_inode);
        }
    }
}

status_t norfs_mount_fs(uint32_t offset)
{
    if (fs_mounted) {
        TRACEF("Filesystem already mounted.\n");
        return ERR_ALREADY_MOUNTED;
    }
    status_t status = 0;
    norfs_nvram_offset = offset;

    list_initialize(&inode_list);
    flash_nor_begin(NORFS_BANK);
    srand(current_time());

    total_remaining_space = NORFS_AVAILABLE_SPACE;
    num_free_blocks = 0;
    TRACEF("Mounting NOR file system.\n");
    for (uint8_t i = 0; i < NORFS_NUM_BLOCKS; i++) {
        write_pointer = i * FLASH_PAGE_SIZE;
        status = read_block_verification(&write_pointer);
        if (status == ERR_BAD_STATE) {
            erase_block(i);
            continue;
        } else if (status == ERR_NOT_CONFIGURED) {
            /* Valid empty block. */
            block_free[i] = true;
            num_free_blocks++;
            continue;
        } else if (status != NO_ERROR) {
            TRACEF("Unexpected status: %d.  Exiting.\n", status);
            flash_nor_end(NORFS_BANK);
            return status;
        }
        block_free[i] = false;
        while (!block_full(i, write_pointer)) {
            status = mount_next_obj();
            if (status)
                break;
        }
    }

    purge_unreferenced_inodes();

    write_pointer = rand() % NORFS_NVRAM_SIZE;
    status = initialize_next_block(&write_pointer);
    if (status) {
        TRACEF("Failed to find free block after mount.\n");
        flash_nor_end(NORFS_BANK);
        return status;
    }

    TRACEF("NOR filesystem successfully mounted.\n");
    flash_nor_end(NORFS_BANK);
    fs_mounted = true;
    return NO_ERROR;
}

void norfs_unmount_fs(void)
{
    TRACEF("Unmounting NOR file system\n");
    struct list_node *curr_lnode;
    struct norfs_inode *curr_inode;
    struct list_node *temp_node;

    if (!fs_mounted) {
        TRACEF("Filesystem not mounted.\n");
        return;
    }
    if (!list_is_empty(&inode_list)) {
        list_for_every_safe(&inode_list, curr_lnode, temp_node) {
            if (curr_lnode) {
                curr_inode = containerof(curr_lnode, struct norfs_inode, lnode);
                remove_inode(curr_inode);
            }
        }
    }
    write_pointer = rand() % NORFS_NVRAM_SIZE;
    total_remaining_space = NORFS_AVAILABLE_SPACE;
    num_free_blocks = 0;
    for (uint8_t i = 0; i < NORFS_NUM_BLOCKS; i++) {
        block_free[i] = false;
    }
    fs_mounted = false;
}

void norfs_wipe_fs(void)
{
    norfs_unmount_fs();
    flash_nor_begin(0);
    nvram_erase_pages(0, 8 * FLASH_PAGE_SIZE);
    flash_nor_end(0);
    norfs_mount_fs(norfs_nvram_offset);
}
