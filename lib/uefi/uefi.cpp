/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "uefi/uefi.h"

#include <lib/bio.h>
#include <lib/fs.h>
#include <lib/heap.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <platform.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <uefi/boot_service.h>
#include <uefi/protocols/simple_text_output_protocol.h>
#include <uefi/runtime_service.h>
#include <uefi/system_table.h>

#include "blockio_protocols.h"
#include "boot_service_provider.h"
#include "charset.h"
#include "configuration_table.h"
#include "debug_support.h"
#include "defer.h"
#include "memory_protocols.h"
#include "pe.h"
#include "relocation.h"
#include "runtime_service_provider.h"
#include "switch_stack.h"
#include "text_protocol.h"
#include "uefi/types.h"
#include "uefi_platform.h"
#include "variable_mem.h"

namespace {

constexpr auto EFI_SYSTEM_TABLE_SIGNATURE =
    static_cast<u64>(0x5453595320494249ULL);

using EfiEntry = int (*)(void *, struct EfiSystemTable *);

const char16_t firmwareVendor[] = u"Little Kernel";

class ImageReader {
public:
  virtual ssize_t read(char *buf, off_t offset, size_t len) = 0;
  virtual void get_name(char *buf, size_t buf_size) = 0;
};

class ImageReaderBdev final : public ImageReader {
private:
  bdev_t *dev;

public:
  ImageReaderBdev(bdev_t *dev1): dev(dev1) {}

  ssize_t read(char *buf, off_t offset, size_t len) {
    return bio_read(dev, static_cast<void *>(buf), offset, len);
  }

  void get_name(char *buf, size_t buf_size) {
    if (buf_size <= 0) {
      return;
    }
    strncpy(buf, dev->name, buf_size - 1);
  }
};

class ImageReaderFilehandle final : public ImageReader {
private:
  filehandle *file_handle;
  const char *path;

public:
  ImageReaderFilehandle(filehandle *file_handle1, const char *path1): file_handle(file_handle1), path(path1) {}

  ssize_t read(char *buf, off_t offset, size_t len) {
    return fs_read_file(file_handle, buf, offset, len);
  }

  void get_name(char *buf, size_t buf_size) {
    if (buf_size <= 0) {
      return;
    }
    strncpy(buf, path, buf_size);
    for (size_t i = 0; i < buf_size && buf[i]; i++) {
      if (buf[i] == '/') {
        buf[i] = '\\';
      }
    }
  }
};

// PE/COFF optional-header rules (not the host kernel's page geometry):
// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-windows-specific-fields-image-only
constexpr uint16_t kPe32PlusMagic = 0x20b;
constexpr uint32_t kPePageSize = 4096;
constexpr uint32_t kPeMinFileAlignment = 512;
constexpr uint32_t kPeMaxFileAlignment = 64 * 1024;  // FileAlignment upper limit.
constexpr uint64_t kPeImageBaseAlignment = 64 * 1024;  // ImageBase granularity.

// The caller has already checked the fixed fields and optional-header extent.
int validate_optional_header(const IMAGE_FILE_HEADER *file_header,
                             const IMAGE_OPTIONAL_HEADER64 *optional_header,
                             size_t section_table_end) {
  if (optional_header->Subsystem != SubsystemType::EFIApplication) {
    printf("Unsupported Subsystem type: %d %s\n", optional_header->Subsystem,
           ToString(optional_header->Subsystem));
    return ERR_NOT_SUPPORTED;
  }
  if (optional_header->Magic != kPe32PlusMagic) {
    printf("Expected PE32+ optional header magic\n");
    return ERR_BAD_STATE;
  }
  const size_t directory_capacity =
      (file_header->SizeOfOptionalHeader -
       offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory)) /
      sizeof(IMAGE_DATA_DIRECTORY);
  if (optional_header->NumberOfRvaAndSizes > directory_capacity) {
    printf("PE data directories exceed optional header\n");
    return ERR_BAD_STATE;
  }
  const uint32_t image_size = optional_header->SizeOfImage;
  const uint32_t headers_size = optional_header->SizeOfHeaders;
  const uint32_t section_align = optional_header->SectionAlignment;
  const uint32_t file_align = optional_header->FileAlignment;
  if (file_align == 0 || (file_align & (file_align - 1)) != 0) {
    printf("Invalid PE file alignment: file_align=%u\n", file_align);
    return ERR_BAD_STATE;
  }
  if (section_align == 0 || (section_align & (section_align - 1)) != 0 ||
      section_align < file_align) {
    printf("Invalid PE section alignment: section_align=%u file_align=%u\n",
           section_align, file_align);
    return ERR_BAD_STATE;
  }
  // PE permits sub-page section alignment only when file alignment matches it.
  if (section_align < kPePageSize
          ? file_align != section_align
          : (file_align < kPeMinFileAlignment ||
             file_align > kPeMaxFileAlignment)) {
    printf("Invalid PE alignment combination: section_align=%u file_align=%u\n",
           section_align, file_align);
    return ERR_BAD_STATE;
  }
  // Check the remaining size_t range before rounding up to a native page.
  if (image_size == 0 ||
      SIZE_MAX - static_cast<size_t>(image_size) < PAGE_SIZE - 1) {
    printf("Invalid PE image size: image_size=%u page_size=%zu\n",
           image_size, static_cast<size_t>(PAGE_SIZE));
    return ERR_BAD_STATE;
  }
  if (headers_size < section_table_end || headers_size > image_size ||
      headers_size % file_align != 0) {
    printf("Invalid PE header size: headers_size=%u section_table_end=%zu "
           "image_size=%u file_align=%u\n",
           headers_size, section_table_end, image_size, file_align);
    return ERR_BAD_STATE;
  }
  if (optional_header->ImageBase % kPeImageBaseAlignment != 0 ||
      optional_header->ImageBase > UINT64_MAX - image_size) {
    printf("Invalid PE image base: ImageBase=0x%llx image_size=%u "
           "base_alignment=%llu\n",
           optional_header->ImageBase, image_size,
           static_cast<unsigned long long>(kPeImageBaseAlignment));
    return ERR_BAD_STATE;
  }
  return NO_ERROR;
}

