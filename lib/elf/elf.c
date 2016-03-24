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
#include <lib/elf.h>
#include <assert.h>
#include <debug.h>
#include <endian.h>
#include <err.h>
#include <trace.h>
#include <stdlib.h>
#include <string.h>
#include <arch/ops.h>

#define LOCAL_TRACE 0

/* conditionally define a 32 or 64 bit version of the data structures
 * we care about, based on our bitness.
 */
#if WITH_ELF32
typedef struct Elf32_Ehdr elf_ehdr_t;
typedef struct Elf32_Phdr elf_phdr_t;

#define ELF_OFF_PRINT_U "%u"
#define ELF_OFF_PRINT_X "%x"
#define ELF_ADDR_PRINT_U "%u"
#define ELF_ADDR_PRINT_X "%x"
#else
typedef struct Elf64_Ehdr elf_ehdr_t;
typedef struct Elf64_Phdr elf_phdr_t;

#define ELF_OFF_PRINT_U "%llu"
#define ELF_OFF_PRINT_X "%llx"
#define ELF_ADDR_PRINT_U "%llu"
#define ELF_ADDR_PRINT_X "%llx"
#endif

struct read_hook_memory_args {
    const uint8_t *ptr;
    size_t len;
};

static ssize_t elf_read_hook_memory(struct elf_handle *handle, void *buf, uint64_t offset, size_t len)
{
    LTRACEF("handle %p, buf %p, offset %lld, len %zu\n", handle, buf, offset, len);

    struct read_hook_memory_args *args = handle->read_hook_arg;

    DEBUG_ASSERT(args);
    DEBUG_ASSERT(buf);
    DEBUG_ASSERT(handle);
    DEBUG_ASSERT(handle->open);

    ssize_t toread = len;
    if (offset >= args->len)
        toread = 0;
    if (offset + len >= args->len)
        toread = args->len - offset;

    memcpy(buf, args->ptr + offset, toread);

    LTRACEF("returning %ld\n", toread);

    return toread;
}

status_t elf_open_handle(elf_handle_t *handle, elf_read_hook_t read_hook, void *read_hook_arg, bool free_read_hook_arg)
{
    if (!handle)
        return ERR_INVALID_ARGS;
    if (!read_hook)
        return ERR_INVALID_ARGS;

    memset(handle, 0, sizeof(*handle));

    handle->read_hook = read_hook;
    handle->read_hook_arg = read_hook_arg;
    handle->free_read_hook_arg = free_read_hook_arg;

    handle->open = true;

    return NO_ERROR;
}

status_t elf_open_handle_memory(elf_handle_t *handle, const void *ptr, size_t len)
{
    struct read_hook_memory_args *args = malloc(sizeof(struct read_hook_memory_args));

    args->ptr = ptr;
    args->len = len;

    status_t err = elf_open_handle(handle, elf_read_hook_memory, (void *)args, true);
    if (err < 0)
        free(args);

    return err;
}

void elf_close_handle(elf_handle_t *handle)
{
    if (!handle || !handle->open)
        return;

    handle->open = false;

    if (handle->free_read_hook_arg)
        free(handle->read_hook_arg);

    free(handle->pheaders);
}

static int verify_eheader(const void *header)
{
    const elf_ehdr_t *eheader = header;

    if (memcmp(eheader->e_ident, ELF_MAGIC, 4) != 0)
        return ERR_NOT_FOUND;

#if WITH_ELF32
    if (eheader->e_ident[EI_CLASS] != ELFCLASS32)
        return ERR_NOT_FOUND;
#else
    if (eheader->e_ident[EI_CLASS] != ELFCLASS64)
        return ERR_NOT_FOUND;
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
    if (eheader->e_ident[EI_DATA] != ELFDATA2LSB)
        return ERR_NOT_FOUND;
#elif BYTE_ORDER == BIG_ENDIAN
    if (eheader->e_ident[EI_DATA] != ELFDATA2MSB)
        return ERR_NOT_FOUND;
#endif

    if (eheader->e_ident[EI_VERSION] != EV_CURRENT)
        return ERR_NOT_FOUND;

    if (eheader->e_phoff == 0)
        return ERR_NOT_FOUND;

    if (eheader->e_phentsize < sizeof(elf_phdr_t))
        return ERR_NOT_FOUND;

#if ARCH_ARM
    if (eheader->e_machine != EM_ARM)
        return ERR_NOT_FOUND;
#elif ARCH_ARM64
    if (eheader->e_machine != EM_AARCH64)
        return ERR_NOT_FOUND;
#elif ARCH_X86
    if (eheader->e_machine != EM_386)
        return ERR_NOT_FOUND;
#elif ARCH_X86_64
    if (eheader->e_machine != EM_X86_64)
        return ERR_NOT_FOUND;
#elif ARCH_MICROBLAZE
    if (eheader->e_machine != EM_MICROBLAZE)
        return ERR_NOT_FOUND;
#else
#error find proper EM_ define for your machine
#endif

    return NO_ERROR;
}

