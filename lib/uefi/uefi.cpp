#include "defer.h"
#include "kernel/vm.h"
#include "pe.h"

#include <lib/bio.h>
#include <lib/heap.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <platform.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "efi.h"

// ASCII "PE\x0\x0"

EfiStatus output_string(struct EfiSimpleTextOutputProtocol *self,
                        char16_t *string) {
  char buffer[512];
  size_t i = 0;
  while (string[i]) {
    size_t j = 0;
    for (j = 0; j < sizeof(buffer) - 1 && string[i + j]; j++) {
      buffer[j] = string[i + j];
    }
    i += j;
    buffer[j] = 0;

    printf("%s", reinterpret_cast<const char *>(buffer));
  }
  return SUCCESS;
}

typedef int (*EfiEntry)(void *handle, struct EfiSystemTable *system);

void *alloc_page(size_t size) {
  void *vptr{};
  status_t err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), "uefi_program",
                                      size, &vptr, 0, 0, 0);
  if (err) {
    printf("Failed to allocate memory for uefi program %d\n", err);
    return nullptr;
  }
  return vptr;
}

int load_sections_and_execute(bdev_t *dev,
                              const IMAGE_NT_HEADERS64 *pe_header) {
  const auto file_header = &pe_header->FileHeader;
  const auto optional_header = &pe_header->OptionalHeader;
  const auto sections = file_header->NumberOfSections;
  const auto section_header = reinterpret_cast<const IMAGE_SECTION_HEADER *>(
      reinterpret_cast<const char *>(pe_header) + sizeof(*pe_header));
  for (size_t i = 0; i < sections; i++) {
    if (section_header[i].NumberOfRelocations != 0) {
      printf("Section %s requires relocation, which is not supported.\n",
             section_header[i].Name);
      return -6;
    }
  }
  const auto &last_section = section_header[sections - 1];
  const auto virtual_size =
      last_section.VirtualAddress + last_section.Misc.VirtualSize;
  const auto image_base = reinterpret_cast<char *>(alloc_page(virtual_size));
  memset(image_base, 0, virtual_size);

  for (size_t i = 0; i < sections; i++) {
    const auto &section = section_header[i];
    bio_read(dev, image_base + section.VirtualAddress, section.PointerToRawData,
             section.SizeOfRawData);
  }
  auto entry = reinterpret_cast<EfiEntry>(image_base +
                                          optional_header->AddressOfEntryPoint);
  printf("Entry function located at %p\n", entry);

  EfiSystemTable table;
  EfiSimpleTextOutputProtocol console_out;
  console_out.output_string = output_string;
  table.con_out = &console_out;
  return entry(nullptr, &table);
}

int load_pe_file(const char *blkdev) {
  bdev_t *dev = bio_open(blkdev);
  if (!dev) {
    printf("error opening block device %s\n", blkdev);
    return -1;
  }
  DEFER { bio_close(dev); };
  constexpr size_t kBlocKSize = 4096;

  lk_time_t t = current_time();
  uint8_t *address = (uint8_t *)malloc(kBlocKSize);
  ssize_t err = bio_read(dev, (void *)address, 0, kBlocKSize);
  t = current_time() - t;
  dprintf(INFO, "bio_read returns %d, took %u msecs (%d bytes/sec)\n", (int)err,
          (uint)t, (uint32_t)((uint64_t)err * 1000 / t));

  const auto dos_header = reinterpret_cast<const IMAGE_DOS_HEADER *>(address);
  if (!dos_header->CheckMagic()) {
    printf("DOS Magic check failed %x\n", dos_header->e_magic);
    return -2;
  }
  if (dos_header->e_lfanew > kBlocKSize - sizeof(IMAGE_FILE_HEADER)) {
    printf("Invalid PE header offset %d exceeds maximum read size of %d - %d\n",
           dos_header->e_lfanew, kBlocKSize, sizeof(IMAGE_FILE_HEADER));
    return -3;
  }
  const auto pe_header = dos_header->GetPEHeader();
  const auto file_header = &pe_header->FileHeader;
  if (LE32(file_header->Signature) != kPEHeader) {
    printf("COFF Magic check failed %x\n", LE32(file_header->Signature));
    return -4;
  }
  printf("PE header machine type: %x\n",
         static_cast<u16>(file_header->Machine));
  if (file_header->SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER64)) {
    printf("Unexpected size of optional header %d, expected %d\n",
           file_header->SizeOfOptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER64));
    return -5;
  }
  const auto optional_header = &pe_header->OptionalHeader;
  if (optional_header->Subsystem != SubsystemType::EFIApplication) {
    printf("Unsupported Subsystem type: %d %s\n", optional_header->Subsystem,
           ToString(optional_header->Subsystem));
  }
  printf("Valid UEFI application found.\n");
  return load_sections_and_execute(dev, pe_header);
}

int cmd_uefi_load(int argc, const console_cmd_args *argv) {
  if (argc != 2) {
    printf("Usage: %s <name of block device to load from>\n", argv[0].str);
    return 1;
  }
  load_pe_file(argv[1].str);
  return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("uefi_load", "load UEFI application and run it", &cmd_uefi_load)
STATIC_COMMAND_END(uefi);
