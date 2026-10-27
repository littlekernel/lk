/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <endian.h>
#include <lib/bio.h>
#include <lib/fs/rofs.h>
#include <lk/compiler.h>
#include <lk/err.h>
#include <lk/init.h>
#include <stdio.h>
#include <string.h>

// ROFS_EMBED_IMAGE, linked in read-only at ROFS_EMBED_ALIGN, and the memory block device
// ROFS_EMBED_NAME over it, registered before the platform's own block devices appear and long
// before the apps run, so anything from then on can mount it.

#define EMBED_STR(x) #x
#define EMBED_XSTR(x) EMBED_STR(x)

__asm__(".section .rodata.rofs_embed, \"a\"\n"
        ".balign " EMBED_XSTR(ROFS_EMBED_ALIGN) "\n"
        "rofs_embed_start:\n"
        ".incbin \"" ROFS_EMBED_IMAGE "\"\n"
        "rofs_embed_end:\n"
        ".previous\n");
extern const uint8_t rofs_embed_start[];
extern const uint8_t rofs_embed_end[];

static void rofs_embed_init(uint level) {
    const size_t len = (size_t)(rofs_embed_end - rofs_embed_start);
    struct rofs_header h;
    if (len < sizeof(h)) {
        printf("rofs: the embedded image is %zu bytes, too short to be one\n", len);
        return;
    }
    memcpy(&h, rofs_embed_start, sizeof(h));
    // a file's address in the image is only as aligned as the image is
    const uint32_t align = LE32(h.align);
    if ((uintptr_t)rofs_embed_start % align) {
        printf("rofs: the embedded image's files are %u aligned, but it is linked at %#lx\n",
               align, (unsigned long)(uintptr_t)rofs_embed_start);
        return;
    }
    // the device only reads it
    status_t err = create_membdev(ROFS_EMBED_NAME, (void *)rofs_embed_start, len);
    if (err < 0) {
        printf("rofs: no device %s for the embedded image: %d\n", ROFS_EMBED_NAME, err);
    }
}

LK_INIT_HOOK(rofs_embed, &rofs_embed_init, LK_INIT_LEVEL_PLATFORM - 1);
