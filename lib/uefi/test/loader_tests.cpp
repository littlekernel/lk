/*
 * Copyright (c) 2026 Kelvin Zhang
 *
 * Use of this source code is governed by a MIT-style license that can be
 * found in the LICENSE file or at https://opensource.org/licenses/MIT
 */

#include <kernel/vm.h>
#include <lib/bio.h>
#include <lib/unittest.h>
#include <lk/compiler.h>
#include <lk/err.h>
#include <stdlib.h>
#include <string.h>
#include <uefi/uefi.h>

#include "../defer.h"
#include "../pe.h"

// Use the same executable as the documented virtio smoke test. No host disk
// or fixture generator is needed to run these tests with `ut all`.
INCFILE(uefi_test_image, uefi_test_image_size, LOCAL_DIR "/../helloworld_aa64.efi");

namespace {

constexpr size_t kFileGap = 32768;

struct Fixture {
    uint8_t *data;
    size_t size;

    IMAGE_DOS_HEADER &dos() { return *reinterpret_cast<IMAGE_DOS_HEADER *>(data); }
    IMAGE_FILE_HEADER &file() { return dos().GetPEHeader()->FileHeader; }
    IMAGE_OPTIONAL_HEADER64 &opt() { return dos().GetPEHeader()->OptionalHeader; }
    IMAGE_SECTION_HEADER *sections() {
        return reinterpret_cast<IMAGE_SECTION_HEADER *>(
                data + dos().e_lfanew + sizeof(IMAGE_FILE_HEADER) + file().SizeOfOptionalHeader);
    }
};

struct LoaderCase {
    const char *name;
    int expected;
    void (*mutate)(Fixture &);
};

const LoaderCase cases[] = {
    {"valid", NO_ERROR, [](Fixture &) {}},
    {"dos_magic", ERR_BAD_STATE, [](Fixture &f) { f.dos().e_magic = 0; }},
    {"pe_offset", ERR_BAD_STATE, [](Fixture &f) { f.dos().e_lfanew = 4090; }},
    {"pe_in_dos", ERR_BAD_STATE, [](Fixture &f) { f.dos().e_lfanew = 0; }},
    {"pe_signature", ERR_BAD_STATE, [](Fixture &f) { f.file().Signature = 0; }},
    {"machine", ERR_NOT_SUPPORTED, [](Fixture &f) { f.file().Machine = ArchitectureType::AMD64; }},
    {"optional_size", ERR_BAD_STATE, [](Fixture &f) { f.file().SizeOfOptionalHeader = 0; }},
    {"section_table", ERR_BAD_STATE, [](Fixture &f) { f.file().NumberOfSections = 65535; }},
    {"optional_magic", ERR_BAD_STATE, [](Fixture &f) { f.opt().Magic = 0; }},
    {"pe32_magic", ERR_BAD_STATE, [](Fixture &f) { f.opt().Magic = 0x10b; }},
    {"subsystem", ERR_NOT_SUPPORTED, [](Fixture &f) { f.opt().Subsystem = WindowsCUI; }},
    {"directories", ERR_BAD_STATE, [](Fixture &f) { f.opt().NumberOfRvaAndSizes = 17; }},
    {"section_align_zero", ERR_BAD_STATE, [](Fixture &f) { f.opt().SectionAlignment = 0; }},
    {"section_align_nonpower", ERR_BAD_STATE, [](Fixture &f) { f.opt().SectionAlignment = 4095; }},
    {"file_align_zero", ERR_BAD_STATE, [](Fixture &f) { f.opt().FileAlignment = 0; }},
    {"file_align_large", ERR_BAD_STATE, [](Fixture &f) { f.opt().FileAlignment = 8192; }},
    {"image_zero", ERR_BAD_STATE, [](Fixture &f) { f.opt().SizeOfImage = 0; }},
    {"image_unaligned", NO_ERROR, [](Fixture &f) { ++f.opt().SizeOfImage; }},
    {"headers_small", ERR_BAD_STATE, [](Fixture &f) { f.opt().SizeOfHeaders = 64; }},
    {"headers_large", ERR_BAD_STATE, [](Fixture &f) { f.opt().SizeOfHeaders = f.opt().SizeOfImage + 512; }},
    {"base_unaligned", ERR_BAD_STATE, [](Fixture &f) { f.opt().ImageBase = 1; }},
    {"base_wrap", ERR_BAD_STATE, [](Fixture &f) {
        f.opt().ImageBase = UINT64_C(0xffffffffffff0000);
        f.opt().SizeOfImage = 0x20000;
    }},
    {"section_rva", ERR_BAD_STATE, [](Fixture &f) { f.sections()[0].VirtualAddress = 0x20000000; }},
    {"section_wrap", ERR_BAD_STATE, [](Fixture &f) { f.sections()[0].VirtualAddress = 0xfffff000; }},
    {"section_virtual_size", ERR_BAD_STATE, [](Fixture &f) { f.sections()[0].Misc.VirtualSize = UINT32_MAX; }},
    {"section_raw_size", ERR_BAD_STATE, [](Fixture &f) { f.sections()[0].SizeOfRawData = 0xfffffe00; }},
    {"section_headers", ERR_BAD_STATE, [](Fixture &f) { f.sections()[0].VirtualAddress = 0; }},
    {"sections_overlap", ERR_BAD_STATE, [](Fixture &f) {
        f.sections()[1].VirtualAddress = f.sections()[0].VirtualAddress;
    }},
    {"raw_in_headers", ERR_BAD_STATE, [](Fixture &f) { f.sections()[0].PointerToRawData = 0; }},
    {"raw_unaligned", ERR_BAD_STATE, [](Fixture &f) {
        f.sections()[0].PointerToRawData = f.opt().SizeOfHeaders + 1;
    }},
    {"later_section_raw_size", ERR_BAD_STATE, [](Fixture &f) { f.sections()[1].SizeOfRawData = 0xfffffe00; }},
    {"entry_outside", ERR_BAD_STATE, [](Fixture &f) { f.opt().AddressOfEntryPoint = 0x20000000; }},
    {"entry_headers", ERR_BAD_STATE, [](Fixture &f) { f.opt().AddressOfEntryPoint = 0; }},
    {"entry_nonexec", ERR_BAD_STATE, [](Fixture &f) {
        f.opt().AddressOfEntryPoint = f.sections()[1].VirtualAddress;
    }},
    {"entry_gap", ERR_BAD_STATE, [](Fixture &f) {
        f.opt().AddressOfEntryPoint = f.sections()[0].VirtualAddress + f.sections()[0].SizeOfRawData;
    }},
    {"no_sections", ERR_BAD_STATE, [](Fixture &f) { f.file().NumberOfSections = 0; }},
    {"entry_bss", ERR_BAD_STATE, [](Fixture &f) {
        auto &text = f.sections()[0];
        text.Misc.VirtualSize = text.SizeOfRawData + 256;
        f.opt().AddressOfEntryPoint = text.VirtualAddress + text.SizeOfRawData;
    }},
    {"subpage_alignment_mismatch", ERR_BAD_STATE, [](Fixture &f) { f.opt().SectionAlignment = 1024; }},
    {"file_alignment_below_min", ERR_BAD_STATE, [](Fixture &f) { f.opt().FileAlignment = 256; }},
    {"entry_image_end", ERR_BAD_STATE, [](Fixture &f) { f.opt().AddressOfEntryPoint = f.opt().SizeOfImage; }},
    {"headers_unaligned", ERR_BAD_STATE, [](Fixture &f) { ++f.opt().SizeOfHeaders; }},
    {"raw_past_eof", ERR_IO, [](Fixture &f) { f.sections()[0].PointerToRawData = 0xfffffe00; }},
    {"truncated_headers", ERR_IO, [](Fixture &f) { f.size = 512; }},
    {"truncated_section", ERR_IO, [](Fixture &f) { f.size = 4096; }},
    {"file_alignment_above_max", ERR_BAD_STATE, [](Fixture &f) {
        // Keep every later check valid so only the maximum alignment rejects it.
        f.opt().FileAlignment = f.opt().SectionAlignment = 131072;
        f.opt().SizeOfHeaders = 131072;
        f.opt().SizeOfImage = 3 * 131072;
        f.opt().AddressOfEntryPoint = 131072;
        for (size_t i = 0; i < f.file().NumberOfSections; ++i) {
            f.sections()[i].VirtualAddress = (i + 1) * 131072;
            f.sections()[i].PointerToRawData = (i + 1) * 131072;
        }
    }},
    {"section_rva_unaligned", ERR_BAD_STATE, [](Fixture &f) {
        ++f.sections()[0].VirtualAddress;
    }},
    {"subpage_raw_offset_mismatch", ERR_BAD_STATE, [](Fixture &f) {
        f.opt().FileAlignment = f.opt().SectionAlignment = 512;
    }},
    {"raw_size_unaligned", NO_ERROR, [](Fixture &f) {
        ++f.sections()[1].SizeOfRawData;
        f.size = MAX(f.size, static_cast<size_t>(f.sections()[1].PointerToRawData) +
                            f.sections()[1].SizeOfRawData);
    }},
    {"empty_placeholder", NO_ERROR, [](Fixture &f) {
        f.sections()[2] = f.sections()[1];
        f.sections()[1] = {};
        f.file().NumberOfSections = 3;
    }},
    {"empty_then_header_overlap", ERR_BAD_STATE, [](Fixture &f) {
        f.sections()[2] = f.sections()[1];
        f.sections()[2].VirtualAddress = 0;
        f.sections()[1] = {};
        f.file().NumberOfSections = 3;
    }},
    {"empty_then_section_overlap", ERR_BAD_STATE, [](Fixture &f) {
        f.sections()[2] = f.sections()[1];
        f.sections()[2].VirtualAddress = f.sections()[0].VirtualAddress;
        f.sections()[1] = {};
        f.file().NumberOfSections = 3;
    }},
    {"placeholder_with_raw_data", ERR_BAD_STATE, [](Fixture &f) {
        f.sections()[2] = f.sections()[1];
        f.sections()[1] = {};
        f.sections()[1].SizeOfRawData = f.opt().FileAlignment;
        f.sections()[1].PointerToRawData = f.sections()[2].PointerToRawData;
        f.file().NumberOfSections = 3;
        f.size = 4096;
    }},
    {"section_rva_past_image", ERR_BAD_STATE, [](Fixture &f) {
        f.sections()[1].VirtualAddress = 0x20000000;
        f.size = 4096;
    }},
    {"section_overlaps_headers", ERR_BAD_STATE, [](Fixture &f) {
        f.opt().AddressOfEntryPoint -= f.sections()[0].VirtualAddress;
        f.sections()[0].VirtualAddress = 0;
        f.size = 4096;
    }},
    {"valid_contiguous_sections", NO_ERROR, [](Fixture &f) {
        f.sections()[0].Misc.VirtualSize =
                f.sections()[1].VirtualAddress - f.sections()[0].VirtualAddress;
    }},
    {"valid_bss", NO_ERROR, [](Fixture &f) {
        auto &bss = f.sections()[2];
        bss = {};
        memcpy(bss.Name, ".bss", 4);
        bss.Misc.VirtualSize = 4096;
        bss.VirtualAddress = f.opt().SizeOfImage;
        bss.Characteristics = 0xc0000080;
        f.file().NumberOfSections = 3;
        f.opt().SizeOfImage += 4096;
    }},
    {"valid_file_gap", NO_ERROR, [](Fixture &f) {
        size_t headers = f.opt().SizeOfHeaders;
        memmove(f.data + headers + kFileGap, f.data + headers, f.size - headers);
        memset(f.data + headers, 0, kFileGap);
        f.size += kFileGap;
        for (size_t i = 0; i < f.file().NumberOfSections; ++i)
            f.sections()[i].PointerToRawData += kFileGap;
    }},
    {"valid_no_directories", NO_ERROR, [](Fixture &f) {
        auto *sections = f.sections();
        f.file().SizeOfOptionalHeader = offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory);
        f.opt().NumberOfRvaAndSizes = 0;
        memmove(f.sections(), sections, f.file().NumberOfSections * sizeof(*sections));
    }},
    {"valid_raw_padding", NO_ERROR, [](Fixture &f) {
        f.sections()[1].SizeOfRawData = 4096;
        f.size = f.sections()[1].PointerToRawData + 4096;
    }},
};

