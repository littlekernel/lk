/*
 * Copyright (c) 2013 Google, Inc.
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
#include <lib/buildsig.h>

#include <debug.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <compiler.h>
#include <lib/version.h>

#define MAGIC  ((uint32_t)'BSIG')
#define MAGIC2 (~MAGIC)
#define MAGIC3 ((uint32_t)'BSG2')

struct buildsig {
    uint32_t magic;

    const lk_version_t *version;

    uint32_t magic2;

    uint32_t buildtype;
    uint32_t start;
    uint32_t end;
    uint32_t crc32;

    uint32_t magic3;
};

extern char __rom_start;
extern char __rom_end;

const struct buildsig buildsig __SECTION(".text.boot") = {
    .magic = MAGIC,
    .version = &version,
    .magic2 = MAGIC2,
#if WITH_APP_BOOTLOADER
    .buildtype = 1, /* TODO: pull from systemwide headers */
#else
    .buildtype = 0,
#endif
    .start = (uint32_t) &__rom_start,
    .end = (uint32_t) &__rom_end,
    .crc32 = 0, /* filled in via an external tool */
    .magic3 = MAGIC3
};

status_t buildsig_search(const void *_ptr, size_t search_len, size_t max_len, const lk_version_t **version)
{
    if (max_len < search_len)
        return ERR_INVALID_ARGS;

    if (max_len < sizeof(lk_version_t))
        return ERR_INVALID_ARGS;

    /* search for the build signature on 4 byte boundaries */
    const uint32_t *ptr = _ptr;
    for (size_t pos = 0; pos < search_len / 4; pos++) {
        const struct buildsig *sig = (void *)&ptr[pos];

        /* see if the buildsig's magic matches */
        if (sig->magic != MAGIC || sig->magic2 != MAGIC2)
            continue;

        /* make sure the pointer to the version struct makes sense */
        if ((size_t)sig->version - (size_t)ptr > max_len - sizeof(lk_version_t))
            continue;

        /* validate the strings in the version struct make sense */
        /* ensure they lie within the search area (ptr .. ptr+max_len) */
#define VALIDSTR(str) \
        (((size_t)(str) >= (size_t)ptr) && (((size_t)(str) - (size_t)ptr) < max_len))
        if (!VALIDSTR(sig->version->arch))
            continue;
        if (!VALIDSTR(sig->version->platform))
            continue;
        if (!VALIDSTR(sig->version->target))
            continue;
        if (!VALIDSTR(sig->version->project))
            continue;
        if (!VALIDSTR(sig->version->buildid))
            continue;
#undef VALIDSTR

        *version = sig->version;
        return NO_ERROR;
    }

    return ERR_NOT_FOUND;
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>

extern char __rom_start;

static int cmd_buildsig(int argc, const cmd_args *argv)
{
    if (argc < 2) {
//notenoughargs:
        printf("not enough args\n");
usage:
        printf("usage: %s dump [offset]\n", argv[0].str);
        return -1;
    }

    if (!strcmp(argv[1].str, "dump")) {
        const void *offset = &__rom_start;
        if (argc >= 3) {
            offset = argv[2].p;
        }

        const lk_version_t *v;
        status_t err = buildsig_search(offset, DEFAULT_BUILDSIG_SEARCH_LEN, 256*1024, &v);
        if (err < 0) {
            printf("could not find build signature\n");
            return ERR_NOT_FOUND;
        }

        printf("found signature:\n");
        printf("\tarch: %s\n\tplatform: %s\n\ttarget: %s\n\tproject: %s\n\tbuildid: %s\n",
               v->arch, v->platform, v->target, v->project, v->buildid);
    } else {
        goto usage;
    }

    return NO_ERROR;
}


STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 1
STATIC_COMMAND("buildsig", "scan for and dump build signature", &cmd_buildsig)
#endif
STATIC_COMMAND_END(buildid);

#endif

