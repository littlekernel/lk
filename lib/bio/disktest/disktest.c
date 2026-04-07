//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disktest_backend.h"

#if DISKTEST_BACKEND_LK
#include <lk/console_cmd.h>
#endif

#define DEFAULT_BLOCK_SIZE 512ULL
#define DEFAULT_ITERATIONS 1000ULL

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s help\n"
            "  %s fill <path> [block_size]\n"
            "  %s verify <path> [block_size]\n"
            "  %s read_rand <path> [block_size] [iterations]\n"
            "  %s read_write <path> [block_size] [iterations]\n",
            prog, prog, prog, prog, prog);
}

static void print_help(const char *prog) {
    printf("disktest: block-oriented disk/file stress tool\n\n");
    printf("Commands:\n");
    printf("  help\n");
    printf("    Show this help text.\n\n");
    printf("  fill <path> [block_size]\n");
    printf("    Fill the target with an initial self-describing pattern.\n\n");
    printf("  verify <path> [block_size]\n");
    printf("    Sequentially verify every block matches the expected pattern.\n\n");
    printf("  read_rand <path> [block_size] [iterations]\n");
    printf("    Randomly read and verify blocks (default iterations: %llu).\n\n",
            DEFAULT_ITERATIONS);
    printf("  read_write <path> [block_size] [iterations]\n");
    printf("    Randomly read/verify, then overwrite blocks with a new seed and verify.\n\n");
    printf("Defaults:\n");
    printf("  block_size: %llu bytes\n", DEFAULT_BLOCK_SIZE);
    printf("  iterations: %llu\n", DEFAULT_ITERATIONS);
    printf("\nPattern per block:\n");
    printf("  [u32 offset][u32 seed][LFSR payload...][u32 ~offset]\n\n");
    printf("Examples:\n");
    printf("  %s fill test.img 512\n", prog);
    printf("  %s verify test.img 512\n", prog);
    printf("  %s read_rand test.img 4096 1000\n", prog);
    printf("  %s read_write test.img 4096 1000\n", prog);
}

static int parse_u64(const char *s, uint64_t *out) {
    uint64_t v = 0;
    int base = 10;

    if (s == NULL || *s == '\0') {
        return -1;
    }

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
        if (*s == '\0') {
            return -1;
        }
    }

    while (*s != '\0') {
        int digit;

        if (*s >= '0' && *s <= '9') {
            digit = *s - '0';
        } else if (*s >= 'a' && *s <= 'f') {
            digit = 10 + (*s - 'a');
        } else if (*s >= 'A' && *s <= 'F') {
            digit = 10 + (*s - 'A');
        } else {
            return -1;
        }

        if (digit >= base) {
            return -1;
        }

        if (v > ((UINT64_MAX - (uint64_t)digit) / (uint64_t)base)) {
            return -1;
        }

        v = v * (uint64_t)base + (uint64_t)digit;
        s++;
    }

    if (out == NULL) {
        return -1;
    }

    *out = v;
    return 0;
}