// Called only after validate_optional_header and section-table bounds checks.
int validate_section_header(const IMAGE_FILE_HEADER *file_header,
                            const IMAGE_OPTIONAL_HEADER64 *optional_header,
                            const IMAGE_SECTION_HEADER *section_header) {
  const uint32_t image_size = optional_header->SizeOfImage;
  const uint32_t headers_size = optional_header->SizeOfHeaders;
  const uint32_t section_align = optional_header->SectionAlignment;
  const uint32_t file_align = optional_header->FileAlignment;
  uint32_t previous_end = headers_size;
  for (size_t i = 0; i < file_header->NumberOfSections; i++) {
    const auto &section = section_header[i];
    const uint32_t span = MAX(section.Misc.VirtualSize, section.SizeOfRawData);
    // Empty placeholder sections do not occupy memory or advance previous_end.
    if (section.VirtualAddress == 0 && span == 0)
      continue;
    // Subtraction after the RVA check avoids wrapping a 32-bit section end.
    // Ordered, disjoint sections must not overwrite the validated headers.
    if (section.VirtualAddress < previous_end ||
        section.VirtualAddress % section_align != 0 ||
        section.VirtualAddress > image_size ||
        section.Misc.VirtualSize > image_size - section.VirtualAddress) {
      printf("Invalid PE section %.8s range or alignment\n", section.Name);
      return ERR_BAD_STATE;
    }
    if (section.SizeOfRawData != 0 &&
        (section.SizeOfRawData > image_size - section.VirtualAddress ||
         section.PointerToRawData < headers_size ||
         section.PointerToRawData % file_align != 0 ||
         (section_align < kPePageSize &&
          section.PointerToRawData != section.VirtualAddress))) {
      printf("Invalid PE section %.8s raw data range or alignment\n", section.Name);
      return ERR_BAD_STATE;
    }
    previous_end = section.VirtualAddress + span;
  }
  // Validate every section before accepting an entry in an earlier section.
  for (size_t i = 0; i < file_header->NumberOfSections; i++) {
    const auto &section = section_header[i];
    // The entry must be in executable file data, not headers, gaps or BSS.
    // Instruction size and alignment belong to the target architecture, not
    // this format/range check.
    const uint32_t entry = optional_header->AddressOfEntryPoint;
    constexpr uint32_t kSectionMemExecute = 0x20000000;
    if ((section.Characteristics & kSectionMemExecute) != 0 &&
        entry >= section.VirtualAddress &&
        entry - section.VirtualAddress < section.SizeOfRawData) {
      return NO_ERROR;
    }
  }
  printf("PE entry point is not in executable section data\n");
  return ERR_BAD_STATE;
}

