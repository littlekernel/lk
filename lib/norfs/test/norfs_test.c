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
#include <unittest.h>
#include <lib/norfs.h>
#include <lib/norfs_inode.h>
#include <lib/norfs_config.h>
#include <norfs_test_helper.h>
#include <dev/flash_nor.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <platform/flash_nor_config.h>
#include <platform.h>
#include <rand.h>

extern uint32_t total_remaining_space;
extern uint8_t num_free_blocks;

static uint8_t *norfs_test_bank;
static uint8_t norfs_test_bank_len;
extern uint32_t norfs_nvram_offset = 0;

static const unsigned char erase_header[4] = {'T', 'O', 'F', 'U'};
static const unsigned char gc_header[4] = {'S', 'O', 'U', 'P'};
/* Object with a good checksum. key = 2; version = 0; obj = {0, 1, 2, 3} */
static const unsigned char good_object[16] = {2, 0, 0, 0, 0, 0, 4, 0, 0, 0xFF,
                                              0x3d, 0x02, 0, 1, 2, 3
                                             };
/* Object with a good checksum. key = 2; version = 1; obj = {5, 4, 3, 2} */
static const unsigned char good_object_v1[16] = {2, 0, 0, 0, 1, 0, 4, 0, 0, 0xFF,
                                                 0xbb, 0x9d, 5, 4, 3, 2
                                                };
/* A much later version of good_object to test wrapping of versions. */
static const unsigned char good_object_v127[16] = {2, 0, 0, 0, 0xFF, 0x7F, 4,
                                                   0, 0, 0xFF, 0x76, 0x3c, 6,
                                                   7, 8, 9
                                                  };
/* A much later version of good_object to test wrapping of versions. */
static const unsigned char good_object_v128[16] = {2, 0, 0, 0, 0x00, 0x80, 4,
                                                   0, 0, 0xFF, 0x04, 0x24, 6,
                                                   7, 8, 9
                                                  };
/* Deleted object. key = 2; version = 2 */
static const unsigned char good_object_deleted_v2[12] = {2, 0, 0, 0, 2, 0, 0, 0,
                                                         1, 0xFF, 0xb7, 0x8a
                                                        };
/* Deleted object. key = 2; version = 3 */
static const unsigned char good_object_deleted_v3[12] = {2, 0, 0, 0, 3, 0, 0, 0,
                                                         1, 0xFF, 0xe6, 0x20
                                                        };
/* Object with a bad checksum. key = 4; version = 2; obj = {3, 2, 1, 0} */
static const unsigned char bad_object[16] = {4, 0, 0, 0, 2, 0, 4, 0, 0, 0xFF, 4,
                                             7, 3, 2, 1, 0
                                            };
/* Object with a good checksum. key = 0; version = 0; obj = {9, 8, 7, 6} */
static const unsigned char another_good_object[16] = {0, 0, 0, 0, 0, 0, 4, 0, 0,
                                                      0xFF, 0x4d, 0x4a, 9, 8, 7,
                                                      6
                                                     };

const unsigned char NORFS_BLOCK_HEADER[4] = {'T', 'O', 'F', 'U'};
const unsigned char NORFS_BLOCK_GC_STARTED_HEADER[2] = {'S', 'O'};
const unsigned char NORFS_BLOCK_GC_FINISHED_HEADER[2] = {'U', 'P'};
const int NORFS_OBJ_HEADER_SIZE = 12;

/* Stores the current bank for future retrieval. */
static void save_bank(void)
{
    free(norfs_test_bank);
    norfs_test_bank = malloc(8 * FLASH_PAGE_SIZE);
    const struct flash_nor_bank *bank;
    bank = flash_nor_get_bank(0);
    memcpy(norfs_test_bank, (void *)bank->base, 8 * FLASH_PAGE_SIZE);
    norfs_test_bank_len = bank->len;
}

