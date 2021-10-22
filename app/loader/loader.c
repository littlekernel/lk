/*
 * Copyright (c) 2015 Carlos Pizano-Uribe <cpu@chromium.org>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <lib/tftp.h>
#include <lib/cksum.h>
#include <lib/elf.h>

#include <kernel/thread.h>

#include <lk/console_cmd.h>

#if defined(SDRAM_BASE)
#define DOWNLOAD_BASE ((unsigned char*)SDRAM_BASE)
#else
#define DOWNLOAD_BASE ((unsigned char*)0)
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

static download_t *make_download(const char *name) {
    download_t *d = malloc(sizeof(download_t));
    memset(d, 0, sizeof(download_t));
    strncpy(d->name, name, FNAME_SIZE);
    return d;
}

static void set_ram_zone(download_t *d, unsigned char *spot, int slot) {
    d->start = spot + (DOWNLOAD_SLOT_SIZE * slot);
    d->end = d->start;
    d->max = d->end + DOWNLOAD_SLOT_SIZE;
    memset(spot, 0, DOWNLOAD_SLOT_SIZE);
}

static size_t output_result(const download_t *download) {
    size_t len = download->end - download->start;
    unsigned long crc = crc32(0, download->start, len);
    printf("[%s] done, start at: %p - %zu bytes, crc32 = %lu\n",
           download->name, download->start, len, crc);
    return len;
}

static int run_elf(void *entry_point) {
    void (*elf_start)(void) = (void *)entry_point;
    printf("elf (%p) running ...\n", entry_point);
    thread_sleep(10);
    elf_start();
    printf("elf (%p) finished\n", entry_point);
    return 0;
}

static void process_elf_blob(const void *start, size_t len) {
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

static int tftp_callback(void *data, size_t len, void *arg) {
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

static int loader(int argc, const console_cmd_args *argv) {
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

    set_ram_zone(download, DOWNLOAD_BASE, slot);
    tftp_set_write_client(download->name, &tftp_callback, download);
    printf("ready for %s over tftp (at %p)\n", argv[2].str, download->start);
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("load", "download and run via tftp", &loader)
STATIC_COMMAND_END(loader);

