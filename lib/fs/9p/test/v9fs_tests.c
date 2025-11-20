/*
 * Copyright (c) 2024 Cody Wong
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fs.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>

// TODO: find a way to convert to a unit test

#define _LOGF(fmt, args...) \
    printf("[%s:%d] " fmt, __PRETTY_FUNCTION__, __LINE__, ##args)
#define LOGF(x...) _LOGF(x)

#define V9FS_MOUNT_POINT "/v9p"
#define V9FS_NAME        "9p"
#define V9P_BDEV_NAME    "v9p0"

#define BUF_SIZE 1024

int v9fs_tests(int argc, const console_cmd_args *argv) {
    status_t status;
    ssize_t readbytes;
    filehandle *handle;
    char buf[BUF_SIZE];

    status = fs_mount(V9FS_MOUNT_POINT, V9FS_NAME, V9P_BDEV_NAME);
    if (status != NO_ERROR) {
        LOGF("failed to mount v9p bdev (%s) onto mount point (%s): %d\n",
             V9P_BDEV_NAME, V9FS_MOUNT_POINT, status);
        return status;
    }

    status = fs_open_file(V9FS_MOUNT_POINT "/LICENSE", &handle);
    if (status != NO_ERROR) {
        LOGF("failed to open the target file: %d\n", status);
        return status;
    }

    readbytes = fs_read_file(handle, buf, 0, BUF_SIZE);
    if (readbytes < 0) {
        LOGF("failed to read the target file: %ld\n", readbytes);
        return status;
    }

    hexdump8(buf, BUF_SIZE);

    status = fs_close_file(handle);
    if (status != NO_ERROR) {
        LOGF("failed to close the target file: %d\n", status);
        return status;
    }

    status = fs_unmount(V9FS_MOUNT_POINT);
    if (status != NO_ERROR) {
        LOGF("failed to unmount v9p on mount point (%s): %d\n",
             V9FS_MOUNT_POINT, status);
        return status;
    }

    return NO_ERROR;
}

STATIC_COMMAND_START
STATIC_COMMAND("v9fs_tests", "test lib/fs/9p", &v9fs_tests)
STATIC_COMMAND_END(v9fs_test);