static uint32_t load_u32_le(const uint8_t *p) {
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void store_u32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

static uint64_t lfsr_next(uint64_t *state) {
    uint64_t x = *state;

    if (x == 0) {
        x = 0x9e3779b97f4a7c15ULL;
    }

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static uint64_t rng_next(uint64_t *state) {
    uint64_t x = *state;

    if (x == 0) {
        x = 0x9e3779b97f4a7c15ULL;
    }

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static void fill_payload(uint8_t *buf, uint64_t block_size, uint32_t seed) {
    uint64_t state = (uint64_t)seed;
    uint64_t i;
    uint64_t chunk = 0;
    int chunk_idx = 8;

    if (state == 0) {
        state = 1;
    }

    for (i = 8; i < block_size - 4; ++i) {
        if (chunk_idx >= 8) {
            chunk = lfsr_next(&state);
            chunk_idx = 0;
        }
        buf[i] = (uint8_t)((chunk >> (chunk_idx * 8)) & 0xff);
        chunk_idx++;
    }
}

static void generate_block(uint8_t *buf, uint64_t block_size, uint32_t block_number, uint32_t seed) {
    store_u32_le(buf + 0, block_number);
    store_u32_le(buf + 4, seed);
    fill_payload(buf, block_size, seed);
    store_u32_le(buf + (block_size - 4), ~block_number);
}

typedef struct VerifyResult {
    int ok;
    uint32_t stored_offset;
    uint32_t stored_seed;
    uint64_t mismatch_offset_in_block;
    const char *reason;
} VerifyResult;

static VerifyResult verify_block(const uint8_t *buf, uint64_t block_size, uint32_t expected_block_number) {
    VerifyResult vr;
    uint64_t state;
    uint64_t chunk = 0;
    int chunk_idx = 8;

    vr.ok = 0;
    vr.stored_offset = load_u32_le(buf + 0);
    vr.stored_seed = load_u32_le(buf + 4);
    vr.mismatch_offset_in_block = 0;
    vr.reason = "unknown";

    uint32_t offset = vr.stored_offset;
    uint32_t inverse = load_u32_le(buf + (block_size - 4));

    if (offset != expected_block_number) {
        vr.reason = "offset header mismatch";
        return vr;
    }

    if (inverse != ~expected_block_number) {
        vr.reason = "offset trailer mismatch";
        vr.mismatch_offset_in_block = block_size - 4;
        return vr;
    }

    state = (uint64_t)vr.stored_seed;
    if (state == 0) {
        state = 1;
    }

    for (uint64_t i = 8; i < block_size - 4; ++i) {
        uint8_t expected;
        if (chunk_idx >= 8) {
            chunk = lfsr_next(&state);
            chunk_idx = 0;
        }
        expected = (uint8_t)((chunk >> (chunk_idx * 8)) & 0xff);
        chunk_idx++;
        if (buf[i] != expected) {
            vr.reason = "payload mismatch";
            vr.mismatch_offset_in_block = i;
            return vr;
        }
    }

    vr.ok = 1;
    vr.reason = "ok";
    return vr;
}

// Initialize every block with a self-describing pattern using a per-block random seed.
static int do_fill(void *backend_ctx, uint64_t block_size) {
    uint64_t total_bytes;
    uint64_t blocks;

    if (dt_backend_get_target_size(backend_ctx, block_size, &total_bytes, &blocks) != 0) {
        return 1;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)block_size);
    if (buf == NULL) {
        dt_backend_perror("malloc");
        return 1;
    }

    for (uint64_t block_index = 0; block_index < blocks; ++block_index) {
        uint32_t block_num = (uint32_t)(block_index & 0xffffffffULL);
        uint32_t random_seed = (uint32_t)rand();
        int64_t wr;

        generate_block(buf, block_size, block_num, random_seed);

        wr = dt_backend_write_block(backend_ctx, block_index, block_size, buf);
        if (wr != (int64_t)block_size) {
            if (wr < 0) {
                dt_backend_perror("block_write");
            } else {
                fprintf(stderr, "Short write at block %" PRIu64 "\n", block_index);
            }
            free(buf);
            return 1;
        }
    }

    if (dt_backend_flush(backend_ctx) != 0) {
        dt_backend_perror("fsync");
        free(buf);
        return 1;
    }

    free(buf);
    printf("fill: wrote %" PRIu64 " blocks (%" PRIu64 " bytes)\n", blocks, total_bytes);
    return 0;
}

// Sequentially read every block and validate the recorded header/trailer/payload pattern.
static int do_verify(void *backend_ctx, uint64_t block_size) {
    uint64_t total_bytes;
    uint64_t blocks;

    if (dt_backend_get_target_size(backend_ctx, block_size, &total_bytes, &blocks) != 0) {
        return 1;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)block_size);
    if (buf == NULL) {
        dt_backend_perror("malloc");
        return 1;
    }

    for (uint64_t block_index = 0; block_index < blocks; ++block_index) {
        uint32_t block_num = (uint32_t)(block_index & 0xffffffffULL);
        int64_t rd;
        VerifyResult vr;

        rd = dt_backend_read_block(backend_ctx, block_index, block_size, buf);
        if (rd != (int64_t)block_size) {
            if (rd < 0) {
                dt_backend_perror("block_read");
            } else {
                fprintf(stderr, "Short read at block %llu\n", (unsigned long long)block_index);
            }
            free(buf);
            return 1;
        }

        vr = verify_block(buf, block_size, block_num);
        if (!vr.ok) {
            fprintf(stderr,
                    "verify failed at block %" PRIu64 " (byte offset %" PRIu64 "): %s; "
                    "header_offset=0x%08x seed=0x%08x mismatch_offset_in_block=%" PRIu64 "\n",
                    block_index,
                    block_index * block_size,
                    vr.reason,
                    vr.stored_offset,
                    vr.stored_seed,
                    vr.mismatch_offset_in_block);
            free(buf);
            return 1;
        }
    }

    free(buf);
    printf("verify: ok for %" PRIu64 " blocks (%" PRIu64 " bytes)\n", blocks, total_bytes);
    return 0;
}

// Perform random block reads across the device and verify each sampled block.
static int do_read_rand(void *backend_ctx, uint64_t block_size, uint64_t iterations) {
    uint64_t total_bytes;
    uint64_t blocks;

    if (dt_backend_get_target_size(backend_ctx, block_size, &total_bytes, &blocks) != 0) {
        return 1;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)block_size);
    if (buf == NULL) {
        dt_backend_perror("malloc");
        return 1;
    }

    uint64_t rng_state = ((uint64_t)dt_backend_seed() << 32) ^ total_bytes;

    for (uint64_t i = 0; i < iterations; ++i) {
        uint64_t block_index = rng_next(&rng_state) % blocks;
        uint32_t block_num = (uint32_t)(block_index & 0xffffffffULL);
        int64_t rd;
        VerifyResult vr;

        rd = dt_backend_read_block(backend_ctx, block_index, block_size, buf);
        if (rd != (int64_t)block_size) {
            if (rd < 0) {
                dt_backend_perror("block_read");
            } else {
                fprintf(stderr, "Short read at random iteration %" PRIu64 "\n", i);
            }
            free(buf);
            return 1;
        }

        vr = verify_block(buf, block_size, block_num);
        if (!vr.ok) {
            fprintf(stderr,
                    "read_rand failed at iter %" PRIu64 ", block %" PRIu64 ": %s; "
                    "header_offset=0x%08x seed=0x%08x mismatch_offset_in_block=%" PRIu64 "\n",
                    i,
                    block_index,
                    vr.reason,
                    vr.stored_offset,
                    vr.stored_seed,
                    vr.mismatch_offset_in_block);
            free(buf);
            return 1;
        }
    }

    free(buf);
    printf("read_rand: ok for %" PRIu64 " iterations across %" PRIu64 " blocks\n", iterations, blocks);
    return 0;
}