int load_sections_and_execute(ImageReader *reader,
                              const IMAGE_NT_HEADERS64 *pe_header,
                              const uint8_t *headers, size_t header_bytes) {
  const auto file_header = &pe_header->FileHeader;
  const auto optional_header = &pe_header->OptionalHeader;
  const auto sections = file_header->NumberOfSections;
  const auto section_header = reinterpret_cast<const IMAGE_SECTION_HEADER *>(
      reinterpret_cast<const char *>(pe_header) + sizeof(IMAGE_FILE_HEADER) +
      file_header->SizeOfOptionalHeader);
  if (sections <= 0) {
    printf("This PE file does not have any sections, unsupported.\n");
    return ERR_BAD_STATE;
  }
  for (size_t i = 0; i < sections; i++) {
    if (section_header[i].NumberOfRelocations != 0) {
      printf("Section %.8s requires relocation, which is not supported.\n",
             section_header[i].Name);
      return ERR_NOT_SUPPORTED;
    }
  }
  setup_heap();
  DEFER { reset_heap(); };
  DEFER { release_boot_buffers(); };
  DEFER { close_tracked_bdevs(); };
  const size_t virtual_size = ROUNDUP(
      static_cast<size_t>(optional_header->SizeOfImage), PAGE_SIZE);
  // For casting ImageBase to optional_header
  // NOLINTBEGIN(performance-no-int-to-ptr)
  const auto image_base = reinterpret_cast<char *>(
      alloc_page(reinterpret_cast<void *>(optional_header->ImageBase),
                 virtual_size, 21 /* Kernel requires 2MB alignment */));
  // NOLINTEND(performance-no-int-to-ptr)
  if (image_base == nullptr) {
    return ERR_NO_MEMORY;
  }
  memset(image_base, 0, virtual_size);
  DEFER { free_pages(image_base, virtual_size / PAGE_SIZE); };
  // Keep the validated header snapshot: do not parse freshly reread metadata
  // if the backing device/file changed between reads.
  const size_t copied_headers = MIN(header_bytes, optional_header->SizeOfHeaders);
  memcpy(image_base, headers, copied_headers);
  const size_t remaining_headers = optional_header->SizeOfHeaders - copied_headers;
  if (remaining_headers != 0 &&
      reader->read(image_base + copied_headers, copied_headers, remaining_headers) !=
          static_cast<ssize_t>(remaining_headers)) {
    printf("Failed to read PE headers\n");
    return ERR_IO;
  }

  for (size_t i = 0; i < sections; i++) {
    const auto &section = section_header[i];
    if (section.SizeOfRawData == 0) {
      continue;
    }
    const ssize_t bytes_read =
        reader->read(image_base + section.VirtualAddress,
                     section.PointerToRawData, section.SizeOfRawData);
    if (bytes_read != section.SizeOfRawData) {
      printf("Failed to read section %.8s %zd\n", section.Name, bytes_read);
      return ERR_IO;
    }
  }
  printf("Relocating image from 0x%llx to %p\n", optional_header->ImageBase,
         image_base);
  if (relocate_image(image_base, optional_header->SizeOfImage) != 0) {
    printf("Failed to relocate image\n");
    return ERR_BAD_STATE;
  }
  auto entry = reinterpret_cast<int (*)(void *, void *)>(
      image_base + optional_header->AddressOfEntryPoint);
  printf("Entry function located at %p\n", entry);

  auto *system_table = static_cast<EfiSystemTable *>(alloc_page(PAGE_SIZE));
  if (system_table == nullptr) {
    return ERR_NO_MEMORY;
  }
  EfiSystemTable &table = *system_table;
  memset(&table, 0, sizeof(EfiSystemTable));
  DEFER { free_pages(&table, 1); };
  EfiBootService boot_service{};
  EfiRuntimeService runtime_service{};
  setup_runtime_service_table(&runtime_service);
  setup_boot_service_table(&boot_service);
  table.firmware_vendor = reinterpret_cast<const EfiChar16*>(firmwareVendor);
  table.runtime_services = &runtime_service;
  table.boot_services = &boot_service;
  table.header.signature = EFI_SYSTEM_TABLE_SIGNATURE;
  table.header.revision = 2 << 16;
  EfiSimpleTextOutputProtocol console_out = get_text_output_protocol();
  table.con_out = &console_out;
  auto configuration_table =
      reinterpret_cast<EfiConfigurationTable *>(alloc_page(PAGE_SIZE));
  if (configuration_table == nullptr) {
    return ERR_NO_MEMORY;
  }
  table.configuration_table = configuration_table;
  DEFER { free_pages(configuration_table, 1); };
  memset(configuration_table, 0, PAGE_SIZE);
  setup_configuration_table(&table, configuration_table);
  auto status = platform_setup_system_table(&table);
  if (status != EFI_STATUS_SUCCESS) {
    printf("platform_setup_system_table failed: %lu\n", status);
    return -static_cast<int>(status);
  }
  status = efi_initialize_system_table_pointer(&table);
  if (status != EFI_STATUS_SUCCESS) {
    printf("efi_initialize_system_table_pointer failed: %lu\n", status);
    return -static_cast<int>(status);
  }
  DEFER { efi_uninitialize_system_table_pointer(); };
  char path[FS_MAX_PATH_LEN];
  reader->get_name(path, sizeof(path));
  path[sizeof(path) - 1] = '\0';
  setup_debug_support(table, image_base, virtual_size, path);
  DEFER { teardown_debug_support(image_base); };

  constexpr size_t kStackSize = 1 * 1024ul * 1024;
  auto stack = reinterpret_cast<char *>(alloc_page(kStackSize, 23));
  if (stack == nullptr) {
    return ERR_NO_MEMORY;
  }
  memset(stack, 0, kStackSize);
  DEFER {
    free_pages(stack, kStackSize / PAGE_SIZE);
    stack = nullptr;
  };
  printf("Calling kernel with stack [%p, %p]\n", stack, stack + kStackSize - 1);
  int ret = static_cast<int>(
      call_with_stack(stack + kStackSize, entry, image_base, &table));

  return ret;
}

