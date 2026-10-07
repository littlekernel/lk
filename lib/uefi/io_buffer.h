// Copyright (c) 2026
// Use of this source code is governed by a MIT-style license in LICENSE.
#pragma once

#include <stddef.h>
#include <sys/types.h>

// A nonempty buffer is either a kernel VA or a UEFI boot-services buffer.
// UEFI's identity-mapping contract allows its VA to be treated directly as a PA;
// LK's paddr_to_kvaddr() then supplies the kernel alias. Kernel VAs are unchanged.
// This does not validate arbitrary user mappings or pin pages.
// The caller must retain the storage until I/O completes.
status_t uefi_buffer_to_kernel(void *buffer, size_t size, void **kernel_buffer);