// Repeatedly pick random blocks, verify current data, rewrite with a new seed, and verify again.
static int do_read_write(void *backend_ctx, uint64_t block_size, uint64_t iterations) {
    uint64_t total_bytes;
    uint64_t blocks;

    if (dt_backend_get_target_size(backend_ctx, block_size, &total_bytes, &blocks) != 0) {
        return 1;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)block_size);
    if (buf == NULL) {
        dt_backend_perror("malloc");
        return 1;
    }

    uint64_t rng_state = ((uint64_t)dt_backend_seed() << 32) ^ (total_bytes << 1);

    for (uint64_t i = 0; i < iterations; ++i) {
        uint64_t block_index = rng_next(&rng_state) % blocks;
        uint32_t block_num = (uint32_t)(block_index & 0xffffffffULL);
        int64_t rd;
        int64_t wr;
        VerifyResult vr;

        rd = dt_backend_read_block(backend_ctx, block_index, block_size, buf);
        if (rd != (int64_t)block_size) {
            if (rd < 0) {
                dt_backend_perror("block_read");
            } else {
                fprintf(stderr, "Short read at read_write iter %" PRIu64 "\n", i);
            }
            free(buf);
            return 1;
        }

        vr = verify_block(buf, block_size, block_num);
        if (!vr.ok) {
            fprintf(stderr,
                    "read_write pre-verify failed at iter %" PRIu64 ", block %" PRIu64 ": %s; "
                    "header_offset=0x%08x seed=0x%08x mismatch_offset_in_block=%" PRIu64 "\n",
                    i,
                    block_index,
                    vr.reason,
                    vr.stored_offset,
                    vr.stored_seed,
                    vr.mismatch_offset_in_block);
            free(buf);
            return 1;
        }

        {
            uint32_t random_seed = (uint32_t)rand();
            generate_block(buf, block_size, block_num, random_seed);
        }

        wr = dt_backend_write_block(backend_ctx, block_index, block_size, buf);
        if (wr != (int64_t)block_size) {
            if (wr < 0) {
                dt_backend_perror("block_write");
            } else {
                fprintf(stderr, "Short write at read_write iter %" PRIu64 "\n", i);
            }
            free(buf);
            return 1;
        }

        rd = dt_backend_read_block(backend_ctx, block_index, block_size, buf);
        if (rd != (int64_t)block_size) {
            if (rd < 0) {
                dt_backend_perror("block_read");
            } else {
                fprintf(stderr, "Short read after write at iter %" PRIu64 "\n", i);
            }
            free(buf);
            return 1;
        }

        vr = verify_block(buf, block_size, block_num);
        if (!vr.ok) {
            fprintf(stderr,
                    "read_write post-verify failed at iter %" PRIu64 ", block %" PRIu64 "; "
                    "reason=%s\n",
                    i,
                    block_index,
                    vr.reason);
            free(buf);
            return 1;
        }
    }

    if (dt_backend_flush(backend_ctx) != 0) {
        dt_backend_perror("fsync");
        free(buf);
        return 1;
    }

    free(buf);
    printf("read_write: completed %" PRIu64 " random read/write iterations\n", iterations);
    return 0;
}