int cmd_uefi_load(int argc, const console_cmd_args *argv) {
  if (argc != 2) {
    printf("Usage: %s <name of block device to load from>\n", argv[0].str);
    return ERR_INVALID_ARGS;
  }
  if (argv[1].str[0] == '/') {
    load_pe_fs(argv[1].str);
  } else {
    load_pe_blockdev(argv[1].str);
  }
  return 0;
}

int cmd_uefi_set_variable(int argc, const console_cmd_args *argv) {
  if (argc != 3) {
    printf("Usage: %s <variable> <data>\n", argv[0].str);
    return ERR_INVALID_ARGS;
  }
  EfiGuid guid = EFI_GLOBAL_VARIABLE_GUID;
  char16_t buffer[128];
  utf8_to_utf16(buffer, argv[1].str, sizeof(buffer) / sizeof(buffer[0]));
  efi_set_variable(buffer, &guid, EFI_VARIABLE_BOOTSERVICE_ACCESS, argv[2].str,
                   strlen(argv[2].str));
  return 0;
}

int cmd_uefi_list_variable(int argc, const console_cmd_args *argv) {
  efi_list_variable();
  return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("uefi_load", "load UEFI application and run it", &cmd_uefi_load)
STATIC_COMMAND("uefi_set_var", "set UEFI variable", &cmd_uefi_set_variable)
STATIC_COMMAND("uefi_list_var", "list UEFI variable", &cmd_uefi_list_variable)
STATIC_COMMAND_END(uefi);

} // namespace

