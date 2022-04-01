/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>
#include <dev/virtio.h>

status_t virtio_net_init(struct virtio_device *dev, uint32_t host_features) __NONNULL();
status_t virtio_net_start(void);

/* return the count of virtio interfaces found */
int virtio_net_found(void);