static int disktest_main(int argc, const char **argv) {
    uint64_t block_size = DEFAULT_BLOCK_SIZE;
    uint64_t iterations = DEFAULT_ITERATIONS;
    void *backend_ctx = NULL;
    int ret;

    srand(dt_backend_seed());

    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0) {
        if (argc != 2) {
            usage(argv[0]);
            return 2;
        }
        print_help(argv[0]);
        return 0;
    }

    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    const char *path = argv[2];

    if (argc >= 4) {
        if (parse_u64(argv[3], &block_size) != 0 || block_size < 16) {
            fprintf(stderr, "Invalid block size: %s\n", argv[3]);
            return 2;
        }
    }

    if (strcmp(cmd, "read_rand") == 0 || strcmp(cmd, "read_write") == 0) {
        if (argc >= 5) {
            if (parse_u64(argv[4], &iterations) != 0 || iterations == 0) {
                fprintf(stderr, "Invalid iterations: %s\n", argv[4]);
                return 2;
            }
        }
        if (argc > 5) {
            usage(argv[0]);
            return 2;
        }
    } else if (argc > 4) {
        usage(argv[0]);
        return 2;
    }

    bool need_write = (strcmp(cmd, "fill") == 0 || strcmp(cmd, "read_write") == 0);
    if (dt_backend_open(path, need_write, &backend_ctx) != 0) {
        dt_backend_perror("open");
        return 1;
    }

    if (strcmp(cmd, "fill") == 0) {
        ret = do_fill(backend_ctx, block_size);
    } else if (strcmp(cmd, "verify") == 0) {
        ret = do_verify(backend_ctx, block_size);
    } else if (strcmp(cmd, "read_rand") == 0) {
        ret = do_read_rand(backend_ctx, block_size, iterations);
    } else if (strcmp(cmd, "read_write") == 0) {
        ret = do_read_write(backend_ctx, block_size, iterations);
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        usage(argv[0]);
        ret = 2;
    }

    if (dt_backend_close(backend_ctx) != 0) {
        dt_backend_perror("close");
        if (ret == 0) {
            ret = 1;
        }
    }

    return ret;
}

#if DISKTEST_BACKEND_LK
static int cmd_disktest(int argc, const console_cmd_args *argv) {
    const char *args[8];

    if (argv == NULL) {
        printf("invalid arguments\n");
        return 2;
    }

    if (argc > (int)(sizeof(args) / sizeof(args[0]))) {
        printf("too many args\n");
        return 2;
    }

    for (int i = 0; i < argc; ++i) {
        if (argv[i].str == NULL) {
            printf("invalid argument %d\n", i);
            return 2;
        }
        args[i] = argv[i].str;
    }

    return disktest_main(argc, args);
}

STATIC_COMMAND_START
STATIC_COMMAND("disktest", "block device stress test", &cmd_disktest)
STATIC_COMMAND_END(disktest);
#else
int main(int argc, char **argv) {
    return disktest_main(argc, (const char **)argv);
}
#endif