int load_pe_file(ImageReader *reader) {
  constexpr size_t kBlocKSize = 4096;

  lk_time_t t = current_time();
  uint8_t *address = static_cast<uint8_t *>(malloc(kBlocKSize));
  if (address == nullptr) {
    printf("failed to allocate %zu bytes memory for PE header\n", kBlocKSize);
    return ERR_NO_MEMORY;
  }
  DEFER { free(address); };
  ssize_t err = reader->read(reinterpret_cast<char *>(address), 0, kBlocKSize);
  // Prevent divide by 0 errors
  t = MAX(current_time() - t, 1);
  if (err < 0) {
    char name[128];
    reader->get_name(name, sizeof(name));
    name[sizeof(name) - 1] = '\0';
    printf("error reading PE header from %s: %zd\n", name, err);
    return ERR_IO;
  }
  dprintf(INFO, "bio_read returns %d, took %u msecs (%d bytes/sec)\n", (int)err,
          (uint)t, (uint32_t)((uint64_t)err * 1000 / t));

  const size_t header_bytes = static_cast<size_t>(err);
  if (header_bytes < sizeof(IMAGE_DOS_HEADER)) {
    printf("File too small for a DOS header: %zu bytes\n", header_bytes);
    return ERR_BAD_STATE;
  }
  const auto dos_header = reinterpret_cast<const IMAGE_DOS_HEADER *>(address);
  if (!dos_header->CheckMagic()) {
    printf("DOS Magic check failed %x\n", dos_header->e_magic);
    return ERR_BAD_STATE;
  }
  if (dos_header->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
      dos_header->e_lfanew > header_bytes - sizeof(IMAGE_FILE_HEADER)) {
    printf(
        "Invalid PE header offset %u for %zu bytes (COFF header size %zu)\n",
        dos_header->e_lfanew, header_bytes, sizeof(IMAGE_FILE_HEADER));
    return ERR_BAD_STATE;
  }
  const auto pe_header = dos_header->GetPEHeader();
  const auto file_header = &pe_header->FileHeader;
  if (LE32(file_header->Signature) != kPEHeader) {
    printf("COFF Magic check failed %x\n", LE32(file_header->Signature));
    return ERR_BAD_STATE;
  }
  if (file_header->Machine != ArchitectureType::ARM64) {
    printf("Unsupported PE header machine type: %x\n",
           static_cast<int>(file_header->Machine));
    return ERR_NOT_SUPPORTED;
  }
  if (file_header->SizeOfOptionalHeader > sizeof(IMAGE_OPTIONAL_HEADER64) ||
      file_header->SizeOfOptionalHeader <
          sizeof(IMAGE_OPTIONAL_HEADER64) -
              sizeof(IMAGE_OPTIONAL_HEADER64::DataDirectory)) {
    printf("Unexpected size of optional header %d, expected %zu\n",
           file_header->SizeOfOptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER64));
    return ERR_BAD_STATE;
  }
  const size_t nt_headers_end = static_cast<size_t>(dos_header->e_lfanew) +
                                sizeof(IMAGE_FILE_HEADER) +
                                file_header->SizeOfOptionalHeader;
  if (nt_headers_end > header_bytes) {
    printf("PE optional header does not fit in the %zu bytes read\n",
           header_bytes);
    return ERR_BAD_STATE;
  }
  const size_t section_table_end =
      nt_headers_end + static_cast<size_t>(file_header->NumberOfSections) *
                           sizeof(IMAGE_SECTION_HEADER);
  if (section_table_end > header_bytes) {
    printf("PE section table does not fit in the %zu bytes read\n",
           header_bytes);
    return ERR_BAD_STATE;
  }
  const auto optional_header = &pe_header->OptionalHeader;
  auto status = validate_optional_header(file_header, optional_header,
                                         section_table_end);
  if (status != NO_ERROR) {
    return status;
  }
  const auto section_header = reinterpret_cast<const IMAGE_SECTION_HEADER *>(
      address + nt_headers_end);
  status = validate_section_header(file_header, optional_header, section_header);
  if (status != NO_ERROR) {
    return status;
  }
  printf("Valid UEFI application found.\n");
  auto ret = load_sections_and_execute(reader, pe_header, address, header_bytes);
  printf("UEFI Application return code: %d\n", ret);
  return ret;
}

int load_pe_blockdev(const char *blkdev) {
  bdev_t *dev = bio_open(blkdev);

  if (!dev) {
    printf("error opening block device %s\n", blkdev);
    return -1;
  }

  DEFER {
    bio_close(dev);
    dev = nullptr;
  };

  ImageReaderBdev reader(dev);

  return load_pe_file(&reader);
}

int load_pe_fs(const char *path) {
  filehandle *file_handle = nullptr;

  status_t status = fs_open_file(path, &file_handle);
  if (status < 0) {
    printf("error opening file %s\n", path);
    return -1;
  }

  DEFER {
    fs_close_file(file_handle);
    file_handle = nullptr;
  };

  ImageReaderFilehandle reader(file_handle, path);

  return load_pe_file(&reader);

}
