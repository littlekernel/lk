// Copyright (c) 2026
// Use of this source code is governed by a MIT-style license in LICENSE.
#include <kernel/vm.h>
#include <lib/bio.h>
#include <lib/unittest.h>
#include <lk/err.h>
#include <lk/console_cmd.h>
#include <stdint.h>
#include <string.h>
#include <uefi/protocols/block_io2_protocol.h>

#include "../blockio_protocols.h"
#include "../blockio2_protocols.h"
#include "../defer.h"
#include "../events.h"
#include "../io_buffer.h"
#include "../memory_protocols.h"
#include "../uefi_platform.h"

namespace {

bool buffer_aliases() {
  BEGIN_TEST;
  DEFER { reset_heap(); };
  auto *memory = static_cast<char *>(alloc_page(3 * PAGE_SIZE));
  ASSERT_NONNULL(memory, "identity allocation");
  DEFER { free_pages(memory, 3); };
  void *alias = nullptr;
  ASSERT_EQ(NO_ERROR, uefi_buffer_to_kernel(memory + 37, PAGE_SIZE + 29, &alias), "cross-page alias");
  ASSERT_NONNULL(alias, "kernel alias");
  ASSERT_EQ(reinterpret_cast<uintptr_t>(paddr_to_kvaddr(reinterpret_cast<paddr_t>(memory) + 37)),
            reinterpret_cast<uintptr_t>(alias), "preserve offset");
  auto *old = vmm_set_active_aspace(nullptr);
  memset(alias, 0x5a, PAGE_SIZE + 29);
  vmm_set_active_aspace(old);
  ASSERT_EQ(0x5a, static_cast<unsigned char>(memory[37]), "start visible in UEFI");
  ASSERT_EQ(0x5a, static_cast<unsigned char>(memory[PAGE_SIZE + 65]), "end visible in UEFI");
  void *same = nullptr;
  ASSERT_EQ(NO_ERROR, uefi_buffer_to_kernel(alias, PAGE_SIZE + 29, &same), "kernel input");
  ASSERT_EQ(alias, same, "kernel VA unchanged");
  ASSERT_EQ(ERR_INVALID_ARGS, uefi_buffer_to_kernel(nullptr, 512, &alias), "null");
  ASSERT_EQ(ERR_INVALID_ARGS, uefi_buffer_to_kernel(memory, 0, &alias), "empty helper input");
  ASSERT_EQ(ERR_INVALID_ARGS, uefi_buffer_to_kernel(reinterpret_cast<void *>(UINTPTR_MAX - 8), 32, &alias), "overflow");

  // The physical/identity contract does not depend on the calling thread
  // having the UEFI address space installed at conversion time.
  old = vmm_set_active_aspace(nullptr);
  const auto status = uefi_buffer_to_kernel(memory + 37, PAGE_SIZE + 29, &alias);
  vmm_set_active_aspace(old);
  ASSERT_EQ(NO_ERROR, status, "translate without UEFI address space");
  ASSERT_EQ(reinterpret_cast<uintptr_t>(paddr_to_kvaddr(reinterpret_cast<paddr_t>(memory) + 37)),
            reinterpret_cast<uintptr_t>(alias), "same physical alias");
  ASSERT_EQ(ERR_NOT_SUPPORTED,
            uefi_buffer_to_kernel(reinterpret_cast<void *>(USER_ASPACE_SIZE - PAGE_SIZE),
                                  PAGE_SIZE, &alias),
            "physical address has no kernel alias");
  END_TEST;
}

size_t read_calls;

// Force the BIO consumer into a kernel-only address space. The pre-fix
// protocol passes a low identity VA here, and fails before any dereference.
ssize_t kernel_read(bdev_t *, void *buffer, bnum_t, uint count) {
  ++read_calls;
  auto *old = vmm_set_active_aspace(nullptr);
  DEFER { vmm_set_active_aspace(old); };
  if (!is_kernel_address(reinterpret_cast<vaddr_t>(buffer))) {
    return ERR_INVALID_ARGS;
  }
  memset(buffer, 0xa7, count * 512);
  return count * 512;
}

status_t kernel_read_async(bdev_t *dev, void *buffer, off_t, size_t size,
                          bio_async_callback_t callback, void *cookie) {
  auto *old = vmm_set_active_aspace(nullptr);
  DEFER { vmm_set_active_aspace(old); };
  const auto bytes = kernel_read(dev, buffer, 0, size / 512);
  callback(cookie, dev, bytes);
  return NO_ERROR;
}

bool protocol_buffers() {
  BEGIN_TEST;
  DEFER { reset_heap(); };
  setup_heap();
  auto *memory = static_cast<char *>(alloc_page(3 * PAGE_SIZE));
  ASSERT_NONNULL(memory, "identity buffer");
  DEFER { free_pages(memory, 3); };
  // Stack-owned device; release tracked references before unregistering it.
  bdev_t device{};
  auto *dev = &device;
  bio_initialize_bdev(dev, "uefi-buffer-test", 512, 128, 0, nullptr, BIO_FLAGS_NONE);
  ASSERT_NONNULL(dev->name, "test device name");
  bio_register_device(dev);
  DEFER { bio_unregister_device(dev); };
  DEFER { close_tracked_bdevs(); };
  dev->read_block = kernel_read;
  dev->read_async = kernel_read_async;
  const void *intf;
  ASSERT_EQ(EFI_STATUS_SUCCESS, open_block_device(dev->name, &intf), "sync interface");
  auto *sync = const_cast<EfiBlockIoProtocol *>(static_cast<const EfiBlockIoProtocol *>(intf));
  const auto sync_reads_before = read_calls;
  ASSERT_EQ(EFI_STATUS_SUCCESS, sync->read_blocks(sync, 0, 0, 0, nullptr),
            "empty sync read accepts null buffer");
  ASSERT_EQ(sync_reads_before, read_calls, "empty sync read must not submit BIO");
  char *buffer = memory + 512;
  const size_t length = PAGE_SIZE;
  memset(memory, 0x3c, 3 * PAGE_SIZE);
  ASSERT_EQ(EFI_STATUS_SUCCESS, sync->read_blocks(sync, 0, 0, length, buffer), "kernel-only sync BIO");
  ASSERT_EQ(0xa7, static_cast<unsigned char>(buffer[length - 1]), "cross-page read result");
  ASSERT_EQ(0x3c, static_cast<unsigned char>(buffer[-1]), "leading canary");
  ASSERT_EQ(0x3c, static_cast<unsigned char>(buffer[length]), "trailing canary");

  ASSERT_EQ(EFI_STATUS_SUCCESS, open_async_block_device(dev->name, &intf), "async interface");
  auto *async = const_cast<EfiBlockIo2Protocol *>(static_cast<const EfiBlockIo2Protocol *>(intf));
  auto *token = static_cast<EfiBlockIo2Token *>(uefi_malloc(sizeof(EfiBlockIo2Token)));
  ASSERT_NONNULL(token, "UEFI token");
  ASSERT_EQ(EFI_STATUS_SUCCESS, create_event(0, 0, nullptr, nullptr, &token->event), "completion event");
  DEFER { close_event(token->event); };
  token->transaction_status = EFI_STATUS_NOT_READY;
  memset(buffer, 0, length);
  ASSERT_EQ(EFI_STATUS_SUCCESS, async->read_blocks_ex(async, 0, 0, token, length, buffer), "kernel-only async BIO");
  ASSERT_EQ(EFI_STATUS_SUCCESS, token->transaction_status, "completion can access UEFI token");
  ASSERT_EQ(EFI_STATUS_SUCCESS, check_event(token->event), "event signaled");
  ASSERT_EQ(0xa7, static_cast<unsigned char>(buffer[length - 1]), "async read visible in UEFI");
  ASSERT_EQ(0x3c, static_cast<unsigned char>(buffer[-1]), "async leading canary");
  ASSERT_EQ(0x3c, static_cast<unsigned char>(buffer[length]), "async trailing canary");
  // Both native-async and thread-fallback devices must complete empty reads
  // immediately without submitting BIO, even when the buffer is null.
  const auto reads_before = read_calls;
  for (unsigned backend = 0; backend < 2; ++backend) {
    dev->read_async = backend == 0 ? kernel_read_async : nullptr;
    for (unsigned null_buffer = 0; null_buffer < 2; ++null_buffer) {
      auto *empty = static_cast<EfiBlockIo2Token *>(uefi_malloc(sizeof(EfiBlockIo2Token)));
      ASSERT_NONNULL(empty, "empty-read token");
      ASSERT_EQ(EFI_STATUS_SUCCESS, create_event(0, 0, nullptr, nullptr, &empty->event), "empty-read event");
      const auto event = empty->event;
      DEFER { close_event(event); };
      empty->transaction_status = EFI_STATUS_NOT_READY;
      ASSERT_EQ(EFI_STATUS_NOT_READY, check_event(empty->event), "event initially unsignaled");
      ASSERT_EQ(EFI_STATUS_SUCCESS,
                async->read_blocks_ex(async, 0, 0, empty, 0, null_buffer ? nullptr : buffer),
                "empty read succeeds");
      ASSERT_EQ(EFI_STATUS_SUCCESS, empty->transaction_status, "empty read completes immediately");
      ASSERT_EQ(EFI_STATUS_SUCCESS, check_event(empty->event), "empty read signals event");
      ASSERT_EQ(reads_before, read_calls, "empty read must not submit BIO");
      empty->event = nullptr;
      empty->transaction_status = EFI_STATUS_NOT_READY;
      ASSERT_EQ(EFI_STATUS_SUCCESS,
                async->read_blocks_ex(async, 0, 0, empty, 0, null_buffer ? nullptr : buffer),
                "empty blocking read succeeds without an event");
      ASSERT_EQ(EFI_STATUS_SUCCESS, empty->transaction_status, "blocking token completed");
      ASSERT_EQ(reads_before, read_calls, "empty blocking read must not submit BIO");
    }
  }
  END_TEST;
}
// The real UEFI pool needs a contiguous 300 MiB allocation. Run this fixture
// explicitly on a fresh 512 MiB guest, not after the heap-fragmenting ut all.
int cmd_protocol_test(int, const console_cmd_args *) {
  const bool passed = protocol_buffers();
  printf("UEFI IO protocol test: %s\n", passed ? "PASS" : "FAIL");
  return passed ? 0 : -1;
}
STATIC_COMMAND_START
STATIC_COMMAND("uefi_io_protocol_test", "test UEFI buffers in kernel-only BIO", &cmd_protocol_test)
STATIC_COMMAND_END(uefi_io_protocol_test);
} // namespace

BEGIN_TEST_CASE(uefi_io_buffer_tests)
RUN_TEST(buffer_aliases)
END_TEST_CASE(uefi_io_buffer_tests)