static void write_block_header(uint *ptr)
{
    *ptr += flash_nor_write(0, *ptr, sizeof(NORFS_BLOCK_HEADER),
                            NORFS_BLOCK_HEADER);
    *ptr += flash_nor_write(0, *ptr, sizeof(NORFS_BLOCK_GC_STARTED_HEADER),
                            NORFS_BLOCK_GC_STARTED_HEADER);
    *ptr += flash_nor_write(0, *ptr, sizeof(NORFS_BLOCK_GC_FINISHED_HEADER),
                            NORFS_BLOCK_GC_FINISHED_HEADER);
}

static bool test_basic_read_write(void)
{
    BEGIN_TEST;
    unsigned char char_array[5] = {4, 6, 7, 2, 1};
    uint32_t char_key = 47;

    int big_int = 2000000000;
    uint32_t int_key = 21;
    size_t char_array_bytes_read;
    int big_int_buffer;
    size_t big_int_bytes_read;
    unsigned char *char_array_buffer = malloc(sizeof(unsigned char)*6);
    status_t status;
    unsigned char alpha[2] = {'a', 'b'};
    unsigned char beta[3] = {'c', 'd', 'e'};
    size_t obj_len;

    int iov_in_count = 2;
    iovec_t *iov_in = malloc(iov_in_count*sizeof(iovec_t));
    iov_in[0].iov_len = 2;
    iov_in[0].iov_base = alpha;
    iov_in[1].iov_len = 3;
    iov_in[1].iov_base = beta;

    int iov_out_count = 3;
    iovec_t *iov_out = malloc(iov_out_count*sizeof(iovec_t));
    iov_out[0].iov_len = 1;
    iov_out[0].iov_base = malloc(sizeof(unsigned char));
    iov_out[1].iov_len = 3;
    iov_out[1].iov_base = malloc(iov_out_count*sizeof(unsigned char));
    iov_out[2].iov_len = 1;
    iov_out[2].iov_base = malloc(sizeof(unsigned char));

    wipe_fs();

    EXPECT_EQ(ERR_NOT_MOUNTED, norfs_put_obj(char_key, char_array, sizeof(char_array),
              0), "Calls to file system should fail when not mounted");
    EXPECT_EQ(ERR_NOT_MOUNTED, norfs_read_obj(char_key, char_array_buffer,
              sizeof(unsigned char)*6, &char_array_bytes_read, 0),
              "Calls to file system should fail when not mounted");
    EXPECT_EQ(ERR_NOT_MOUNTED, norfs_put_obj_iovec(17, iov_in, iov_in_count, 0),
              "Calls to file system should fail when not mounted");
    EXPECT_EQ(ERR_NOT_MOUNTED, norfs_read_obj_iovec(17, iov_out, iov_out_count,
              &obj_len, 0),
              "Calls to file system should fail when not mounted");
    EXPECT_EQ(ERR_NOT_MOUNTED, norfs_remove_obj(17),
              "Calls to file system should fail when not mounted");

    EXPECT_EQ(NO_ERROR, norfs_mount_fs(norfs_nvram_offset), "Error during mount");

    status = norfs_put_obj(char_key, char_array, sizeof(char_array), 0);
    EXPECT_EQ(NO_ERROR, status, "norfs_put_obj returned error");
    status = norfs_put_obj(int_key, (unsigned char *)&big_int, sizeof(big_int), 0);
    EXPECT_EQ(NO_ERROR, status, "norfs_put_obj returned error");

    status = norfs_read_obj(char_key, char_array_buffer, sizeof(unsigned char)*6,
                            &char_array_bytes_read, 0);
    EXPECT_EQ(NO_ERROR, status, "norfs_read_obj returned error");
    if (status)
        return false;

    status = norfs_read_obj(int_key, (unsigned char *)&big_int_buffer,
                            sizeof(big_int_buffer), &big_int_bytes_read, 0);
    EXPECT_EQ(NO_ERROR, status, "norfs_read_obj returned error");
    if (status)
        return false;

    if (!status) {
        EXPECT_EQ(sizeof(char_array), char_array_bytes_read,
                  "Not all bytes of object read");
        EXPECT_EQ(sizeof(big_int), big_int_bytes_read,
                  "Not all bytes of object read");
        for (int i = 0; i < 5; i++) {
            EXPECT_EQ(char_array[i], char_array_buffer[i],
                      "Did not read object from NVRAM correctly");
        }

        EXPECT_EQ(big_int, big_int_buffer,
                  "Did not read object from NVRAM correctly");
    }

    norfs_put_obj_iovec(17, iov_in, iov_in_count, 0);

    status = norfs_read_obj_iovec(17, iov_out, iov_out_count, &obj_len, 0);
    EXPECT_EQ(NO_ERROR, status, "norfs_read_obj returned error for iovec");

    if (!status) {
        EXPECT_EQ('a', *((unsigned char *)iov_out[0].iov_base),
                  "Iovec not retrieved correctly");
        EXPECT_EQ('b', *((unsigned char *)iov_out[1].iov_base),
                  "Iovec not retrieved correctly");
        EXPECT_EQ('c', *((unsigned char *)iov_out[1].iov_base + 1),
                  "Iovec not retrieved correctly");
        EXPECT_EQ('d', *((unsigned char *)iov_out[1].iov_base + 2),
                  "Iovec not retrieved correctly");
        EXPECT_EQ('e', *((unsigned char *)iov_out[2].iov_base),
                  "Iovec not retrieved correctly");
    } else {
        return false;
    }

    EXPECT_EQ(5, obj_len, "Iovec did not return length correctly");

    /* Ensure static data is cleared when you remount. */
    EXPECT_EQ(norfs_mount_fs(norfs_nvram_offset), ERR_ALREADY_MOUNTED, "Remounting should fail");
    save_bank();
    norfs_unmount_fs();
    EXPECT_EQ(NO_ERROR, norfs_mount_fs(norfs_nvram_offset), "Error during mount");
    memset(char_array_buffer, 0, sizeof(char_array_buffer));
    big_int_buffer = 0;

    status = norfs_read_obj(char_key, char_array_buffer, sizeof(unsigned char)*6,
                            &char_array_bytes_read, 0);
    EXPECT_EQ(NO_ERROR, status, "norfs_read_obj returned error");
    if (status)
        return false;
    status = norfs_read_obj(int_key, (unsigned char *)&big_int_buffer,
                            sizeof(big_int_buffer), &big_int_bytes_read, 0);
    EXPECT_EQ(NO_ERROR, status, "norfs_read_obj returned error");
    if (status)
        return false;

    if (!status) {
        EXPECT_EQ(char_array_bytes_read, sizeof(char_array),
                  "Not all bytes of object read");
        EXPECT_EQ(big_int_bytes_read, sizeof(big_int),
                  "Not all bytes of object read");
        for (int i = 0; i < 5; i++) {
            EXPECT_EQ(char_array[i], char_array_buffer[i],
                      "Did not read object from NVRAM correctly");
        }

        EXPECT_EQ(big_int, big_int_buffer,
                  "Did not read object from NVRAM correctly");
    }

    status = norfs_read_obj_iovec(17, iov_out, iov_out_count - 1, &obj_len, 0);

    EXPECT_EQ(NO_ERROR, status, "norfs_read_obj returned error for iovec");

    if (!status) {
        EXPECT_EQ('a', *((unsigned char *)iov_out[0].iov_base),
                  "Iovec not retrieved correctly");
        EXPECT_EQ('b', *((unsigned char *)iov_out[1].iov_base),
                  "Iovec not retrieved correctly");
        EXPECT_EQ('c', *((unsigned char *)iov_out[1].iov_base + 1),
                  "Iovec not retrieved correctly");
        EXPECT_EQ('d', *((unsigned char *)iov_out[1].iov_base + 2),
                  "Iovec not retrieved correctly");
    } else {
        return false;
    }

    EXPECT_EQ(4, obj_len, "Iovec did not return length correctly");

    status = norfs_remove_obj(17);
    EXPECT_EQ(NO_ERROR, status, "Error when removing object");

    status = norfs_read_obj_iovec(17, iov_out, 2, &obj_len, 0);
    EXPECT_EQ(ERR_NOT_FOUND, status, "Found object when unexpected");

    status = norfs_remove_obj(17);
    EXPECT_EQ(ERR_NOT_FOUND, status, "Allowed redundant delete of object");

    wipe_fs();
    END_TEST;
}

