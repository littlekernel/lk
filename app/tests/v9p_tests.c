/*
 * Copyright (c) 2023 Cody Wong
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <app/tests.h>
#include <lk/err.h>
#include <lk/debug.h>

#define FID_ROOT 1
#define FID_RO 2

#define _LOGF(fmt, args...) \
    printf("[%s:%d] " fmt, __PRETTY_FUNCTION__, __LINE__, ##args)
#define LOGF(x...) _LOGF(x)

#if WITH_DEV_VIRTIO_9P
#include <dev/virtio/9p.h>
#include <dev/virtio.h>

int v9p_tests(int argc, const console_cmd_args *argv) {
    struct virtio_device *dev = virtio_get_9p_device(0);
    status_t status;

    if (dev == NULL) {
        LOGF("v9p device doesn't exist\n");
        return ERR_NOT_FOUND;
    }

    virtio_9p_msg_t tatt = {
        .msg_type = P9_TATTACH,
        .tag = P9_TAG_DEFAULT,
        .msg.tattach = {
            .fid = FID_ROOT,
            .afid = P9_FID_NOFID,
            .uname = "root",
            .aname = V9P_MOUNT_ANAME,
            .n_uname = P9_UNAME_NONUNAME
        }
    };
    virtio_9p_msg_t ratt = {};

    status = virtio_9p_rpc(dev, &tatt, &ratt);
    if (status != NO_ERROR) {
        LOGF("failed to attach to the host shared folder: %d\n", status);
        return status;
    }

    virtio_9p_msg_t twalk = {
        .msg_type = P9_TWALK,
        .tag = P9_TAG_DEFAULT,
        .msg.twalk = {
            .fid = FID_ROOT, .newfid = FID_RO, .nwname = 1,
            .wname = {"LICENSE"}
        }
    };
    virtio_9p_msg_t rwalk = {};

    status = virtio_9p_rpc(dev, &twalk, &rwalk);
    if (status != NO_ERROR) {
        LOGF("failed to walk to the target file: %d\n", status);
        return status;
    }

    virtio_9p_msg_t tlopen = {
        .msg_type= P9_TLOPEN,
        .tag = P9_TAG_DEFAULT,
        .msg.tlopen = {
            .fid = FID_RO, .flags = O_RDWR,
        }
    };
    virtio_9p_msg_t rlopen = {};

    status = virtio_9p_rpc(dev, &tlopen, &rlopen);
    if (status != NO_ERROR) {
        LOGF("failed to open the target file: %d\n", status);
        return status;
    }

    virtio_9p_msg_t tread = {
        .msg_type= P9_TREAD,
        .tag = P9_TAG_DEFAULT,
        .msg.tread = {
            .fid = FID_RO, .offset = 0, .count = 1024
        }
    };
    virtio_9p_msg_t rread = {};

    status = virtio_9p_rpc(dev, &tread, &rread);
    if (status != NO_ERROR) {
        LOGF("failed to read the target file: %d\n", status);
        return status;
    }

    hexdump8(rread.msg.rread.data, rread.msg.rread.count);

    return NO_ERROR;
}
#else
int v9p_tests(int argc, const console_cmd_args *argv) {
    LOGF("platform didn't have dev/virtio/9p supported\n");
    return ERR_NOT_SUPPORTED;
}
#endif // WITH_DEV_VIRTIO_9P
