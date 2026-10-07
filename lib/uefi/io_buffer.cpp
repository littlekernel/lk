// Copyright (c) 2026
// Use of this source code is governed by a MIT-style license in LICENSE.
#include "io_buffer.h"

#include <kernel/vm.h>
#include <lk/err.h>
#include <stdint.h>

status_t uefi_buffer_to_kernel(void *buffer, size_t size, void **kernel_buffer) {
  *kernel_buffer = nullptr;
  const uintptr_t start = reinterpret_cast<uintptr_t>(buffer);
  if (buffer == nullptr || size == 0 || size - 1 > UINTPTR_MAX - start) {
    return ERR_INVALID_ARGS;
  }
  // The UEFI boot-services identity-mapping contract makes a UEFI VA
  // trivially a PA. Pass that PA to LK's paddr_to_kvaddr() to obtain the
  // kernel alias. Existing kernel pointers need no conversion.
  *kernel_buffer = is_kernel_address(start)
      ? buffer : paddr_to_kvaddr(static_cast<paddr_t>(start));
  return *kernel_buffer != nullptr ? NO_ERROR : ERR_NOT_SUPPORTED;
}