/* This test may need to be revised if no longer using round-robin garbage
 * collection, since there will be no guarantee the block containing the
 * object with key 4 has been collected.
 */
static bool test_garbage_collection(void)
{
    BEGIN_TEST;
    int size = FLASH_PAGE_SIZE/16;
    unsigned char array[size];
    size_t bytes_read;
    status_t status = NO_ERROR;

    norfs_mount_fs(norfs_nvram_offset);

    /* Store an object that will not be overwritten. */
    memset(array, 4, size);
    status = norfs_put_obj(4, array, size, 0);
    EXPECT_EQ(NO_ERROR, status,
              "Error putting object");
    /* Store enough versions of objects to ensure block containing
     * first object is collected.
     */
    for (int i = 0; i < 51; i++) {
        for (int j = 0; j < 4; j++) {
            memset(array, j, size);
            status = norfs_put_obj(j, array, size, 0);
            EXPECT_EQ(NO_ERROR, status,
                      "Error putting object");
            if (status)
                break;
        }
        if (status)
            break;
    }

    status = norfs_read_obj(4, array, size, &bytes_read, 0);
    EXPECT_EQ(NO_ERROR, status,
              "Error reading object");
    /* Verify first object still intact. */
    for (int i = 0; i < size; i++) {
        EXPECT_EQ(4, array[i], "Bad value for object.\n");
    }

    wipe_fs();
    END_TEST;
}

