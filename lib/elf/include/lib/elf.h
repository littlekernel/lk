/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

