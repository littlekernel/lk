/*
 * Copyright (c) 2015 Carlos Pizano-Uribe <cpu@chromium.org>
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


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <lib/tftp.h>
#include <lib/cksum.h>
#include <lib/elf.h>

#include <kernel/thread.h>

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>
#else
#error "loader app needs a console"
#endif

#if defined(SDRAM_BASE)
#define DOWNLOAD_BASE ((void*)SDRAM_BASE)
#else
#define DOWNLOAD_BASE ((void*)0)
#endif

#define FNAME_SIZE 64
#define DOWNLOAD_SLOT_SIZE (512 * 1024)

typedef enum {
    DOWNLOAD_ANY,
    DOWNLOAD_ELF,
} download_type;

typedef struct {
    unsigned char *start;
    unsigned char *end;
    unsigned char *max;
    char name[FNAME_SIZE];
    download_type type;
} download_t;

static download_t *make_download(const char *name)
{
    download_t *d = malloc(sizeof(download_t));
    memset(d, 0, sizeof(download_t));
    strncpy(d->name, name, FNAME_SIZE);
    return d;
}

static void set_ram_zone(download_t *d, int slot)
{
    d->start = DOWNLOAD_BASE + (DOWNLOAD_SLOT_SIZE * slot);
    d->end = d->start;
    d->max = d->end + DOWNLOAD_SLOT_SIZE;
    memset(d->start, 0, DOWNLOAD_SLOT_SIZE);
}

static size_t output_result(const download_t *download)
{
    size_t len = download->end - download->start;
    unsigned long crc = crc32(0, download->start, len);
    printf("[%s] done, start at: %p - %zu bytes, crc32 = %lu\n",
           download->name, download->start, len, crc);
    return len;
}

static int run_elf(void *entry_point)
{
    void (*elf_start)(void) = (void *)entry_point;
    printf("elf (%p) running ...\n", entry_point);
    thread_sleep(10);
    elf_start();
    printf("elf (%p) finished\n", entry_point);
    return 0;
}

static void process_elf_blob(const void *start, size_t len)
{
    void *entrypt;
    elf_handle_t elf;

    status_t st = elf_open_handle_memory(&elf, start, len);
    if (st < 0) {
        printf("unable to open elf handle\n");
        return;
    }

    st = elf_load(&elf);
    if (st < 0) {
        printf("elf processing failed, status : %d\n", st);
        goto exit;
    }

    entrypt = (void *)elf.entry;
    if (entrypt < start || entrypt >= (void *)((char *)start + len)) {
        printf("out of bounds entrypoint for elf : %p\n", entrypt);
        goto exit;
    }

    printf("elf looks good\n");
    thread_resume(thread_create("elf_runner", &run_elf, entrypt,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
exit:
    elf_close_handle(&elf);
}

int tftp_callback(void *data, size_t len, void *arg)
{
    download_t *download = arg;
    size_t final_len;

    if (!data) {
        final_len = output_result(download);
        if (download->type == DOWNLOAD_ELF) {
            process_elf_blob(download->start, final_len);
        }

        download->end = download->start;
        return 0;
    }

    if ((download->end + len) > download->max) {
        printf("transfer too big, aborting\n");
        return -1;
    }
    if (len) {
        memcpy(download->end, data, len);
        download->end += len;
    }
    return 0;
}

static int loader(int argc, const cmd_args *argv)
{
    static int any_slot = 0;
    static int elf_slot = 1;

    download_t *download;
    int slot;

    if (!DOWNLOAD_BASE) {
        printf("loader not available. it needs sdram\n");
        return 0;
    }

    if (argc < 3) {
usage:
        printf("load any [filename] <slot>\n"
               "load elf [filename] <slot>\n"
               "protocol is tftp and <slot> is optional\n");
        return 0;
    }

    download = make_download(argv[2].str);

    if (strcmp(argv[1].str, "any") == 0) {
        download->type = DOWNLOAD_ANY;
        slot = any_slot;
        any_slot += 2;
    } else if (strcmp(argv[1].str, "elf") == 0) {
        download->type = DOWNLOAD_ELF;
        slot = elf_slot;
        elf_slot += 2;
    } else {
        goto usage;
    }

    if (argc == 4) {
        slot = argv[3].i;
    }

    set_ram_zone(download, slot);
    tftp_set_write_client(download->name, &tftp_callback, download);
    printf("ready for %s over tftp (at %p)\n", argv[2].str, download->start);
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("load", "download and run via tftp", &loader)
STATIC_COMMAND_END(loader);