static bool test_total_remaining_space(void)
{
    BEGIN_TEST;
    uint32_t prev_remaining_space;

    unsigned char array[4];

    wipe_fs();
    norfs_mount_fs(norfs_nvram_offset);

    norfs_put_obj(0, array, 3, 0);
    norfs_put_obj(0, array, 4, 0);

    prev_remaining_space = total_remaining_space;

    norfs_unmount_fs();
    norfs_mount_fs(norfs_nvram_offset);

    EXPECT_EQ(total_remaining_space, prev_remaining_space,
              "Remaining space incorrectly maintained");
    wipe_fs();
    END_TEST;
}

static bool test_deletion(void)
{
    BEGIN_TEST;

    status_t status;
    struct norfs_inode *inode = NULL;
    uint ptr = 0;

    wipe_fs();
    ptr += flash_nor_write(0, ptr, sizeof(erase_header), erase_header);
    ptr += flash_nor_write(0, ptr, sizeof(gc_header), gc_header);
    ptr += flash_nor_write(0, ptr, sizeof(good_object), good_object);
    ptr += flash_nor_write(0, ptr, sizeof(good_object_deleted_v2),
                           good_object_deleted_v2);

    EXPECT_EQ(NO_ERROR, norfs_mount_fs(norfs_nvram_offset), "Error during mount");
    status = get_inode(2, &inode);
    EXPECT_EQ(true, status,
              "Inode should be in list since references remain");
    if (!status)
        return false;
    EXPECT_EQ(2, (inode)->reference_count,
              "Reference count should count one non-deleted version of object");
    EXPECT_EQ(24, (inode)->location, "Not pointing to current version");
    write_pointer = FLASH_PAGE_SIZE + sizeof(erase_header);
    write_pointer += flash_nor_write(0, write_pointer, sizeof(erase_header),
                                     erase_header);
    EXPECT_EQ(NO_ERROR, collect_block(0, &write_pointer),
              "Error when collecting block");
    EXPECT_EQ(false, get_inode(2, &inode),
              "Inode still in list after references have been collected");

    wipe_fs();
    ptr = 0;
    ptr += flash_nor_write(0, ptr, sizeof(erase_header), erase_header);
    ptr += flash_nor_write(0, ptr, sizeof(gc_header), gc_header);
    ptr += flash_nor_write(0, ptr, sizeof(good_object_deleted_v2),
                           good_object_deleted_v2);
    EXPECT_EQ(NO_ERROR, norfs_mount_fs(norfs_nvram_offset), "Error during mount");
    EXPECT_EQ(true, get_inode(2, &inode),
              "Inode should be in list");

    wipe_fs();
    END_TEST;
}

