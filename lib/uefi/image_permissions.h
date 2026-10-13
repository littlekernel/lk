// Copyright (c) 2026 Kelvin Zhang
// Use of this source code is governed by a MIT-style license in LICENSE.
#pragma once

#include "pe.h"

// Identity-mapped, physically contiguous image storage, initially RW/NX.
// Base-page mappings allow later protection without splitting large pages.
void *alloc_uefi_image(size_t size, paddr_t preferred);

// Sections must already have passed the loader's bounds, overlap and
// native-page-alignment checks.
// Reject sections requesting W+X or sharing a native page where NX is supported.
status_t protect_uefi_image(void *image, size_t size,
                            const IMAGE_SECTION_HEADER *sections, size_t count);
