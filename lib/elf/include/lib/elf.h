/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/elf_defines.h>
#include <sys/types.h>
#include <stdbool.h>

/* based on our bitness, support 32 or 64 bit elf */
#if IS_64BIT
#define WITH_ELF64 1
#else
#define WITH_ELF32 1
#endif

/* api */
struct elf_handle;
typedef ssize_t (*elf_read_hook_t)(struct elf_handle *, void *buf, uint64_t offset, size_t len);
typedef status_t (*elf_mem_alloc_t)(struct elf_handle *, void **ptr, size_t len, uint num, uint flags);

typedef struct elf_handle {
    bool open;

    // read hook to load binary out of memory
    elf_read_hook_t read_hook;
    void *read_hook_arg;
    bool free_read_hook_arg;

    // memory allocation callback
    elf_mem_alloc_t mem_alloc_hook;
    void *mem_alloc_hook_arg;

    // loaded info about the elf file
#if WITH_ELF32
    struct Elf32_Ehdr eheader;    // a copy of the main elf header
    struct Elf32_Phdr *pheaders;  // a pointer to a buffer of program headers
#else
    struct Elf64_Ehdr eheader;    // a copy of the main elf header
    struct Elf64_Phdr *pheaders;  // a pointer to a buffer of program headers
#endif

    addr_t load_address;
    addr_t entry;
} elf_handle_t;

status_t elf_open_handle(elf_handle_t *handle, elf_read_hook_t read_hook, void *read_hook_arg, bool free_read_hook_arg);
status_t elf_open_handle_memory(elf_handle_t *handle, const void *ptr, size_t len);
void     elf_close_handle(elf_handle_t *handle);

status_t elf_load(elf_handle_t *handle);

