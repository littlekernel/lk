/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>
#include <sys/types.h>

// GICv2m MSI frames. A frame turns a write of an SPI number to its MSI_SETSPI_NS register into
// that SPI, so an MSI from one is an ordinary edge triggered SPI; which SPIs a frame can raise
// comes from its MSI_TYPER. Frames sit beside a GICv2, or a GICv3 without an ITS.

// Record a frame mapped at vaddr. Fails if its SPI window is empty or entirely past MAX_INT.
status_t arm_gic_v2m_add_frame(paddr_t paddr, vaddr_t vaddr);

// Allocate an SPI from the frame at paddr. ERR_NOT_FOUND if paddr is not a known frame,
// ERR_NO_RESOURCES if its window is used up.
status_t arm_gic_v2m_alloc(paddr_t paddr, unsigned int *vector, uint64_t *msg_addr,
                           uint32_t *msg_data);
status_t arm_gic_v2m_free(paddr_t paddr, unsigned int vector);
