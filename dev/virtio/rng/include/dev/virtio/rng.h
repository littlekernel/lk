/*
 * Copyright (c) 2026 Kuan-Wei Chiu <visitorckw@gmail.com>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/err.h>

struct virtio_device;

status_t virtio_rng_init(virtio_device *dev);
ssize_t virtio_rng_read(void *buf, size_t len);