// A private read-only BIO device, with the descriptor first for the read hook.
// Unlike an external virtio disk, this also supports exact truncated lengths.
struct TestDevice {
    bdev_t dev;
    const uint8_t *data;
    size_t reads;
};

ssize_t read_fixture(bdev_t *dev, void *buf, off_t offset, size_t len) {
    auto *fixture = reinterpret_cast<TestDevice *>(dev);
    ++fixture->reads;
    if (offset < 0 || static_cast<uint64_t>(offset) >= static_cast<uint64_t>(dev->total_size))
        return 0;
    len = MIN(len, static_cast<size_t>(dev->total_size - offset));
    memcpy(buf, fixture->data + offset, len);
    return static_cast<ssize_t>(len);
}

size_t free_pages() {
    size_t count = 0;
    pmm_arena_t *arena;
    list_for_every_entry(get_arena_list(), arena, pmm_arena_t, node) {
        count += arena->free_count;
    }
    return count;
}

bool run_case(const LoaderCase &test, bool warmup) {
    BEGIN_TEST;
    auto *data = static_cast<uint8_t *>(calloc(1, uefi_test_image_size + kFileGap + 4096));
    ASSERT_NONNULL(data, "fixture allocation");
    DEFER { free(data); };
    memcpy(data, uefi_test_image, uefi_test_image_size);
    Fixture fixture{data, uefi_test_image_size};
    // These mutations depend on the checked-in payload's two-section layout.
    ASSERT_EQ(2u, static_cast<unsigned>(fixture.file().NumberOfSections), "fixture sections");
    ASSERT_TRUE(fixture.sections()[0].Characteristics & 0x20000000, "text must be executable");
    ASSERT_FALSE(fixture.sections()[1].Characteristics & 0x20000000, "data must not be executable");
    ASSERT_LE(fixture.sections()[0].VirtualAddress + fixture.sections()[0].SizeOfRawData + 256,
              fixture.sections()[1].VirtualAddress, "room for BSS entry mutation");
    test.mutate(fixture);

    TestDevice device{};
    device.data = data;
    bio_initialize_bdev(&device.dev, "uefi-loader-test", 1, fixture.size, 0, nullptr, BIO_FLAGS_NONE);
    ASSERT_NONNULL(device.dev.name, "device name allocation");
    device.dev.read = read_fixture;
    bio_register_device(&device.dev);
    DEFER { bio_unregister_device(&device.dev); };

    // The first successful load can grow the kernel heap, which retains its
    // backing pages. Warm up once, not once per mutation; then compare exact
    // PMM counts across both measured loads, including every error path.
    if (warmup) {
        ASSERT_EQ(NO_ERROR, load_pe_blockdev(device.dev.name), "baseline warmup");
        ASSERT_EQ(1, device.dev.ref, "warmup leaked a BIO reference");
    }
    const size_t before = free_pages();
    for (int run = 0; run < 2; ++run) {
        device.reads = 0;
        ASSERT_EQ(test.expected, load_pe_blockdev(device.dev.name), test.name);
        if (test.expected == ERR_BAD_STATE) {
            ASSERT_EQ(1u, device.reads, "invalid layout must fail before image reads");
        }
        ASSERT_EQ(1, device.dev.ref, "loader leaked a BIO reference");
        ASSERT_EQ(before, free_pages(), "loader leaked PMM pages");
    }
    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(uefi_loader_tests)
    for (const auto &test : cases) {
        RUN_NAMED_TEST(test.name, [&test]() { return run_case(test, &test == cases); });
    }
END_TEST_CASE(uefi_loader_tests)
