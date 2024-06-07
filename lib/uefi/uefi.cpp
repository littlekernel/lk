#include "defer.h"
#include "pe.h"

#include <lib/bio.h>
#include <lib/heap.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <platform.h>
#include <string.h>

// ASCII "PE\x0\x0"
static constexpr uint32_t kPEHeader = 0x4550;

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
    printf("Invalid PE header offset %d exceeds maximum read size of %u - %u\n",
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
         static_cast<int>(file_header->Machine));
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

  return 0;
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