static bool test_corruption(void)
{
    BEGIN_TEST;

    unsigned char buff[4];
    size_t bytes_read;
    wipe_fs();
    norfs_mount_fs(norfs_nvram_offset);

    uint ptr = 4;
    ptr += flash_nor_write(0, ptr, 4, gc_header);
    ptr += flash_nor_write(0, ptr, 16, good_object);
    EXPECT_EQ(24, ptr, "Write failed");
    ptr += flash_nor_write(0, ptr, 16, bad_object);
    EXPECT_EQ(40, ptr, "Write failed");
    ptr += flash_nor_write(0, ptr, 16, good_object_v1);
    EXPECT_EQ(56, ptr, "Write failed");
    ptr += flash_nor_write(0, ptr, 16, another_good_object);
    EXPECT_EQ(72, ptr, "Write failed");

    norfs_unmount_fs();
    norfs_mount_fs(norfs_nvram_offset);

    status_t status = norfs_read_obj(2, buff, 4, &bytes_read, 0);
    EXPECT_EQ(NO_ERROR, status,
              "Object prior to corruption should still be readable");

    /* good_object_v1, appearing after the corruption, should be discarded. */
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(i, buff[i],
                  "First version of object not correctly retrieved");
    }

    status = norfs_read_obj(4, buff, 4, &bytes_read, 0);
    EXPECT_EQ(ERR_NOT_FOUND, status, "Corrupted object should not be found");
    status = norfs_read_obj(0, buff, 4, &bytes_read, 0);
    EXPECT_EQ(ERR_NOT_FOUND, status,
              "Object after corrupted object should not be found");

    wipe_fs();
    END_TEST;
}

static bool test_deletion_objects_retained_during_gc(void)
{
    BEGIN_TEST;
    static const uint16_t BLOCK_HEADER_SIZE = 8, OBJ_HEADER_LENGTH = 12;

    /* Define an object size that will tightly fill a block with objects. */
    uint16_t obj_len = (FLASH_PAGE_SIZE - BLOCK_HEADER_SIZE) / 20
                       - OBJ_HEADER_LENGTH;
    status_t status;
    unsigned char obj[obj_len];
    uint32_t key = 17;
    size_t bytes_read;

    memset(obj, 7, obj_len);

    wipe_fs();
    EXPECT_EQ(NO_ERROR, norfs_mount_fs(norfs_nvram_offset), "Error during mount");

    for (int i = 0; i < 20; i++) {
        status = norfs_put_obj(key, obj, obj_len, 0);
        EXPECT_EQ(NO_ERROR, status, "Error putting object");
    }

    status = norfs_remove_obj(key);
    EXPECT_EQ(status, NO_ERROR, "Error removing object");

    status = norfs_read_obj(key, obj, obj_len, &bytes_read, 0);
    EXPECT_EQ(ERR_NOT_FOUND, status, "Object not removed");

    find_free_block(&write_pointer);

    status = norfs_read_obj(key, obj, obj_len, &bytes_read, 0);
    EXPECT_EQ(ERR_NOT_FOUND, status,
              "Error: Object removal lost after garbage collection");

    status = norfs_put_obj(key, obj, obj_len, 0);
    EXPECT_EQ(NO_ERROR, status, "Error putting object");

    wipe_fs();
    END_TEST;
}

