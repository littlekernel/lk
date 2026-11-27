/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

// CMOS drive type codes (register 0x10, one nibble per drive)
enum floppy_drive_type {
    FLOPPY_TYPE_NONE = 0,
    FLOPPY_TYPE_360K = 1,  // 5.25" double density
    FLOPPY_TYPE_1200K = 2, // 5.25" high density
    FLOPPY_TYPE_720K = 3,  // 3.5" double density
    FLOPPY_TYPE_1440K = 4, // 3.5" high density
    FLOPPY_TYPE_2880K = 5, // 3.5" extra density
};

// An 8272/765 or 82077 compatible floppy controller and the drives the firmware says
// are attached to it. Each drive is registered with bio as fd<n>, read only, sized for
// the drive's native format until a disk has been read; the size follows the density of
// the disk in the drive from then on.
struct floppy_config {
    uint16_t io_base; // 0x3f0 for the primary controller
    unsigned int irq; // interrupt vector
    uint dma_channel; // 8 bit ISA DMA channel, 2 on a PC
    enum floppy_drive_type drive_type[2];
};

// ERR_NOT_FOUND if no controller answers at io_base
status_t floppy_init(const struct floppy_config *config);

const char *floppy_drive_type_name(enum floppy_drive_type type);

__END_CDECLS
