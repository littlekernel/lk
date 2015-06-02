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
#ifndef __NORFS_H
#define __NORFS_H

#include <iovec.h>
#include <sys/types.h>
#include <stdint.h>

/*
 * File system must be mounted prior to usage.
 */
status_t norfs_mount_fs(uint32_t offset);

/*
 * File system can be unmounted.  All data is stored.
 */
void norfs_unmount_fs(void);

/*
 * Remove an object from filesystem.
 */
status_t norfs_remove_obj(uint32_t key);

/*
 * Put an IO vector object in filesystem.  Previous versions of the object will
 * be overwritten.
 */
status_t norfs_put_obj_iovec(uint32_t key, const iovec_t *obj_iov, uint32_t iov_count,
		uint8_t flags);

/*
 * Put an object in filesystem.  Previous versions of the object will be
 * overwritten.
 */
status_t norfs_put_obj(uint32_t key, unsigned char *obj, uint16_t obj_len,
				 uint8_t flags);


/*
 * Read object into an iovec buffer.  Will fail with ERR_TOO_BIG if
 * iovec does not have enough space to store entire object.  Will
 * fail with ERR_CRC_FAIL if CRC fails. obj_len field will return
 * the amount of bytes in the object as per the object header.
 */
status_t norfs_read_obj_iovec(uint32_t key, iovec_t *obj_iov, uint32_t iov_count,
						size_t *bytes_read, uint8_t flags);

/*
 * Equivalent to read_obj_iovec, but objects read into a single buffer.
 */
status_t norfs_read_obj(uint32_t key, unsigned char *buffer, uint16_t buffer_len,
				  size_t *bytes_read, uint8_t flags);

/*
 * Wipe NVRAM.  Leaves filesystem unmounted.
 */
void norfs_wipe_fs(void);

#endif