static bool test_garbage_collection_interruption(void)
{
    BEGIN_TEST;
    wipe_fs();
    size_t obj_len;
    uint32_t ptr = 0;

    unsigned char buffer[16];
    /* Write partial block header, indicating interrupted garbage collection. */
    ptr += flash_nor_write(0, ptr, sizeof(NORFS_BLOCK_HEADER),
                           NORFS_BLOCK_HEADER);
    ptr += flash_nor_write(0, ptr, sizeof(NORFS_BLOCK_GC_STARTED_HEADER),
                           NORFS_BLOCK_GC_STARTED_HEADER);
    ptr += sizeof(NORFS_BLOCK_GC_FINISHED_HEADER);
    flash_nor_write(0, ptr, sizeof(good_object), good_object);

    /* Move to second block. */
    ptr = FLASH_PAGE_SIZE;
    write_block_header(&ptr);
    flash_nor_write(0, ptr, sizeof(another_good_object), another_good_object);

    EXPECT_EQ(NO_ERROR, norfs_mount_fs(norfs_nvram_offset), "Error during mount");

    EXPECT_EQ(NO_ERROR, norfs_read_obj(0, buffer, sizeof(buffer), &obj_len, 0),
              "Failed to read object in block with correct header");
    EXPECT_EQ(ERR_NOT_FOUND, norfs_read_obj(2, buffer, sizeof(buffer), &obj_len, 0),
              "Object in corrupt block should not be readable");

    wipe_fs();
    END_TEST;
}

static bool test_thrash_fs(void)
{
    status_t status;
    size_t bytes_read;
    int int_buff;
    int state[16];

    for (int i = 0; i < 16; i++) {
        state[i] = -1;
    }

    BEGIN_TEST;
    wipe_fs();
    norfs_mount_fs(norfs_nvram_offset);
    srand(current_time());
    iovec_t *iov = malloc(sizeof(iovec_t));
    iov->iov_len = sizeof(int);

    for (int i = 0; i < 10000; i++) {
        int action = rand() % 6;
        uint16_t key = (uint16_t)(rand() % 16);
        switch (action) {
            case 0:
                status = norfs_remove_obj(key);
                if (state[key] == -1) {
                    EXPECT_EQ(ERR_NOT_FOUND, status,
                              "Removed object not returning correctly from read");
                } else {
                    EXPECT_EQ(NO_ERROR, status,
                              "Object not returning correctly from remove");
                    if (status) {
                        dump_bank();
                        return false;
                    }
                }
                state[key] = -1;
                break;
            case 1:
                iov->iov_base = &i;
                status = norfs_put_obj_iovec(key, iov, 1, 0);
                EXPECT_EQ(NO_ERROR, status,
                          "Object not returning correctly from put");
                state[key] = i;
                break;
            case 2:
                status = norfs_put_obj(key, (unsigned char *)&i, sizeof(i), 0);
                EXPECT_EQ(NO_ERROR, status,
                          "Object not returning correctly from put");
                state[key] = i;
                break;
            case 3:
                iov->iov_base = &int_buff;
                status = norfs_read_obj_iovec(key, iov, 1, &bytes_read, 0);
                if (state[key] == -1) {
                    EXPECT_EQ(ERR_NOT_FOUND, status,
                              "Removed object not returning correctly from read");
                } else {
                    EXPECT_EQ(NO_ERROR, status,
                              "Object not returning correctly from read");
                    EXPECT_EQ(state[key], *(int *)iov->iov_base, "Object not correct value");
                    if (status || state[key] != *(int *)iov->iov_base) {
                        dump_bank();
                        return false;
                    }
                }
                break;
            case 4:
                status = norfs_read_obj(key, (unsigned char *)&int_buff, sizeof(int),
                                        &bytes_read, 0);
                if (state[key] == -1) {
                    EXPECT_EQ(ERR_NOT_FOUND, status,
                              "Removed object not returning correctly from read");
                } else {
                    EXPECT_EQ(NO_ERROR, status,
                              "Object not returning correctly from read");
                    EXPECT_EQ(state[key], int_buff, "Object not correct value");
                    if (status || state[key] != int_buff) {
                        dump_bank();
                        return false;
                    }
                }
                break;
            case 5:
                norfs_unmount_fs();
                norfs_mount_fs(norfs_nvram_offset);
                break;
            default:
                /* Should not end up here. */
                return false;
        }
        EXPECT_GE(8 * FLASH_PAGE_SIZE - 1, write_pointer,
                  "Write pointer has overflowed NVRAM");
    }
    for (int i = 0; i < 16; i++) {
        status = norfs_read_obj(i, (unsigned char *)&int_buff, sizeof(int), &bytes_read, 0);
        if (state[i] == -1) {
            EXPECT_EQ(ERR_NOT_FOUND, status,
                      "Removed object not returning correctly from read");
        } else {
            EXPECT_EQ(NO_ERROR, status,
                      "Object not returning correctly from read");
            EXPECT_EQ(state[i], int_buff, "Object not correct value");
        }
    }
    wipe_fs();
    END_TEST;
}