status_t elf_load(elf_handle_t *handle)
{
    if (!handle)
        return ERR_INVALID_ARGS;
    if (!handle->open)
        return ERR_NOT_READY;

    // validate that this is an ELF file
    ssize_t readerr = handle->read_hook(handle, &handle->eheader, 0, sizeof(handle->eheader));
    if (readerr < (ssize_t)sizeof(handle->eheader)) {
        LTRACEF("couldn't read elf header\n");
        return ERR_NOT_FOUND;
    }

    if (verify_eheader(&handle->eheader)) {
        LTRACEF("header not valid\n");
        return ERR_NOT_FOUND;
    }

    // sanity check number of program headers
    LTRACEF("number of program headers %u, entry size %u\n", handle->eheader.e_phnum, handle->eheader.e_phentsize);
    if (handle->eheader.e_phnum > 16 ||
            handle->eheader.e_phentsize != sizeof(elf_phdr_t)) {
        LTRACEF("too many program headers or bad size\n");
        return ERR_NO_MEMORY;
    }

    // allocate and read in the program headers
    handle->pheaders = calloc(1, handle->eheader.e_phnum * handle->eheader.e_phentsize);
    if (!handle->pheaders) {
        LTRACEF("failed to allocate memory for program headers\n");
        return ERR_NO_MEMORY;
    }

    readerr = handle->read_hook(handle, handle->pheaders, handle->eheader.e_phoff, handle->eheader.e_phnum * handle->eheader.e_phentsize);
    if (readerr < (ssize_t)(handle->eheader.e_phnum * handle->eheader.e_phentsize)) {
        LTRACEF("failed to read program headers\n");
        return ERR_NO_MEMORY;
    }

    LTRACEF("program headers:\n");
    uint load_count = 0;
    for (uint i = 0; i < handle->eheader.e_phnum; i++) {
        // parse the program headers
        elf_phdr_t *pheader = &handle->pheaders[i];

        LTRACEF("%u: type %u offset 0x" ELF_OFF_PRINT_X " vaddr "
                ELF_ADDR_PRINT_X " paddr " ELF_ADDR_PRINT_X " memsiz "
                ELF_ADDR_PRINT_U " filesize " ELF_ADDR_PRINT_U "\n",
                i, pheader->p_type, pheader->p_offset, pheader->p_vaddr,
                pheader->p_paddr, pheader->p_memsz, pheader->p_filesz);

        // we only care about PT_LOAD segments at the moment
        if (pheader->p_type == PT_LOAD) {
            // if the memory allocation hook exists, call it
            void *ptr = (void *)(uintptr_t)pheader->p_vaddr;

            if (handle->mem_alloc_hook) {
                status_t err = handle->mem_alloc_hook(handle, &ptr, pheader->p_memsz, load_count, 0);
                if (err < 0) {
                    LTRACEF("mem hook failed, abort\n");
                    // XXX clean up what we got so far
                    return err;
                }
            }

            // read the file portion of the segment into memory at vaddr
            LTRACEF("reading segment at offset " ELF_OFF_PRINT_U " to address %p\n", pheader->p_offset, ptr);
            readerr = handle->read_hook(handle, ptr, pheader->p_offset, pheader->p_filesz);
            if (readerr < (ssize_t)pheader->p_filesz) {
                LTRACEF("error %ld reading program header %u\n", readerr, i);
                return (readerr < 0) ? readerr : ERR_IO;
            }

            // zero out he difference between memsz and filesz
            size_t tozero = pheader->p_memsz - pheader->p_filesz;
            if (tozero > 0) {
                uint8_t *ptr2 = (uint8_t *)ptr + pheader->p_filesz;
                LTRACEF("zeroing memory at %p, size %zu\n", ptr2, tozero);
                memset(ptr2, 0, tozero);
            }

            // make sure the i&d cache are coherent, if they exist
            arch_sync_cache_range((addr_t)ptr, pheader->p_memsz);

            // track the number of load segments we have seen to pass the mem alloc hook
            load_count++;
        }
    }

    // save the entry point
    handle->entry = handle->eheader.e_entry;

    return NO_ERROR;
}