static bool test_overflow_filesystem(void)
{
    BEGIN_TEST;

    wipe_fs();
    norfs_mount_fs(norfs_nvram_offset);

    int i = 0;
    bool filesystem_full;
    status_t status;
    unsigned char obj = 0b01010101;
    while (!filesystem_full) {
        status = norfs_put_obj(i, &obj, sizeof(obj), 0);
        if (status != NO_ERROR) {
            EXPECT_EQ(ERR_NO_MEMORY, status, "put_obj returned unexpected error");
            EXPECT_LE(sizeof(obj) + NORFS_OBJ_HEADER_SIZE, total_remaining_space,
                      "Returning memory error when filesystem not full");
            if (status != ERR_TOO_BIG) {
                return false;
            }
            filesystem_full = true;
        }
        i++;
    }

    for (int j = i - 1; j >= 0; j--) {
        status = norfs_remove_obj(j);
        if (total_remaining_space >= sizeof(obj) + NORFS_OBJ_HEADER_SIZE) {
            status = norfs_put_obj(i, &obj, sizeof(obj), 0);
            EXPECT_EQ(NO_ERROR, status, "Error after clearing space for new objects");
            i++;
        }
    }

    wipe_fs();
    END_TEST;
}

static bool test_wrapping(void)
{
    BEGIN_TEST;
    size_t bytes_read;
    unsigned char buffer[4];
    uint ptr = 0;

    wipe_fs();
    flash_nor_begin(0);

    write_block_header(&ptr);
    flash_nor_write(0, ptr, sizeof(good_object), good_object);
    ptr = FLASH_PAGE_SIZE;
    write_block_header(&ptr);
    flash_nor_write(0, ptr, sizeof(good_object_v127), good_object_v127);

    flash_nor_end(0);
    norfs_mount_fs(norfs_nvram_offset);

    EXPECT_EQ(NO_ERROR, norfs_read_obj(2, buffer, sizeof(buffer), &bytes_read, 0),
              "Error during read");
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(*(buffer + i), *(good_object_v127 + NORFS_OBJ_OFFSET + i),
                  "Object not read correctly");
    }

    wipe_fs();

    flash_nor_begin(0);
    ptr = 0;

    write_block_header(&ptr);
    flash_nor_write(0, ptr, sizeof(good_object), good_object);
    ptr = FLASH_PAGE_SIZE;
    write_block_header(&ptr);
    flash_nor_write(0, ptr, sizeof(good_object_v128), good_object_v128);

    flash_nor_end(0);

    norfs_mount_fs(norfs_nvram_offset);

    EXPECT_EQ(NO_ERROR, norfs_read_obj(2, buffer, sizeof(buffer), &bytes_read,
                                       0),
              "Error during read");
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(*(buffer + i), *(good_object + NORFS_OBJ_OFFSET + i),
                  "Object not read correctly when wrapped");
    }

    wipe_fs();
    END_TEST;
}

static void init_tests(void)
{
    platform_init();
    wipe_fs();
}

BEGIN_TEST_CASE(norfs_tests);
init_tests();
RUN_TEST(test_basic_read_write);
RUN_TEST(test_garbage_collection);
RUN_TEST(test_deletion);
RUN_TEST(test_corruption);
RUN_TEST(test_deletion_objects_retained_during_gc);
RUN_TEST(test_garbage_collection_interruption);
RUN_TEST(test_total_remaining_space);
RUN_TEST(test_thrash_fs);
RUN_TEST(test_wrapping);
RUN_TEST(test_overflow_filesystem);
END_TEST_CASE(norfs_tests);
